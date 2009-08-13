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
#include <stdlib.h>
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
		exit(1);
	}

	sqlite3_stmt *cluname_query;
	rc = sqlite3_prepare_v2(dbconn,
		"SELECT DISTINCT cluster_name FROM ClusterMembers;",
		-1, &cluname_query, NULL);

	if (rc != SQLITE_OK)
	{
		fprintf(stderr, "Error: cannot prepare first stmt\n");
		sqlite3_close(dbconn);
		exit(1);
	}

	while(1)
	{
		rc = sqlite3_step(cluname_query);
		if (rc != SQLITE_ROW) break;

		const char * cluname = sqlite3_column_text(cluname_query,0);

printf("got on %s\n", cluname);
	}

	sqlite3_close(dbconn);
	return 0;
}
