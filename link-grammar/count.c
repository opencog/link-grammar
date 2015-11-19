/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2013,2014,2015 Linas Vepstas                            */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <limits.h>
#include "link-includes.h"
#include "api-structures.h"
#include "count.h"
#include "disjunct-utils.h"
#include "fast-match.h"
#include "prune.h"
#include "resources.h"
#include "structures.h"
#include "word-utils.h"

/* This file contains the exhaustive search algorithm. */

typedef struct Table_connector_s Table_connector;
struct Table_connector_s
{
	Table_connector  *next;
	Connector        *le, *re;
	Count_bin        count;
	short            lw, rw;
	unsigned short   null_count;
};

struct count_context_s
{
	Word *  local_sent;
	/* int     null_block; */ /* not used, always 1 */
	bool    islands_ok;
	bool    null_links;
	bool    exhausted;
	int     checktimer;  /* Avoid excess system calls */
	int     table_size;
	int     log2_table_size;
	Table_connector ** table;
	Resources current_resources;
};

static void free_table(count_context_t *ctxt)
{
	int i;
	Table_connector *t, *x;

	for (i=0; i<ctxt->table_size; i++)
	{
		for(t = ctxt->table[i]; t!= NULL; t=x)
		{
			x = t->next;
			xfree((void *) t, sizeof(Table_connector));
		}
	}
	xfree(ctxt->table, ctxt->table_size * sizeof(Table_connector*));
	ctxt->table = NULL;
	ctxt->table_size = 0;
}

static void init_table(count_context_t *ctxt, size_t sent_len)
{
	unsigned int shift;
	/* A piecewise exponential function determines the size of the
	 * hash table. Probably should make use of the actual number of
	 * disjuncts, rather than just the number of words.
	 */
	if (ctxt->table) free_table(ctxt);

	if (sent_len >= 10)
	{
		shift = 12 + (sent_len) / 4 ;
	}
	else
	{
		shift = 12;
	}

	/* Clamp at max 4*(1<<24) == 64 MBytes */
	if (24 < shift) shift = 24;
	ctxt->table_size = (1U << shift);
	ctxt->log2_table_size = shift;
	ctxt->table = (Table_connector**)
		xalloc(ctxt->table_size * sizeof(Table_connector*));
	memset(ctxt->table, 0, ctxt->table_size*sizeof(Table_connector*));
}

/*
 * Returns TRUE if s and t match according to the connector matching
 * rules.
 */
bool do_match(Connector *a, Connector *b, int aw, int bw)
{
	int dist = bw - aw;
	if (NULL == a || NULL == b) return false;
	assert(aw < bw, "do_match() did not receive params in the natural order.");
	if (dist > a->length_limit || dist > b->length_limit) return false;
	return easy_match(a->string, b->string);
}

/**
 * Stores the value in the table.  Assumes it's not already there.
 */
static Table_connector * table_store(count_context_t *ctxt,
                                     int lw, int rw,
                                     Connector *le, Connector *re,
                                     unsigned int null_count)
{
	Table_connector *t, *n;
	unsigned int h;

	n = (Table_connector *) xalloc(sizeof(Table_connector));
	n->lw = lw; n->rw = rw; n->le = le; n->re = re; n->null_count = null_count;
	h = pair_hash(ctxt->table_size, lw, rw, le, re, null_count);
	t = ctxt->table[h];
	n->next = t;
	ctxt->table[h] = n;

	return n;
}

/** returns the pointer to this info, NULL if not there */
static Table_connector *
find_table_pointer(count_context_t *ctxt,
                   int lw, int rw,
                   Connector *le, Connector *re,
                   unsigned int null_count)
{
	Table_connector *t;
	unsigned int h = pair_hash(ctxt->table_size,lw, rw, le, re, null_count);
	t = ctxt->table[h];
	for (; t != NULL; t = t->next) {
		if ((t->lw == lw) && (t->rw == rw)
		    && (t->le == le) && (t->re == re)
		    && (t->null_count == null_count))  return t;
	}

	/* Create a new connector only if resources are exhausted.
	 * (???) Huh? I guess we're in panic parse mode in that case.
	 * checktimer is a device to avoid a gazillion system calls
	 * to get the timer value. On circa-2009 machines, it results
	 * in maybe 5-10 timer calls per second.
	 */
	ctxt->checktimer ++;
	if (ctxt->exhausted || ((0 == ctxt->checktimer%450100) &&
	                       (ctxt->current_resources != NULL) &&
	                       resources_exhausted(ctxt->current_resources)))
	{
		ctxt->exhausted = true;
		t = table_store(ctxt, lw, rw, le, re, null_count);
		t->count = hist_zero();
		return t;
	}
	else return NULL;
}

/** returns the count for this quintuple if there, -1 otherwise */
Count_bin* table_lookup(count_context_t * ctxt,
                       int lw, int rw, Connector *le, Connector *re,
                       unsigned int null_count)
{
	Table_connector *t = find_table_pointer(ctxt, lw, rw, le, re, null_count);

	if (t == NULL) return NULL; else return &t->count;
}

/**
 * psuedocount is used to check to see if a parse is even possible,
 * so that we don't waste cpu time performing an actual count, only
 * to discover that it is zero.
 *
 * Returns false if and only if this entry is in the hash table
 * with a count value of 0. If an entry is not in the hash table,
 * we have to assume the worst case: that the count might be non-zero,
 * and since we don't know, we return true.  However, if the entry is
 * in the hash table, and its zero, then we know, for sure, that the
 * count is zero.
 */
static bool pseudocount(count_context_t * ctxt,
                       int lw, int rw, Connector *le, Connector *re,
                       unsigned int null_count)
{
	Count_bin * count = table_lookup(ctxt, lw, rw, le, re, null_count);
	if (NULL == count) return true;
	if (hist_total(count) == 0) return false;
	return true;
}

static Count_bin do_count(fast_matcher_t *mchxt,
                          count_context_t *ctxt,
                          int lw, int rw,
                          Connector *le, Connector *re,
                          int null_count)
{
	Count_bin zero = hist_zero();
	Count_bin total;
	int start_word, end_word, w;
	Table_connector *t;

	assert (0 <= null_count, "Bad null count");

	t = find_table_pointer(ctxt, lw, rw, le, re, null_count);

	if (t) return t->count;

	/* Create the table entry with a tentative null count of 0.
	 * This count must be updated before we return. */
	t = table_store(ctxt, lw, rw, le, re, null_count);

	if (rw == 1+lw)
	{
		/* lw and rw are neighboring words */
		/* You can't have a linkage here with null_count > 0 */
		if ((le == NULL) && (re == NULL) && (null_count == 0))
		{
			t->count = hist_one();
		}
		else
		{
			t->count = zero;
		}
		return t->count;
	}

	/* The left and right connectors are null, but the two words are
	 * NOT next to each-other. */
	if ((le == NULL) && (re == NULL))
	{
		if (!ctxt->islands_ok && (lw != -1))
		{
			/* If we don't allow islands (a set of words linked together
			 * but separate from the rest of the sentence) then the
			 * null_count of skipping n words is just n. */
			if (null_count == (rw-lw-1))
			{
				t->count = hist_one();
			}
			else
			{
				t->count = zero;
			}
			return t->count;
		}
		if (null_count == 0)
		{
			/* There is no solution without nulls in this case. There is
			 * a slight efficiency hack to separate this null_count==0
			 * case out, but not necessary for correctness */
			t->count = zero;
		}
		else
		{
			t->count = zero;
			Disjunct * d;
			int w = lw + 1;
			for (d = ctxt->local_sent[w].d; d != NULL; d = d->next)
			{
				if (d->left == NULL)
				{
					hist_accumv(&t->count, d->cost,
						do_count(mchxt, ctxt, w, rw, d->right, NULL, null_count-1));
				}
			}
			hist_accumv(&t->count, 0.0,
				do_count(mchxt, ctxt, w, rw, NULL, NULL, null_count-1));
		}
		return t->count;
	}

	if (le == NULL)
	{
		start_word = lw+1;
	}
	else
	{
		start_word = le->word;
	}

	if (re == NULL)
	{
		end_word = rw;
	}
	else
	{
		end_word = re->word +1;
	}

	total = zero;

	for (w = start_word; w < end_word; w++)
	{
		Match_node *m, *m1;
		m1 = m = form_match_list(mchxt, w, le, lw, re, rw);
		for (; m != NULL; m = m->next)
		{
			unsigned int lnull_cnt, rnull_cnt;
			Disjunct * d = m->d;
			bool Lmatch = do_match(le, d->left, lw, w);
			bool Rmatch = do_match(d->right, re, w, rw);

			/* _p1 avoids a gcc warning about unsafe loop opt */
			unsigned int null_count_p1 = null_count + 1;

			for (lnull_cnt = 0; lnull_cnt < null_count_p1; lnull_cnt++)
			{
				bool leftpcount = false;
				bool rightpcount = false;
				bool pseudototal = false;

				rnull_cnt = null_count - lnull_cnt;
				/* Now lnull_cnt and rnull_cnt are the costs we're assigning
				 * to those parts respectively */

				/* Now, we determine if (based on table only) we can see that
				   the current range is not parsable. */

				/* First, perform pseudocounting as an optimization. If
				 * the pseudocount is zero, then we know that the true
				 * count will be zero, and so skip counting entirely,
				 * in that case.
				 */
				if (Lmatch)
				{
					leftpcount = pseudocount(ctxt, lw, w, le->next, d->left->next, lnull_cnt);
					if (!leftpcount && le->multi)
						leftpcount =
							pseudocount(ctxt, lw, w, le, d->left->next, lnull_cnt);
					if (!leftpcount && d->left->multi)
						leftpcount =
							pseudocount(ctxt, lw, w, le->next, d->left, lnull_cnt);
					if (!leftpcount && le->multi && d->left->multi)
						leftpcount =
							pseudocount(ctxt, lw, w, le, d->left, lnull_cnt);
				}

				if (Rmatch)
				{
					rightpcount = pseudocount(ctxt, w, rw, d->right->next, re->next, rnull_cnt);
					if (!rightpcount && d->right->multi)
						rightpcount =
							pseudocount(ctxt, w,rw, d->right, re->next, rnull_cnt);
					if (!rightpcount && re->multi)
						rightpcount =
							pseudocount(ctxt, w, rw, d->right->next, re, rnull_cnt);
					if (!rightpcount && d->right->multi && re->multi)
						rightpcount =
							pseudocount(ctxt, w, rw, d->right, re, rnull_cnt);
				}

				/* Total number where links are used on both sides */
				pseudototal = leftpcount && rightpcount;

				if (!pseudototal && leftpcount) {
					/* Evaluate using the left match, but not the right. */
					pseudototal =
						pseudocount(ctxt, w, rw, d->right, re, rnull_cnt);
				}
				if (!pseudototal && (le == NULL) && rightpcount) {
					/* Evaluate using the right match, but not the left. */
					pseudototal =
						pseudocount(ctxt, lw, w, le, d->left, lnull_cnt);
				}

				/* If pseudototal is zero (false), that implies that
				 * we know that the true total is zero. So we don't
				 * bother counting at all, in that case. */
				if (pseudototal)
				{
					Count_bin leftcount = zero;
					Count_bin rightcount = zero;
					if (Lmatch) {
						leftcount = do_count(mchxt, ctxt, lw, w, le->next, d->left->next, lnull_cnt);
						if (le->multi)
							hist_accumv(&leftcount, d->cost,
								do_count(mchxt, ctxt, lw, w, le, d->left->next, lnull_cnt));
						if (d->left->multi)
							hist_accumv(&leftcount, d->cost,
								 do_count(mchxt, ctxt, lw, w, le->next, d->left, lnull_cnt));
						if (le->multi && d->left->multi)
							hist_accumv(&leftcount, d->cost,
								do_count(mchxt, ctxt, lw, w, le, d->left, lnull_cnt));
					}

					if (Rmatch) {
						rightcount = do_count(mchxt, ctxt, w, rw, d->right->next, re->next, rnull_cnt);
						if (d->right->multi)
							hist_accumv(&rightcount, d->cost,
								do_count(mchxt, ctxt, w, rw, d->right,re->next, rnull_cnt));
						if (re->multi)
							hist_accumv(&rightcount, d->cost,
								do_count(mchxt, ctxt, w, rw, d->right->next, re, rnull_cnt));
						if (d->right->multi && re->multi)
							hist_accumv(&rightcount, d->cost,
								do_count(mchxt, ctxt, w, rw, d->right, re, rnull_cnt));
					}

					/* Total number where links are used on both sides */
					hist_muladd(&total, &leftcount, 0.0, &rightcount);

					if (0 < hist_total(&leftcount))
					{
						/* Evaluate using the left match, but not the right */
						hist_muladdv(&total, &leftcount, d->cost,
							do_count(mchxt, ctxt, w, rw, d->right, re, rnull_cnt));
					}
					if ((le == NULL) && (0 < hist_total(&rightcount)))
					{
						/* Evaluate using the right match, but not the left */
						hist_muladdv(&total, &rightcount, d->cost,
							do_count(mchxt, ctxt, lw, w, le, d->left, lnull_cnt));
					}

					/* Sigh. Overflows can and do occur, esp for the ANY language. */
					if (INT_MAX < hist_total(&total))
					{
#ifdef PERFORM_COUNT_HISTOGRAMMING
						total.total = INT_MAX;
#else
						total = INT_MAX;
#endif /* PERFORM_COUNT_HISTOGRAMMING */
						t->count = total;
						put_match_list(mchxt, m1);
						return total;
					}
				}
			}
		}
		put_match_list(mchxt, m1);
	}
	t->count = total;
	return total;
}


/**
 * Returns the number of ways the sentence can be parsed with the
 * specified null count. Assumes that the fast-matcher and the count
 * context have already been initialized, and will be freed later. The
 * "null_count" argument is the number of words that are allowed to
 * have no links to them.
 *
 * This the full-fledged parser, but it only 'counts', in order to
 * avoid an explosion of allocated memory structures to hold each
 * possible parse.  Thus, to see an 'actual' parse, a second pass
 * must be made, with build_parse_set(), to get actual parse structures.
 *
 * The work is split up this way for two reasons:
 * 1) A given sentence may have thousands of parses, and the user is
 *    interested in only a few.
 * 2) A given sentence may have billions of parses, in which case,
 *    allocating for each would blow out RAM.
 * So, basically, its good to know how many parses to expect, before
 * starting to allocate parse structures.
 *
 * The count returned here is meant to be completely accurate; it is
 * not an approximation!
 *
 * Currently, the code has been designed to maintain a histogram of
 * the cost of each of the parses. The number and width of the bins
 * is adjustable in histogram.c. At this time, the histogram is not
 * used anywhere, and a 3-5% speedup is available if it is avoided.
 * We plan to use this historgram, later ....
 */
Count_bin do_parse(Sentence sent,
                   fast_matcher_t *mchxt,
                   count_context_t *ctxt,
                   int null_count, Parse_Options opts)
{
	Count_bin hist;

	ctxt->current_resources = opts->resources;
	ctxt->exhausted = false;
	ctxt->checktimer = 0;
	ctxt->local_sent = sent->word;

	/* consecutive blocks of this many words are considered as
	 * one null link. */
	/* ctxt->null_block = 1; */
	ctxt->islands_ok = opts->islands_ok;

	hist = do_count(mchxt, ctxt, -1, sent->length, NULL, NULL, null_count+1);

	ctxt->local_sent = NULL;
	ctxt->current_resources = NULL;
	ctxt->checktimer = 0;
	return hist;
}

void delete_unmarked_disjuncts(Sentence sent)
{
	size_t w;
	Disjunct *d_head, *d, *dx;

	for (w=0; w<sent->length; w++) {
		d_head = NULL;
		for (d=sent->word[w].d; d != NULL; d=dx) {
			dx = d->next;
			if (d->marked) {
				d->next = d_head;
				d_head = d;
			} else {
				d->next = NULL;
				free_disjuncts(d);
			}
		}
		sent->word[w].d = d_head;
	}
}

/* sent_length is used only as a hint for the hash table size ... */
count_context_t * alloc_count_context(size_t sent_length)
{
	count_context_t *ctxt = (count_context_t *) xalloc (sizeof(count_context_t));
	memset(ctxt, 0, sizeof(count_context_t));

	init_table(ctxt, sent_length);
	return ctxt;
}

void free_count_context(count_context_t *ctxt)
{
	free_table(ctxt);
	xfree(ctxt, sizeof(count_context_t));
}
