/*
 * read-sql.c
 *
 * Look up words in an SQL DB.
 * Keeping it simple for just right now, and using SQLite3.
 *
 * The goal of using an SQL-backed dictionary is to enable some
 * other process (machine-learning algo) to dynamically update
 * the dictionary.
 *
 * Copyright (c) 2014 Linas Vepstas <linasvepstas@gmail.com>
 */

#ifdef HAVE_SQLITE

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sqlite3.h>

#include "api-structures.h"
#include "connectors.h"
#include "dict-common/dict-api.h"
#include "dict-common/dict-common.h"
#include "dict-common/dict-impl.h"
#include "dict-common/dict-structures.h"
#include "dict-common/dict-utils.h"      // free_Exp()
#include "dict-common/file-utils.h"
#include "dict-file/read-dict.h"         // dictionary_six()
#include "externs.h"
#include "lg_assert.h"
#include "string-set.h"
#include "tokenize/spellcheck.h"
#include "utilities.h"

#include "read-sql.h"

/* ========================================================= */
/* Mini expression-parsing library.  This is a simplified subset of
 * what can be found in the file-backed dictionary.
 *
 * This does NOT support braces {} used to indicate optional connectors,
 * so using this in the SQL tables is NOT valid! It also does not
 * support cost brackets []. Costs are handled out-of-line.
 *
 * This is not meant to be "just like the text files, but different".
 */

static const char * make_expression(Dictionary dict,
                                    const char *exp_str, Exp** pex)
{
	*pex = NULL;
	/* Search for the start of a connector */
	Exp_type etype = CONNECTOR_type;
	const char * p = exp_str;
	while (*p && (lg_isspace(*p))) p++;
	if (0 == *p) return p;

	/* If it's an open paren, assume its the begining of a new list */
	if ('(' == *p)
	{
		p = make_expression(dict, ++p, pex);
	}
	else
	{
		/* Search for the end of a connector */
		const char * con_start = p;
		while (*p && (isalnum(*p) || '*' == *p)) p++;

		/* Connectors always end with a + or - */
		assert (('+' == *p) || ('-' == *p),
				"Missing direction character in connector string: %s", con_start);

		/* Create an expression to hold the connector */
		Exp* e = malloc(sizeof(Exp));
		e->dir = *p;
		e->type = CONNECTOR_type;
		e->cost = 0.0;
		char * constr = NULL;
		if ('@' == *con_start)
		{
			constr = strndup(con_start+1, p-con_start-1);
			e->multi = true;
		}
		else
		{
			constr = strndup(con_start, p-con_start);
			e->multi = false;
		}

		e->u.condesc = condesc_add(&dict->contable,
		                           string_set_add(constr, dict->string_set));
		free(constr);
		*pex = e;
	}

	/* Is there any more? If not, return what we've got. */
	p++;
	while (*p && (lg_isspace(*p))) p++;
	if (')' == *p || 0 == *p)
	{
		return p;
	}

	/* Wait .. there's more! */
	if ('&' == *p)
	{
		etype = AND_type; p++;
	}
	else if ('o' == *p && 'r' == *(p+1))
	{
		etype = OR_type; p+=2;
	}

	Exp* rest = NULL;
	p = make_expression(dict, p, &rest);
	assert(NULL != rest, "Badly formed expression %s", exp_str);

	/* Join it all together. */
	Exp* join = malloc(sizeof(Exp));
	join->type = etype;
	join->cost = 0.0;
	E_list *ell = join->u.l = malloc(sizeof(E_list));
	E_list *elr = ell->next = malloc(sizeof(E_list));
	elr->next = NULL;

	ell->e = *pex;
	elr->e = rest;
	*pex = join;

	return p;
}


/* ========================================================= */
/* Dictionary word lookup procedures. */

typedef struct
{
	Dictionary dict;
	Dict_node* dn;
	bool found;
	Exp* exp;
} cbdata;

static void db_free_llist(Dictionary dict, Dict_node *llist)
{
	Dict_node * dn;
	while (llist != NULL)
	{
		Exp *e;
		dn = llist->right;
		e = llist->exp;
		if (e) free_Exp(e);
		free(llist);
		llist = dn;
	}
}

/* callback -- set bs->exp to the expressions for a class in the dict */
static int exp_cb(void *user_data, int argc, char **argv, char **colName)
{
	cbdata* bs = user_data;

	assert(2 == argc, "Bad column count");
	assert(argv[0], "NULL column value");

	Exp* exp = NULL;
	make_expression(bs->dict, argv[0], &exp);

	if (exp && !strtodC(argv[1], &exp->cost))
	{
		prt_error("Warning: Invalid cost \"%s\" in expression \"%s\" "
		          "(using 1.0)\n", argv[1], argv[0]);
		exp->cost = 1.0;
	}

	if (NULL == bs->exp)
	{
		bs->exp = exp;
		return 0;
	}

	E_list *ell, *elr;
	Exp* orn = malloc(sizeof(Exp));
	orn->type = OR_type;
	orn->cost = 0.0;
	orn->u.l = ell = malloc(sizeof(E_list));
	ell->next = elr = malloc(sizeof(E_list));
	elr->next = NULL;

	ell->e = bs->exp;
	elr->e = exp;
	bs->exp = orn;

	return 0;
}

static void
db_lookup_exp(Dictionary dict, const char *s, cbdata* bs)
{
	sqlite3 *db = dict->db_handle;
	dyn_str *qry;

	/* The token to look up is called the 'morpheme'. */
	qry = dyn_str_new();
	dyn_strcat(qry, "SELECT disjunct, cost FROM Disjuncts WHERE classname = \'");
	dyn_strcat(qry, s);
	dyn_strcat(qry, "\';");

	sqlite3_exec(db, qry->str, exp_cb, bs, NULL);
	dyn_str_delete(qry);

	if (4 < verbosity)
	{
		printf("Found expression for class %s: ", s);
		print_expression(bs->exp);
	}
}


/* callback -- set bs->found to true if the word is in the dict */
static int exists_cb(void *user_data, int argc, char **argv, char **colName)
{
	cbdata* bs = user_data;

	assert(2 == argc, "Bad column count");
	assert(argv[0], "NULL column value");

	bs->found = true;
	return 0;
}

/* callback -- set bs->dn to the dict nodes for a word in the dict */
static int morph_cb(void *user_data, int argc, char **argv, char **colName)
{
	cbdata* bs = user_data;
	Dict_node *dn;
	char *scriword, *wclass;

	assert(2 == argc, "Bad column count");
	assert(argv[0], "NULL column value");
	scriword = argv[0];
	wclass = argv[1];

	/* Now look up the expressions for each word */
	bs->exp = NULL;
	db_lookup_exp(bs->dict, wclass, bs);

	/* Well, if we found a classname for a word, then there really,
	 * really should be able to find one or more corresponding disjuncts.
	 * However, it is possible to have corrupted databases which do not
	 * have any disjuncts for a word class.   We silently ignore these.
	 * Although maybe we should throw an error here?
	 */
	if (NULL == bs->exp) return 0;

	/* Put each word into a Dict_node. */
	dn = malloc(sizeof(Dict_node));
	memset(dn, 0, sizeof(Dict_node));
	dn->string = string_set_add(scriword, bs->dict->string_set);
	dn->right = bs->dn;
	dn->exp = bs->exp;
	bs->dn = dn;

	return 0;
}

static void
db_lookup_common(Dictionary dict, const char *s, const char *equals,
                 int (*cb)(void *, int, char **, char **),
                 cbdata* bs)
{
	sqlite3 *db = dict->db_handle;
	dyn_str *qry;

	/* The token to look up is called the 'morpheme'. */
	qry = dyn_str_new();
	dyn_strcat(qry, "SELECT subscript, classname FROM Morphemes WHERE morpheme ");
	dyn_strcat(qry, equals);
	dyn_strcat(qry, " \'");
	dyn_strcat(qry, s);
	dyn_strcat(qry, "\';");

	sqlite3_exec(db, qry->str, cb, bs, NULL);
	dyn_str_delete(qry);
}

static bool db_lookup(Dictionary dict, const char *s)
{
	cbdata bs;
	bs.dict = dict;
	bs.found = false;
	db_lookup_common(dict, s, "=", exists_cb, &bs);
	return bs.found;
}

static Dict_node * db_lookup_list(Dictionary dict, const char *s)
{
	cbdata bs;
	bs.dict = dict;
	bs.dn = NULL;
	db_lookup_common(dict, s, "=", morph_cb, &bs);
	if (3 < verbosity)
	{
		if (bs.dn)
		{
			printf("Found expression for word %s: ", s);
			print_expression(bs.dn->exp);
		}
		else
		{
			printf("No expression for word %s\n", s);
		}
	}
	return bs.dn;
}

/**
 * This is used to support wild-card lookup from the command-line
 * client.  That is, a user can type in `!!foo*` and look up all
 * words that begin with the three letters `foo`.  It works ...
 * but it only works if the dictionary also has UNKNOWN-WORD defined!
 */
static Dict_node * db_lookup_wild(Dictionary dict, const char *s)
{
	cbdata bs;
	bs.dict = dict;
	bs.dn = NULL;
	db_lookup_common(dict, s, "GLOB", morph_cb, &bs);
	if (3 < verbosity)
	{
		if (bs.dn)
		{
			printf("Found expression for glob %s: ", s);
			print_expression(bs.dn->exp);
		}
		else
		{
			printf("No expression for glob %s\n", s);
		}
	}
	return bs.dn;
}

/* ========================================================= */
/* Dictionary creation, setup, open procedures */

bool check_db(const char *lang)
{
	char *dbname = join_path (lang, "dict.db");
	bool retval = file_exists(dbname);
	free(dbname);
	return retval;
}

static void* db_open(const char * fullname, const void * user_data)
{
	int fd;
	struct stat buf;
	sqlite3 *db;

	/* Is there a file here that can be read? */
	FILE * fh =  fopen(fullname, "r");
	if (NULL == fh)
		return NULL;

	/* Get the file size, in bytes. */
	/* SQLite has a habit of leaving zero-length DB's lying around */
	fd = fileno(fh);
	fstat(fd, &buf);
	fclose(fh);
	if (0 == buf.st_size)
		return NULL;

	/* Found a file, of non-zero length. See if that works. */
	if (sqlite3_open(fullname, &db))
	{
		prt_error("Error: Can't open database %s: %s\n",
			fullname, sqlite3_errmsg(db));
		sqlite3_close(db);
		return NULL;
	}
	return (void *) db;
}

static void db_close(Dictionary dict)
{
	sqlite3 *db = dict->db_handle;
	if (db)
		sqlite3_close(db);

	dict->db_handle = NULL;
}

Dictionary dictionary_create_from_db(const char *lang)
{
	char *dbname;
	const char * t;
	Dictionary dict;

	dict = (Dictionary) malloc(sizeof(struct Dictionary_s));
	memset(dict, 0, sizeof(struct Dictionary_s));

	/* Language and file-name stuff */
	dict->string_set = string_set_create();
	t = strrchr (lang, '/');
	t = (NULL == t) ? lang : t+1;
	dict->lang = string_set_add(t, dict->string_set);
	lgdebug(D_USER_FILES, "Debug: Language: %s\n", dict->lang);

#if 0 /* FIXME: Spell checking should be done according to the dict locale. */
	/* To disable spell-checking, just set the checker to NULL */
	dict->spell_checker = spellcheck_create(dict->lang);
#endif
#if defined HAVE_HUNSPELL || defined HAVE_ASPELL
		/* FIXME: Move to spellcheck-*.c */
		if (verbosity_level(D_USER_BASIC) && (NULL == dict->spell_checker))
			prt_error("Info: %s: Spell checker disabled.\n", dict->lang);
#endif
	dict->base_knowledge = NULL;
	dict->hpsg_knowledge = NULL;

	dbname = join_path (lang, "dict.db");
	dict->name = string_set_add(dbname, dict->string_set);
	free(dbname);

	/* Set up the database */
	dict->db_handle = object_open(dict->name, db_open, NULL);

	dict->lookup_list = db_lookup_list;
	dict->lookup_wild = db_lookup_wild;
	dict->free_lookup = db_free_llist;
	dict->lookup = db_lookup;
	dict->close = db_close;
	condesc_init(dict, 1<<8);

	/* Setup the affix table */
	char *affix_name = join_path (lang, "4.0.affix");
	dict->affix_table = dictionary_six(lang, affix_name, NULL, NULL, NULL, NULL);
	if (dict->affix_table == NULL)
	{
		prt_error("Error: Could not open affix file %s\n", affix_name);
		free(affix_name);
		goto failure;
	}
	free(affix_name);
	if (!afdict_init(dict))
		goto failure;

	dictionary_setup_locale(dict);

	dictionary_setup_defines(dict);

	return dict;

failure:
	dictionary_delete(dict);
	return NULL;
}

#endif /* HAVE_SQLITE */
