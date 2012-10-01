/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
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
#include "disjunct-utils.h"

/* This file contains the exhaustive search algorithm. */

typedef struct Table_connector_s Table_connector;
struct Table_connector_s
{
	short            lw, rw;
	Connector        *le, *re;
	short            cost;
	s64              count;
	Table_connector  *next;
};

struct count_context_s
{
#ifdef USE_FAT_LINKAGES
	char ** deletable;
	char ** effective_dist; 
#endif /* USE_FAT_LINKAGES */
	Word *  local_sent;
	int     null_block;
	int     islands_ok;
	int     null_links;
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

static void init_table(Sentence sent)
{
	int shift;
	/* A piecewise exponential function determines the size of the
	 * hash table. Probably should make use of the actual number of
	 * disjuncts, rather than just the number of words.
	 */
	count_context_t *ctxt = sent->count_ctxt;

	if (ctxt->table) free_table(ctxt);

	if (sent->length >= 10)
	{
		shift = 12 + (sent->length) / 6 ; 
	} 
	else
	{
		shift = 12;
	}

	/* Clamp at max 4*(1<<24) == 64 MBytes */
	if (24 < shift) shift = 24;
	ctxt->table_size = (1 << shift);
	ctxt->log2_table_size = shift;
	ctxt->table = (Table_connector**) 
		xalloc(ctxt->table_size * sizeof(Table_connector*));
	memset(ctxt->table, 0, ctxt->table_size*sizeof(Table_connector*));
}

int x_match(Sentence sent, Connector *a, Connector *b)
{
	return do_match(sent, a, b, 0, 0);
}

#ifdef USE_FAT_LINKAGES
void count_set_effective_distance(Sentence sent)
{
	sent->count_ctxt->effective_dist = sent->effective_dist;
}

void count_unset_effective_distance(Sentence sent)
{
	sent->count_ctxt->effective_dist = NULL;
}
#endif /* USE_FAT_LINKAGES */

/*
 * Returns TRUE if s and t match according to the connector matching
 * rules.  The connector strings must be properly formed, starting with
 * zero or more upper case letters, followed by some other letters, and
 * The algorithm is symmetric with respect to a and b.
 *
 * It works as follows:  The labels must match.  The priorities must be
 * compatible (both THIN_priority, or one UP_priority and one DOWN_priority).
 * The sequence of upper case letters must match exactly.  After these comes
 * a sequence of lower case letters or "*"s.  The matching algorithm
 * is different depending on which of the two priority cases is being
 * considered.  See the comments below. 
 */
int do_match(Sentence sent, Connector *a, Connector *b, int aw, int bw)
{
	const char *s, *t;
	int x, y;
#ifdef USE_FAT_LINKAGES
	int dist;
#endif /* USE_FAT_LINKAGES */

	if (a->label != b->label) return FALSE;

	s = a->string;
	t = b->string;

	while(isupper((int)*s) || isupper((int)*t))
	{
		if (*s != *t) return FALSE;
		s++;
		t++;
	}

	x = a->priority;
	y = b->priority;

#ifdef USE_FAT_LINKAGES

	/* Probably not necessary, as long as 
	 * effective_dist[0][0]=0 and is defined */
	if (aw == 0 && bw == 0) {
		dist = 0;
	} else {
		count_context_t *ctxt;
		ctxt = sent->count_ctxt;
		assert(aw < bw, "match() did not receive params in the natural order.");
		dist = ctxt->effective_dist[aw][bw];
	}
	/*	printf("M: a=%4s b=%4s  ap=%d bp=%d  aw=%d  bw=%d  a->ll=%d b->ll=%d  dist=%d\n",
		   s, t, x, y, aw, bw, a->length_limit, b->length_limit, dist); */
	if (dist > a->length_limit || dist > b->length_limit) return FALSE;
#endif /* USE_FAT_LINKAGES */

	if ((x == THIN_priority) && (y == THIN_priority))
	{
		/*
		   Remember that "*" matches anything, and "^" matches nothing
		   (except "*").  Otherwise two characters match if and only if
		   they're equal.  ("^" can be used in the dictionary just like
		   any other connector.)
		   */
		while ((*s != '\0') && (*t != '\0'))
		{
#ifdef USE_FAT_LINKAGES
			if ((*s == '*') || (*t == '*') ||
				((*s == *t) && (*s != '^')))
#else
			if ((*s == '*') || (*t == '*') || (*s == *t))
#endif /* USE_FAT_LINKAGES */
			{
				s++;
				t++;
			}
			else
				return FALSE;
		}
		return TRUE;
	}
	else if ((x == UP_priority) && (y == DOWN_priority))
	{
		/*
		   As you go up (namely from x to y) the set of strings that
		   match (in the normal THIN sense above) should get no larger.
		   Read the comment in and.c to understand this.
		   In other words, the y string (t) must be weaker (or at least
		   no stronger) that the x string (s).
		
		   This code is only correct if the strings are the same
		   length.  This is currently true, but perhaps for safty
		   this assumption should be removed.
		   */
		while ((*s != '\0') && (*t != '\0'))
		{
#ifdef USE_FAT_LINKAGES
			if ((*s == *t) || (*s == '*') || (*t == '^'))
#else
			if ((*s == *t) || (*s == '*'))
#endif /* USE_FAT_LINKAGES */
			{
				s++;
				t++;
			} else return FALSE;
		}
		return TRUE;
	}
	else if ((y == UP_priority) && (x == DOWN_priority))
	{
		while ((*s != '\0') && (*t != '\0'))
		{
#ifdef USE_FAT_LINKAGES
			if ((*s == *t) || (*t == '*') || (*s == '^'))
#else
			if ((*s == *t) || (*t == '*'))
#endif /* USE_FAT_LINKAGES */
			{
				s++;
				t++;
			}
			else
				return FALSE;
		}
		return TRUE;
	}
	else
		return FALSE;
}

/** 
 * Stores the value in the table.  Assumes it's not already there.
 */
static Table_connector * table_store(count_context_t *ctxt,
                                     int lw, int rw,
                                     Connector *le, Connector *re,
                                     int cost, s64 count)
{
	Table_connector *t, *n;
	int h;

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
                   int cost)
{
	Table_connector *t;
	int h = pair_hash(ctxt->log2_table_size,lw, rw, le, re, cost);
	t = ctxt->table[h];
	for (; t != NULL; t = t->next) {
		if ((t->lw == lw) && (t->rw == rw) && (t->le == le) && (t->re == re)
			&& (t->cost == cost))  return t;
	}

	/* Create a new connector only if resources are exhausted.
	 * (???) Huh? I guess we're in panic parse mode in that case.
	 */
	if ((ctxt->current_resources != NULL) && 
	     resources_exhausted(ctxt->current_resources))
	{
		return table_store(ctxt, lw, rw, le, re, cost, 0);
	}
	else return NULL;
}

/** returns the count for this quintuple if there, -1 otherwise */
s64 table_lookup(Sentence sent, 
                 int lw, int rw, Connector *le, Connector *re, int cost)
{
	Table_connector *t = find_table_pointer(sent->count_ctxt, lw, rw, le, re, cost);

	if (t == NULL) return -1; else return t->count;
}

#ifdef USE_FAT_LINKAGES
/**
 * Stores the value in the table.  Unlike table_store, it assumes 
 * it's already there
 */
static void table_update(count_context_t *ctxt, int lw, int rw, 
                         Connector *le, Connector *re,
                         int cost, s64 count)
{
	Table_connector *t = find_table_pointer(ctxt, lw, rw, le, re, cost);

	assert(t != NULL, "This entry is supposed to be in the table.");
	t->count = count;
}
#endif /* USE_FAT_LINKAGES */

/**
 * Returns 0 if and only if this entry is in the hash table 
 * with a count value of 0.
 */
static s64 pseudocount(Sentence sent,
                       int lw, int rw, Connector *le, Connector *re, int cost)
{
	s64 count;
	count = table_lookup(sent, lw, rw, le, re, cost);
	if (count == 0) return 0; else return 1;
}

static s64 do_count(Sentence sent, int lw, int rw,
                    Connector *le, Connector *re, int null_count)
{
	s64 total, pseudototal;
	int start_word, end_word, w;
	s64 leftcount, rightcount;
	int lcost, rcost, Lmatch, Rmatch;

	Match_node * m, *m1;
	Table_connector *t;

	count_context_t *ctxt = sent->count_ctxt;

	if (null_count < 0) return 0;  /* can this ever happen?? */

	t = find_table_pointer(ctxt, lw, rw, le, re, null_count);

	if (t == NULL) {
		/* Create the table entry with a tentative null count of 0. 
	    * This count must be updated before we return. */
		t = table_store(ctxt, lw, rw, le, re, null_count, 0);
	} else {
		return t->count;
	}

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
			 * null_count of skipping n words is just n */
			if (null_count == ((rw-lw-1) + ctxt->null_block-1)/ctxt->null_block)
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
			total = 0;
			w = lw+1;
			for (d = ctxt->local_sent[w].d; d != NULL; d = d->next)
			{
				if (d->left == NULL)
				{
					total += do_count(sent, w, rw, d->right, NULL, null_count-1);
				}
			}
			total += do_count(sent, w, rw, NULL, NULL, null_count-1);
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
		m1 = m = form_match_list(sent, w, le, lw, re, rw);
		for (; m != NULL; m = m->next)
		{
			Disjunct * d;
			d = m->d;
			for (lcost = 0; lcost <= null_count; lcost++)
			{
				rcost = null_count - lcost;
				/* Now lcost and rcost are the costs we're assigning
				 * to those parts respectively */

				/* Now, we determine if (based on table only) we can see that
				   the current range is not parsable. */
				Lmatch = (le != NULL) && (d->left != NULL) && 
				         do_match(sent, le, d->left, lw, w);
				Rmatch = (d->right != NULL) && (re != NULL) && 
				         do_match(sent, d->right, re, w, rw);

				rightcount = leftcount = 0;
				if (Lmatch)
				{
					leftcount = pseudocount(sent, lw, w, le->next, d->left->next, lcost);
					if (le->multi) leftcount += pseudocount(sent, lw, w, le, d->left->next, lcost);
					if (d->left->multi) leftcount += pseudocount(sent, lw, w, le->next, d->left, lcost);
					if (le->multi && d->left->multi) leftcount += pseudocount(sent, lw, w, le, d->left, lcost);
				}

				if (Rmatch)
				{
					rightcount = pseudocount(sent, w, rw, d->right->next, re->next, rcost);
					if (d->right->multi) rightcount += pseudocount(sent, w,rw,d->right,re->next, rcost);
					if (re->multi) rightcount += pseudocount(sent, w, rw, d->right->next, re, rcost);
					if (d->right->multi && re->multi) rightcount += pseudocount(sent, w, rw, d->right, re, rcost);
				}

				/* total number where links are used on both sides */
				pseudototal = leftcount*rightcount;

				if (leftcount > 0) {
					/* evaluate using the left match, but not the right */
					pseudototal += leftcount * pseudocount(sent, w, rw, d->right, re, rcost);
				}
				if ((le == NULL) && (rightcount > 0)) {
					/* evaluate using the right match, but not the left */
					pseudototal += rightcount * pseudocount(sent, lw, w, le, d->left, lcost);
				}

				/* now pseudototal is 0 implies that we know that the true total is 0 */
				if (pseudototal != 0) {
					rightcount = leftcount = 0;
					if (Lmatch) {
						leftcount = do_count(sent, lw, w, le->next, d->left->next, lcost);
						if (le->multi) leftcount += do_count(sent, lw, w, le, d->left->next, lcost);
						if (d->left->multi) leftcount += do_count(sent, lw, w, le->next, d->left, lcost);
						if (le->multi && d->left->multi) leftcount += do_count(sent, lw, w, le, d->left, lcost);
					}

					if (Rmatch) {
						rightcount = do_count(sent, w, rw, d->right->next, re->next, rcost);
						if (d->right->multi) rightcount += do_count(sent, w,rw,d->right,re->next, rcost);
						if (re->multi) rightcount += do_count(sent, w, rw, d->right->next, re, rcost);
						if (d->right->multi && re->multi) rightcount += do_count(sent, w, rw, d->right, re, rcost);
					}

					total += leftcount*rightcount;  /* total number where links are used on both sides */

					if (leftcount > 0) {
						/* evaluate using the left match, but not the right */
						total += leftcount * do_count(sent, w, rw, d->right, re, rcost);
					}
					if ((le == NULL) && (rightcount > 0)) {
						/* evaluate using the right match, but not the left */
						total += rightcount * do_count(sent, lw, w, le, d->left, lcost);
					}
				}
			}
		}
		put_match_list(sent, m1);
	}
	t->count = total;
	return total;
}


/** 
 * Returns the number of ways the sentence can be parsed with the
 * specified null count. Assumes that the hash table has already been
 * initialized, and is freed later. The "null_count" here is the
 * number of words that are allowed to have no links to them.
 */
s64 do_parse(Sentence sent, int null_count, Parse_Options opts)
{
	s64 total;
	count_context_t *ctxt = sent->count_ctxt;

	ctxt->current_resources = opts->resources;
	ctxt->local_sent = sent->word;
#ifdef USE_FAT_LINKAGES
	count_set_effective_distance(sent);
	ctxt->deletable = sent->deletable;
#endif /* USE_FAT_LINKAGES */
	ctxt->null_block = opts->null_block;
	ctxt->islands_ok = opts->islands_ok;

	total = do_count(sent, -1, sent->length, NULL, NULL, null_count+1);

	ctxt->local_sent = NULL;
	ctxt->current_resources = NULL;
	return total;
}

#ifdef USE_FAT_LINKAGES
/**
   CONJUNCTION PRUNING.

   The basic idea is this.  Before creating the fat disjuncts,
   we run a modified version of the exhaustive search procedure.
   Its purpose is to mark the disjuncts that can be used in any
   linkage.  It's just like the normal exhaustive search, except that
   if a subrange of words are deletable, then we treat them as though
   they were not even there.  So, if we call the function in the
   situation where the set of words between the left and right one
   are deletable, and the left and right connector pointers
   are NULL, then that range is considered to have a solution.

   There are actually two procedures to implement this.  One is
   mark_region() and the other is region_valid().  The latter just
   checks to see if the given region can be completed (within it).
   The former actually marks those disjuncts that can be used in
   any valid linkage of the given region.

   As in the standard search procedure, we make use of the fast-match
   data structure (which requires power pruning to have been done), and
   we also use a hash table.  The table is used differently in this case.
   The meaning of values stored in the table are as follows:

   -1  Nothing known (Actually, this is not stored.  It's returned
   by table_lookup when nothing is known.)
   0  This region can't be completed (marking is therefore irrelevant)
   1  This region can be completed, but it's not yet marked
   2  This region can be completed, and it's been marked.
   */

static int x_prune_match(count_context_t *ctxt,
                         Connector *le, Connector *re, int lw, int rw)
{
	int dist;

	assert(lw < rw, "prune_match() did not receive params in the natural order.");
	dist = ctxt->effective_dist[lw][rw];
	return prune_match(dist, le, re);
}

/**
 * Returns 0 if this range cannot be successfully filled in with
 * links.  Returns 1 if it can, and it's not been marked, and returns
 * 2 if it can and it has been marked.
 */
static int region_valid(Sentence sent, int lw, int rw, Connector *le, Connector *re)
{
	Disjunct * d;
	int left_valid, right_valid, found;
	int i, start_word, end_word;
	int w;
	Match_node * m, *m1;

	count_context_t *ctxt = sent->count_ctxt;

	i = table_lookup(sent, lw, rw, le, re, 0);
	if (i >= 0) return i;

	if ((le == NULL) && (re == NULL) && ctxt->deletable[lw][rw]) {
		table_store(ctxt, lw, rw, le, re, 0, 1);
		return 1;
	}

	if (le == NULL) {
		start_word = lw+1;
	} else {
		start_word = le->word;
	}
	if (re == NULL) {
		end_word = rw;
	} else {
		end_word = re->word + 1;
	}

	found = 0;

	for (w=start_word; w < end_word; w++)
	{
		m1 = m = form_match_list(sent, w, le, lw, re, rw);
		for (; m!=NULL; m=m->next)
		{
			d = m->d;
			/* mark_cost++;*/
			/* in the following expressions we use the fact that 0=FALSE. Could eliminate
			   by always saying "region_valid(...) != 0"  */
			left_valid = (((le != NULL) && (d->left != NULL) && x_prune_match(ctxt, le, d->left, lw, w)) &&
						  ((region_valid(sent, lw, w, le->next, d->left->next)) ||
						   ((le->multi) && region_valid(sent, lw, w, le, d->left->next)) ||
						   ((d->left->multi) && region_valid(sent, lw, w, le->next, d->left)) ||
						   ((le->multi && d->left->multi) && region_valid(sent, lw, w, le, d->left))));
			if (left_valid && region_valid(sent, w, rw, d->right, re)) {
				found = 1;
				break;
			}
			right_valid = (((d->right != NULL) && (re != NULL) && x_prune_match(ctxt, d->right, re, w, rw)) &&
						   ((region_valid(sent, w, rw, d->right->next,re->next))	||
							((d->right->multi) && region_valid(sent, w,rw,d->right,re->next))  ||
							((re->multi) && region_valid(sent, w, rw, d->right->next, re))  ||
							((d->right->multi && re->multi) && region_valid(sent, w, rw, d->right, re))));
			if ((left_valid && right_valid) || (right_valid && region_valid(sent, lw, w, le, d->left))) {
				found = 1;
				break;
			}
		}
		put_match_list(sent, m1);
		if (found != 0) break;
	}
	table_store(ctxt, lw, rw, le, re, 0, found);
	return found;
}

/**
 * Mark as useful all disjuncts involved in some way to complete the
 * structure within the current region.  Note that only disjuncts
 * strictly between lw and rw will be marked.  If it so happens that
 * this region itself is not valid, then this fact will be recorded
 * in the table, and nothing else happens.
 */
static void mark_region(Sentence sent,
                        int lw, int rw, Connector *le, Connector *re)
{

	Disjunct * d;
	int left_valid, right_valid, i;
	int start_word, end_word;
	int w;
	Match_node * m, *m1;
	count_context_t *ctxt = sent->count_ctxt;

	i = region_valid(sent, lw, rw, le, re);
	if ((i==0) || (i==2)) return;
	/* we only reach this point if it's a valid unmarked region, i=1 */
	table_update(ctxt, lw, rw, le, re, 0, 2);

	if ((le == NULL) && (re == NULL) && (ctxt->null_links) && (rw != 1+lw)) {
		w = lw+1;
		for (d = ctxt->local_sent[w].d; d != NULL; d = d->next) {
			if ((d->left == NULL) && region_valid(sent, w, rw, d->right, NULL)) {
				d->marked = TRUE;
				mark_region(sent, w, rw, d->right, NULL);
			}
		}
		mark_region(sent, w, rw, NULL, NULL);
		return;
	}

	if (le == NULL) {
		start_word = lw+1;
	} else {
		start_word = le->word;
	}
	if (re == NULL) {
		end_word = rw;
	} else {
		end_word = re->word + 1;
	}

	for (w=start_word; w < end_word; w++)
	{
		m1 = m = form_match_list(sent, w, le, lw, re, rw);
		for (; m!=NULL; m=m->next)
		{
			d = m->d;
			/* mark_cost++;*/
			left_valid = (((le != NULL) && (d->left != NULL) && x_prune_match(ctxt, le, d->left, lw, w)) &&
						  ((region_valid(sent, lw, w, le->next, d->left->next)) ||
						   ((le->multi) && region_valid(sent, lw, w, le, d->left->next)) ||
						   ((d->left->multi) && region_valid(sent, lw, w, le->next, d->left)) ||
						   ((le->multi && d->left->multi) && region_valid(sent, lw, w, le, d->left))));
			right_valid = (((d->right != NULL) && (re != NULL) && x_prune_match(ctxt, d->right, re, w, rw)) &&
						   ((region_valid(sent, w, rw, d->right->next,re->next)) ||
							((d->right->multi) && region_valid(sent, w,rw,d->right,re->next))  ||
							((re->multi) && region_valid(sent, w, rw, d->right->next, re)) ||
							((d->right->multi && re->multi) && region_valid(sent, w, rw, d->right, re))));

			/* The following if statements could be restructured to avoid superfluous calls
			   to mark_region.  It didn't seem a high priority, so I didn't optimize this.
			   */

			if (left_valid && region_valid(sent, w, rw, d->right, re))
			{
				d->marked = TRUE;
				mark_region(sent, w, rw, d->right, re);
				mark_region(sent, lw, w, le->next, d->left->next);
				if (le->multi) mark_region(sent, lw, w, le, d->left->next);
				if (d->left->multi) mark_region(sent, lw, w, le->next, d->left);
				if (le->multi && d->left->multi) mark_region(sent, lw, w, le, d->left);
			}

			if (right_valid && region_valid(sent, lw, w, le, d->left))
			{
				d->marked = TRUE;
				mark_region(sent, lw, w, le, d->left);
				mark_region(sent, w, rw, d->right->next,re->next);
				if (d->right->multi) mark_region(sent, w,rw,d->right,re->next);
				if (re->multi) mark_region(sent, w, rw, d->right->next, re);
				if (d->right->multi && re->multi) mark_region(sent, w, rw, d->right, re);
			}

			if (left_valid && right_valid)
			{
				d->marked = TRUE;
				mark_region(sent, lw, w, le->next, d->left->next);
				if (le->multi) mark_region(sent, lw, w, le, d->left->next);
				if (d->left->multi) mark_region(sent, lw, w, le->next, d->left);
				if (le->multi && d->left->multi) mark_region(sent, lw, w, le, d->left);
				mark_region(sent, w, rw, d->right->next,re->next);
				if (d->right->multi) mark_region(sent, w,rw,d->right,re->next);
				if (re->multi) mark_region(sent, w, rw, d->right->next, re);
				if (d->right->multi && re->multi) mark_region(sent, w, rw, d->right, re);
			}
		}
		put_match_list(sent, m1);
	}
}
#endif /* USE_FAT_LINKAGES */

void delete_unmarked_disjuncts(Sentence sent)
{
	int w;
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

#ifdef USE_FAT_LINKAGES
/**
 * We've already built the sentence disjuncts, and we've pruned them
 * and power_pruned(GENTLE) them also.  The sentence contains a
 * conjunction.  deletable[][] has been initialized to indicate the
 * ranges which may be deleted in the final linkage.
 *
 * This routine deletes irrelevant disjuncts.  It finds them by first
 * marking them all as irrelevant, and then marking the ones that
 * might be useable.  Finally, the unmarked ones are removed.
 */
void conjunction_prune(Sentence sent, Parse_Options opts)
{
	Disjunct * d;
	int w;
	count_context_t *ctxt = sent->count_ctxt;

	ctxt->current_resources = opts->resources;
	ctxt->deletable = sent->deletable;
	count_set_effective_distance(sent);

	/* We begin by unmarking all disjuncts.  This would not be necessary if
	   whenever we created a disjunct we cleared its marked field.
	   I didn't want to search the program for all such places, so
	   I did this way. XXX FIXME, someday ... 
	   */
	for (w=0; w<sent->length; w++) {
		for (d=sent->word[w].d; d != NULL; d=d->next) {
			d->marked = FALSE;
		}
	}

	init_fast_matcher(sent);
	ctxt->local_sent = sent->word;
	ctxt->null_links = (opts->min_null_count > 0);
	/*
	for (d = sent->word[0].d; d != NULL; d = d->next) {
		if ((d->left == NULL) && region_valid(sent, 0, sent->length, d->right, NULL)) {
			mark_region(sent, 0, sent->length, d->right, NULL);
			d->marked = TRUE;
		}
	}
	mark_region(sent, 0, sent->length, NULL, NULL);
	*/

	if (ctxt->null_links) {
		mark_region(sent, -1, sent->length, NULL, NULL);
	} else {
		for (w=0; w<sent->length; w++) {
		  /* consider removing the words [0,w-1] from the beginning
			 of the sentence */
			if (ctxt->deletable[-1][w]) {
				for (d = sent->word[w].d; d != NULL; d = d->next) {
					if ((d->left == NULL) && region_valid(sent, w, sent->length, d->right, NULL)) {
						mark_region(sent, w, sent->length, d->right, NULL);
						d->marked = TRUE;
					}
				}
			}
		}
	}

	delete_unmarked_disjuncts(sent);

	free_fast_matcher(sent);

	ctxt->local_sent = NULL;
	ctxt->current_resources = NULL;
	ctxt->deletable = NULL;
	count_unset_effective_distance(sent);
}
#endif /* USE_FAT_LINKAGES */

void init_count(Sentence sent)
{
	if (NULL == sent->count_ctxt)
		sent->count_ctxt = (count_context_t *) malloc (sizeof(count_context_t));
	memset(sent->count_ctxt, 0, sizeof(count_context_t));

	init_table(sent);
}

void free_count(Sentence sent)
{
	if (NULL == sent->count_ctxt) return;

	free_table(sent->count_ctxt);
	free(sent->count_ctxt);
	sent->count_ctxt = NULL;
}
