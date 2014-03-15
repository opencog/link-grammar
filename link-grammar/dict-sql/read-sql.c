
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

static int morph_cb(void *user_data, int argc, char **argv, char **colName)
{
	sqlite3* db = user_data;

	int i;
	for (i=0; i<argc; i++)
	{
printf("duude %s = %s\n", colName[i], argv[i] ? argv[i] : "NULL");
	}
printf("\n");
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
	printf ("Hello world\n");

	rc = sqlite3_exec(db, "SELECT * FROM Morphemes;", morph_cb, db, &exec_err_msg);
	if (SQLITE_OK != rc)
	{
		prt_error("Error: Failure reading %s: %s\n",
			dbname, exec_err_msg);
		goto failure;
	}

	/* Close the database */
failure:
	free(dbname);
	sqlite3_close(db);
	return NULL;
}


#endif /* HAVE_SQLITE */
