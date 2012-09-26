/**************************************************************************/
/* Copyright (c) 2004                                                     */
/* Daniel Sleator, David Temperley, and John Lafferty                     */
/* All rights reserved                                                    */
/*                                                                        */
/* Use of the link grammar parsing system is subject to the terms of the  */
/* license set forth in the LICENSE file included with this software,     */
/* and also available at http://www.link.cs.cmu.edu/link/license.html     */
/* This license allows free redistribution and use in source and binary   */
/* forms, with or without modification, subject to certain conditions.    */
/*                                                                        */
/**************************************************************************/

#include "api.h"
#include "fast-match.h"

/** 
 * returns the number of disjuncts in the list that have non-null
 * left connector lists.
 */
static int left_disjunct_list_length(Disjunct * d)
{
	int i;
	for (i=0; d!=NULL; d=d->next) {
		if (d->left != NULL) i++;
	}
	return i;
}

static int right_disjunct_list_length(Disjunct * d)
{
	int i;
	for (i=0; d!=NULL; d=d->next) {
		if (d->right != NULL) i++;
	}
	return i;
}

struct match_context_s
{
	int match_cost;
	int l_table_size[MAX_SENTENCE];  /* the sizes of the hash tables */
	int r_table_size[MAX_SENTENCE];

	/* the beginnings of the hash tables */
	Match_node ** l_table[MAX_SENTENCE];
	Match_node ** r_table[MAX_SENTENCE];

	/* I'll pedantically maintain my own list of these cells */
	Match_node * mn_free_list;
};


/**
 * Return a match node to be used by the caller
 */
static Match_node * get_match_node(match_context_t *ctxt)
{
	Match_node * m;
	if (ctxt->mn_free_list != NULL)
	{
		m = ctxt->mn_free_list;
		ctxt->mn_free_list = m->next;
	}
	else
	{
		m = (Match_node *) xalloc(sizeof(Match_node));
	}
	return m;
}

/**
 * Put these nodes back onto my free list
 */
void put_match_list(Sentence sent, Match_node *m)
{
	Match_node * xm;
	match_context_t *ctxt = sent->match_ctxt;

	for (; m != NULL; m = xm)
	{
		xm = m->next;
		m->next = ctxt->mn_free_list;
		ctxt->mn_free_list = m;
	}
}

static void free_match_list(Match_node * t)
{
	Match_node *xt;
	for (; t!=NULL; t=xt) {
		xt = t->next;
		xfree((char *)t, sizeof(Match_node));
	}
}

/**
 * Free all of the hash tables and Match_nodes 
 */
void free_fast_matcher(Sentence sent)
{
	int w;
	int i;
	match_context_t *ctxt = sent->match_ctxt;

	if (verbosity > 1) printf("%d Match cost\n", ctxt->match_cost);
	for (w = 0; w < sent->length; w++)
	{
		for (i = 0; i < ctxt->l_table_size[w]; i++)
		{
			free_match_list(ctxt->l_table[w][i]);
		}
		xfree((char *)ctxt->l_table[w], ctxt->l_table_size[w] * sizeof (Match_node *));
		for (i = 0; i < ctxt->r_table_size[w]; i++)
		{
			free_match_list(ctxt->r_table[w][i]);
		}
		xfree((char *)ctxt->r_table[w], ctxt->r_table_size[w] * sizeof (Match_node *));
	}
	free_match_list(ctxt->mn_free_list);
	ctxt->mn_free_list = NULL;

	free(ctxt);
	sent->match_ctxt = NULL;
}

/**
 * Adds the match node m to the sorted list of match nodes l.
 * The parameter dir determines the order of the sorting to be used.
 * Makes the list sorted from smallest to largest.
 */
static Match_node * add_to_right_table_list(Match_node * m, Match_node * l)
{
	Match_node *p, *prev;

	if (l == NULL) return m;

	/* Insert m at head of list */
	if ((m->d->right->word) <= (l->d->right->word)) {
		m->next = l;
		return m;
	}

	/* Walk list to insertion point */
	prev = l;
	p = prev->next;
	while (p != NULL && ((m->d->right->word) > (p->d->right->word))) {
		prev = p;
		p = p->next;
	}

	m->next = p;
	prev->next = m;

	return l;  /* return pointer to original head */
}

/**
 * Adds the match node m to the sorted list of match nodes l.
 * The parameter dir determines the order of the sorting to be used.
 * Makes the list sorted from largest to smallest
 */
static Match_node * add_to_left_table_list(Match_node * m, Match_node * l)
{
	Match_node *p, *prev;

	if (l == NULL) return m;

	/* Insert m at head of list */
	if ((m->d->left->word) >= (l->d->left->word)) {
		m->next = l;
		return m;
	}

	/* Walk list to insertion point */
	prev = l;
	p = prev->next;
	while (p != NULL && ((m->d->left->word) < (p->d->left->word))) {
		prev = p;
		p = p->next;
	}

	m->next = p;
	prev->next = m;

	return l;  /* return pointer to original head */
}

/**
 * The disjunct d (whose left or right pointer points to c) is put
 * into the appropriate hash table
 * dir =  1, we're putting this into a right table.
 * dir = -1, we're putting this into a left table.
 */
static void put_into_match_table(int size, Match_node ** t,
								 Disjunct * d, Connector * c, int dir )
{
	int h;
	Match_node * m;
	h = connector_hash(c) & (size-1);
	m = (Match_node *) xalloc (sizeof(Match_node));
	m->next = NULL;
	m->d = d;
	if (dir == 1) {
		t[h] = add_to_right_table_list(m, t[h]);
	} else {
		t[h] = add_to_left_table_list(m, t[h]);
	}
}

void init_fast_matcher(Sentence sent)
{
	int w, len, size, i;
	Match_node ** t;
	Disjunct * d;
	match_context_t *ctxt;

	ctxt = (match_context_t *) malloc(sizeof(match_context_t));
	sent->match_ctxt = ctxt;

	ctxt->match_cost = 0;
	ctxt->mn_free_list = NULL;

	for (w=0; w<sent->length; w++)
	{
		len = left_disjunct_list_length(sent->word[w].d);
		size = next_power_of_two_up(len);
		ctxt->l_table_size[w] = size;
		t = ctxt->l_table[w] = (Match_node **) xalloc(size * sizeof(Match_node *));
		for (i = 0; i < size; i++) t[i] = NULL;

		for (d = sent->word[w].d; d != NULL; d = d->next)
		{
			if (d->left != NULL)
			{
				put_into_match_table(size, t, d, d->left, -1);
			}
		}

		len = right_disjunct_list_length(sent->word[w].d);
		size = next_power_of_two_up(len);
		ctxt->r_table_size[w] = size;
		t = ctxt->r_table[w] = (Match_node **) xalloc(size * sizeof(Match_node *));
		for (i = 0; i < size; i++) t[i] = NULL;

		for (d = sent->word[w].d; d != NULL; d = d->next)
		{
			if (d->right != NULL)
			{
				put_into_match_table(size, t, d, d->right, 1);
			}
		}
	}
}

/**
 * Forms and returns a list of disjuncts that might match lc or rc or both.
 * lw and rw are the words from which lc and rc came respectively.
 * The list is formed by the link pointers of Match_nodes.
 * The list contains no duplicates.  A quadratic algorithm is used to
 * eliminate duplicates.  In practice the match_cost is less than the
 * parse_cost (and the loop is tiny), so there's no reason to bother
 * to fix this.
 */
Match_node * 
form_match_list(Sentence sent, int w, 
                Connector *lc, int lw, Connector *rc, int rw)
{
	Match_node *ml, *mr, *mx, *my, * mz, *front, *free_later;

	match_context_t *ctxt = sent->match_ctxt;

	if (lc != NULL) {
		ml = ctxt->l_table[w][connector_hash(lc) & (ctxt->l_table_size[w]-1)];
	} else {
		ml = NULL;
	}
	if (rc != NULL) {
		mr = ctxt->r_table[w][connector_hash(rc) & (ctxt->r_table_size[w]-1)];
	} else {
		mr = NULL;
	}

	front = NULL;
	for (mx = ml; mx != NULL; mx = mx->next)
	{
		if (mx->d->left->word < lw) break;
		my = get_match_node(ctxt);
		my->d = mx->d;
		my->next = front;
		front = my;
	}
	ml = front;   /* ml is now the list of things that could match the left */

	front = NULL;
	for (mx = mr; mx != NULL; mx = mx->next)
	{
		if (mx->d->right->word > rw) break;
		my = get_match_node(ctxt);
		my->d = mx->d;
		my->next = front;
		front = my;
	}
	mr = front;   /* mr is now the list of things that could match the right */

	/* now we want to eliminate duplicates from the lists */

	free_later = NULL;
	front = NULL;
	for (mx = mr; mx != NULL; mx = mz)
	{
		/* see if mx in first list, put it in if its not */
		mz = mx->next;
		ctxt->match_cost++;
		for (my=ml; my!=NULL; my=my->next) {
			ctxt->match_cost++;
			if (mx->d == my->d) break;
		}
		if (my != NULL) { /* mx was in the l list */
			mx->next = free_later;
			free_later = mx;
		}
		if (my==NULL) {  /* it was not there */
			mx->next = front;
			front = mx;
		}
	}
	mr = front;  /* mr is now the abbreviated right list */
	put_match_list(sent, free_later);

	/* now catenate the two lists */
	if (mr == NULL) return ml;
	for (mx = mr; mx->next != NULL; mx = mx->next)
	  ;
	mx->next = ml;
	return mr;
}
