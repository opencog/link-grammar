/*
 * cluster.c
 *
 * Data for related-word clusters. Meant to expand disjunct covereage
 * for the case where a parse cannot be completed without ommitting
 * a word.
 *
 * Copyright (c) 2009 Linas Vepstas <linasvepstas@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "cluster.h"

struct cluster_s
{
	char * dbname;
	sqlite3 *dbconn;
	sqlite3_stmt *clu_query;
	sqlite3_stmt *dj_query;
	char *errmsg;
	int rc;
};

/* ========================================================= */

static void * db_file_open(const char * dbname, void * user_data)
{
	Cluster *c = (Cluster *) user_data;
	int rc;
	sqlite3 *dbconn;
	c->rc = sqlite3_open_v2(dbname, &dbconn, SQLITE_OPEN_READONLY, NULL);
	if (c->rc)
	{
		sqlite3_close(dbconn);
		return NULL;
	}

	c->dbname = strdup(dbname);
	return dbconn;
}


/**
 * Initialize the cluster statistics subsystem.
 */
Corpus * lg_cluster_new(void)
{
	int rc;

	Corpus *c = (Corpus *) malloc(sizeof(Corpus));
	c->clu_query = NULL;
	c->dj_query = NULL;
	c->errmsg = NULL;
	c->dbname = NULL;

	/* dbname = "/link-grammar/data/en/sql/clusters.db"; */
#define DBNAME "sql/clusters.db"
	c->dbconn = object_open(DBNAME, db_file_open, c);
	if (NULL == c->dbconn)
	{
		/* Very weird .. but if the database is not found, then sqlite
		 * reports an "out of memory" error! So hide this misleading
		 * error message.
		 */
		if (SQLITE_CANTOPEN == c->rc)
		{
			prt_error("Warning: Can't open database: File not found\n"
			          "\tWas looking for: " DBNAME);
		}
		else
		{ 
			prt_error("Warning: Can't open database: %s\n"
			          "\tWas looking for: " DBNAME,
				sqlite3_errmsg(c->dbconn));
		}
		return c;
	}

	/* Now prepare the statements we plan to use */
	rc = sqlite3_prepare_v2(c->dbconn, 	
		"SELECT cluster_name FROM ClusterMembers "
		"WHERE inflected_word = ?;",
		-1, &c->clu_query, NULL);
	if (rc != SQLITE_OK)
	{
		prt_error("Error: Can't prepare the cluster member statment: %s\n",
			sqlite3_errmsg(c->dbconn));
	}

	rc = sqlite3_prepare_v2(c->dbconn, 	
		"SELECT disjunct, cost FROM ClusterDisjuncts "
		"WHERE cluster_name = ?;",
		-1, &c->dj_query, NULL);
	if (rc != SQLITE_OK)
	{
		prt_error("Error: Can't prepare the disjunct statment: %s\n",
			sqlite3_errmsg(c->dbconn));
	}

	prt_error("Info: Cluster grouping database found at %s\n", c->dbname);
	return c;
}

/**
 * lg_cluster_delete -- shut down the cluster statistics subsystem.
 */ 
void lg_cluster_delete(Corpus *c)
{
	if (NULL == c) return;

	if (c->clu_query)
	{
		sqlite3_finalize(c->clu_query);
		c->clu_query = NULL;
	}

	if (c->dj_query)
	{
		sqlite3_finalize(c->dj_query);
		c->dj_query = NULL;
	}

	if (c->dbconn)
	{
		sqlite3_close(c->dbconn);
		c->dbconn = NULL;
	}

	if (c->dbname)
	{
		free(c->dbname);
		c->dbname = NULL;
	}
	free(c);
}

/* ========================================================= */


/* ======================= END OF FILE ===================== */
