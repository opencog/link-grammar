/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2010 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "api.h"

/**
 * The first thing we do is we build a data structure to represent the
 * result of the entire parse search.  There will be a set of nodes
 * built for each call to the count() function that returned a non-zero
 * value, AND which is part of a valid linkage.  Each of these nodes
 * represents a valid continuation, and contains pointers to two other
 * sets (one for the left continuation and one for the right
 * continuation).
 */

static Parse_set * dummy_set(void)
{
	static Parse_set ds;
	ds.first = ds.current = NULL;
	ds.count = 1;
	return &ds;
}

/** Returns an empty set of parses */
static Parse_set * empty_set(void)
{
	Parse_set *s;
	s = (Parse_set *) xalloc(sizeof(Parse_set));
	s->first = s->current = NULL;
	s->count = 0;
	return s;
}

static void free_set(Parse_set *s)
{
	Parse_choice *p, *xp;
	if (s == NULL) return;
	for (p=s->first; p != NULL; p = xp) {
		xp = p->next;
		xfree((void *)p, sizeof(*p));
	}
	xfree((void *)s, sizeof(*s));
}

static Parse_choice * make_choice(Parse_set *lset, int llw, int lrw, Connector * llc, Connector * lrc,
						   Parse_set *rset, int rlw, int rrw, Connector * rlc, Connector * rrc,
						   Disjunct *ld, Disjunct *md, Disjunct *rd)
{
	Parse_choice *pc;
	pc = (Parse_choice *) xalloc(sizeof(*pc));
	pc->next = NULL;
	pc->set[0] = lset;
	pc->link[0].l = llw;
	pc->link[0].r = lrw;
	pc->link[0].lc = llc;
	pc->link[0].rc = lrc;
	pc->set[1] = rset;
	pc->link[1].l = rlw;
	pc->link[1].r = rrw;
	pc->link[1].lc = rlc;
	pc->link[1].rc = rrc;
	pc->ld = ld;
	pc->md = md;
	pc->rd = rd;
	return pc;
}

/**
 * Put this parse_choice into a given set.  The current pointer is always
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
		s->current->next = pc;
	}
	s->current = pc;
	pc->next = NULL;
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
	pi->parse_set = NULL;

	pi->chosen_disjuncts = (Disjunct **) xalloc(nwords * sizeof(Disjunct *));
	memset(pi->chosen_disjuncts, 0, nwords * sizeof(Disjunct *));

	pi->image_array = (Image_node **) xalloc(nwords * sizeof(Image_node *));
	memset(pi->image_array, 0, nwords * sizeof(Image_node *));

	pi->has_fat_down = (char *) xalloc(nwords * sizeof(Boolean));
	memset(pi->has_fat_down, 0, nwords * sizeof(Boolean));

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
	int i, len;
	X_table_connector *t, *x;

	len = pi->N_words;
	xfree(pi->chosen_disjuncts, len * sizeof(Disjunct *));
	xfree(pi->image_array, len * sizeof(Image_node*));
	xfree(pi->has_fat_down, len * sizeof(Boolean));

	for (i=0; i<pi->x_table_size; i++)
	{
		for(t = pi->x_table[i]; t!= NULL; t=x)
		{
			x = t->next;
			free_set(t->set);
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
static X_table_connector * x_table_pointer(int lw, int rw, Connector *le, Connector *re,
									int cost, Parse_info pi)
{
	X_table_connector *t;
	t = pi->x_table[pair_hash(pi->log2_x_table_size, lw, rw, le, re, cost)];
	for (; t != NULL; t = t->next) {
		if ((t->lw == lw) && (t->rw == rw) && (t->le == le) && (t->re == re) && (t->cost == cost))  return t;
	}
	return NULL;
}

#if DEAD_CODE
Parse_set * x_table_lookup(int lw, int rw, Connector *le, Connector *re,
						   int cost, Parse_info pi) {
	/* returns the count for this quintuple if there, -1 otherwise */
	X_table_connector *t = x_table_pointer(lw, rw, le, re, cost, pi);

	if (t == NULL) return -1; else return t->set;
}
#endif

/**
 * Stores the value in the x_table.  Assumes it's not already there.
 */
static X_table_connector * x_table_store(int lw, int rw, Connector *le, Connector *re,
								  int cost, Parse_set * set, Parse_info pi)
{
	X_table_connector *t, *n;
	int h;

	n = (X_table_connector *) xalloc(sizeof(X_table_connector));
	n->set = set;
	n->lw = lw; n->rw = rw; n->le = le; n->re = re; n->cost = cost;
	h = pair_hash(pi->log2_x_table_size, lw, rw, le, re, cost);
	t = pi->x_table[h];
	n->next = t;
	pi->x_table[h] = n;
	return n;
}

#ifdef UNUSED_FUNCTION
static void x_table_update(int lw, int rw, Connector *le, Connector *re,
					int cost, Parse_set * set, Parse_info pi) {
	/* Stores the value in the x_table.  Unlike x_table_store, it assumes it's already there */
	X_table_connector *t = x_table_pointer(lw, rw, le, re, cost, pi);

	assert(t != NULL, "This entry is supposed to be in the x_table.");
	t->set = set;
}
#endif


/**
 * returns NULL if there are no ways to parse, or returns a pointer
 * to a set structure representing all the ways to parse.
 *
 * This code is similar to code in count.c
 * (grep for end_word in these files).
 */
static Parse_set * parse_set(Sentence sent,
                 Disjunct *ld, Disjunct *rd, int lw, int rw,
					  Connector *le, Connector *re, int cost,
                 int islands_ok, Parse_info pi)
{
	Disjunct * d, * dis;
	int start_word, end_word, w;
	int lcost, rcost, Lmatch, Rmatch;
	int i, j;
	Parse_set *ls[4], *rs[4], *lset, *rset;
	Parse_choice * a_choice;

	Match_node * m, *m1;
	X_table_connector *xt;
	s64 count;

	assert(cost >= 0, "parse_set() called with cost < 0.");

	count = table_lookup(sent, lw, rw, le, re, cost);

	/*
	  assert(count >= 0, "parse_set() called on params that were not in the table.");
	  Actually, we can't assert this, because of the pseudocount technique that's
	  used in count().  It's not the case that every call to parse_set() has already
	  been put into the table.
	 */

	if ((count == 0) || (count == -1)) return NULL;

	xt = x_table_pointer(lw, rw, le, re, cost, pi);

	if (xt != NULL) return xt->set;  /* we've already computed it */

	/* Start it out with the empty set of options. */
	/* This entry must be updated before we return. */
	xt = x_table_store(lw, rw, le, re, cost, empty_set(), pi);

	xt->set->count = count;  /* the count we already computed */
	/* this count is non-zero */

	if (rw == 1 + lw) return xt->set;

	if ((le == NULL) && (re == NULL))
	{
		if (!islands_ok && (lw != -1)) return xt->set;

		if (cost == 0) return xt->set;

		w = lw + 1;
		for (dis = sent->word[w].d; dis != NULL; dis = dis->next)
		{
			if (dis->left == NULL)
			{
				rs[0] = parse_set(sent, dis, NULL, w, rw, dis->right,
				                  NULL, cost-1, islands_ok, pi);
				if (rs[0] == NULL) continue;
				a_choice = make_choice(dummy_set(), lw, w, NULL, NULL,
									   rs[0], w, rw, NULL, NULL,
									   NULL, NULL, NULL);
				put_choice_in_set(xt->set, a_choice);
			}
		}
		rs[0] = parse_set(sent, NULL, NULL, w, rw, NULL, NULL,
		                  cost-1, islands_ok, pi);
		if (rs[0] != NULL)
		{
			a_choice = make_choice(dummy_set(), lw, w, NULL, NULL,
								   rs[0], w, rw, NULL, NULL,
								   NULL, NULL, NULL);
			put_choice_in_set(xt->set, a_choice);
		}
		return xt->set;
	}

	if (le == NULL)
	{
		start_word = lw + 1;
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
		end_word = re->word + 1;
	}

	for (w = start_word; w < end_word; w++)
	{
		m1 = m = form_match_list(sent, w, le, lw, re, rw);
		for (; m!=NULL; m=m->next)
		{
			d = m->d;
			for (lcost = 0; lcost <= cost; lcost++)
			{
				rcost = cost-lcost;
				/* now lcost and rcost are the costs we're assigning to
				 * those parts respectively */

				/* Now, we determine if (based on table only) we can see that
				   the current range is not parsable. */

				Lmatch = (le != NULL) && (d->left != NULL) && do_match(sent, le, d->left, lw, w);
				Rmatch = (d->right != NULL) && (re != NULL) && do_match(sent, d->right, re, w, rw);
				for (i=0; i<4; i++) {ls[i] = rs[i] = NULL;}
				if (Lmatch)
				{
					ls[0] = parse_set(sent, ld, d, lw, w, le->next, d->left->next, lcost, islands_ok, pi);
					if (le->multi) ls[1] = parse_set(sent, ld, d, lw, w, le, d->left->next, lcost, islands_ok, pi);
					if (d->left->multi) ls[2] = parse_set(sent, ld, d, lw, w, le->next, d->left, lcost, islands_ok, pi);
					if (le->multi && d->left->multi) ls[3] = parse_set(sent, ld, d, lw, w, le, d->left, lcost, islands_ok, pi);
				}
				if (Rmatch)
				{
					rs[0] = parse_set(sent, d, rd, w, rw, d->right->next, re->next, rcost, islands_ok, pi);
					if (d->right->multi) rs[1] = parse_set(sent, d, rd, w,rw,d->right,re->next, rcost, islands_ok, pi);
					if (re->multi) rs[2] = parse_set(sent, d, rd, w, rw, d->right->next, re, rcost, islands_ok, pi);
					if (d->right->multi && re->multi) rs[3] = parse_set(sent, d, rd, w, rw, d->right, re, rcost, islands_ok, pi);
				}

				for (i=0; i<4; i++)
				{
					/* this ordering is probably not consistent with that
					 *  needed to use list_links */
					if (ls[i] == NULL) continue;
					for (j=0; j<4; j++)
					{
						if (rs[j] == NULL) continue;
						a_choice = make_choice(ls[i], lw, w, le, d->left,
											   rs[j], w, rw, d->right, re,
											   ld, d, rd);
						put_choice_in_set(xt->set, a_choice);
					}
				}

				if (ls[0] != NULL || ls[1] != NULL || ls[2] != NULL || ls[3] != NULL)
				{
					/* evaluate using the left match, but not the right */
					rset = parse_set(sent, d, rd, w, rw, d->right, re, rcost, islands_ok, pi);
					if (rset != NULL)
					{
						for (i=0; i<4; i++)
						{
							if (ls[i] == NULL) continue;
							/* this ordering is probably not consistent with
							 * that needed to use list_links */
							a_choice = make_choice(ls[i], lw, w, le, d->left,
							         rset, w, rw, NULL /* d->right */,
							         re,  /* the NULL indicates no link*/
							         ld, d, rd);
							put_choice_in_set(xt->set, a_choice);
						}
					}
				}
				if ((le == NULL) && (rs[0] != NULL ||
				     rs[1] != NULL || rs[2] != NULL || rs[3] != NULL))
				{
					/* evaluate using the right match, but not the left */
					lset = parse_set(sent, ld, d, lw, w, le, d->left, lcost, islands_ok, pi);

					if (lset != NULL)
					{
						for (i=0; i<4; i++)
						{
							if (rs[i] == NULL) continue;
							/* this ordering is probably not consistent with
							 * that needed to use list_links */
							a_choice = make_choice(lset, lw, w, NULL /* le */,
							          d->left,  /* NULL indicates no link */
							          rs[i], w, rw, d->right, re,
							          ld, d, rd);
							put_choice_in_set(xt->set, a_choice);
						}
					}
				}
			}
		}
		put_match_list(sent, m1);
	}
	xt->set->current = xt->set->first;
	return xt->set;
}

/**
 * return TRUE if and only if overflow in the number of parses occured.
 * Use a 64-bit int for counting.
 */
static int verify_set_node(Parse_set *set)
{
	Parse_choice *pc;
	s64 n;
	if (set == NULL || set->first == NULL) return FALSE;
	n = 0;
	for (pc = set->first; pc != NULL; pc = pc->next)
	{
		n  += pc->set[0]->count * pc->set[1]->count;
		if (PARSE_NUM_OVERFLOW < n) return TRUE;
	}
	return FALSE;
}

static int verify_set(Parse_info pi)
{
	int i;

	assert(pi->x_table != NULL, "called verify_set when x_table==NULL");
	for (i=0; i<pi->x_table_size; i++)
	{
		X_table_connector *t;
		for(t = pi->x_table[i]; t != NULL; t = t->next)
		{
			if (verify_set_node(t->set)) return TRUE;
		}
	}
	return FALSE;
}

/**
 * This is the top level call that computes the whole parse_set.  It
 * points whole_set at the result.  It creates the necessary hash
 * table (x_table) which will be freed at the same time the
 * whole_set is freed.
 *
 * It also assumes that count() has been run, and that hash table is
 * filled with the values thus computed.  Therefore this function
 * must be structured just like parse() (the main function for
 * count()).
 *
 * If the number of linkages gets huge, then the counts can overflow.
 * We check if this has happened when verifying the parse set.
 * This routine returns TRUE iff overflowed occurred.
 */

int build_parse_set(Sentence sent, int cost, Parse_Options opts)
{
	Parse_set * whole_set;

	whole_set =
		parse_set(sent, NULL, NULL, -1, sent->length, NULL, NULL, cost+1,
		          opts->islands_ok, sent->parse_info);

	if ((whole_set != NULL) && (whole_set->current != NULL)) {
		whole_set->current = whole_set->first;
	}

	sent->parse_info->parse_set = whole_set;

	return verify_set(sent->parse_info);
}

static void initialize_links(Parse_info pi)
{
	pi->N_links = 0;
	memset(pi->chosen_disjuncts, 0, pi->N_words * sizeof(Disjunct *));
}

static void issue_link(Parse_info pi, Disjunct * ld, Disjunct * rd, Link link)
{
	assert(pi->N_links <= MAX_LINKS-1, "Too many links");
	pi->link_array[pi->N_links] = link;
	pi->N_links++;

	pi->chosen_disjuncts[link.l] = ld;
	pi->chosen_disjuncts[link.r] = rd;
}

static void issue_links_for_choice(Parse_info pi, Parse_choice *pc)
{
	if (pc->link[0].lc != NULL) { /* there is a link to generate */
		issue_link(pi, pc->ld, pc->md, pc->link[0]);
	}
	if (pc->link[1].lc != NULL) {
		issue_link(pi, pc->md, pc->rd, pc->link[1]);
	}
}

#ifdef NOT_USED_ANYWHERE
static void build_current_linkage_recursive(Parse_info pi, Parse_set *set)
{
	if (set == NULL) return;
	if (set->current == NULL) return;

	issue_links_for_choice(pi, set->current);
	build_current_linkage_recursive(pi, set->current->set[0]);
	build_current_linkage_recursive(pi, set->current->set[1]);
}

/**
 * This function takes the "current" point in the given set and
 * generates the linkage that it represents.
 */
void build_current_linkage(Parse_info pi)
{
	initialize_links(pi);
	build_current_linkage_recursive(pi, pi->parse_set);
}

/**
 * Advance the "current" linkage to the next one
 * return 1 if there's a "carry" from this node,
 * which indicates that the scan of this node has
 * just been completed, and it's now back to it's
 * starting state.
 */
static int advance_linkage(Parse_info pi, Parse_set * set)
{
	if (set == NULL) return 1;  /* probably can't happen */
	if (set->first == NULL) return 1;  /* the empty set */
	if (advance_linkage(pi, set->current->set[0]) == 1) {
		if (advance_linkage(pi, set->current->set[1]) == 1) {
			if (set->current->next == NULL) {
				set->current = set->first;
				return 1;
			}
			set->current = set->current->next;
		}
	}
	return 0;
}

static void advance_parse_set(Parse_info pi)
{
	 advance_linkage(pi, pi->parse_set);
}
#endif

static void list_links(Parse_info pi, Parse_set * set, int index)
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
	 issue_links_for_choice(pi, pc);
	 list_links(pi, pc->set[0], index % pc->set[0]->count);
	 list_links(pi, pc->set[1], index / pc->set[0]->count);
}

static void list_random_links(Parse_info pi, Parse_set * set)
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
	 issue_links_for_choice(pi, pc);
	 list_random_links(pi, pc->set[0]);
	 list_random_links(pi, pc->set[1]);
}

/**
 * Generate the list of all links of the index'th parsing of the
 * sentence.  For this to work, you must have already called parse, and
 * already built the whole_set.
 */
void extract_links(int index, int cost, Parse_info pi)
{
	initialize_links(pi);
	pi->rand_state = index;
	if (index < 0) {
		list_random_links(pi, pi->parse_set);
	}
	else {
		list_links(pi, pi->parse_set, index);
	}
}
