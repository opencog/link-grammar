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

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

#include "../build-disjuncts.h"
#include "../externs.h"
#include "../disjunct-utils.h"
#include "../link-includes.h"
#include "../read-dict.h"
#include "../structures.h"

int main (int argc, char * argv[])
{
	sqlite3 *dbconn;

	const char * dbname = "../../data/sql/clusters.db";

	setlocale(LC_CTYPE, "en_US.UTF-8");

	/* Open the database for update */
	int rc = sqlite3_open_v2(dbname, &dbconn, SQLITE_OPEN_READWRITE, NULL);
	if (rc)
	{
		fprintf(stderr, "Error: cannot open the database\n");
		sqlite3_close(dbconn);
		exit(1);
	}

	/* ------------------------------------------------- */
	/* Prepare assorted queries */

	sqlite3_stmt *cluname_query;
	rc = sqlite3_prepare_v2(dbconn,
		"SELECT DISTINCT cluster_name FROM ClusterMembers;",
		-1, &cluname_query, NULL);

	if (rc != SQLITE_OK)
	{
		fprintf(stderr, "Error: cannot prepare cluster-name stmt\n");
		sqlite3_close(dbconn);
		exit(1);
	}

	/* ---- */
	sqlite3_stmt *cluword_query;
	rc = sqlite3_prepare_v2(dbconn,
		"SELECT inflected_word FROM ClusterMembers WHERE cluster_name = ?;",
		-1, &cluword_query, NULL);

	if (rc != SQLITE_OK)
	{
		fprintf(stderr, "Error: cannot prepare cluword select stmt\n");
		sqlite3_close(dbconn);
		exit(1);
	}

	/* ---- */
	sqlite3_stmt *cludelete;
	rc = sqlite3_prepare_v2(dbconn,
		"DELETE FROM ClusterMembers WHERE cluster_name = ?;",
		-1, &cludelete, NULL);

	if (rc != SQLITE_OK)
	{
		fprintf(stderr, "Error: cannot prepare cluster delete stmt\n");
		sqlite3_close(dbconn);
		exit(1);
	}

	/* ---- */
	sqlite3_stmt *djinsert;
	rc = sqlite3_prepare_v2(dbconn,
		"INSERT INTO ClusterDisjuncts (cluster_name, disjunct, cost) "
		"VALUES (?,?,?);",
		-1, &djinsert, NULL);

	if (rc != SQLITE_OK)
	{
		fprintf(stderr, "Error: cannot prepare dj insert stmt\n");
		sqlite3_close(dbconn);
		exit(1);
	}

	/* ------------------------------------------------- */
	/* Open the dictionary */
	Dictionary dict = dictionary_create_default_lang();
	if (NULL == dict)
	{
		fprintf(stderr, "Error: couldn't read the dict\n");
		exit(1);
	}

	/* Loop over all cluster names */
	int clu_cnt = 0;
	int rec_cnt = 0;
	int dj_cnt = 0;
	int wrd_cnt = 0;
	int warn_cnt = 0;
	int del_cnt = 0;
	while(1)
	{
		rc = sqlite3_step(cluname_query);
		if (rc != SQLITE_ROW) break;

		const char * cluname = sqlite3_column_text(cluname_query,0);

		Disjunct *dj_union = NULL;
		int do_keep = 0;

		printf("Info: Processing cluster %s\n", cluname);
		rc = sqlite3_bind_text(cluword_query, 1, cluname, -1, SQLITE_STATIC);
		while(1)
		{
			rc = sqlite3_step(cluword_query);
			if (rc != SQLITE_ROW) break;
			const char * cluword = sqlite3_column_text(cluword_query,0);

			Dict_node *dn = dictionary_lookup_list(dict, cluword);
			if (NULL == dn) continue;

			Disjunct *dj = build_disjuncts_for_dict_node(dn);
			if (strcmp(dj->string, cluword))
			{
				if (strncmp(dj->string, cluword, strlen(cluword)))
				{
					printf ("Error: asked for %s got %s\n", cluword, dj->string);
					// exit(1);
				}
				else
				{
					printf ("Warning: asked for %s got %s\n", cluword, dj->string);
				}
				warn_cnt++;
				free_disjuncts(dj);
				continue;
			}
			Disjunct *d = dj;
			int cnt = 0;
			while(d)
			{
				d->string = cluword;
				d = d->next;
				cnt ++;
			}
			dj_union = catenate_disjuncts(dj_union, dj);
			dj_union = eliminate_duplicate_disjuncts(dj_union);

			int ucnt = count_disjuncts(dj_union);
			if (cnt != ucnt) do_keep = 1;

			printf("\tprocessing word %s with %d disjuncts\n",
				cluword, cnt);
			wrd_cnt ++;
		}

		clu_cnt ++;

		if (do_keep)
		{
			int cnt = count_disjuncts(dj_union);
			printf("\tWill record %s of %d disjuncts\n", cluname, cnt);

			Disjunct *d = dj_union;
			while(d)
			{
				char * sdj = print_one_disjunct(d);
				// printf("duude %s  -- %s\n", cluname, sdj);
				
				double cost = d->cost;
				rc = sqlite3_bind_text(djinsert, 1, cluname, -1, SQLITE_STATIC);
				rc = sqlite3_bind_text(djinsert, 2, sdj, -1, SQLITE_STATIC);
				rc = sqlite3_bind_double(djinsert, 3, cost);

				rc = sqlite3_step(djinsert);
				if (rc != SQLITE_DONE)
				{
					printf("Error: unexpected return value %d on insert!\n", rc);
					exit(1);
				}
				sqlite3_reset(djinsert);
				sqlite3_clear_bindings(djinsert);
				free(sdj);
				d = d->next;
			}

			rec_cnt ++;
			dj_cnt += cnt;
		}
		else
		{
			printf("\tDeleting pointless cluster %s\n", cluname);
			rc = sqlite3_bind_text(cludelete, 1, cluname, -1, SQLITE_STATIC);
			rc = sqlite3_step(cludelete);
			if (rc != SQLITE_DONE)
			{
				printf("Error: unexpected return value %d on delete!\n", rc);
				exit(1);
			}
			sqlite3_reset(cludelete);
			sqlite3_clear_bindings(cludelete);
			del_cnt ++;
		}

		free_disjuncts(dj_union);
		sqlite3_reset(cluword_query);
		sqlite3_clear_bindings(cluword_query);
	}
	sqlite3_reset(cluname_query);
	sqlite3_clear_bindings(cluname_query);

	sqlite3_close(dbconn);

	dictionary_delete(dict);

	printf("Examined %d clusters, recorded %d\n", clu_cnt, rec_cnt);
	printf("Examined %d words, and %d disjuncts\n", wrd_cnt, dj_cnt);
	float avg_wrd = ((float) wrd_cnt) / clu_cnt;
	float avg_dj = ((float) dj_cnt) / rec_cnt;
	printf("Average %f words/cluster; average %f dj's/recored-cluster\n", 
		avg_wrd, avg_dj);

	printf("Got %d mismatch warnings\n", warn_cnt);
	printf("Deleted %d pointless clusters\n", del_cnt);

	return 0;
}
