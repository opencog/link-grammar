
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

char * get_db_contents(const char *dbname_)
{
	sqlite3 *db;
	int rc;
	char *slash, *dbn, *dbname;

	/* XXX HACK ALERT: for right now, we're going to just mangle
	 * the dbname from 4.0.dict to dict.db. Some more elegant
	 * solution should be provided in the future.
	 */
	dbn = strdup(dbname_);
	slash = strrchr (dbn, '/'); /* DIR_SEPARATOR */
	if (slash) *slash = 0x0;
	dbname = join_path (dbn, "dict.db");
	free(dbn);

	rc = sqlite3_open(dbname, &db);
	if (rc)
	{
		prt_error("Error: Can't open %s database: %s\n",
			dbname, sqlite3_errmsg(db));
		free(dbname);
		sqlite3_close(db);
		return NULL;
	}
	free(dbname);
	printf ("Hello world\n");


	sqlite3_close(db);
	return NULL;
}


#endif /* HAVE_SQLITE */
