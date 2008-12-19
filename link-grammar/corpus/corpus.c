/*
 * corpus.c
 *
 * Data for corpus statistics, used to provide a parse ranking
 * to drive the SAT solver, as well as parse ranking with the
 * ordinary solver. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "corpus.h"
#include "../api-structures.h"

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

double lg_corpus_score(Corpus *corp, Sentence sent, Linkage_info *li)
{
	char djstr[MAX_TOKEN_LENGTH*20]; /* no word will have more than 20 links */
	int i, w;
	int nwords = sent->length;
	Parse_info pi = sent->parse_info;
	int nlinks = pi->N_links;
	int *djlist, *djloco, *djcount;

	djcount = (int *) malloc (sizeof(int) * (nwords + 2*nwords*nlinks));
	djlist = djcount + nwords;
	djloco = djlist + nwords*nlinks;

	for (w=0; w<nwords; w++)
	{
		djcount[w] = 0;
	}

	/* Create a table of disjuncts for each word. */
	for (i=0; i<nlinks; i++)
	{
		int lword = pi->link_array[i].l;
		int rword = pi->link_array[i].r;
		int slot = djcount[lword];
		djlist[lword*nlinks + slot] = i;
      djloco[lword*nlinks + slot] = rword;
		djcount[lword] ++;

		slot = djcount[rword];
		djlist[rword*nlinks + slot] = i;
      djloco[rword*nlinks + slot] = lword;
		djcount[rword] ++;
	}

	/* Sort the table of disjuncts, left to right */
	for (w=0; w<nwords; w++)
	{
		/* sort the disjuncts for this word. -- bubble sort */
		int slot = djcount[w];
		for (i=0; i<slot; i++)
		{
			int j;
			for (j=i+1; j<slot; j++)
			{
				if (djloco[w*nlinks + i] > djloco[w*nlinks + j])
				{
					int tmp = djloco[w*nlinks + i];
					djloco[w*nlinks + i] = djloco[w*nlinks + j];
					djloco[w*nlinks + j] = tmp;
					tmp = djlist[w*nlinks + i];
					djlist[w*nlinks + i] = djlist[w*nlinks + j];
					djlist[w*nlinks + j] = tmp;
				}
			}
		}

		/* Create the disjunct string */
		djstr[0] = 0;
		for (i=0; i<slot; i++)
		{
			strcat(djstr, pi->link_array[i].name);
			if (djloco[w*nlinks + i] < w)
				strcat(djstr, "-");
			else
				strcat(djstr, "+");
			strcat(djstr, " ");
		}
		printf ("dduuude wd=%s  dj=%s\n", sent->word[w].d->string, djstr);
	}
#if 0
	int i;

	for (i=0; i<sent->length; i++)
	{
		printf("%s ", sent->word[i].d->string);
	}
	printf ("\n");

	for (i=0; i<nlinks; i++)
	{
		int lword = pi->link_array[i].l;
		int rword = pi->link_array[i].r;
		printf("duuude %d is %s-%s-%s\n", i, 
sent->word[l].string,
pi->link_array[i].name,
sent->word[r].string);

	}
linkage_get_num_links

linkage_get_link_lword

linkage->sublinkage[linkage->current].link[index];
#endif

}

