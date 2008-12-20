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
#include "../utilities.h"

typedef struct 
{
	double score;
	int found;
} DJscore;

Corpus * lg_corpus_new(void)
{
	char * dbname;
	int rc;

	Corpus *c = (Corpus *) malloc(sizeof(Corpus));
	c->errmsg = NULL;

	dbname = "/home/linas/src/novamente/src/link-grammar/trunk/data/sql/disjuncts.db";

	rc = sqlite3_open(dbname, &c->dbconn);
	if (rc)
	{
		prt_error("Warning: Can't open database: %s\n", sqlite3_errmsg(c->dbconn));
		sqlite3_close(c->dbconn);
		c->dbconn = NULL;
	}

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

/* ========================================================= */

/* LOW_SCORE is what is assumed if a disjunct-word pair is not found
 * in the dictionary. It is meant to be -log_2(prob(d|w)) where
 * prob(d|w) is the conditional probability of seeing the disjunct d
 * given the word w. A value of 17 is about equal to 1 in 100,000.
 */
#define LOW_SCORE 17.0

static int prob_cb(void *user_data, int argc, char **argv, char **colname)
{
	DJscore *score = (DJscore *) user_data;

	if (1 < argc)
	{
		prt_error("Error: sql table entry not unique\n");
	}
	score->found = 1;
	score->score = atof(argv[0]);
	return 0;
}

double lg_corpus_score(Corpus *corp, Sentence sent)
{
	char djstr[MAX_TOKEN_LENGTH*20]; /* no word will have more than 20 links */
	char querystr[MAX_TOKEN_LENGTH*25];
	size_t copied, left;
	int i, w;
	int nwords = sent->length;
	Parse_info pi = sent->parse_info;
	int nlinks = pi->N_links;
	int *djlist, *djloco, *djcount;
	const char *infword;
	DJscore score;
	int rc;
	char *errmsg;
	double tot_score = 0.0;

	if (NULL == corp->dbconn) return 0.0;

	djcount = (int *) malloc (sizeof(int) * (nwords + 2*nwords*nlinks));
	djlist = djcount + nwords;
	djloco = djlist + nwords*nlinks;

	/* Decrement nwords, so as to ignore the RIGHT-WALL */
	nwords --;

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

		/* Skip over RW link to the right wall */
		if (nwords <= rword) continue;

		djlist[lword*nlinks + slot] = i;
      djloco[lword*nlinks + slot] = rword;
		djcount[lword] ++;

		slot = djcount[rword];
		djlist[rword*nlinks + slot] = i;
      djloco[rword*nlinks + slot] = lword;
		djcount[rword] ++;

#ifdef DEBUG
		printf("Link: %d is %s--%s--%s\n", i, 
			sent->word[lword].string, pi->link_array[i].name,
			sent->word[rword].string);
#endif
	}

	/* Process each word in the sentence (skipping LEFT-WALL, which is
	 * word 0. */
	for (w=1; w<nwords; w++)
	{
		/* Sort the disjuncts for this word. -- bubble sort */
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
		left = sizeof(djstr);
		copied = 0;
		for (i=0; i<slot; i++)
		{
			int dj = djlist[w*nlinks + i];
			copied += strlcpy(djstr+copied, pi->link_array[dj].name, left);
			left = sizeof(djstr) - copied;
			if (djloco[w*nlinks + i] < w)
				copied += strlcpy(djstr+copied, "-", left--);
			else
				copied += strlcpy(djstr+copied, "+", left--);
			copied += strlcpy(djstr+copied, " ", left--);
		}

		if (sent->word[w].d)
			infword = sent->word[w].d->string;
		else
			infword = sent->word[w].string;

		/* Look up the disjunct in the database */
		left = sizeof(querystr);
		copied = strlcpy(querystr,
			"SELECT log_cond_probability FROM Disjuncts WHERE inflected_word = '",
			left);
		left = sizeof(querystr) - copied;
		copied += strlcpy(querystr + copied, infword, left);
		left = sizeof(querystr) - copied;
		copied += strlcpy(querystr + copied, "' AND disjunct = '", left);
		left = sizeof(querystr) - copied;
		copied += strlcpy(querystr + copied, djstr, left);
		left = sizeof(querystr) - copied;
		strlcpy(querystr + copied, "';", left);

		score.found = 0;
		rc = sqlite3_exec(corp->dbconn, querystr, prob_cb, &score, &errmsg);
		if (rc != SQLITE_OK)
		{
			prt_error("Error: SQLite: %s\n", errmsg);
			sqlite3_free(errmsg);
		}

		/* Total up the score */
		if (score.found)
		{
			printf ("Word=%s dj=%s score=%f\n", infword, djstr, score.score);
			tot_score += score.score;
		}
		else
		{
			printf ("Word=%s dj=%s not found in dict, assume score=%f\n",
				infword, djstr, LOW_SCORE);
			tot_score += LOW_SCORE;
		}
	}

	free (djcount);

	tot_score /= nwords;
	return tot_score;
}

