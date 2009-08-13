/*
 * cluster-pop.c
 *
 * Populate the cluster database with a precomputed collection of
 * disjuncts.  This is a stand-alone program that is used to generate
 * the cluster-disjunct table, which is used only as a lookup table
 * during parsing.
 *
 * Copyright (c) 2009 Linas Vepstas <linasvepstas@gmail.com>
 */

#include <stdio.h>
#include <sqlite3.h>


int main (int argc, char * argv[])
{
	sqlite3 *dbconn;

	const char * dbname = "../../data/sql/clusters.db";

	int rc = sqlite3_open_v2(dbname, &dbconn, SQLITE_OPEN_READWRITE, NULL);
	if (rc)
	{
		fprintf(stderr, "Error: cannot open the database\n");
		sqlite3_close(dbconn);
	}
#if 0

#endif

	sqlite3_close(dbconn);
	return 0;
}
