/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2010, 2014 Linas Vepstas                                */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <limits.h>                     // INT_MAX
#include <math.h>                       // log2

#include "connectors.h"
#include "count.h"
#include "disjunct-utils.h"             // Disjunct
#include "extract-links.h"
#include "fast-match.h"
#include "memory-pool.h"
#include "utilities.h"                  // Windows rand_r()
#include "linkage/linkage.h"
#include "tokenize/word-structures.h"   // Word_Struct

//#define RECOUNT
//#define DEBUG_X_TABLE

typedef struct Parse_choice_struct Parse_choice;

/* Parse_choice records a parse of the word range set[0]->lw to
 * set[1]->rw, when the middle disjunct md is of word set[0]->rw
 * (which is always equal to set[1]->lw).
 * See make_choice() below.
 * The number of linkages in this parse is the product of the
 * counts of the two Parse_set elements. */
typedef struct Parse_set_struct Parse_set;
struct Parse_choice_struct
{
	Parse_choice * next;
	Parse_set * set[2];
	Disjunct    *md;           /* the chosen disjunct for the middle word */
	Connector   *lc, *rc;      /* the connectors on the middle word */
};

/* Parse_set serves as a header of Parse_choice chained elements, that
 * describe the possible parses with the specified null_count, using
 * tracons l_id and r_id on words lw and rw, correspondingly. */
struct Parse_set_struct
{
	Parse_choice   *first;
	Connector      *le, *re;
	uint8_t        lw, rw;     /* left and right word index */
	uint8_t        null_count; /* number of island words */
	int32_t        l_id, r_id; /* tracons on words lw, rw */

	count_t count;             /* The number of ways to parse. */
#ifdef RECOUNT
	count_t recount;  /* Exactly the same as above, but counted at a later stage. */
	count_t cut_count;  /* Count only low-cost parses, i.e. below the cost cutoff */
	//float cost_cutoff;
#undef RECOUNT
#define RECOUNT(X) X
#else
#define RECOUNT(X)  /* Make it disappear... */
#endif
};

typedef struct Pset_bucket_struct Pset_bucket;
struct Pset_bucket_struct
{
	Parse_set set;
	Pset_bucket *next;
};

struct extractor_s
{
	unsigned int   x_table_size;
	unsigned int   log2_x_table_size; /* Not used */
	Pset_bucket ** x_table;           /* Hash table */
	Parse_set *    parse_set;
	Word           *words;
	Pool_desc *    Pset_bucket_pool;
	Pool_desc *    Parse_choice_pool;
	bool           islands_ok;
	bool           sort_match_list;

	/* thread-safe random number state */
	unsigned int rand_state;
};

/**
 * The first thing we do is we build a data structure to represent the
 * result of the entire parse search.  There will be a set of nodes
 * built for each call to the do_count() function that returned a non-zero
 * value, AND which is part of a valid linkage.  Each of these nodes
 * represents a valid continuation, and contains pointers to two other
 * sets (one for the left continuation and one for the right
 * continuation).
 */

static Parse_choice *
make_choice(Parse_set *lset, Connector * lrc,
            Parse_set *rset, Connector * rlc,
            Disjunct *md, extractor_t* pex)
{
	Parse_choice *pc;
	pc = pool_alloc(pex->Parse_choice_pool);
	pc->next = NULL;
	pc->set[0] = lset;
	pc->set[1] = rset;
	pc->lc = lrc;
	pc->rc = rlc;
	pc->md = md;
	return pc;
}

static void record_choice(
    Parse_set *lset, Connector * lrc,
    Parse_set *rset, Connector * rlc,
    Disjunct *md, Parse_set *s, extractor_t* pex)
{
	Parse_choice *pc = make_choice(lset, lrc, rset, rlc, md, pex);

	// Chain it into the parse set.
	pc->next = s->first;
	s->first = pc;
}

/**
 * Return an estimate of the required hash table size. The estimate is
 * based on actual measurements, presented at
 * https://github.com/opencog/link-grammar/discussions/1402#discussioncomment-4872249
 * That estimate is 0.5 * (num dj / sqrt(num words))**1.5
 * I have no intuition as to why it would be that. It's just what the
 * data shows. Note, BTW, this is an upper bound, two or three times
 * larger than what is needed. Thus, hash table load factor will be
 * small, usually around 0.25 or even less.
 */
static int estimate_log2_table_size(Sentence sent)
{
	/* Size estimate based on measurements (see #1402) */
	double lscale = log2(sent->num_disjuncts) - 0.5 * log2(sent->length);
	double lo_est = lscale + 4.0;
	double hi_est = 1.5 * lscale;
	int log2_table_size = floor(fmax(lo_est, hi_est));

	if (24 < log2_table_size) log2_table_size = 24;

#if LATER
	// TODO: Maybe the above size estimate is OK, for generation?
	// perhaps it should not be clamped at 24 ??
	if (IS_GENERATION(sent->dict))
		log2_table_size = 28;
#endif

	return log2_table_size;
}

/// Provide an upper-bound estimate for the number of Parse_choice
/// elements that will be allocated. This bound is reasonable for
/// the English-language dictionary, almost never exceeding 100K
/// on normal text. However, for MST dictionaries, it can easily
/// get above 100M entries, and thus is clamped to a more reasonable
/// size. This is the block size; if more is needed, more blocks will
/// be allocated.
static size_t estimate_parse_choice_allocations(Sentence sent)
{
	size_t expsz = pool_num_elements_issued(sent->Exp_pool);
	size_t pcsze = (expsz * expsz) / 100000;
	if (pcsze < 1020) pcsze = 1020;

	// At this time, sizeof(Parse_choice) is 48 bytes.
	// Putting an upper limit of 16*1024*1024 elements corresponds to
	// a limit of 800 MBytes RAM. No conventional hand-built dict ever
	// comes close to this limit, but sizes well above this are seen
	// with MST dictionaries. Presumably, this is an "MST thing", and
	// goes away with more sophisticated dicts.
#define MAX_PC_ELTS (16*1024*1024 - 10)

	if (MAX_PC_ELTS < pcsze) pcsze = MAX_PC_ELTS;

	return pcsze;
}

/**
 * Allocate the parse info struct
 */
extractor_t * extractor_new(Sentence sent)
{
	extractor_t * pex = (extractor_t *) xalloc(sizeof(extractor_t));
	memset(pex, 0, sizeof(extractor_t));
	pex->rand_state = sent->rand_state;

	/* Alloc the x_table */
	int log2_table_size = estimate_log2_table_size(sent);
	pex->log2_x_table_size = log2_table_size;
	pex->x_table_size = (1 << log2_table_size);

#ifdef DEBUG_X_TABLE
		printf("Allocating x_table of size %u (nwords %d)\n",
		       pex->x_table_size, nwords);
#endif /* DEBUG_X_TABLE */

	pex->x_table = (Pset_bucket**) xalloc(pex->x_table_size * sizeof(Pset_bucket*));
	memset(pex->x_table, 0, pex->x_table_size * sizeof(Pset_bucket*));

	// What's good for the goose is good for the gander.
	// The pex->x_table_size is a good upper-bound estimate for how
	// many buckets we will be allocating. Divide by two, based on
	// the observed fill ratio.
	size_t pbsze = pex->x_table_size / 2;
	pex->Pset_bucket_pool =
		pool_new(__func__, "Pset_bucket",
		         /*num_elements*/pbsze, sizeof(Pset_bucket),
		         /*zero_out*/false, /*align*/false, /*exact*/false);

	size_t pcsze = estimate_parse_choice_allocations(sent);
	pex->Parse_choice_pool =
		pool_new(__func__, "Parse_choice",
		         /*num_elements*/pcsze, sizeof(Parse_choice),
		         /*zero_out*/false, /*align*/false, /*exact*/false);

	return pex;
}

/**
 * This is the function that should be used to free the set structure. Since
 * it's a dag, a recursive free function won't work.  Every time we create
 * a set element, we put it in the hash table, so this is OK.
 */
void free_extractor(extractor_t * pex)
{
	if (!pex) return;

#ifdef DEBUG_X_TABLE
	int N = 0;
	for (unsigned int i = 0; i < pex->x_table_size; i++)
	{
		int c = 0;
		for (Pset_bucket *t = pex->x_table[i]; t != NULL; t = t->next)
			c++;

		if (c > 0)
			;//printf("I %d: chain %d\n", i, c);
		else
			N++;
	}
	printf("Used x_table %u/%u %.2f%%\n",
	       pex->x_table_size-N, pex->x_table_size,
	       100.0f*(pex->x_table_size-N)/pex->x_table_size);
#endif /* DEBUG_X_TABLE */

	pex->parse_set = NULL;

	//printf("Freeing x_table of size %d\n", pex->x_table_size);
	xfree((void *) pex->x_table, pex->x_table_size * sizeof(Pset_bucket*));
	pex->x_table_size = 0;
	pex->x_table = NULL;

	pool_delete(pex->Pset_bucket_pool);
	pool_delete(pex->Parse_choice_pool);

	xfree((void *) pex, sizeof(extractor_t));
}

/**
 * Returns the pointer to this info, NULL if not there.
 * Note that there is no need to use (lw, rw) as keys because tracon_id
 * values are not shared between words.
 */
static Pset_bucket * x_table_pointer(int lw, int rw,
                              Connector *le, Connector *re,
                              unsigned int null_count, extractor_t * pex)
{
	Pset_bucket *t;
	int l_id = (NULL != le) ? le->tracon_id : lw;
	int r_id = (NULL != re) ? re->tracon_id : rw;
	unsigned int hash = pair_hash(lw, rw, l_id, r_id, null_count);
	t = pex->x_table[hash & (pex->x_table_size-1)];

	for (; t != NULL; t = t->next) {
		if ((t->set.l_id == l_id) && (t->set.r_id == r_id) &&
		    (t->set.null_count == null_count)) return t;
	}
	return NULL;
}

/**
 * Stores the value in the x_table.  Assumes it's not already there.
 */
static Pset_bucket * x_table_store(int lw, int rw,
                                  Connector *le, Connector *re,
                                  unsigned int null_count, extractor_t * pex)
{
	Pset_bucket **t, *n;
	unsigned int h;

	n = pool_alloc(pex->Pset_bucket_pool);
	n->set.lw = lw;
	n->set.rw = rw;
	n->set.null_count = null_count;
	n->set.l_id = (NULL != le) ? le->tracon_id : lw;
	n->set.r_id = (NULL != re) ? re->tracon_id : rw;
	n->set.le = le;
	n->set.re = re;
	n->set.count = 0;
	n->set.first = NULL;

	h = pair_hash(lw, rw, n->set.l_id, n->set.r_id, null_count);
	t = &pex->x_table[h & (pex->x_table_size -1)];
	n->next = *t;
	*t = n;
	return n;
}

/** Create a bogus parse set that only holds lw, rw. */
static Parse_set* dummy_set(int lw, int rw,
                            unsigned int null_count, extractor_t * pex)
{
	Pset_bucket *dummy;
	dummy = x_table_pointer(lw, rw, NULL, NULL, null_count, pex);
	if (dummy) return &dummy->set;

	dummy = x_table_store(lw, rw, NULL, NULL, null_count, pex);
	dummy->set.count = 1;
	return &dummy->set;
}

static int cost_compare(const void *a, const void *b)
{
	const Disjunct * const * da = a;
	const Disjunct * const * db = b;
	if ((*da)->cost < (*db)->cost) return -1;
	if ((*da)->cost > (*db)->cost) return 1;
	return 0;
}

/**
 * Sort the match-list into ascending disjunct cost. The goal here
 * is to issue the lowest-cost disjuncts first, so that the parse
 * set ends up quasi-sorted. This is not enough to get us a totally
 * sorted parse set, but it does guarantee that at least the very
 * first parse really will be the lowest cost.
 */
static void sort_match_list(fast_matcher_t *mchxt, size_t mlb)
{
	size_t i = mlb;

	while (get_match_list_element(mchxt, i) != NULL)
		i++;

	if (i - mlb < 2) return;

	qsort(get_match_list(mchxt, mlb), i - mlb, sizeof(Disjunct *), cost_compare);
}

/**
 * returns NULL if there are no ways to parse, or returns a pointer
 * to a set structure representing all the ways to parse.
 *
 * This code is similar to do_count() in count.c -- for a good reason:
 * the do_count() function did a full parse, but didn't actually
 * allocate a memory structures to hold the parse.  This also does
 * a full parse, but it also allocates and fills out the various
 * parse structures.
 */
static
Parse_set * mk_parse_set(fast_matcher_t *mchxt,
                 count_context_t * ctxt,
                 int lw, int rw,
                 Connector *le, Connector *re, unsigned int null_count,
                 extractor_t * pex)
{
	int start_word, end_word, w;
	Pset_bucket *xt;
	Count_bin *count;

	if (!valid_nearest_words(le, re, lw, rw)) return NULL;

	assert(null_count < 0x7fff, "Called with null_count < 0.");

	count = table_lookup(ctxt, lw, rw, le, re, null_count, NULL);

	/* If there's no counter, then there's no way to parse. */
	if (NULL == count) return NULL;
	if (hist_total(count) == 0) return NULL;

	xt = x_table_pointer(lw, rw, le, re, null_count, pex);

	/* Perhaps we've already computed it; if so, return it. */
	if (xt != NULL) return &xt->set;

	/* Start it out with the empty set of parse choices. */
	/* This entry must be updated before we return. */
	xt = x_table_store(lw, rw, le, re, null_count, pex);

	/* The count we previously computed; it's non-zero. */
	xt->set.count = hist_total(count);

	//#define NUM_PARSES 4
	// xt->set.cost_cutoff = hist_cost_cutoff(count, NUM_PARSES);
	// xt->set.cut_count = hist_cut_total(count, NUM_PARSES);

	RECOUNT({xt->set.recount = 1;})

	/* If the two words are next to each other, the count == 1 */
	if (lw + 1 == rw) return &xt->set;

	/* The left and right connectors are null, but the two words are
	 * NOT next to each-other.  */
	if ((le == NULL) && (re == NULL))
	{
		Parse_set* pset;
		Parse_set* dummy;
		Disjunct* dis;

		if (!pex->islands_ok && (lw != -1) && (pex->words[lw].d != NULL))
			return &xt->set;
		if (null_count == 0) return &xt->set;

		RECOUNT({xt->set.recount = 0;})

		w = lw + 1;
		for (int opt = 0; opt <= (int)pex->words[w].optional; opt++)
		{
			null_count += opt;
			for (dis = pex->words[w].d; dis != NULL; dis = dis->next)
			{
				if (dis->left == NULL)
				{
					pset = mk_parse_set(mchxt, ctxt,
					                    w, rw, dis->right, NULL,
					                    null_count-1, pex);
					if (pset == NULL) continue;
					dummy = dummy_set(lw, w, null_count-1, pex);
					record_choice(dummy, NULL,
					              pset, dis->right,
					              dis, &xt->set, pex);
					RECOUNT({xt->set.recount += pset->recount;})
				}
			}
			pset = mk_parse_set(mchxt, ctxt,
			                    w, rw, NULL, NULL,
			                    null_count-1, pex);
			if (pset != NULL)
			{
				dummy = dummy_set(lw, w, null_count-1, pex);
				record_choice(dummy, NULL,
				              pset,  NULL,
				              NULL, &xt->set, pex);
				RECOUNT({xt->set.recount += pset->recount;})
			}
		}
		return &xt->set;
	}

	if (le == NULL)
	{
		start_word = MAX(lw+1, re->farthest_word);
	}
	else
	{
		start_word = le->nearest_word;
	}

	if (re == NULL)
	{
		end_word = MIN(rw, le->farthest_word+1);
	}
	else
	{
		if ((le != NULL) && (re->nearest_word > le->farthest_word))
			end_word = le->farthest_word + 1;
		else
			end_word = re->nearest_word + 1;
	}

	/* This condition can never be true here. It is included so GCC
	 * will be able to optimize the loop over "null_count".  Without
	 * this check, GCC thinks this loop may be an infinite loop and
	 * it may omit some optimizations. */
	if (UINT_MAX == null_count) return NULL;

	RECOUNT({xt->set.recount = 0;})
	for (w = start_word; w < end_word; w++)
	{
		/* Start of nonzero leftcount/rightcount range cache check. */
		Connector *fml_re = re;       /* For form_match_list() only */

		if (le != NULL)
		{
			if (no_count(ctxt, 0, le, w - le->nearest_word, null_count)) continue;
			if ((re != NULL) && (re->farthest_word <= w))
			{
				if (no_count(ctxt, 1, re, w - re->farthest_word, null_count))
					fml_re = NULL;
			}
		}
		else
		{
			/* Here re != NULL. */
			if (no_count(ctxt, 1, re, w - re->farthest_word, null_count)) continue;
		}
		/* End of nonzero leftcount/rightcount range cache check. */

		Disjunct ***mlcl = NULL, ***mlcr = NULL;

		if (le != NULL)
			mlcl = get_cached_match_list(ctxt, 0, w, le);
		if (fml_re != NULL && ((le == NULL) || (re->farthest_word <= w)))
			mlcr = get_cached_match_list(ctxt, 1, w, re);

		size_t mlb = form_match_list(mchxt, w, le, lw, fml_re, rw, mlcl, mlcr);
		if (pex->sort_match_list) sort_match_list(mchxt, mlb);

		for (size_t mle = mlb; get_match_list_element(mchxt, mle) != NULL; mle++)
		{
			Disjunct *d = get_match_list_element(mchxt, mle);
			bool Lmatch = d->match_left;
			bool Rmatch = d->match_right;

			for (unsigned int lnull_count = 0; lnull_count <= null_count; lnull_count++)
			{
				int i, j;
				Parse_set *ls[4], *rs[4];
				bool ls_exists = false;

				/* Here, lnull_count and rnull_count are the null_counts
				 * we're assigning to those parts respectively. */
				unsigned int rnull_count = null_count - lnull_count;

				/* Now, we determine if (based on table only) we can see that
				   the current range is not parsable. */

				for (i=0; i<4; i++) { ls[i] = rs[i] = NULL; }
				if (Lmatch)
				{
					ls[0] = mk_parse_set(mchxt, ctxt,
					             lw, w, le->next, d->left->next,
					             lnull_count, pex);

					if (le->multi)
						ls[1] = mk_parse_set(mchxt, ctxt,
						              lw, w, le, d->left->next,
						              lnull_count, pex);

					if (d->left->multi)
						ls[2] = mk_parse_set(mchxt, ctxt,
						              lw, w, le->next, d->left,
						              lnull_count, pex);

					if (le->multi && d->left->multi)
						ls[3] = mk_parse_set(mchxt, ctxt,
						              lw, w, le, d->left,
						              lnull_count, pex);

					if (ls[0] != NULL || ls[1] != NULL || ls[2] != NULL || ls[3] != NULL)
					{
						ls_exists = true;
						/* Evaluate using the left match, but not the right */
						Parse_set* rset = mk_parse_set(mchxt, ctxt,
						                               w, rw, d->right, re,
						                               rnull_count, pex);
						if (rset != NULL)
						{
							for (i=0; i<4; i++)
							{
								if (ls[i] == NULL) continue;
								/* this ordering is probably not consistent with
								 * that needed to use list_links */
								record_choice(ls[i], d->left,
								              rset,  NULL /* d->right */,
								              d, &xt->set, pex);
								RECOUNT({xt->set.recount += (w_count_t)ls[i]->recount * rset->recount;})
							}
						}
					}
				}

				if (Rmatch && (ls_exists || le == NULL))
				{
					rs[0] = mk_parse_set(mchxt, ctxt,
					                 w, rw, d->right->next, re->next,
					                 rnull_count, pex);

					if (d->right->multi)
						rs[1] = mk_parse_set(mchxt, ctxt,
					                 w, rw, d->right, re->next,
						              rnull_count, pex);

					if (re->multi)
						rs[2] = mk_parse_set(mchxt, ctxt,
						              w, rw, d->right->next, re,
						              rnull_count, pex);

					if (d->right->multi && re->multi)
						rs[3] = mk_parse_set(mchxt, ctxt,
						              w, rw, d->right, re,
						              rnull_count, pex);

					if ((rs[0] != NULL || rs[1] != NULL || rs[2] != NULL || rs[3] != NULL))
					{
						if (le == NULL)
						{
							/* Evaluate using the right match, but not the left */
							Parse_set* lset = mk_parse_set(mchxt, ctxt,
							                               lw, w, le, d->left,
							                               lnull_count, pex);

							if (lset != NULL)
							{
								for (j=0; j<4; j++)
								{
									if (rs[j] == NULL) continue;
									/* this ordering is probably not consistent with
									 * that needed to use list_links */
									record_choice(lset,
									              d->left,  /* NULL indicates no link */
									              rs[j], d->right,
									              d, &xt->set, pex);
									RECOUNT({xt->set.recount += lset->recount * rs[j]->recount;})
								}
							}
						}
						else
						{
							for (i=0; i<4; i++)
							{
								/* This ordering is probably not consistent with that
								 * needed to use list_links. (??) */
								if (ls[i] == NULL) continue;
								for (j=0; j<4; j++)
								{
									if (rs[j] == NULL) continue;
									record_choice(ls[i], d->left,
									              rs[j], d->right,
									              d, &xt->set, pex);
									RECOUNT({xt->set.recount += ls[i]->recount * rs[j]->recount;})
								}
							}
						}
					}
				}
			}
		}
		pop_match_list(mchxt, mlb);
	}
	return &xt->set;
}

/**
 * Return TRUE if and only if an overflow in the number of parses
 * occurred. Use a 64-bit int for counting.
 */
static bool set_node_overflowed(Parse_set *set)
{
	Parse_choice *pc;
	w_count_t n = 0;
	if (set == NULL || set->first == NULL) return false;

	for (pc = set->first; pc != NULL; pc = pc->next)
	{
		n  += (w_count_t)pc->set[0]->count * pc->set[1]->count;
		if (PARSE_NUM_OVERFLOW < n) return true;
	}
	return false;
}

static bool set_overflowed(extractor_t * pex)
{
	unsigned int i;

	assert(pex->x_table != NULL, "called set_overflowed with x_table==NULL");
	for (i=0; i<pex->x_table_size; i++)
	{
		Pset_bucket *t;
		for (t = pex->x_table[i]; t != NULL; t = t->next)
		{
			if (set_node_overflowed(&t->set)) return true;
		}
	}
	return false;
}

/**
 * This is the top level call that computes the whole parse-set.
 * It creates the necessary hash table (x_table) which will be freed at
 * the same time the parse-set related memory is freed.
 *
 * This assumes that do_parse() has been run, and that the count_context
 * is filled with the values thus computed.  This function is structured
 * much like do_parse(), which wraps the main workhorse do_count().
 *
 * If the number of linkages gets huge, then the counts can overflow.
 * We check if this has happened when verifying the parse-set.
 * This routine returns TRUE iff an overflow occurred.
 */

bool build_parse_set(extractor_t* pex, Sentence sent,
                    fast_matcher_t *mchxt,
                    count_context_t *ctxt,
                    unsigned int null_count, Parse_Options opts)
{
	pex->words = sent->word;
	pex->islands_ok = opts->islands_ok;
	pex->sort_match_list = test_enabled("sort-match-list");

	pex->parse_set =
		mk_parse_set(mchxt, ctxt,
		             -1, sent->length, NULL, NULL, null_count+1, pex);

	return set_overflowed(pex);
}

/**
 * Assemble the link array and the chosen_disjuncts of a linkage.
 */
static void issue_link(Linkage lkg, int lr, Parse_choice *pc,
                       const Parse_set *set)
{
	Connector *lc = lr ? pc->rc : set->le;
	if (lc == NULL) return; /* No choice to record. */

	lkg->chosen_disjuncts[lr ? pc->set[1]->lw : pc->set[0]->rw] = pc->md;

	Connector *rc = lr ? set->re : pc->lc;
	if (rc == NULL) return; /* No link to generate. */

	assert(lkg->num_links < lkg->lasz, "Linkage array too small!");
	Link *link = &lkg->link_array[lkg->num_links];
	link->lw = pc->set[lr]->lw;
	link->rw = pc->set[lr]->rw;
	link->lc = lc;
	link->rc = rc;
	lkg->num_links++;
}

static void issue_links_for_choice(Linkage lkg, Parse_choice *pc,
                                   const Parse_set *set)
{
	issue_link(lkg, /*lr*/0, pc, set);
	issue_link(lkg, /*lr*/1, pc, set);
}

/**
 * Recursively find the \p index'th path in the parse-set tree provided by
 * \p Parse_set and construct the corresponding linkage into the result
 * parameter \p lkg.
 *
 * In order to generate all the possible linkages, the top-level function
 * is repetitively invoked, when \p index is changing from 0 to
 * \c num_linkages_found-1 (by extract_links(), see process_linkages()).
 *
 * How it works:
 *
 * Each "level" in the parse-set tree consists of a linked lists of
 * Parse_choice elements. Each such element is pointed to by a
 * Parse_choice element of an upper level. Each parse_choice element
 * contains two Parse_set elements, that are denoted below as S0 and S1.
 *
 * The algo is based on our knowledge of the exact number of paths in each
 * Parse_set element. Note that the count of the root Parse_set (used at
 * the top-level invocation) is equal to num_linkages_found.
 *
 * Each list_links() invocation is done with an \p index parameter
 * within the range of 0 to \c set->count-1 in order to extract all the
 * paths from this set. All the \p index values in that range are used.
 *
 * First a selection of the Parse_choice element within the given
 * Parse_set (with cardinality c) is done.
 * We know that:
 *             c-1
 * set->count = ∑ S0ₘ * S1ₘ
 *             m=0
 * when S0ₘ and S1ₘ are the number of elements in S0  and S1
 * (correspondingly) of the m'th Parse_choice element in the chain.
 *
 * The linkage paths of of a Parse_set element are distributed between its
 * Parse_choice elements, each has its share ((S0ₘ * S1ₘ) for the m'th
 * element). We scan these Parse_choice elements until we find the element
 * that corresponds to \p index. A new index (called Nindex below) is
 * computed, which has the property of ranging between 0 and
 * (S0ₘ * S1ₘ)-1. It is used to further select a path in the selected
 * Parse_choice element m. To that end we need to use its S0 and S1
 * components in all possible combinations (when the total number of
 * combinations is (S0ₘ * S1ₘ)):
 *
 * For S0: (Nindex % pc->set[0]->count) ranges from 0 to (S0ₘ-1).
 * For S1: (Nindex / pc->set[0]->count) ranges from 0 to (S1ₘ-1).
 */
static void list_links(Linkage lkg, const Parse_set * set, int index)
{
	Parse_choice *pc;
	count_t n; /* No overflow - see extract_links() and process_linkages() */

	if (set == NULL || set->first == NULL) return;
	for (pc = set->first; pc != NULL; pc = pc->next) {
		n = pc->set[0]->count * pc->set[1]->count;
		if (index < n) break;
		index -= n;
	}
	assert(pc != NULL, "walked off the end in list_links");
	issue_links_for_choice(lkg, pc, set);
	list_links(lkg, pc->set[0], index % pc->set[0]->count);
	list_links(lkg, pc->set[1], index / pc->set[0]->count);
}

static void list_random_links(Linkage lkg, unsigned int *rand_state,
                              const Parse_set * set)
{
	Parse_choice *pc;
	int num_pc, new_index;

	if (set == NULL || set->first == NULL) return;

	/* Most of the time, there is only one list element. */
	if (set->first->next == NULL)
	{
		pc = set->first;
	}
	else
	{
		num_pc = 0;
		for (pc = set->first; pc != NULL; pc = pc->next) {
			num_pc++;
		}

		new_index = rand_r(rand_state) % num_pc;

		num_pc = 0;
		for (pc = set->first; new_index != num_pc; pc = pc->next) {
			num_pc++;
		}
	}

	issue_links_for_choice(lkg, pc, set);
	list_random_links(lkg, rand_state, pc->set[0]);
	list_random_links(lkg, rand_state, pc->set[1]);
}

/**
 * Generate the list of all links of the index'th parsing of the
 * sentence.  For this to work, you must have already called parse, and
 * already built the whole_set.
 */
void extract_links(extractor_t * pex, Linkage lkg)
{
	int index = lkg->lifo.index;
	if (index < 0)
	{
		bool repeatable = false;
		if (0 == pex->rand_state) repeatable = true;
		if (repeatable) pex->rand_state = index;
		list_random_links(lkg, &pex->rand_state, pex->parse_set);
		if (repeatable)
			pex->rand_state = 0;
		else
			lkg->sent->rand_state = pex->rand_state;
	}
	else {
		list_links(lkg, pex->parse_set, index);
	}
}

static void mark_used_disjunct(Parse_set *set, bool *disjunct_used)
{
	if (set == NULL || set->first == NULL) return;

	for (Parse_choice *pc = set->first; pc != NULL; pc = pc->next)
	{
		if (pc->md->ordinal != -1)
			disjunct_used[pc->md->ordinal] = true;
	}
}

void mark_used_disjuncts(extractor_t *pex, bool *disjunct_used)
{
	assert(pex->x_table != NULL, "x_table==NULL");

	for (unsigned int i = 0; i < pex->x_table_size; i++)
	{
		for (Pset_bucket *t = pex->x_table[i]; t != NULL; t = t->next)
			mark_used_disjunct(&t->set, disjunct_used);
	}
}
