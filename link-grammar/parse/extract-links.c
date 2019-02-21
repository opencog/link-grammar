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

#include "connectors.h"
#include "count.h"
#include "disjunct-utils.h"             // Disjunct
#include "extract-links.h"
#include "utilities.h"                  // Windows rand_r()
#include "linkage/linkage.h"
#include "tokenize/word-structures.h"   // Word_Struct

//#define RECOUNT

//#define DEBUG_X_TABLE
#ifdef DEBUG_X_TABLE
#undef DEBUG_X_TABLE
#define DEBUG_X_TABLE(...) __VA_ARGS__
#else
#define DEBUG_X_TABLE(...)
#endif

typedef struct Parse_choice_struct Parse_choice;

/* The parse_choice is used to extract links for a given parse */
typedef struct Parse_set_struct Parse_set;
struct Parse_choice_struct
{
	Parse_choice * next;
	Parse_set * set[2];
	Link        link[2]; /* the lc fields of these is NULL if there is no link used */
	Disjunct    *md;     /* the chosen disjunct for the middle word */
};

struct Parse_set_struct
{
	short          lw, rw; /* left and right word index */
	unsigned int   null_count; /* number of island words */
	int            l_id, r_id; /* pending, unconnected connectors */

	s64 count;      /* The number of ways to parse. */
#ifdef RECOUNT
	s64 recount;  /* Exactly the same as above, but counted at a later stage. */
	s64 cut_count;  /* Count only low-cost parses, i.e. below the cost cutoff */
	//double cost_cutoff;
#undef RECOUNT
#define RECOUNT(X) X
#else
#define RECOUNT(X)  /* Make it disappear... */
#endif
	Parse_choice * first;
	Parse_choice * tail;
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
	unsigned int   log2_x_table_size;
	Pset_bucket ** x_table;  /* Hash table */
	Parse_set *    parse_set;
	Word           *words;
	bool           islands_ok;
	bool           sort_match_list;

	/* thread-safe random number state */
	unsigned int rand_state;
};

/**
 * The first thing we do is we build a data structure to represent the
 * result of the entire parse search.  There will be a set of nodes
 * built for each call to the count() function that returned a non-zero
 * value, AND which is part of a valid linkage.  Each of these nodes
 * represents a valid continuation, and contains pointers to two other
 * sets (one for the left continuation and one for the right
 * continuation).
 */

static void free_set(Parse_set *s)
{
	Parse_choice *p, *xp;
	if (s == NULL) return;
	for (p=s->first; p != NULL; p = xp)
	{
		xp = p->next;
		xfree((void *)p, sizeof(*p));
	}
}

static Parse_choice *
make_choice(Parse_set *lset, Connector * llc, Connector * lrc,
            Parse_set *rset, Connector * rlc, Connector * rrc,
            Disjunct *md)
{
	Parse_choice *pc;
	pc = (Parse_choice *) xalloc(sizeof(*pc));
	pc->next = NULL;
	pc->set[0] = lset;
	pc->link[0].link_name = NULL;
	pc->link[0].lw = lset->lw;
	pc->link[0].rw = lset->rw;
	pc->link[0].lc = llc;
	pc->link[0].rc = lrc;
	pc->set[1] = rset;
	pc->link[1].link_name = NULL;
	pc->link[1].lw = rset->lw;
	pc->link[1].rw = rset->rw;
	pc->link[1].lc = rlc;
	pc->link[1].rc = rrc;
	pc->md = md;
	return pc;
}

/**
 * Put this parse_choice into a given set.  The tail pointer is always
 * left pointing to the end of the list.
 */
static void put_choice_in_set(Parse_set *s, Parse_choice *pc)
{
	if (s->first == NULL)
	{
		s->first = pc;
	}
	else
	{
		s->tail->next = pc;
	}
	s->tail = pc;
	pc->next = NULL;
}

static void record_choice(
    Parse_set *lset, Connector * llc, Connector * lrc,
    Parse_set *rset, Connector * rlc, Connector * rrc,
    Disjunct *md, Parse_set *s)
{
	put_choice_in_set(s, make_choice(lset, llc, lrc,
	                                 rset, rlc, rrc,
	                                 md));
}

/**
 * Allocate the parse info struct
 *
 * A piecewise exponential function determines the size of the hash
 * table.  Probably should make use of the actual number of disjuncts,
 * rather than just the number of words.
 */
extractor_t * extractor_new(int nwords, unsigned int ranstat)
{
	int log2_table_size;
	extractor_t * pex;

	pex = (extractor_t *) xalloc(sizeof(extractor_t));
	memset(pex, 0, sizeof(extractor_t));
	pex->rand_state = ranstat;

	/* Alloc the x_table */
	if (nwords > 96) {
		log2_table_size = 15 + nwords / 48;
	} else if (nwords >= 10) {
		log2_table_size = 14 + nwords / 24;
	} else if (nwords >= 4) {
		log2_table_size = 1 + 1.5 * nwords;
	} else {
		log2_table_size = 5;
	}
	/* if (log2_table_size > 21) log2_table_size = 21; */
	pex->log2_x_table_size = log2_table_size;
	pex->x_table_size = (1 << log2_table_size);

	DEBUG_X_TABLE(
		printf("Allocating x_table of size %d (nwords %d)\n",
		       pex->x_table_size, nwords);
	)
	pex->x_table = (Pset_bucket**) xalloc(pex->x_table_size * sizeof(Pset_bucket*));
	memset(pex->x_table, 0, pex->x_table_size * sizeof(Pset_bucket*));

	return pex;
}

/**
 * This is the function that should be used to free the set structure. Since
 * it's a dag, a recursive free function won't work.  Every time we create
 * a set element, we put it in the hash table, so this is OK.
 */
void free_extractor(extractor_t * pex)
{
	unsigned int i;
	Pset_bucket *t, *x;
	if (!pex) return;

	DEBUG_X_TABLE(int N = 0;)
	for (i=0; i<pex->x_table_size; i++)
	{
		DEBUG_X_TABLE(int c = 0;)
		for (t = pex->x_table[i]; t!= NULL; t=x)
		{
			DEBUG_X_TABLE(c++;)
			x = t->next;
			free_set(&t->set);
			xfree((void *) t, sizeof(Pset_bucket));
		}
		DEBUG_X_TABLE(
			if (c > 0)
				;//printf("I %d: chain %d\n", i, c);
			else
				N++;
		)
	}
	DEBUG_X_TABLE(
		printf("Used x_table %d/%d %.2f%%\n",
				 pex->x_table_size-N, pex->x_table_size,
				 100.0f*(pex->x_table_size-N)/pex->x_table_size);
	)
	pex->parse_set = NULL;

	//printf("Freeing x_table of size %d\n", pex->x_table_size);
	xfree((void *) pex->x_table, pex->x_table_size * sizeof(Pset_bucket*));
	pex->x_table_size = 0;
	pex->x_table = NULL;

	xfree((void *) pex, sizeof(extractor_t));
}

static inline unsigned int el_pair_hash(extractor_t * pex,
                            int lw, int rw,
                            const Connector *le, const Connector *re,
                            unsigned int null_count)
{
	unsigned int i;
	size_t table_size = pex->x_table_size;
	int l_id = 0, r_id = 0;

	if (NULL != le) l_id = le->suffix_id;
	if (NULL != re) r_id = re->suffix_id;

#ifdef DEBUG
	assert(((NULL == le) || le->suffix_id) &&
	       ((NULL == re) || re->suffix_id));
#endif

   /* sdbm-based hash */
	i = null_count;
	i = lw + (i << 6) + (i << 16) - i;
	i = rw + (i << 6) + (i << 16) - i;
	i = l_id + (i << 6) + (i << 16) - i;
	i = r_id + (i << 6) + (i << 16) - i;

	return i & (table_size-1);
}
/**
 * Returns the pointer to this info, NULL if not there.
 * Note that there is no need to use (lw, rw) as keys because suffix_id
 * is also encoded by the word number.
 */
static Pset_bucket * x_table_pointer(int lw, int rw,
                              Connector *le, Connector *re,
                              unsigned int null_count, extractor_t * pex)
{
	Pset_bucket *t;
	t = pex->x_table[el_pair_hash(pex, lw, rw, le, re, null_count)];
	int l_id = (NULL != le) ? le->suffix_id : lw;
	int r_id = (NULL != re) ? re->suffix_id : rw;

	for (; t != NULL; t = t->next) {
		if ((t->set.l_id == l_id) && (t->set.r_id == r_id) &&
		    (t->set.null_count == null_count)) return t;
	}
	return NULL;
}

/**
 * Stores the value in the x_table.  Assumes it's not already there.
 * Since disjuncts of connectors with the same suffix_id may have
 * different costs and may belong to different words, the disjunct cost
 * and word_string are saved, to be used as retrieval keys in
 * x_table_pointer().
 */
static Pset_bucket * x_table_store(int lw, int rw,
                                  Connector *le, Connector *re,
                                  unsigned int null_count, extractor_t * pex)
{
	Pset_bucket *t, *n;
	unsigned int h;

	n = (Pset_bucket *) xalloc(sizeof(Pset_bucket));
	n->set.lw = lw;
	n->set.rw = rw;
	n->set.null_count = null_count;
	n->set.l_id = (NULL != le) ? le->suffix_id : lw;
	n->set.r_id = (NULL != re) ? re->suffix_id : rw;
	n->set.count = 0;
	n->set.first = NULL;
	n->set.tail = NULL;

	h = el_pair_hash(pex, lw, rw, le, re, null_count);
	t = pex->x_table[h];
	n->next = t;
	pex->x_table[h] = n;
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
	Count_bin * count;

	assert(null_count < 0x7fff, "mk_parse_set() called with null_count < 0.");

	count = table_lookup(ctxt, lw, rw, le, re, null_count);

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

#define NUM_PARSES 4
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
		for (int opt = 0; opt <= !!pex->words[w].optional; opt++)
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
					record_choice(dummy, NULL, NULL,
									  pset, dis->right, NULL,
									  dis, &xt->set);
					RECOUNT({xt->set.recount += pset->recount;})
				}
			}
			pset = mk_parse_set(mchxt, ctxt,
									  w, rw, NULL, NULL,
									  null_count-1, pex);
			if (pset != NULL)
			{
				dummy = dummy_set(lw, w, null_count-1, pex);
				record_choice(dummy, NULL, NULL,
								  pset,  NULL, NULL,
								  NULL, &xt->set);
				RECOUNT({xt->set.recount += pset->recount;})
			}
		}
		return &xt->set;
	}

	if (le == NULL)
	{
		start_word = lw + 1;
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
		size_t mlb = form_match_list(mchxt, w, le, lw, re, rw);
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

					ls_exists =
						ls[0] != NULL || ls[1] != NULL || ls[2] != NULL || ls[3] != NULL;
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
				}

				for (i=0; i<4; i++)
				{
					/* This ordering is probably not consistent with that
					 * needed to use list_links. (??) */
					if (ls[i] == NULL) continue;
					for (j=0; j<4; j++)
					{
						if (rs[j] == NULL) continue;
						record_choice(ls[i], le, d->left,
						              rs[j], d->right, re,
						              d, &xt->set);
						RECOUNT({xt->set.recount += ls[i]->recount * rs[j]->recount;})
					}
				}

				if (ls_exists)
				{
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
							record_choice(ls[i], le, d->left,
							              rset,  NULL /* d->right */,
							              re,  /* the NULL indicates no link*/
							              d, &xt->set);
							RECOUNT({xt->set.recount += ls[i]->recount * rset->recount;})
						}
					}
				}
				else if ((le == NULL) && (rs[0] != NULL ||
				     rs[1] != NULL || rs[2] != NULL || rs[3] != NULL))
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
							record_choice(lset, NULL /* le */,
							              d->left,  /* NULL indicates no link */
							              rs[j], d->right, re,
							              d, &xt->set);
							RECOUNT({xt->set.recount += lset->recount * rs[j]->recount;})
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
	s64 n = 0;
	if (set == NULL || set->first == NULL) return false;

	for (pc = set->first; pc != NULL; pc = pc->next)
	{
		n  += pc->set[0]->count * pc->set[1]->count;
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
 * This is the top level call that computes the whole parse_set.  It
 * points whole_set at the result.  It creates the necessary hash
 * table (x_table) which will be freed at the same time the
 * whole_set is freed.
 *
 * This assumes that do_parse() has been run, and that the count_context
 * is filled with the values thus computed.  This function is structured
 * much like do_parse(), which wraps the main workhorse do_count().
 *
 * If the number of linkages gets huge, then the counts can overflow.
 * We check if this has happened when verifying the parse set.
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

// Cannot be static, also called by SAT-solver.
void check_link_size(Linkage lkg)
{
	if (lkg->lasz <= lkg->num_links)
	{
		lkg->lasz = 2 * lkg->lasz + 10;
		lkg->link_array = realloc(lkg->link_array, lkg->lasz * sizeof(Link));
	}
}

/**
 * Assemble the link array and the chosen_disjuncts of a linkage.
 */
static void issue_link(Linkage lkg, bool lr, Disjunct *md, Link *link)
{
	if (link->rc != NULL)
	{
		check_link_size(lkg);
		lkg->link_array[lkg->num_links] = *link;
		lkg->num_links++;
	}

	lkg->chosen_disjuncts[lr ? link->lw : link->rw] = md;
}

static void issue_links_for_choice(Linkage lkg, Parse_choice *pc)
{
	if (pc->link[0].lc != NULL) { /* there is a link to generate */
		issue_link(lkg, /*lr*/false, pc->md, &pc->link[0]);
	}
	if (pc->link[1].lc != NULL) {
		issue_link(lkg, /*lr*/true, pc->md, &pc->link[1]);
	}
}

static void list_links(Linkage lkg, const Parse_set * set, int index)
{
	 Parse_choice *pc;
	 s64 n;

	 if (set == NULL || set->first == NULL) return;
	 for (pc = set->first; pc != NULL; pc = pc->next) {
		  n = pc->set[0]->count * pc->set[1]->count;
		  if (index < n) break;
		  index -= n;
	 }
	 assert(pc != NULL, "walked off the end in list_links");
	 issue_links_for_choice(lkg, pc);
	 list_links(lkg, pc->set[0], index % pc->set[0]->count);
	 list_links(lkg, pc->set[1], index / pc->set[0]->count);
}

static void list_random_links(Linkage lkg, unsigned int *rand_state,
                              const Parse_set * set)
{
	Parse_choice *pc;
	int num_pc, new_index;

	if (set == NULL || set->first == NULL) return;
	num_pc = 0;
	for (pc = set->first; pc != NULL; pc = pc->next) {
		num_pc++;
	}

	new_index = rand_r(rand_state) % num_pc;

	num_pc = 0;
	for (pc = set->first; pc != NULL; pc = pc->next) {
		if (new_index == num_pc) break;
		num_pc++;
	}

	issue_links_for_choice(lkg, pc);
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
