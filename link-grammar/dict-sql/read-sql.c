
/*
 * read-sql.c
 *
 * Read in dictionary from an SQL DB.
 * Keeping it simple for just right now, and using SQLite.
 *
 * The goal of reading the dictioary from SQL is to enable some 
 * other process (machine-learning algo) to dynamically update
 * the dictionary.
 *
 * Copyright (c) 2014 Linas Vepstas <linasvepstas@gmail.com>
 */

#ifdef HAVE_SQLITE

#include <sqlite3.h>

#include "api-structures.h"
#include "dict-api.h"
#include "dict-common.h"
#include "dict-structures.h"
#include "spellcheck.h"
#include "string-set.h"
#include "structures.h"
#include "utilities.h"
#include "word-utils.h"

#include "read-sql.h"

/*


static int morph_cb(void *user_data, int argc, char **argv, char **colName)
{
	bigstr* bs = user_data;

	int i;
	for (i=0; i<argc; i++)
	{
printf("duude %s = %s\n", colName[i], argv[i] ? argv[i] : "NULL");
	}
printf("\n");

	return 0;
}
*/

/* ========================================================= */
/* Dictionary word lookup proceedures. */

typedef struct 
{
	Dictionary dict;
	Dict_node* dn;
	Boolean found;
	Exp* exp;
} cbdata;

static void db_free_llist(Dictionary dict, Dict_node *llist)
{
   Dict_node * dn;
   while (llist != NULL)
   {
      dn = llist->right;
// XXX uncomment later ...
		// xfree((char *)dn->exp, sizeof(Exp));
		xfree((char *)dn, sizeof(Dict_node));
      llist = dn;
   }
}

/* callback -- set bs->exp to the expressions for a class in the dict */
static int exp_cb(void *user_data, int argc, char **argv, char **colName)
{
	cbdata* bs = user_data;

	assert(2 == argc, "Bad column count");
	assert(argv[0], "NULL column value");

printf("duuude exph cb found disj %s cost %s\n", argv[0], argv[1]);
	return 0;
}

static void
db_lookup_exp(Dictionary dict, const char *s, cbdata* bs)
{
	sqlite3 *db = dict->db_handle;
	dyn_str *qry;

printf("duude look for wordexps for  %s\n", s);

	/* The token to look up is called the 'morpheme'. */
	qry = dyn_str_new();
	dyn_strcat(qry, "SELECT disjunct, cost FROM Disjuncts WHERE classname = \'");
	dyn_strcat(qry, s);
	dyn_strcat(qry, "\';");

	sqlite3_exec(db, qry->str, exp_cb, bs, NULL);
	dyn_str_delete(qry);
}


/* callback -- set bs->found to true if the word is in the dict */
static int exists_cb(void *user_data, int argc, char **argv, char **colName)
{
	cbdata* bs = user_data;

	assert(2 == argc, "Bad column count");
	assert(argv[0], "NULL column value");

printf("duuude boolean lookup found %s\n", argv[0]);
	bs->found = TRUE;
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

printf("duuude morph cb found word= %s clas %s\n", argv[0], argv[1]);

	/* Put each word into a Dict_node. */
	dn = (Dict_node *) xalloc(sizeof(Dict_node));
	dn->string = string_set_add(scriword, bs->dict->string_set);
	dn->right = bs->dn;
	bs->dn = dn;

	/* Now look up the expressions for each word */
	bs->exp = NULL;
	db_lookup_exp(bs->dict, wclass, bs);
	bs->dn->exp = bs->exp;

	return 0;
}


static void
db_lookup_common(Dictionary dict, const char *s,
                 int (*cb)(void *, int, char **, char **),
                 cbdata* bs)
{
	sqlite3 *db = dict->db_handle;
	dyn_str *qry;

printf("duude look for word  %s\n", s);

	/* The token to look up is called the 'morpheme'. */
	qry = dyn_str_new();
	dyn_strcat(qry, "SELECT subscript, classname FROM Morphemes WHERE morpheme = \'");
	dyn_strcat(qry, s);
	dyn_strcat(qry, "\';");

	sqlite3_exec(db, qry->str, cb, bs, NULL);
	dyn_str_delete(qry);
}

static Boolean db_lookup(Dictionary dict, const char *s)
{
	cbdata bs;
	bs.dict = dict;
	bs.found = FALSE;
	db_lookup_common(dict, s, exists_cb, &bs);
	return bs.found;
}

static Dict_node * db_lookup_list(Dictionary dict, const char *s)
{
	cbdata bs;
	bs.dict = dict;
	bs.dn = NULL;
	db_lookup_common(dict, s, morph_cb, &bs);
	return bs.dn;
}

/* ========================================================= */
/* Dictionary creation, setup, open proceedures */

Boolean check_db(const char *lang)
{
	char *dbname = join_path (lang, "dict.db");
	Boolean retval = file_exists(dbname);
	free(dbname);
	return retval;
}

static void db_setup(Dictionary dict)
{
	sqlite3 *db;
	int rc;

	/* Open the database */
	rc = sqlite3_open(dict->name, &db);
	if (rc)
	{
		prt_error("Error: Can't open %s database: %s\n",
			dict->name, sqlite3_errmsg(db));
		sqlite3_close(db);
		return;
	}

	dict->db_handle = db;
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
	Dict_node *dict_node;

	dict = (Dictionary) xalloc(sizeof(struct Dictionary_s));
	memset(dict, 0, sizeof(struct Dictionary_s));

	dict->version = NULL;
	dict->num_entries = 0;
	dict->affix_table = NULL;
	dict->regex_root = NULL;

	/* Language and file-name stuff */
	dict->string_set = string_set_create();
	dict->lang = lang;
	t = strrchr (lang, '/');
	if (t) dict->lang = string_set_add(t+1, dict->string_set);

	/* To disable spell-checking, just set the checker to NULL */
	dict->spell_checker = spellcheck_create(dict->lang);
	dict->postprocessor	 = NULL;
	dict->constituent_pp  = NULL;

	dbname = join_path (lang, "dict.db");
	dict->name = string_set_add(dbname, dict->string_set);
	free(dbname);

	/* Set up the database */
	db_setup(dict);

	dict->lookup_list = db_lookup_list;
	dict->free_lookup = db_free_llist;
	dict->lookup = db_lookup;
	dict->close = db_close;

	/* Misc remaining common (generic) dict setup work */
	dict->left_wall_defined  = boolean_dictionary_lookup(dict, LEFT_WALL_WORD);
	dict->right_wall_defined = boolean_dictionary_lookup(dict, RIGHT_WALL_WORD);

	dict->empty_word_defined = boolean_dictionary_lookup(dict, EMPTY_WORD_MARK);

	dict->unknown_word_defined = boolean_dictionary_lookup(dict, UNKNOWN_WORD);
	dict->use_unknown_word = TRUE;

	dict_node = dictionary_lookup_list(dict, UNLIMITED_CONNECTORS_WORD);
	if (dict_node != NULL) {
		dict->unlimited_connector_set = connector_set_create(dict_node->exp);
	} else {
		dict->unlimited_connector_set = NULL;
	}
	free_lookup_list(dict, dict_node);

	return dict;
}

#endif /* HAVE_SQLITE */
