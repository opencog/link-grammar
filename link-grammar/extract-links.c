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

#include <limits.h>  /* For UINT_MAX */

#include "count.h"
#include "disjunct-utils.h" // for Disjunct
#include "extract-links.h"
#include "fast-match.h"
#include "linkage.h"
#include "word-utils.h"
#include "structures.h" // for Word_Struct

typedef struct Parse_choice_struct Parse_choice;

/* The parse_choice is used to extract links for a given parse */
struct Parse_choice_struct
{
	Parse_choice * next;
	Parse_set * set[2];
	Link        link[2];   /* the lc fields of these is NULL if there is no link used */
	Disjunct *ld, *md, *rd;  /* the chosen disjuncts for the relevant three words */
};

struct Parse_set_struct
{
	short          lw, rw; /* left and right word index */
	unsigned short null_count; /* number of island words */
	Connector      *le, *re; /* pending, unconnected connectors */

	s64 count;      /* The number of ways to parse. */
	/* s64 recount;  Exactly the same as above, but counted at a later stage. */
	// s64 cut_count;  /* Count only low-cost parses, i.e. below the cost cutoff */
	// double cost_cutoff;
	Parse_choice * first;
	Parse_choice * tail;
};

struct X_table_connector_struct
{
	Parse_set         set;
	X_table_connector *next;
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
            Disjunct *ld, Disjunct *md, Disjunct *rd)
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
	pc->ld = ld;
	pc->md = md;
	pc->rd = rd;
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
    Disjunct *ld, Disjunct *md, Disjunct *rd, Parse_set *s)
{
	put_choice_in_set(s, make_choice(lset, llc, lrc,
	                                 rset, rlc, rrc,
	                                 ld, md, rd));
}

/**
 * Allocate the parse info struct
 *
 * A piecewise exponential function determines the size of the hash
 * table.  Probably should make use of the actual number of disjuncts,
 * rather than just the number of words.
 */
Parse_info parse_info_new(int nwords)
{
	int log2_table_size;
	Parse_info pi;

	pi = (Parse_info) xalloc(sizeof(struct Parse_info_struct));
	memset(pi, 0, sizeof(struct Parse_info_struct));
	pi->N_words = nwords;

	/* Alloc the x_table */
	if (nwords >= 10) {
		log2_table_size = 14;
	} else if (nwords >= 4) {
		log2_table_size = nwords;
	} else {
		log2_table_size = 4;
	}
	pi->log2_x_table_size = log2_table_size;
	pi->x_table_size = (1 << log2_table_size);

	/*printf("Allocating x_table of size %d\n", x_table_size);*/
	pi->x_table = (X_table_connector**) xalloc(pi->x_table_size * sizeof(X_table_connector*));
	memset(pi->x_table, 0, pi->x_table_size * sizeof(X_table_connector*));

	return pi;
}

/**
 * This is the function that should be used to free the set structure. Since
 * it's a dag, a recursive free function won't work.  Every time we create
 * a set element, we put it in the hash table, so this is OK.
 */
void free_parse_info(Parse_info pi)
{
	unsigned int i;
	X_table_connector *t, *x;
	if (!pi) return;

	for (i=0; i<pi->x_table_size; i++)
	{
		for (t = pi->x_table[i]; t!= NULL; t=x)
		{
			x = t->next;
			free_set(&t->set);
			xfree((void *) t, sizeof(X_table_connector));
		}
	}
	pi->parse_set = NULL;

	/*printf("Freeing x_table of size %d\n", x_table_size);*/
	xfree((void *) pi->x_table, pi->x_table_size * sizeof(X_table_connector*));
	pi->x_table_size = 0;
	pi->x_table = NULL;

	xfree((void *) pi, sizeof(struct Parse_info_struct));
}

/**
 * Returns the pointer to this info, NULL if not there.
 */
static X_table_connector * x_table_pointer(int lw, int rw,
                              Connector *le, Connector *re,
                              unsigned int null_count, Parse_info pi)
{
	X_table_connector *t;
	t = pi->x_table[pair_hash(pi->x_table_size, lw, rw, le, re, null_count)];
	for (; t != NULL; t = t->next) {
		if ((t->set.lw == lw) && (t->set.rw == rw) &&
		    (t->set.le == le) && (t->set.re == re) &&
		    (t->set.null_count == null_count))  return t;
	}
	return NULL;
}

/**
 * Stores the value in the x_table.  Assumes it's not already there.
 */
static X_table_connector * x_table_store(int lw, int rw,
                                  Connector *le, Connector *re,
                                  unsigned int null_count, Parse_info pi)
{
	X_table_connector *t, *n;
	unsigned int h;

	n = (X_table_connector *) xalloc(sizeof(X_table_connector));
	n->set.lw = lw;
	n->set.rw = rw;
	n->set.null_count = null_count;
	n->set.le = le;
	n->set.re = re;
	n->set.count = 0;
	n->set.first = NULL;
	n->set.tail = NULL;

	h = pair_hash(pi->x_table_size, lw, rw, le, re, null_count);
	t = pi->x_table[h];
	n->next = t;
	pi->x_table[h] = n;
	return n;
}

/** Create a bogus parse set that only holds lw, rw. */
static Parse_set* dummy_set(int lw, int rw,
                            unsigned int null_count, Parse_info pi)
{
	X_table_connector *dummy;
	dummy = x_table_pointer(lw, rw, NULL, NULL, null_count, pi);
	if (dummy) return &dummy->set;

	dummy = x_table_store(lw, rw, NULL, NULL, null_count, pi);
	dummy->set.count = 1;
	return &dummy->set;
}

#ifdef FINISH_THIS_IDEA_MAYBE_LATER
static int cost_compare(const void *a, const void *b)
{
	const Match_node* const * ma = a;
	const Match_node* const * mb = b;
	if ((*ma)->d->cost < (*mb)->d->cost) return -1;
	if ((*ma)->d->cost > (*mb)->d->cost) return 1;
	return 0;
}

/**
 * Sort the matchlist into ascending disjunct cost. The goal here
 * is to issue the lowest-cost disjuncts first, so that the parse
 * set ends up quasi-sorted. This is not enough to get us a totally
 * sorted parse set, but it does guarantee that at least the very
 * first parse really will be the lowest cost.
 */
static Match_node* sort_matchlist(Match_node* mlist)
{
	Match_node* mx;
	Match_node** marr;
	size_t len = 1;
	size_t i;

	for (mx = mlist; mx->next != NULL; mx = mx->next) len++;
	if (1 == len) return mlist;

	/* Avoid blowing out the stack. Its hopeless. */
	if (100000 < len) return mlist;

	marr = alloca(len * sizeof(Match_node*));
	i = 0;
	for (mx = mlist; mx != NULL; mx = mx->next) marr[i++] = mx;

	qsort((void *) marr, len, sizeof(Match_node*), cost_compare);
	for (i=0; i<len-1; i++) marr[i]->next = marr[i+1];
	marr[len-1]->next = NULL;
	return marr[0];
}
#endif /* FINISH_THIS_IDEA_MAYBE_LATER */

/**
 * returns NULL if there are no ways to parse, or returns a pointer
 * to a set structure representing all the ways to parse.
 *
 * This code is similar to do_count() in count.c -- for a good reason:
 * the do_count() function did a full parse, but didn't actually
 * allocate an memory structures to hold the parse.  This also does
 * a full parse, but it also allocates and fills out the various
 * parse structures.
 */
static
Parse_set * mk_parse_set(Sentence sent, fast_matcher_t *mchxt,
                 count_context_t * ctxt,
                 Disjunct *ld, Disjunct *rd, int lw, int rw,
                 Connector *le, Connector *re, unsigned int null_count,
                 bool islands_ok, Parse_info pi)
{
	int start_word, end_word, w;
	X_table_connector *xt;
	Count_bin * count;

	assert(null_count < 0x7fff, "mk_parse_set() called with null_count < 0.");

	count = table_lookup(ctxt, lw, rw, le, re, null_count);

	/* If there's no counter, then there's no way to parse. */
	if (NULL == count) return NULL;
	if (hist_total(count) == 0) return NULL;

	xt = x_table_pointer(lw, rw, le, re, null_count, pi);

	/* Perhaps we've already computed it; if so, return it. */
	if (xt != NULL) return &xt->set;

	/* Start it out with the empty set of parse choices. */
	/* This entry must be updated before we return. */
	xt = x_table_store(lw, rw, le, re, null_count, pi);

	/* The count we previously computed; its non-zero. */
	xt->set.count = hist_total(count);

#define NUM_PARSES 4
	// xt->set.cost_cutoff = hist_cost_cutoff(count, NUM_PARSES);
	// xt->set.cut_count = hist_cut_total(count, NUM_PARSES);

#define RECOUNT(X)  /* Make it disappear... */
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

		if (!islands_ok && (lw != -1)) return &xt->set;
		if (null_count == 0) return &xt->set;

		RECOUNT({xt->set.recount = 0;})

		w = lw + 1;
		for (dis = sent->word[w].d; dis != NULL; dis = dis->next)
		{
			if (dis->left == NULL)
			{
				pset = mk_parse_set(sent, mchxt, ctxt,
				                    dis, NULL, w, rw, dis->right, NULL,
				                    null_count-1, islands_ok, pi);
				if (pset == NULL) continue;
				dummy = dummy_set(lw, w, null_count-1, pi);
				record_choice(dummy, NULL, NULL,
				              pset,  NULL, NULL,
				              NULL, NULL, NULL, &xt->set);
				RECOUNT({xt->set.recount += pset->recount;})
			}
		}
		pset = mk_parse_set(sent, mchxt, ctxt,
		                    NULL, NULL, w, rw, NULL, NULL,
		                    null_count-1, islands_ok, pi);
		if (pset != NULL)
		{
			dummy = dummy_set(lw, w, null_count-1, pi);
			record_choice(dummy, NULL, NULL,
			              pset,  NULL, NULL,
			              NULL, NULL, NULL, &xt->set);
			RECOUNT({xt->set.recount += pset->recount;})
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
		size_t mlb, mle;
		mle = mlb = form_match_list(mchxt, w, le, lw, re, rw);
		// if (mlist) mlist = sort_matchlist(mlist);
		for (; get_match_list_element(mchxt, mle) != NULL; mle++)
		{
			unsigned int lnull_count, rnull_count;
			Disjunct *d = get_match_list_element(mchxt, mle);
			bool Lmatch = d->match_left;
			bool Rmatch = d->match_right;

			for (lnull_count = 0; lnull_count <= null_count; lnull_count++)
			{
				int i, j;
				Parse_set *ls[4], *rs[4];

				/* Here, lnull_count and rnull_count are the null_counts
				 * we're assigning to those parts respectively. */
				rnull_count = null_count - lnull_count;

				/* Now, we determine if (based on table only) we can see that
				   the current range is not parsable. */

				for (i=0; i<4; i++) { ls[i] = rs[i] = NULL; }
				if (Lmatch)
				{
					ls[0] = mk_parse_set(sent, mchxt, ctxt,
					             ld, d, lw, w, le->next, d->left->next,
					             lnull_count, islands_ok, pi);

					if (le->multi)
						ls[1] = mk_parse_set(sent, mchxt, ctxt,
						              ld, d, lw, w, le, d->left->next,
						              lnull_count, islands_ok, pi);

					if (d->left->multi)
						ls[2] = mk_parse_set(sent, mchxt, ctxt,
						              ld, d, lw, w, le->next, d->left,
						              lnull_count, islands_ok, pi);

					if (le->multi && d->left->multi)
						ls[3] = mk_parse_set(sent, mchxt, ctxt,
						              ld, d, lw, w, le, d->left,
						              lnull_count, islands_ok, pi);
				}

				if (Rmatch)
				{
					rs[0] = mk_parse_set(sent, mchxt, ctxt,
					                 d, rd, w, rw, d->right->next, re->next,
					                 rnull_count, islands_ok, pi);

					if (d->right->multi)
						rs[1] = mk_parse_set(sent, mchxt, ctxt,
					                 d, rd, w, rw, d->right, re->next,
						              rnull_count, islands_ok, pi);

					if (re->multi)
						rs[2] = mk_parse_set(sent, mchxt, ctxt,
						              d, rd, w, rw, d->right->next, re,
						              rnull_count, islands_ok, pi);

					if (d->right->multi && re->multi)
						rs[3] = mk_parse_set(sent, mchxt, ctxt,
						              d, rd, w, rw, d->right, re,
						              rnull_count, islands_ok, pi);
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
						              ld, d, rd, &xt->set);
						RECOUNT({xt->set.recount += ls[i]->recount * rs[j]->recount;})
					}
				}

				if (ls[0] != NULL || ls[1] != NULL || ls[2] != NULL || ls[3] != NULL)
				{
					/* Evaluate using the left match, but not the right */
					Parse_set* rset = mk_parse_set(sent, mchxt, ctxt,
					                        d, rd, w, rw, d->right, re,
					                        rnull_count, islands_ok, pi);
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
							              ld, d, rd, &xt->set);
							RECOUNT({xt->set.recount += ls[i]->recount * rset->recount;})
						}
					}
				}
				if ((le == NULL) && (rs[0] != NULL ||
				     rs[1] != NULL || rs[2] != NULL || rs[3] != NULL))
				{
					/* Evaluate using the right match, but not the left */
					Parse_set* lset = mk_parse_set(sent, mchxt, ctxt,
					                        ld, d, lw, w, le, d->left,
					                        lnull_count, islands_ok, pi);

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
							              ld, d, rd, &xt->set);
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

static bool set_overflowed(Parse_info pi)
{
	unsigned int i;

	assert(pi->x_table != NULL, "called set_overflowed with x_table==NULL");
	for (i=0; i<pi->x_table_size; i++)
	{
		X_table_connector *t;
		for (t = pi->x_table[i]; t != NULL; t = t->next)
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

bool build_parse_set(Sentence sent, fast_matcher_t *mchxt,
                    count_context_t *ctxt,
                    unsigned int null_count, Parse_Options opts)
{
	sent->parse_info->parse_set =
		mk_parse_set(sent, mchxt, ctxt,
		             NULL, NULL, -1, sent->length, NULL, NULL, null_count+1,
		             opts->islands_ok, sent->parse_info);


	return set_overflowed(sent->parse_info);
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

static void issue_link(Linkage lkg, Disjunct * ld, Disjunct * rd, Link * link)
{
	check_link_size(lkg);
	lkg->link_array[lkg->num_links] = *link;
	lkg->num_links++;

	lkg->chosen_disjuncts[link->lw] = ld;
	lkg->chosen_disjuncts[link->rw] = rd;
}

static void issue_links_for_choice(Linkage lkg, Parse_choice *pc)
{
	if (pc->link[0].lc != NULL) { /* there is a link to generate */
		issue_link(lkg, pc->ld, pc->md, &pc->link[0]);
	}
	if (pc->link[1].lc != NULL) {
		issue_link(lkg, pc->md, pc->rd, &pc->link[1]);
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

static void list_random_links(Linkage lkg, Parse_info pi, const Parse_set * set)
{
	Parse_choice *pc;
	int num_pc, new_index;

	if (set == NULL || set->first == NULL) return;
	num_pc = 0;
	for (pc = set->first; pc != NULL; pc = pc->next) {
		num_pc++;
	}

	new_index = rand_r(&pi->rand_state) % num_pc;

	num_pc = 0;
	for (pc = set->first; pc != NULL; pc = pc->next) {
		if (new_index == num_pc) break;
		num_pc++;
	}

	assert(pc != NULL, "Couldn't get a random parse choice");
	issue_links_for_choice(lkg, pc);
	list_random_links(lkg, pi, pc->set[0]);
	list_random_links(lkg, pi, pc->set[1]);
}

/**
 * Generate the list of all links of the index'th parsing of the
 * sentence.  For this to work, you must have already called parse, and
 * already built the whole_set.
 */
void extract_links(Linkage lkg, Parse_info pi)
{
	int index = lkg->lifo.index;
	if (index < 0)
	{
		bool repeatable = false;
		if (0 == pi->rand_state) repeatable = true;
		if (repeatable) pi->rand_state = index;
		list_random_links(lkg, pi, pi->parse_set);
		if (repeatable) pi->rand_state = 0;
	}
	else {
		list_links(lkg, pi->parse_set, index);
	}
}
