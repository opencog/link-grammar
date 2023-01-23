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
 * Copyright (c) 2014,2021 Linas Vepstas <linasvepstas@gmail.com>
 */

#ifdef HAVE_SQLITE3

#define D_SQL 5 /* Verbosity levels for this module are 5 and 6. */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sqlite3.h>

#include "api-structures.h"
#include "connectors.h"
#include "dict-common/dict-affix-impl.h"
#include "dict-common/dict-api.h"
#include "dict-common/dict-common.h"
#include "dict-common/dict-internals.h"
#include "dict-common/dict-locale.h"
#include "dict-common/dict-structures.h"
#include "dict-common/dict-utils.h"      // patch_subscript()
#include "dict-common/file-utils.h"
#include "dict-file/read-dict.h"         // dictionary_six()
#include "error.h"
#include "externs.h"
#include "memory-pool.h"
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
	while (*p && (lg_isspace((unsigned char)*p))) p++;
	if (0 == *p) return p;

	/* If it's an open paren, assume its the beginning of a new list */
	if ('(' == *p)
	{
		p = make_expression(dict, ++p, pex);
	}
	else
	{
		/* Search for the end of a connector */
		const char * con_start = p;
		while (*p && (isalnum((unsigned char)*p) || '*' == *p)) p++;

		/* Connectors always end with a + or - */
		assert (('+' == *p) || ('-' == *p),
				"Missing direction character in connector string: %s", con_start);

		/* Create an expression to hold the connector */
		char * constr = NULL;
		bool multi = false;
		if ('@' == *con_start)
		{
			constr = strndupa(con_start+1, p-con_start-1);
			multi = true;
		}
		else
			constr = strndupa(con_start, p-con_start);

		Exp* e = make_connector_node(dict, dict->Exp_pool, constr, *p, multi);

		*pex = e;
	}

	/* Is there any more? If not, return what we've got. */
	p++;
	while (*p && (lg_isspace((unsigned char)*p))) p++;
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
	else
	{
		assert(false, "Bad rest of expression %s", exp_str);
	}

	Exp* rest = NULL;
	p = make_expression(dict, p, &rest);
	assert(NULL != rest, "Badly formed expression %s", exp_str);

	/* Join it all together. */
	Exp* join = make_join_node(dict->Exp_pool, *pex, rest, etype);

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
	int count;
	Exp* exp;
	char* classname;
} cbdata;

/* callback -- set bs->exp to the expressions for a class in the dict */
static int exp_cb(void *user_data, int argc, char **argv, char **colName)
{
	cbdata* bs = user_data;
	Dictionary dict = bs->dict;

	assert(2 == argc, "Bad column count");
	assert(argv[0], "NULL column value");

	Exp* exp = NULL;
	make_expression(dict, argv[0], &exp);
	assert(NULL != exp, "Failed expression %s", argv[0]);

	if (!strtofC(argv[1], &exp->cost))
	{
		prt_error("Warning: Invalid cost \"%s\" in expression \"%s\" "
		          "(using 1.0)\n", argv[1], argv[0]);
		exp->cost = 1.0;
	}

	/* If the very first expression, just put it in place */
	if (NULL == bs->exp)
	{
		bs->exp = exp;
		return 0;
	}

	/* If the second expression, OR-it with the existing expression. */
	if (OR_type != bs->exp->type)
	{
		bs->exp = make_or_node(dict->Exp_pool, exp, bs->exp);
		return 0;
	}

	/* Extend the OR-chain for the third and later expressions. */
	exp->operand_next = bs->exp->operand_first;
	bs->exp->operand_first = exp;

	return 0;
}

/* Escape single-quotes.  That is, replace single-quotes by
 * two single-quotes. e.g. don't --> don''t */
static char * escape_quotes(const char * s)
{
	char * q = strchr(s, '\'');
	if (NULL == q) return (char *) s;

	/* If there are two in a row, already, assume that they are
	 * already escaped. This is pathological, and a sign that
	 * something somewhere else has already gone wrong. But I'm
	 * exhausted debugging right now and will punt on this.
	 */
	if ('\'' == *(q+1)) return (char *) s;

	char * es = malloc(2 * strlen(s) + 1);
	char * p = es;
	while (q)
	{
		strncpy(p, s, q-s+1);
		p += q-s+1;
		*p = '\'';
		p++;
		s = q+1;
		q = strchr(s, '\'');
	}
	strcpy(p, s);
	return es;
}

static void
db_lookup_exp(Dictionary dict, const char *s, cbdata* bs)
{
	sqlite3 *db = dict->db_handle;
	dyn_str *qry;

	/* Escape single-quotes.  That is, replace single-quotes by
	 * two single-quotes. e.g. don't --> don''t */
	char * es = escape_quotes(s);

	/* The token to look up is called the 'morpheme'. */
	qry = dyn_str_new();
	dyn_strcat(qry, "SELECT disjunct, cost FROM Disjuncts WHERE classname = \'");
	dyn_strcat(qry, es);
	dyn_strcat(qry, "\';");

	sqlite3_exec(db, qry->str, exp_cb, bs, NULL);
	dyn_str_delete(qry);

	if (es != s) free(es);

	lgdebug(D_SQL+1, "Found expression for class %s: %s\n",
	        s, exp_stringify(bs->exp));
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
	assert(2 == argc, "Bad column count");
	assert(argv[0], "NULL column value");
	char * scriword = argv[0];
	char * wclass = argv[1];

	/* Now look up the expressions for each word */
	cbdata* bs = user_data;
	bs->exp = NULL;
	db_lookup_exp(bs->dict, wclass, bs);

	/* Well, if we found a classname for a word, then there really,
	 * really should be able to find one or more corresponding disjuncts.
	 * However, it is possible to have corrupted databases which do not
	 * have any disjuncts for a word class.  We complain about those.
	 */
	assert(NULL != bs->exp, "Missing disjuncts for word %s %s",
		scriword, wclass);

	/* Put each word into a Dict_node. */
	Dict_node *dn = dict_node_new();
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

	/* Escape single-quotes.  That is, replace single-quotes by
	 * two single-quotes. e.g. don't --> don''t */
	char * es = escape_quotes(s);

	/* The token to look up is called the 'morpheme'. */
	qry = dyn_str_new();
	dyn_strcat(qry, "SELECT subscript, classname FROM Morphemes WHERE morpheme ");
	dyn_strcat(qry, equals);
	dyn_strcat(qry, " \'");
	dyn_strcat(qry, es);
	dyn_strcat(qry, "\';");

	if (es != s) free(es);

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
	if (verbosity_level(D_SQL))
	{
		if (bs.dn)
		{
			printf("Found expression for word %s: %s\n",
	        s, exp_stringify(bs.dn->exp));
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
	if (verbosity_level(D_SQL))
	{
		if (bs.dn)
		{
			printf("Found expression for glob %s: %s\n",
			       s, exp_stringify(bs.dn->exp));
		}
		else
		{
			printf("No expression for glob %s\n", s);
		}
	}
	return bs.dn;
}

/* ========================================================= */
/* Callbacks and functions to support lexical category loading. */

/* Used for `SELECT count(*) FROM foo` type of queries */
static int count_cb(void *user_data, int argc, char **argv, char **colName)
{
	cbdata* bs = user_data;

	assert(1 == argc, "Bad column count");
	bs->count = atol(argv[0]);

	return 0;
}

/* Record the name of each lexical class */
static int classname_cb(void *user_data, int argc, char **argv, char **colName)
{
	cbdata* bs = user_data;
	Dictionary dict = bs->dict;

	/* Assuming here that a class name of a wall is the same as the wall
	 * name, and a class name of a macro is in macro format. */
	if (!dict->generate_walls && is_wall(argv[0])) return 0;
	if (is_macro(argv[0])) return 0;

	/* Add a category. */
	/* This is intentionally off-by-one, per design. */
	dict->num_categories++;
	dict->category[dict->num_categories].num_words = 0;
	dict->category[dict->num_categories].word = NULL;
	char* esc = escape_quotes(argv[0]);
	dict->category[dict->num_categories].name =
		string_set_add(esc, dict->string_set);
	if (esc != argv[0]) free(esc);

	char category_string[16];     /* For the tokenizer - not used here */
	snprintf(category_string, sizeof(category_string), " %x",
	         dict->num_categories); /* ' ': See comment in build_disjuncts() */
	string_set_add(category_string, dict->string_set);

	return 0;
}

/* Record the words in each lexical class */
static int classword_cb(void *user_data, int argc, char **argv, char **colName)
{
	cbdata* bs = user_data;
	Dictionary dict = bs->dict;

	char *word = strdupa(argv[0]);
	patch_subscript(word);

	/* Add the word. */
	dict->category[dict->num_categories].word[bs->count] =
		string_set_add(word, dict->string_set);
	bs->count++;

	return 0;
}

/* The current design for generation requires that all word categories
 * be loaded into RAM before generation starts. This is required because
 * a wild-card appearing in the generator forces a loop over all
 * categories, so we may as well have them instantly available.
 */
static void db_add_categories(Dictionary dict)
{
	sqlite3 *db = dict->db_handle;
	cbdata bs;
	bs.dict = dict;

	/* How many lexical categories are there? Find out. */
	sqlite3_exec(db, "SELECT count(DISTINCT classname) FROM Disjuncts;",
		count_cb, &bs, NULL);

	dict->num_categories = 0;
	dict->num_categories_alloced = 1 + bs.count + 1; // skip slot 0 + terminator
	dict->category = malloc(dict->num_categories_alloced *
	                        sizeof(*dict->category));

	sqlite3_exec(db, "SELECT DISTINCT classname FROM Disjuncts;",
		classname_cb, &bs, NULL);

	/* Category 0 is unused, intentionally. Not sure why. */
	unsigned int ncat = dict->num_categories;
	for (unsigned int i=1; i<=ncat; i++)
	{
		/* For each category, get the expression. */
		dyn_str *qry = dyn_str_new();
		dyn_strcat(qry,
			"SELECT disjunct, cost FROM Disjuncts WHERE classname = \'");
		dyn_strcat(qry, dict->category[i].name);
		dyn_strcat(qry, "\';");

		bs.exp = NULL;
		sqlite3_exec(db, qry->str, exp_cb, &bs, NULL);
		dyn_str_delete(qry);

		dict->category[i].exp = bs.exp;

		/* ------------------ */
		/* For each category, get the number of words in the category */
		qry = dyn_str_new();
		dyn_strcat(qry,
			"SELECT count(*) FROM Morphemes WHERE classname = \'");
		dyn_strcat(qry, dict->category[i].name);
		dyn_strcat(qry, "\';");

		sqlite3_exec(db, qry->str, count_cb, &bs, NULL);
		dyn_str_delete(qry);

		dict->category[i].num_words = bs.count;
		dict->category[i].word =
			malloc(bs.count * sizeof(*dict->category[0].word));

		/* ------------------ */
		/* For each category, get the (subscripted) words in the category */
		qry = dyn_str_new();
		dyn_strcat(qry,
			"SELECT subscript FROM Morphemes WHERE classname = \'");
		dyn_strcat(qry, dict->category[i].name);
		dyn_strcat(qry, "\';");

		dict->num_categories = i;
		bs.count = 0;
		sqlite3_exec(db, qry->str, classword_cb, &bs, NULL);
		dyn_str_delete(qry);
	}

	/* Set the termination entry. */
	dict->category[dict->num_categories + 1].num_words = 0;
}

/* ========================================================= */
/* Dictionary creation, setup, open procedures */

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

static void db_start_lookup(Dictionary dict, Sentence sent)
{
}

static void db_end_lookup(Dictionary dict, Sentence sent)
{
	condesc_setup(dict);
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
	dict->free_lookup = dict_node_free_lookup;
	dict->exists_lookup = db_lookup;
	dict->start_lookup = db_start_lookup;
	dict->end_lookup = db_end_lookup;
	dict->clear_cache = dict_node_noop;
	dict->close = db_close;

	dict->dynamic_lookup = true;
	condesc_init(dict, 1<<8);

	dict->dfine.set = string_id_create();

	dict->Exp_pool = pool_new(__func__, "Exp", /*num_elements*/4096,
	                          sizeof(Exp), /*zero_out*/false,
	                          /*align*/false, /*exact*/false);

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

	if (!dictionary_setup_defines(dict))
		goto failure;

	/* Initialize word categories, for text generation. */
	if (dictionary_generation_request(dict))
		db_add_categories(dict);

	return dict;

failure:
	dictionary_delete(dict);
	return NULL;
}

#endif /* HAVE_SQLITE3 */
