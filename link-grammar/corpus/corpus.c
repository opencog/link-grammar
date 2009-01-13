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

/* ========================================================= */

/**
 * Initialize the corpus statistics subsystem.
 */
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

	prt_error("Info: Statistics database opened\n");
	return c;
}

/**
 * lg_corpus_delete -- shut down the corpus statistics subsystem.
 */ 
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

/**
 * get_disjunct_score -- get log probability of observing disjunt.
 *
 * Given an "inflected" word and a disjunct, thris routine returns the
 * -log_2 conditional probability prob(d|w) of seeing the disjunct 'd'
 * given that the word 'w' was observed.  Here, "inflected word" means
 * the link-grammar dictionary entry, complete with its trailing period
 * and tag -- e.g. run.v or running.g -- everything after the dot is the
 * "inflection".
 */
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

/**
 * lg_corpus_senses -- Given word and disjunct, look up senses.
 */
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

/**
 * lg_corpus_score -- compute parse-ranking score for sentence.
 *
 * Given a parsed sentence, this routine will compute a parse ranking
 * score, based on the probabilites of observing the indicated set of
 * disjuncts in the statistics database.
 *
 * The score is stored in the Linkage_info->corpus_cost struct member.
 *
 * The score is currently computed as the average -log_2 conditional
 * probability p(d|w) of observing disjunct 'd', given word 'w'.
 * Lower scores are better -- they indicate more likely parses.
 */
void lg_corpus_score(Corpus *corp, Sentence sent, Linkage_info *lifo)
{
	const char *infword, *djstr;
	double tot_score = 0.0f;
	int nwords = sent->length;
	int w;

	/* No-op if the database is not open */
	if (NULL == corp->dbconn) return;

	if (NULL == lifo->disjunct_list_str)
	{
		lg_compute_disjunct_strings(sent, lifo);
	}

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

		djstr = lifo->disjunct_list_str[w];
		tot_score += get_disjunct_score(corp, infword, djstr);
	}

	/* Decrement nwords, so as to ignore the LEFT-WALL */
	--nwords;
	tot_score /= nwords;
	lifo->corpus_cost = tot_score;
}
