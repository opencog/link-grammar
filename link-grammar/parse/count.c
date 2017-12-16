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
#include "connectors.h"
#include "count.h"
#include "disjunct-utils.h"
#include "fast-match.h"
#include "resources.h"
#include "tokenize/word-structures.h" // for Word_struct

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
	fast_matcher_t *mchxt;
	Word *  local_sent;
	/* int     null_block; */ /* not used, always 1 */
	bool    islands_ok;
	bool    null_links;
	bool    exhausted;
	unsigned checktimer;  /* Avoid excess system calls */
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
	 * to get the timer value. On circa-2017 machines, it results
	 * in about 0.5-1 timer calls per second.
	 */
	ctxt->checktimer ++;
	if (ctxt->exhausted || ((0 == ctxt->checktimer%(1<<21)) &&
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

#define NO_COUNT -1
#ifdef PERFORM_COUNT_HISTOGRAMMING
Count_bin count_unknown = {.total = NO_COUNT};
#else
Count_bin count_unknown = NO_COUNT;
#endif

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
static Count_bin pseudocount(count_context_t * ctxt,
                       int lw, int rw, Connector *le, Connector *re,
                       unsigned int null_count)
{
	Count_bin * count = table_lookup(ctxt, lw, rw, le, re, null_count);
	if (NULL == count) return count_unknown;
	return *count;
}

/**
 * Return the number of optional words strictly between w1 and w2.
 */
static int num_optional_words(count_context_t *ctxt, int w1, int w2)
{
	int n = 0;

	for (int w = w1+1; w < w2; w++)
		if (ctxt->local_sent[w].optional) n++;

	return n;
}

#ifdef DEBUG
#define DO_COUNT_TRACE
#endif

#ifdef DO_COUNT_TRACE
#define V(c) (!c?"(nil)":c->string)
static Count_bin do_count1(int lineno, count_context_t *ctxt,
                          int lw, int rw,
                          Connector *le, Connector *re,
                          int null_count);

static Count_bin do_count(int lineno, count_context_t *ctxt,
                          int lw, int rw,
                          Connector *le, Connector *re,
                          int null_count)
{
	static int level;

	if (!verbosity_level(8))
		return do_count1(lineno, ctxt, lw, rw, le, re, null_count);

	Table_connector *t = find_table_pointer(ctxt, lw, rw, le, re, null_count);

	level++;
	prt_error("%*sdo_count%.*s:%d lw=%d rw=%d le=%s re=%s null_count=%d\n\\",
		level*2, "", (!t)*3, "(R)", lineno, lw, rw, V(le), V(re), null_count);
	Count_bin r = do_count1(lineno, ctxt, lw, rw, le, re, null_count);
	prt_error("%*sreturn%.*s:%d=%lld\n", level*2, "", (!!t)*3, "(M)", lineno, r);
	level--;

	return r;
}

static Count_bin do_count1(int lineno,
#define do_count(...) do_count(__LINE__, __VA_ARGS__)
#else
static Count_bin do_count(
#endif
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

	int unparseable_len = rw-lw-1;

#if 1
	/* This check is not necessary for correctness, as it is handled in
	 * the general case below. It looks like it should be slightly faster. */
	if (unparseable_len == 0)
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
#endif

	/* The left and right connectors are null, but the two words are
	 * NOT next to each-other. */
	if ((le == NULL) && (re == NULL))
	{
		int nopt_words = num_optional_words(ctxt, lw, rw);

		if ((null_count == 0) || (!ctxt->islands_ok && (lw != -1)) )
		{
			/* The null_count of skipping n words is just n.
			 * In case the unparsable range contains optional words, we
			 * don't know here how many of them are actually skipped, because
			 * they may belong to different alternatives and essentially just
			 * be ignored.  Hence the inequality - sane_linkage_morphism()
			 * will discard the linkages with extra null words. */
			if ((null_count <= unparseable_len) &&
			    (null_count >= unparseable_len - nopt_words))

			{
				t->count = hist_one();
			}
			else
			{
				t->count = zero;
			}
			return t->count;
		}

		/* Here null_count != 0 and we allow islands (a set of words
		 * linked together but separate from the rest of the sentence).
		 * Because we don't know here if an optional word is just
		 * skipped or is a real null-word (see the comment above) we
		 * try both possibilities: If a real null is encountered, the
		 * rest of the sentence must contain one less null-word. Else
		 * the rest of the sentence still contains the required number
		 * of null words. */
		t->count = zero;
		w = lw + 1;
		for (int opt = 0; opt <= !!ctxt->local_sent[w].optional; opt++)
		{
			null_count += opt;
			for (Disjunct *d = ctxt->local_sent[w].d; d != NULL; d = d->next)
			{
				if (d->left == NULL)
				{
					hist_accumv(&t->count, d->cost,
						do_count(ctxt, w, rw, d->right, NULL, null_count-1));
				}
			}
			hist_accumv(&t->count, 0.0,
				do_count(ctxt, w, rw, NULL, NULL, null_count-1));
		}
		return t->count;
	}

	if (le == NULL)
	{
		start_word = lw+1;
	}
	else
	{
		start_word = le->nearest_word;
	}

	if (re == NULL)
	{
		end_word = rw;
	}
	else
	{
		end_word = re->nearest_word +1;
	}

	total = zero;
	fast_matcher_t *mchxt = ctxt->mchxt;

	for (w = start_word; w < end_word; w++)
	{
		size_t mlb, mle;
		mle = mlb = form_match_list(mchxt, w, le, lw, re, rw);
#ifdef VERIFY_MATCH_LIST
		int id = get_match_list_element(mchxt, mlb) ?
		            get_match_list_element(mchxt, mlb)->match_id : 0;
#endif
		for (; get_match_list_element(mchxt, mle) != NULL; mle++)
		{
			int lnull_cnt, rnull_cnt;
			Disjunct *d = get_match_list_element(mchxt, mle);
			bool Lmatch = d->match_left;
			bool Rmatch = d->match_right;

#ifdef VERIFY_MATCH_LIST
			assert(id == d->match_id, "Modified id (%d!=%d)", id, d->match_id);
#endif

			for (lnull_cnt = 0; lnull_cnt <= null_count; lnull_cnt++)
			{
				bool leftpcount = false;
				bool rightpcount = false;

				Count_bin l_any;
				Count_bin l_cmulti = NO_COUNT;
				Count_bin l_dmulti = NO_COUNT;
				Count_bin l_dcmulti = NO_COUNT;
				Count_bin l_bnr = NO_COUNT;
				Count_bin r_any = NO_COUNT; /* gcc: may be used uninitialized */
				Count_bin r_cmulti = NO_COUNT;
				Count_bin r_dmulti = NO_COUNT;
				Count_bin r_dcmulti = NO_COUNT;
				Count_bin r_bnl = NO_COUNT;

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
					l_any = pseudocount(ctxt, lw, w, le->next, d->left->next, lnull_cnt);
					leftpcount = (hist_total(&l_any) != 0);
					if (!leftpcount && le->multi)
					{
						l_cmulti =
							pseudocount(ctxt, lw, w, le, d->left->next, lnull_cnt);
						leftpcount |= (hist_total(&l_cmulti) != 0);
					}
					if (!leftpcount && d->left->multi)
					{
						l_dmulti =
							pseudocount(ctxt, lw, w, le->next, d->left, lnull_cnt);
						leftpcount |= (hist_total(&l_dmulti) != 0);
					}
					if (!leftpcount && le->multi && d->left->multi)
					{
						l_dcmulti =
							pseudocount(ctxt, lw, w, le, d->left, lnull_cnt);
						leftpcount |= (hist_total(&l_dcmulti) != 0);
					}
				}

				if (Rmatch && (leftpcount || (le == NULL)))
				{
					r_any = pseudocount(ctxt, w, rw, d->right->next, re->next, rnull_cnt);
					rightpcount = (hist_total(&r_any) != 0);
					if (!rightpcount && re->multi)
					{
						r_cmulti =
							pseudocount(ctxt, w, rw, d->right->next, re, rnull_cnt);
						rightpcount |= (hist_total(&r_cmulti) != 0);
					}
					if (!rightpcount && d->right->multi)
					{
						r_dmulti =
							pseudocount(ctxt, w,rw, d->right, re->next, rnull_cnt);
						rightpcount |= (hist_total(&r_dmulti) != 0);
					}
					if (!rightpcount && d->right->multi && re->multi)
					{
						r_dcmulti =
							pseudocount(ctxt, w, rw, d->right, re, rnull_cnt);
						rightpcount |= (hist_total(&r_dcmulti) != 0);
					}
				}

				if (!leftpcount && !rightpcount) continue;

				if (!(leftpcount && rightpcount))
				{
					if (leftpcount)
					{
						/* Evaluate using the left match, but not the right. */
						l_bnr = do_count(ctxt, w, rw, d->right, re, rnull_cnt);
					}
					else if (le == NULL)
					{
						/* Evaluate using the right match, but not the left. */
						r_bnl = do_count(ctxt, lw, w, le, d->left, lnull_cnt);
					}
				}

#define CACHE_COUNT(c, how_to_count, do_count) \
{ \
	Count_bin count = (hist_total(&c) == NO_COUNT) ? do_count : hist_total(&c); \
	how_to_count; \
}
			/* If pseudototal is zero (false), that implies that
			 * we know that the true total is zero. So we don't
			 * bother counting at all, in that case. */
				Count_bin leftcount = zero;
				Count_bin rightcount = zero;
				if (leftpcount &&
				    (rightpcount || (0 != hist_total(&l_bnr))))
				{
					CACHE_COUNT(l_any, leftcount = count,
						do_count(ctxt, lw, w, le->next, d->left->next, lnull_cnt));
					if (le->multi)
						CACHE_COUNT(l_cmulti, hist_accumv(&leftcount, d->cost, count),
							do_count(ctxt, lw, w, le, d->left->next, lnull_cnt));
					if (d->left->multi)
						CACHE_COUNT(l_dmulti, hist_accumv(&leftcount, d->cost, count),
							do_count(ctxt, lw, w, le->next, d->left, lnull_cnt));
					if (d->left->multi && le->multi)
						CACHE_COUNT(l_dcmulti, hist_accumv(&leftcount, d->cost, count),
							do_count(ctxt, lw, w, le, d->left, lnull_cnt));

					if (0 < hist_total(&leftcount))
					{
						/* Evaluate using the left match, but not the right */
						CACHE_COUNT(l_bnr, hist_muladdv(&total, &leftcount, d->cost, count),
							do_count(ctxt, w, rw, d->right, re, rnull_cnt));
					}
				}

				if (rightpcount &&
				    ((0 < hist_total(&leftcount)) || (0 != hist_total(&r_bnl))))
				{
					CACHE_COUNT(r_any, rightcount = count,
						do_count(ctxt, w, rw, d->right->next, re->next, rnull_cnt));
					if (re->multi)
						CACHE_COUNT(r_cmulti, hist_accumv(&rightcount, d->cost, count),
							do_count(ctxt, w, rw, d->right->next, re, rnull_cnt));
					if (d->right->multi)
						CACHE_COUNT(r_dmulti, hist_accumv(&rightcount, d->cost, count),
							do_count(ctxt, w, rw, d->right, re->next, rnull_cnt));
					if (d->right->multi && re->multi)
						CACHE_COUNT(r_dcmulti, hist_accumv(&rightcount, d->cost, count),
							do_count(ctxt, w, rw, d->right, re, rnull_cnt));

					if (0 < hist_total(&rightcount))
					{
						/* Total number where links are used on both sides */
						hist_muladd(&total, &leftcount, 0.0, &rightcount);

						/* Evaluate using the right match, but not the left */
						if (le == NULL)
							CACHE_COUNT(r_bnl, hist_muladdv(&total, &rightcount, d->cost, count),
								do_count(ctxt, lw, w, le, d->left, lnull_cnt));
					}
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
					pop_match_list(mchxt, mlb);
					return total;
				}
			}
		}
		pop_match_list(mchxt, mlb);
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
 * We plan to use this histogram, later ....
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
	ctxt->mchxt = mchxt;

	hist = do_count(ctxt, -1, sent->length, NULL, NULL, null_count+1);

	ctxt->local_sent = NULL;
	ctxt->current_resources = NULL;
	ctxt->checktimer = 0;
	return hist;
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
