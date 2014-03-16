
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

#include "utilities.h"

#include "read-sql.h"

typedef struct 
{
	dyn_str *ds;
	sqlite3 *db;
} bigstr;


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

static int morph_cb(void *user_data, int argc, char **argv, char **colName)
{
	bigstr* bs = user_data;

	assert(1 == argc, "Bad column count");
	assert(argv[0], "NULL column value");

	int i;
	for (i=0; i<argc; i++)
	{
printf("duude %s = %s\n", colName[i], argv[i] ? argv[i] : "NULL");
	}
printf("\n");

	return 0;
}

static int classname_cb(void *user_data, int argc, char **argv, char **colName)
{
	bigstr* bs = user_data;
	char *exec_err_msg;
	int rc;
	dyn_str *qry;

	assert(1 == argc, "Bad column count");
	assert(argv[0], "NULL column value");

	/* The classname field joins the two tables together. */
	qry = dyn_str_new();
	dyn_strcat(qry, "SELECT subscript FROM Morphemes WHERE classname = \'");
	dyn_strcat(qry, argv[0]);
	dyn_strcat(qry, "\';");

	rc = sqlite3_exec(bs->db, qry->str, morph_cb, bs, &exec_err_msg);
	if (SQLITE_OK != rc)
	{
		prt_error("Error: Failure reading classname %s: %s\n",
			argv[0], exec_err_msg);
	}
	dyn_str_delete(qry);

	return 0;
}

/**
 * This is a very cheap, cheesy hack to pull all of the data out of
 * of the database, and return it as one giant string.  We really
 * should not do this, and instead offer the interfaces to query
 * singles words/morphemes from the database, and return Exp's, just
 * like the file-backed dictionary.  However, right now, I'm just being
 * lazy, and doing the quick-n-easy, if somewhat wasteful thing, which
 * is to return a big fat string, and let the old code deal with it.
 */
char * get_db_contents(const char *dbname_)
{
	sqlite3 *db;
	int rc;
	char *slash, *dbn, *dbname;
	char *exec_err_msg;
	bigstr bs;
	bs.ds = dyn_str_new();

	/* XXX HACK ALERT: for right now, we're going to just mangle
	 * the dbname from 4.0.dict to dict.db. Some more elegant
	 * solution should be provided in the future.
	 */
	dbn = strdup(dbname_);
	slash = strrchr (dbn, '/'); /* DIR_SEPARATOR */
	if (slash) *slash = 0x0;
	dbname = join_path (dbn, "dict.db");
	free(dbn);

	/* Open the database */
	rc = sqlite3_open(dbname, &db);
	if (rc)
	{
		prt_error("Error: Can't open %s database: %s\n",
			dbname, sqlite3_errmsg(db));
		goto failure;
	}
	bs.db = db;

	/* The classname field joins the two tables together. */
	rc = sqlite3_exec(db,
		"SELECT DISTINCT classname FROM Morphemes;",
		 classname_cb, &bs, &exec_err_msg);
	if (SQLITE_OK != rc)
	{
		prt_error("Error: Failure reading %s: %s\n",
			dbname, exec_err_msg);
		goto failure;
	}

	/* Close the database */
failure:
	dyn_str_delete(bs.ds);
	free(dbname);
	sqlite3_close(db);
	return NULL;
}


#endif /* HAVE_SQLITE */
