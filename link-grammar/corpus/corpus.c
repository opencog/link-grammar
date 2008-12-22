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

Corpus * lg_corpus_new(void)
{
	char * dbname;
	int rc;

	Corpus *c = (Corpus *) malloc(sizeof(Corpus));
	c->rank_query = NULL;
	c->sense_query = NULL;
	c->errmsg = NULL;

	dbname = "/home/linas/src/novamente/src/link-grammar/trunk/data/sql/disjuncts.db";

	rc = sqlite3_open(dbname, &c->dbconn);
	if (rc)
	{
		prt_error("Warning: Can't open database: %s\n",
			sqlite3_errmsg(c->dbconn));
		sqlite3_close(c->dbconn);
		c->dbconn = NULL;
		return c;
	}

	/* Now prepare the statements we plan to use */
	rc = sqlite3_prepare_v2(c->dbconn, 	
		"SELECT log_cond_probability FROM Disjuncts "
		"WHERE inflected_word = ? AND disjunct = ?;",
		-1, &c->rank_query, NULL);
	if (rc != SQLITE_OK)
	{
		prt_error("Error: Can't prepare the ranking statment: %s\n",
			sqlite3_errmsg(c->dbconn));
	}

	rc = sqlite3_prepare_v2(c->dbconn, 	
		"SELECT word_sense, log_cond_probability FROM DisjunctSenses "
		"WHERE inflected_word = ? AND disjunct = ?;",
		-1, &c->sense_query, NULL);
	if (rc != SQLITE_OK)
	{
		prt_error("Error: Can't prepare the sense statment: %s\n",
			sqlite3_errmsg(c->dbconn));
	}

	return c;
}

void lg_corpus_delete(Corpus *c)
{
	if (c->rank_query)
	{
		sqlite3_finalize(c->rank_query);
		c->rank_query = NULL;
	}

	if (c->sense_query)
	{
		sqlite3_finalize(c->sense_query);
		c->sense_query = NULL;
	}

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

static double get_disjunct_score(Corpus *corp,
                                 const char * inflected_word,
                                 const char * disjunct)
{
	double val;
	int rc;

	/* Look up the disjunct in the database */
	rc = sqlite3_bind_text(corp->rank_query, 1,
		inflected_word, -1, SQLITE_STATIC);
	if (rc != SQLITE_OK)
	{
		prt_error("Error: SQLite can't bind word: rc=%d \n", rc);
		return LOW_SCORE;
	}

	rc = sqlite3_bind_text(corp->rank_query, 2,
		disjunct, -1, SQLITE_STATIC);
	if (rc != SQLITE_OK)
	{
		prt_error("Error: SQLite can't bind disjunct: rc=%d \n", rc);
		return LOW_SCORE;
	}

	rc = sqlite3_step(corp->rank_query);
	if (rc != SQLITE_ROW)
	{
		val = LOW_SCORE;
#ifdef DEBUG
		printf ("Word=%s dj=%s not found in dict, assume score=%f\n",
			inflected_word, disjunct, val);
#endif
	}
	else
	{
		val = sqlite3_column_double(corp->rank_query, 0);
#ifdef DEBUG
		printf ("Word=%s dj=%s score=%f\n", inflected_word, disjunct, val);
#endif
	}

	/* Failure to do both a reset *and* a clear will cause subsequent
	 * binds tp fail. */
	sqlite3_reset(corp->rank_query);
	sqlite3_clear_bindings(corp->rank_query);
	return val;
}

/* ========================================================= */

void lg_corpus_senses(Corpus *corp,
                      const char * inflected_word,
                      const char * disjunct)
{
	double log_prob;
	const unsigned char *sense;
	int rc;

	/* Look up the disjunct in the database */
	rc = sqlite3_bind_text(corp->sense_query, 1,
		inflected_word, -1, SQLITE_STATIC);
	if (rc != SQLITE_OK)
	{
		prt_error("Error: SQLite can't bind word in sense query: rc=%d \n", rc);
		return;
	}

	rc = sqlite3_bind_text(corp->sense_query, 2,
		disjunct, -1, SQLITE_STATIC);
	if (rc != SQLITE_OK)
	{
		prt_error("Error: SQLite can't bind disjunct in sense query: rc=%d \n", rc);
		return;
	}

	rc = sqlite3_step(corp->sense_query);
	if (rc != SQLITE_ROW)
	{
		// printf ("Word=%s dj=%s not found in sense dict\n",
		//	inflected_word, disjunct);
	}
	else
	{
		sense = sqlite3_column_text(corp->sense_query, 0);
		log_prob = sqlite3_column_double(corp->sense_query, 1);
		printf ("Word=%s dj=%s sense=%s score=%f\n", 
			inflected_word, disjunct, sense, log_prob);
	}

	/* Failure to do both a reset *and* a clear will cause subsequent
	 * binds tp fail. */
	sqlite3_reset(corp->sense_query);
	sqlite3_clear_bindings(corp->sense_query);
}

/* ========================================================= */

void lg_corpus_disjuncts(Corpus *corp, Sentence sent, Linkage_info *lifo)
{
	char djstr[MAX_TOKEN_LENGTH*20]; /* no word will have more than 20 links */
	size_t copied, left;
	int i, w;
	int nwords = sent->length;
	Parse_info pi = sent->parse_info;
	int nlinks = pi->N_links;
	int *djlist, *djloco, *djcount;

	if (NULL == corp->dbconn) return;

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

		lifo->disjunct_list_str[w] = strdup(djstr);
	}

	free (djcount);
}

void lg_corpus_score(Corpus *corp, Sentence sent, Linkage_info *lifo)
{
	const char * infword;
	double tot_score = 0.0f;
	int nwords = sent->length;
	int w;

	/* Decrement nwords, so as to ignore the RIGHT-WALL */
	nwords --;

	/* Loop over each word in the sentence (skipping LEFT-WALL, which is
	 * word 0. */
	for (w=1; w<nwords; w++)
	{
		/* If the word is not inflected, then sent->word[w].d is NULL */
		if (sent->word[w].d)
			infword = sent->word[w].d->string;
		else
			infword = sent->word[w].string;

		tot_score += get_disjunct_score(corp, infword, lifo->disjunct_list_str[w]);
	}

	/* Decrement nwords, so as to ignore the LEFT-WALL */
	--nwords;
	tot_score /= nwords;
	lifo->corpus_cost = tot_score;
}
