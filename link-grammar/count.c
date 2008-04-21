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

#include <link-grammar/api.h>

/* This file contains the exhaustive search algorithm. */

static char ** deletable;
static char ** effective_dist; 
static Word *  local_sent;
static int	 null_block, islands_ok, null_links;
static Resources current_resources;

int x_match(Connector *a, Connector *b) {
	return match(a, b, 0, 0);
}

void count_set_effective_distance(Sentence sent) {
	effective_dist = sent->effective_dist;
}

void count_unset_effective_distance(Sentence sent) {
	effective_dist = NULL;
}

/*
 * Returns TRUE if s and t match according to the connector matching
 * rules.  The connector strings must be properly formed, starting with
 * zero or more upper case letters, followed by some other letters, and
 * The algorithm is symmetric with respect to a and b.
 *
 * It works as follows:  The labels must match.  The priorities must be
 * compatible (both THIN_priority, or one UP_priority and one DOWN_priority).
 * The sequence of upper case letters must match exactly.  After these comes
 * a sequence of lower case letters "*"s or "^"s.  The matching algorithm
 * is different depending on which of the two priority cases is being
 * considered.  See the comments below. 
 */
int match(Connector *a, Connector *b, int aw, int bw)
{
	const char *s, *t;
	int x, y, dist;
	if (a->label != b->label) return FALSE;
	x = a->priority;
	y = b->priority;

	s = a->string;
	t = b->string;

	while(isupper((int)*s) || isupper((int)*t)) {
		if (*s != *t) return FALSE;
		s++;
		t++;
	}

	if (aw==0 && bw==0) {  /* probably not necessary, as long as effective_dist[0][0]=0 and is defined */
		dist = 0;
	} else {
		assert(aw < bw, "match() did not receive params in the natural order.");
		dist = effective_dist[aw][bw];
	}
	/*	printf("M: a=%4s b=%4s  ap=%d bp=%d  aw=%d  bw=%d  a->ll=%d b->ll=%d  dist=%d\n",
		   s, t, x, y, aw, bw, a->length_limit, b->length_limit, dist); */
	if (dist > a->length_limit || dist > b->length_limit) return FALSE;

	if ((x==THIN_priority) && (y==THIN_priority)) {
		/*
		   Remember that "*" matches anything, and "^" matches nothing
		   (except "*").  Otherwise two characters match if and only if
		   they're equal.  ("^" can be used in the dictionary just like
		   any other connector.)
		   */
		while ((*s!='\0') && (*t!='\0')) {
			if ((*s == '*') || (*t == '*') ||
				((*s == *t) && (*s != '^'))) {
				s++;
				t++;
			} else return FALSE;
		}
		return TRUE;
	} else if ((x==UP_priority) && (y==DOWN_priority)) {
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
		while ((*s!='\0') && (*t!='\0')) {
			if ((*s == *t) || (*s == '*') || (*t == '^')) {
				s++;
				t++;
			} else return FALSE;
		}
		return TRUE;
	} else if ((y==UP_priority) && (x==DOWN_priority)) {
		while ((*s!='\0') && (*t!='\0')) {
			if ((*s == *t) || (*t == '*') || (*s == '^')) {
				s++;
				t++;
			} else return FALSE;
		}
		return TRUE;
	} else return FALSE;
}

typedef struct Table_connector_s Table_connector;
struct Table_connector_s
{
	short            lw, rw;
	Connector        *le, *re;
	short            cost;
	s64              count;
	Table_connector  *next;
};

static int table_size;
static Table_connector ** table;

void init_table(Sentence sent) {
	/* A piecewise exponential function determines the size of the hash table.	  */
	/* Probably should make use of the actual number of disjuncts, rather than just */
	/* the number of words														  */
	int i;
	if (sent->length >= 10) {
		table_size = (1<<16);
		/*  } else if (sent->length >= 10) {
			table_size = (1 << (((6*(sent->length-10))/30) + 10)); */
	} else if (sent->length >= 4) {
		table_size = (1 << (((6*(sent->length-4))/6) + 4));
	} else {
		table_size = (1 << 4);
	}
	table = (Table_connector**) xalloc(table_size * sizeof(Table_connector*));
	for (i=0; i<table_size; i++) {
		table[i] = NULL;
	}
}

static int hash(int lw, int rw, Connector *le, Connector *re, int cost) {
	int i;
	i = 0;

	i = i + (i<<1) + randtable[(lw + i) & (RTSIZE - 1)];
	i = i + (i<<1) + randtable[(rw + i) & (RTSIZE - 1)];
	i = i + (i<<1) + randtable[(((long) le + i) % (table_size+1)) & (RTSIZE - 1)];
	i = i + (i<<1) + randtable[(((long) re + i) % (table_size+1)) & (RTSIZE - 1)];
	i = i + (i<<1) + randtable[(cost+i) & (RTSIZE - 1)];
	return i & (table_size-1);
}

void free_table(Sentence sent) {
	int i;
	Table_connector *t, *x;

	for (i=0; i<table_size; i++) {
		for(t = table[i]; t!= NULL; t=x) {
			x = t->next;
			xfree((void *) t, sizeof(Table_connector));
		}
	}
	xfree((void *) table, table_size * sizeof(Table_connector*));
}

/** 
 * Stores the value in the table.  Assumes it's not already there.
 */
static Table_connector * table_store(int lw, int rw,
                                     Connector *le, Connector *re,
                                     int cost, s64 count)
{
	Table_connector *t, *n;
	int h;

	n = (Table_connector *) xalloc(sizeof(Table_connector));
	n->count = count;
	n->lw = lw; n->rw = rw; n->le = le; n->re = re; n->cost = cost;
	h = hash(lw, rw, le, re, cost);
	t = table[h];
	n->next = t;
	table[h] = n;
	return n;
}

/** returns the pointer to this info, NULL if not there */
static Table_connector * find_table_pointer(int lw, int rw, 
                                       Connector *le, Connector *re,
                                       int cost)
{
	Table_connector *t;
	t = table[hash(lw, rw, le, re, cost)];
	for (; t != NULL; t = t->next) {
		if ((t->lw == lw) && (t->rw == rw) && (t->le == le) && (t->re == re)
			&& (t->cost == cost))  return t;
	}

	/* Create a new connector only if resources are exhausted.
	 * (???) Huh? I guess we're in panic parse mode in that case.
	 */
	if ((current_resources != NULL) && resources_exhausted(current_resources)) {
		return table_store(lw, rw, le, re, cost, 0);
	}
	else return NULL;
}

/** returns the count for this quintuple if there, -1 otherwise */
s64 table_lookup(int lw, int rw, Connector *le, Connector *re, int cost)
{
	Table_connector *t = find_table_pointer(lw, rw, le, re, cost);

	if (t == NULL) return -1; else return t->count;
}

/**
 * Stores the value in the table.  Unlike table_store, it assumes 
 * it's already there
 */
static void table_update(int lw, int rw, 
                         Connector *le, Connector *re,
                         int cost, s64 count)
{
	Table_connector *t = find_table_pointer(lw, rw, le, re, cost);

	assert(t != NULL, "This entry is supposed to be in the table.");
	t->count = count;
}

/**
 * Returns 0 if and only if this entry is in the hash table 
 * with a count value of 0.
 */
static s64 pseudocount(int lw, int rw, Connector *le, Connector *re, int cost)
{
	s64 count;
	count = table_lookup(lw, rw, le, re, cost);
	if (count == 0) return 0; else return 1;
}

static s64 count(int lw, int rw, Connector *le, Connector *re, int cost)
{
	Disjunct * d;
	s64 total, pseudototal;
	int start_word, end_word, w;
	s64 leftcount, rightcount;
	int lcost, rcost, Lmatch, Rmatch;

	Match_node * m, *m1;
	Table_connector *t;

	if (cost < 0) return 0;  /* will we ever call it with cost<0 ? */

	t = find_table_pointer(lw, rw, le, re, cost);

	if (t == NULL) {
		/* Create the table entry with a tentative cost of 0. 
	    * This cost must be updated before we return. */
		t = table_store(lw, rw, le, re, cost, 0);
	} else {
		return t->count;
	}

	if (rw == 1+lw) {
		/* lw and rw are neighboring words */
		/* you can't have a linkage here with cost > 0 */
		if ((le == NULL) && (re == NULL) && (cost == 0)) {
			t->count = 1;
		} else {
			t->count = 0;
		}
		return t->count;
	}

	if ((le == NULL) && (re == NULL)) {
		if (!islands_ok && (lw != -1)) {
		  /* if we don't allow islands (a set of words linked together but
			 separate from the rest of the sentence) then  the cost of skipping
			 n words is just n */
			if (cost == ((rw-lw-1)+null_block-1)/null_block) {
				/* if null_block=4 then the cost of
				   1,2,3,4 nulls is 1, 5,6,7,8 is 2 etc. */
				t->count = 1;
			} else {
				t->count = 0;
			}
			return t->count;
		}
		if (cost == 0) {
			/* there is no zero-cost solution in this case */
			/* slight efficiency hack to separate this cost=0 case out */
			/* but not necessary for correctness */
			t->count = 0;
		} else {
			total = 0;
			w = lw+1;
			for (d = local_sent[w].d; d != NULL; d = d->next) {
				if (d->left == NULL) {
					total += count(w, rw, d->right, NULL, cost-1);
				}
			}
			total += count(w, rw, NULL, NULL, cost-1);
			t->count = total;
		}
		return t->count;
	}

	if (le == NULL) {
		start_word = lw+1;
	} else {
		start_word = le->word;
	}

	if (re == NULL) {
		end_word = rw-1;
	} else {
		end_word = re->word;
	}

	total = 0;

	for (w=start_word; w < end_word+1; w++) {
		m1 = m = form_match_list(w, le, lw, re, rw);
		for (; m!=NULL; m=m->next) {
			d = m->d;
			for (lcost = 0; lcost <= cost; lcost++) {
				rcost = cost-lcost;
				/* Now lcost and rcost are the costs we're assigning
				 * to those parts respectively */

				/* Now, we determine if (based on table only) we can see that
				   the current range is not parsable. */
				Lmatch = (le != NULL) && (d->left != NULL) && match(le, d->left, lw, w);
				Rmatch = (d->right != NULL) && (re != NULL) && match(d->right, re, w, rw);

				rightcount = leftcount = 0;
				if (Lmatch) {
					leftcount = pseudocount(lw, w, le->next, d->left->next, lcost);
					if (le->multi) leftcount += pseudocount(lw, w, le, d->left->next, lcost);
					if (d->left->multi) leftcount += pseudocount(lw, w, le->next, d->left, lcost);
					if (le->multi && d->left->multi) leftcount += pseudocount(lw, w, le, d->left, lcost);
				}

				if (Rmatch) {
					rightcount = pseudocount(w, rw, d->right->next, re->next, rcost);
					if (d->right->multi) rightcount += pseudocount(w,rw,d->right,re->next, rcost);
					if (re->multi) rightcount += pseudocount(w, rw, d->right->next, re, rcost);
					if (d->right->multi && re->multi) rightcount += pseudocount(w, rw, d->right, re, rcost);
				}

				pseudototal = leftcount*rightcount;  /* total number where links are used on both sides */

				if (leftcount > 0) {
					/* evaluate using the left match, but not the right */
					pseudototal += leftcount * pseudocount(w, rw, d->right, re, rcost);
				}
				if ((le == NULL) && (rightcount > 0)) {
					/* evaluate using the right match, but not the left */
					pseudototal += rightcount * pseudocount(lw, w, le, d->left, lcost);
				}

				/* now pseudototal is 0 implies that we know that the true total is 0 */
				if (pseudototal != 0) {
					rightcount = leftcount = 0;
					if (Lmatch) {
						leftcount = count(lw, w, le->next, d->left->next, lcost);
						if (le->multi) leftcount += count(lw, w, le, d->left->next, lcost);
						if (d->left->multi) leftcount += count(lw, w, le->next, d->left, lcost);
						if (le->multi && d->left->multi) leftcount += count(lw, w, le, d->left, lcost);
					}

					if (Rmatch) {
						rightcount = count(w, rw, d->right->next, re->next, rcost);
						if (d->right->multi) rightcount += count(w,rw,d->right,re->next, rcost);
						if (re->multi) rightcount += count(w, rw, d->right->next, re, rcost);
						if (d->right->multi && re->multi) rightcount += count(w, rw, d->right, re, rcost);
					}

					total += leftcount*rightcount;  /* total number where links are used on both sides */

					if (leftcount > 0) {
						/* evaluate using the left match, but not the right */
						total += leftcount * count(w, rw, d->right, re, rcost);
					}
					if ((le == NULL) && (rightcount > 0)) {
						/* evaluate using the right match, but not the left */
						total += rightcount * count(lw, w, le, d->left, lcost);
					}
				}
			}
		}

		put_match_list(m1);
	}
	t->count = total;
	return total;
}

/** 
 * Returns the number of ways the sentence can be parsed with the
 * specified cost Assumes that the hash table has already been
 * initialized, and is freed later.
 */
s64 parse(Sentence sent, int cost, Parse_Options opts)
{
	s64 total;

	count_set_effective_distance(sent);
	current_resources = opts->resources;
	local_sent = sent->word;
	deletable = sent->deletable;
	null_block = opts->null_block;
	islands_ok = opts->islands_ok;

	total = count(-1, sent->length, NULL, NULL, cost+1);
	if (verbosity > 1) {
		printf("Total count with %d null links:   %lld\n", cost, total);
	}
	if ((verbosity > 0) && (PARSE_NUM_OVERFLOW < total)) {
		printf("WARNING: Overflow in count! cnt=%lld\n", total);
	}

	local_sent = NULL;
	current_resources = NULL;
	return total;
}

/*

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

static int x_prune_match(Connector *le, Connector *re, int lw, int rw)
{
	int dist;

	assert(lw < rw, "prune_match() did not receive params in the natural order.");
	dist = effective_dist[lw][rw];
	return prune_match(dist, le, re);
}

/**
 * Returns 0 if this range cannot be successfully filled in with
 * links.  Returns 1 if it can, and it's not been marked, and returns
 * 2 if it can and it has been marked.
 */
static int region_valid(int lw, int rw, Connector *le, Connector *re)
{
	Disjunct * d;
	int left_valid, right_valid, found;
	int i, start_word, end_word;
	int w;
	Match_node * m, *m1;

	i = table_lookup(lw, rw, le, re, 0);
	if (i >= 0) return i;

	if ((le == NULL) && (re == NULL) && deletable[lw][rw]) {
		table_store(lw, rw, le, re, 0, 1);
		return 1;
	}

	if (le == NULL) {
		start_word = lw+1;
	} else {
		start_word = le->word;
	}
	if (re == NULL) {
		end_word = rw-1;
	} else {
		end_word = re->word;
	}

	found = 0;

	for (w=start_word; w < end_word+1; w++) {
		m1 = m = form_match_list(w, le, lw, re, rw);
		for (; m!=NULL; m=m->next) {
			d = m->d;
			/* mark_cost++;*/
			/* in the following expressions we use the fact that 0=FALSE. Could eliminate
			   by always saying "region_valid(...) != 0"  */
			left_valid = (((le != NULL) && (d->left != NULL) && x_prune_match(le, d->left, lw, w)) &&
						  ((region_valid(lw, w, le->next, d->left->next)) ||
						   ((le->multi) && region_valid(lw, w, le, d->left->next)) ||
						   ((d->left->multi) && region_valid(lw, w, le->next, d->left)) ||
						   ((le->multi && d->left->multi) && region_valid(lw, w, le, d->left))));
			if (left_valid && region_valid(w, rw, d->right, re)) {
				found = 1;
				break;
			}
			right_valid = (((d->right != NULL) && (re != NULL) && x_prune_match(d->right, re, w, rw)) &&
						   ((region_valid(w, rw, d->right->next,re->next))	||
							((d->right->multi) && region_valid(w,rw,d->right,re->next))  ||
							((re->multi) && region_valid(w, rw, d->right->next, re))  ||
							((d->right->multi && re->multi) && region_valid(w, rw, d->right, re))));
			if ((left_valid && right_valid) || (right_valid && region_valid(lw, w, le, d->left))) {
				found = 1;
				break;
			}
		}
		put_match_list(m1);
		if (found != 0) break;
	}
	table_store(lw, rw, le, re, 0, found);
	return found;
}

/**
 * Mark as useful all disjuncts involved in some way to complete the
 * structure within the current region.  Note that only disjuncts
 * strictly between lw and rw will be marked.  If it so happens that
 * this region itself is not valid, then this fact will be recorded
 * in the table, and nothing else happens.
 */
static void mark_region(int lw, int rw, Connector *le, Connector *re)
{

	Disjunct * d;
	int left_valid, right_valid, i;
	int start_word, end_word;
	int w;
	Match_node * m, *m1;

	i = region_valid(lw, rw, le, re);
	if ((i==0) || (i==2)) return;
	/* we only reach this point if it's a valid unmarked region, i=1 */
	table_update(lw, rw, le, re, 0, 2);

	if ((le == NULL) && (re == NULL) && (null_links) && (rw != 1+lw)) {
		w = lw+1;
		for (d = local_sent[w].d; d != NULL; d = d->next) {
			if ((d->left == NULL) && region_valid(w, rw, d->right, NULL)) {
				d->marked = TRUE;
				mark_region(w, rw, d->right, NULL);
			}
		}
		mark_region(w, rw, NULL, NULL);
		return;
	}

	if (le == NULL) {
		start_word = lw+1;
	} else {
		start_word = le->word;
	}
	if (re == NULL) {
		end_word = rw-1;
	} else {
		end_word = re->word;
	}

	for (w=start_word; w < end_word+1; w++) {
		m1 = m = form_match_list(w, le, lw, re, rw);
		for (; m!=NULL; m=m->next) {
			d = m->d;
			/* mark_cost++;*/
			left_valid = (((le != NULL) && (d->left != NULL) && x_prune_match(le, d->left, lw, w)) &&
						  ((region_valid(lw, w, le->next, d->left->next)) ||
						   ((le->multi) && region_valid(lw, w, le, d->left->next)) ||
						   ((d->left->multi) && region_valid(lw, w, le->next, d->left)) ||
						   ((le->multi && d->left->multi) && region_valid(lw, w, le, d->left))));
			right_valid = (((d->right != NULL) && (re != NULL) && x_prune_match(d->right, re, w, rw)) &&
						   ((region_valid(w, rw, d->right->next,re->next)) ||
							((d->right->multi) && region_valid(w,rw,d->right,re->next))  ||
							((re->multi) && region_valid(w, rw, d->right->next, re)) ||
							((d->right->multi && re->multi) && region_valid(w, rw, d->right, re))));

			/* The following if statements could be restructured to avoid superfluous calls
			   to mark_region.  It didn't seem a high priority, so I didn't optimize this.
			   */

			if (left_valid && region_valid(w, rw, d->right, re)) {
				d->marked = TRUE;
				mark_region(w, rw, d->right, re);
				mark_region(lw, w, le->next, d->left->next);
				if (le->multi) mark_region(lw, w, le, d->left->next);
				if (d->left->multi) mark_region(lw, w, le->next, d->left);
				if (le->multi && d->left->multi) mark_region(lw, w, le, d->left);
			}

			if (right_valid && region_valid(lw, w, le, d->left)) {
				d->marked = TRUE;
				mark_region(lw, w, le, d->left);
				mark_region(w, rw, d->right->next,re->next);
				if (d->right->multi) mark_region(w,rw,d->right,re->next);
				if (re->multi) mark_region(w, rw, d->right->next, re);
				if (d->right->multi && re->multi) mark_region(w, rw, d->right, re);
			}

			if (left_valid && right_valid) {
				d->marked = TRUE;
				mark_region(lw, w, le->next, d->left->next);
				if (le->multi) mark_region(lw, w, le, d->left->next);
				if (d->left->multi) mark_region(lw, w, le->next, d->left);
				if (le->multi && d->left->multi) mark_region(lw, w, le, d->left);
				mark_region(w, rw, d->right->next,re->next);
				if (d->right->multi) mark_region(w,rw,d->right,re->next);
				if (re->multi) mark_region(w, rw, d->right->next, re);
				if (d->right->multi && re->multi) mark_region(w, rw, d->right, re);
			}
		}
		put_match_list(m1);
	}
}

void delete_unmarked_disjuncts(Sentence sent){
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

	current_resources = opts->resources;
	deletable = sent->deletable;
	count_set_effective_distance(sent);

	/* We begin by unmarking all disjuncts.  This would not be necessary if
	   whenever we created a disjunct we cleared its marked field.
	   I didn't want to search the program for all such places, so
	   I did this way.
	   */
	for (w=0; w<sent->length; w++) {
		for (d=sent->word[w].d; d != NULL; d=d->next) {
			d->marked = FALSE;
		}
	}

	init_fast_matcher(sent);
	init_table(sent);
	local_sent = sent->word;
	null_links = (opts->min_null_count > 0);
	/*
	for (d = sent->word[0].d; d != NULL; d = d->next) {
		if ((d->left == NULL) && region_valid(0, sent->length, d->right, NULL)) {
			mark_region(0, sent->length, d->right, NULL);
			d->marked = TRUE;
		}
	}
	mark_region(0, sent->length, NULL, NULL);
	*/

	if (null_links) {
		mark_region(-1, sent->length, NULL, NULL);
	} else {
		for (w=0; w<sent->length; w++) {
		  /* consider removing the words [0,w-1] from the beginning
			 of the sentence */
			if (deletable[-1][w]) {
				for (d = sent->word[w].d; d != NULL; d = d->next) {
					if ((d->left == NULL) && region_valid(w, sent->length, d->right, NULL)) {
						mark_region(w, sent->length, d->right, NULL);
						d->marked = TRUE;
					}
				}
			}
		}
	}

	delete_unmarked_disjuncts(sent);

	free_fast_matcher(sent);
	free_table(sent);

	local_sent = NULL;
	current_resources = NULL;
	deletable = NULL;
	count_unset_effective_distance(sent);
}
