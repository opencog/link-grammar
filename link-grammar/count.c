/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2013,2014 Linas Vepstas                                 */
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
	s64              count;
	short            lw, rw;
	unsigned short   cost; /* Cost, here and below, is actually the
	                        * null-count being considered. */
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
		shift = 12 + (sent_len) / 6 ; 
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
                                     unsigned int cost, s64 count)
{
	Table_connector *t, *n;
	unsigned int h;

	n = (Table_connector *) xalloc(sizeof(Table_connector));
	n->count = count;
	n->lw = lw; n->rw = rw; n->le = le; n->re = re; n->cost = cost;
	h = pair_hash(ctxt->log2_table_size,lw, rw, le, re, cost);
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
                   unsigned int cost)
{
	Table_connector *t;
	unsigned int h = pair_hash(ctxt->log2_table_size,lw, rw, le, re, cost);
	t = ctxt->table[h];
	for (; t != NULL; t = t->next) {
		if ((t->lw == lw) && (t->rw == rw)
		    && (t->le == le) && (t->re == re)
		    && (t->cost == cost))  return t;
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
		return table_store(ctxt, lw, rw, le, re, cost, 0);
	}
	else return NULL;
}

/** returns the count for this quintuple if there, -1 otherwise */
s64 table_lookup(count_context_t * ctxt, 
                 int lw, int rw, Connector *le, Connector *re, unsigned int cost)
{
	Table_connector *t = find_table_pointer(ctxt, lw, rw, le, re, cost);

	if (t == NULL) return -1; else return t->count;
}

/**
 * Returns 0 if and only if this entry is in the hash table 
 * with a count value of 0.
 */
static s64 pseudocount(count_context_t * ctxt,
                       int lw, int rw, Connector *le, Connector *re,
                       unsigned int cost)
{
	s64 count = table_lookup(ctxt, lw, rw, le, re, cost);
	if (count == 0) return 0; else return 1;
}

static s64 do_count(fast_matcher_t *mchxt, 
                    count_context_t *ctxt,
                    int lw, int rw,
                    Connector *le, Connector *re, int null_count)
{
	s64 total;
	int start_word, end_word, w;
	Table_connector *t;

	if (null_count < 0) return 0;  /* can this ever happen?? */

	t = find_table_pointer(ctxt, lw, rw, le, re, null_count);

	if (t) return t->count;

	/* Create the table entry with a tentative null count of 0. 
	 * This count must be updated before we return. */
	t = table_store(ctxt, lw, rw, le, re, null_count, 0);

	if (rw == 1+lw)
	{
		/* lw and rw are neighboring words */
		/* You can't have a linkage here with null_count > 0 */
		if ((le == NULL) && (re == NULL) && (null_count == 0))
		{
			t->count = 1;
		}
		else
		{
			t->count = 0;
		}
		return t->count;
	}

	if ((le == NULL) && (re == NULL))
	{
		if (!ctxt->islands_ok && (lw != -1))
		{
			/* If we don't allow islands (a set of words linked together
			 * but separate from the rest of the sentence) then the
			 * null_count of skipping n words is just n. */
			/* null_block is always 1 these days. */
			/* if (null_count == ((rw-lw-1) + ctxt->null_block-1) / ctxt->null_block) */
			if (null_count == (rw-lw-1))
			{
				/* If null_block=4 then the null_count of
				   1,2,3,4 nulls is 1; and 5,6,7,8 is 2 etc. */
				t->count = 1;
			}
			else
			{
				t->count = 0;
			}
			return t->count;
		}
		if (null_count == 0)
		{
			/* There is no solution without nulls in this case. There is
			 * a slight efficiency hack to separate this null_count==0
			 * case out, but not necessary for correctness */
			t->count = 0;
		}
		else
		{
			Disjunct * d;
			s64 total = 0;
			int w = lw + 1;
			for (d = ctxt->local_sent[w].d; d != NULL; d = d->next)
			{
				if (d->left == NULL)
				{
					total += do_count(mchxt, ctxt, w, rw, d->right, NULL, null_count-1);
				}
			}
			total += do_count(mchxt, ctxt, w, rw, NULL, NULL, null_count-1);
			t->count = total;
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

	total = 0;

	for (w = start_word; w < end_word; w++)
	{
		Match_node *m, *m1;
		m1 = m = form_match_list(mchxt, w, le, lw, re, rw);
		for (; m != NULL; m = m->next)
		{
			unsigned int lcost, rcost;
			unsigned int null_count_p1;
			Disjunct * d;
			d = m->d;
			null_count_p1 = null_count + 1; /* avoid gcc warning: unsafe loop opt */
			for (lcost = 0; lcost < null_count_p1; lcost++)
			{
				bool Lmatch, Rmatch;
				s64 leftcount = 0, rightcount = 0;
				s64 pseudototal;

				rcost = null_count - lcost;
				/* Now lcost and rcost are the costs we're assigning
				 * to those parts respectively */

				/* Now, we determine if (based on table only) we can see that
				   the current range is not parsable. */
				Lmatch = (le != NULL) && (d->left != NULL) && 
				         do_match(le, d->left, lw, w);
				Rmatch = (d->right != NULL) && (re != NULL) && 
				         do_match(d->right, re, w, rw);

				if (Lmatch)
				{
					leftcount = pseudocount(ctxt, lw, w, le->next, d->left->next, lcost);
					if (le->multi) leftcount += pseudocount(ctxt, lw, w, le, d->left->next, lcost);
					if (d->left->multi) leftcount += pseudocount(ctxt, lw, w, le->next, d->left, lcost);
					if (le->multi && d->left->multi) leftcount += pseudocount(ctxt, lw, w, le, d->left, lcost);
				}

				if (Rmatch)
				{
					rightcount = pseudocount(ctxt, w, rw, d->right->next, re->next, rcost);
					if (d->right->multi) rightcount += pseudocount(ctxt, w,rw,d->right,re->next, rcost);
					if (re->multi) rightcount += pseudocount(ctxt, w, rw, d->right->next, re, rcost);
					if (d->right->multi && re->multi) rightcount += pseudocount(ctxt, w, rw, d->right, re, rcost);
				}

				/* total number where links are used on both sides */
				pseudototal = leftcount*rightcount;

				if (leftcount > 0) {
					/* evaluate using the left match, but not the right */
					pseudototal += leftcount * pseudocount(ctxt, w, rw, d->right, re, rcost);
				}
				if ((le == NULL) && (rightcount > 0)) {
					/* evaluate using the right match, but not the left */
					pseudototal += rightcount * pseudocount(ctxt, lw, w, le, d->left, lcost);
				}

				/* now pseudototal is 0 implies that we know that the true total is 0 */
				if (pseudototal != 0) {
					rightcount = leftcount = 0;
					if (Lmatch) {
						leftcount = do_count(mchxt, ctxt, lw, w, le->next, d->left->next, lcost);
						if (le->multi) leftcount += do_count(mchxt, ctxt, lw, w, le, d->left->next, lcost);
						if (d->left->multi) leftcount += do_count(mchxt, ctxt, lw, w, le->next, d->left, lcost);
						if (le->multi && d->left->multi) leftcount += do_count(mchxt, ctxt, lw, w, le, d->left, lcost);
					}

					if (Rmatch) {
						rightcount = do_count(mchxt, ctxt, w, rw, d->right->next, re->next, rcost);
						if (d->right->multi) rightcount += do_count(mchxt, ctxt, w, rw, d->right,re->next, rcost);
						if (re->multi) rightcount += do_count(mchxt, ctxt, w, rw, d->right->next, re, rcost);
						if (d->right->multi && re->multi) rightcount += do_count(mchxt, ctxt, w, rw, d->right, re, rcost);
					}

					/* Total number where links are used on both sides */
					total += leftcount*rightcount;

					if (leftcount > 0) {
						/* Evaluate using the left match, but not the right */
						total += leftcount * do_count(mchxt, ctxt, w, rw, d->right, re, rcost);
					}
					if ((le == NULL) && (rightcount > 0)) {
						/* Evaluate using the right match, but not the left */
						total += rightcount * do_count(mchxt, ctxt, lw, w, le, d->left, lcost);
					}

					/* Sigh. Overflows can and do occur, esp for the ANY language. */
					if (INT_MAX < total)
					{
						total = INT_MAX;
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
 * context have already been initialized, and are freed later. The
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
 */
s64 do_parse(Sentence sent,
             fast_matcher_t *mchxt,
             count_context_t *ctxt,
             int null_count, Parse_Options opts)
{
	s64 total;

	ctxt->current_resources = opts->resources;
	ctxt->exhausted = false;
	ctxt->checktimer = 0;
	ctxt->local_sent = sent->word;

	/* consecutive blocks of this many words are considered as
	 * one null link. */
	/* ctxt->null_block = 1; */
	ctxt->islands_ok = opts->islands_ok;

	total = do_count(mchxt, ctxt, -1, sent->length, NULL, NULL, null_count+1);

	ctxt->local_sent = NULL;
	ctxt->current_resources = NULL;
	ctxt->checktimer = 0;
	return total;
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
