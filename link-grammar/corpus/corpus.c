/*
 * corpus.c
 *
 * Data for corpus statistics, used to provide a parse ranking
 * to drive the SAT solver, as well as parse ranking with the
 * ordinary solver. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "corpus.h"

Corpus * lg_corpus_new(void)
{
	int rc;

	Corpus *c = (Corpus *) malloc(sizeof(Corpus));
	c->errmsg = NULL;

#if LATER
	c->dbname = "/home/linas/src/novamente/src/link-grammar/trunk/data/sql/disjuncts.db";

	rc = sqlite3_open(dbname, &c->dbconn);
	if (rc)
	{
		prt_err("Warning: Can't open database: %s\n", sqlite3_errmsg(c->dbconn));
		sqlite3_close(c->dbconn);
		c->dbconn = NULL;
	}
#endif

	return c;
}

void lg_corpus_delete(Corpus *c)
{
	if (c->dbconn)
	{
		sqlite3_close(c->dbconn);
		c->dbconn = NULL;
	}
	free(c);
}

double lg_corpus_score(Corpus *corp, Linkage_info *li)
{
printf ("hello world\n");
}

