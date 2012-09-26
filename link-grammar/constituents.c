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

#include <stdarg.h>
#include <string.h>
#include "api.h"
#include "error.h"
#include "constituents.h"
#include "post-process.h"

#define MAXCONSTITUENTS 8192
#define MAXSUBL 16
#define OPEN_BRACKET '['
#define CLOSE_BRACKET ']'

typedef enum {OPEN_TOK, CLOSE_TOK, WORD_TOK} CType;
typedef enum {NONE, STYPE, PTYPE, QTYPE, QDTYPE} WType;

typedef struct
{
	const char * type;
	const char * start_link;
	int left;      /* leftmost word */
	int right;     /* rightmost word */
	int subl;
	int canon;
	int valid;
	char domain_type;
#ifdef AUX_CODE_IS_DEAD
	/* The only code that actually sets aux to a non-zero value is code
	 * followed by code that zets it to zero. -- its dead code, and so 
	 * aux is never actually used. Comment this code out. It was here
	 * TreeBank I compatibility, and isn't used in TreeBank II.
	 */
	int start_num; /* starting link */
	int aux;	
	/* 0: it's an ordinary VP (or other type);
	 * 1: it's an AUX, don't print it;
	 * 2: it's an AUX, and print it
	 */
#endif /* AUX_CODE_IS_DEAD */
} constituent_t;

/* XXX it seems like the old code worked fine with MAX_ELTS=10 */
#define MAX_ELTS 100
typedef struct
{
	int num;
	int e[MAX_ELTS];
	int valid;
} andlist_t;

/*
 * Context used to store assorted intermediate data
 * when the constituent string is being generated.
 */
#define MAX_ANDS 1024
typedef struct
{
	String_set * phrase_ss;
	WType wordtype[MAX_SENTENCE];
	int word_used[MAXSUBL][MAX_SENTENCE];
	int templist[MAX_ELTS];
	constituent_t constituent[MAXCONSTITUENTS];
	andlist_t andlist[MAX_ANDS];
} con_context_t;

/* ================================================================ */

static inline int uppercompare(const char * s, const char * t)
{
	return (FALSE == utf8_upper_match(s,t));
}

/**
 * If a constituent c has a comma at either end, we exclude the
 * comma. (We continue to shift the boundary until we get to
 * something inside the current sublinkage)
 */
static void adjust_for_left_comma(con_context_t * ctxt, Linkage linkage, int c)
{
	int w;
	w = ctxt->constituent[c].left;
	if (strcmp(linkage->word[w], ",") == 0)
	{
		w++;
		while (1) {
			if (ctxt->word_used[linkage->current][w] == 1) break;
			w++;
		}
	}
	ctxt->constituent[c].left = w;
}

static void adjust_for_right_comma(con_context_t *ctxt, Linkage linkage, int c)
{
	int w;
	w = ctxt->constituent[c].right;
	if ((strcmp(linkage->word[w], ",") == 0) ||
	    (strcmp(linkage->word[w], "RIGHT-WALL") == 0))
	{
		w--;
		while (1)
		{
			if (ctxt->word_used[linkage->current][w]==1) break;
			w--;
		}
	}
	ctxt->constituent[c].right = w;
}

static void print_constituent(con_context_t *ctxt, Linkage linkage, int c)
{
	int w;
	if (verbosity < 2) return;

	printf("  c %2d %4s [%c] (%2d-%2d): ",
		   c, ctxt->constituent[c].type, ctxt->constituent[c].domain_type,
		   ctxt->constituent[c].left, ctxt->constituent[c].right);
	for (w = ctxt->constituent[c].left; w <= ctxt->constituent[c].right; w++) {
		printf("%s ", linkage->word[w]); /**PV**/
	}
	printf("\n");
}

/******************************************************
 *        These functions do the bulk of the actual
 * constituent-generating; they're called once for each
 *                      sublinkage
 *********************************************************/

typedef enum
{
	CASE_S=1,
	CASE_UNUSED=2,  /* XXX not used anywhere... */
	CASE_REL_CLAUSE=3,
	CASE_APPOS=4,
	CASE_OPENER=5,
	CASE_PPOPEN=6,
	CASE_SVINV=7,
	CASE_PART_MOD=8,
	CASE_PART_OPEN=9,

} case_type;

/**
 * This function looks for constituents of type ctype1. Say it finds
 * one, call it c1. It searches for the next larger constituent of
 * type ctype2, call it c2. It then generates a new constituent of
 * ctype3, containing all the words in c2 but not c1.
 */
static int gen_comp(con_context_t *ctxt, Linkage linkage,
                    int numcon_total, int numcon_subl,
					     const char * ctype1, const char * ctype2,
                    const char * ctype3, case_type x)
{
	int w, w2, w3, c, c1, c2, done;
	c = numcon_total + numcon_subl;

	for (c1=numcon_total; c1<numcon_total + numcon_subl; c1++)
	{
		/* If ctype1 is NP, it has to be an appositive to continue */
		if ((x==CASE_APPOS) && (post_process_match("MX#*", ctxt->constituent[c1].start_link)==0))
			continue;

#ifdef REVIVE_DEAD_CODE
		/* If ctype1 is X, and domain_type is t, it's an infinitive - skip it */
		if ((x==CASE_UNUSED) && (ctxt->constituent[c1].domain_type=='t'))
			continue;
#endif /* REVIVE_DEAD_CODE */

		/* If it's domain-type z, it's a subject-relative clause;
		   the VP doesn't need an NP */
		if (ctxt->constituent[c1].domain_type=='z')
			continue;

		/* If ctype1 is X or VP, and it's not started by an S, don't generate an NP
		 (Neither of the two previous checks are necessary now, right?) */
#ifdef REVIVE_DEAD_CODE
		/* use this ... if ((x==CASE_S || x==CASE_UNUSED) && */
#endif /* REVIVE_DEAD_CODE */
		if ((x==CASE_S) &&
			(((post_process_match("S", ctxt->constituent[c1].start_link) == 0) &&
			  (post_process_match("SX", ctxt->constituent[c1].start_link) == 0) &&
			  (post_process_match("SF", ctxt->constituent[c1].start_link) == 0)) ||
			 (post_process_match("S##w", ctxt->constituent[c1].start_link) != 0)))
			continue;

		/* If it's an SBAR (relative clause case), it has to be a relative clause */
		if ((x==CASE_REL_CLAUSE) &&
			((post_process_match("Rn", ctxt->constituent[c1].start_link) == 0) &&
			 (post_process_match("R*", ctxt->constituent[c1].start_link) == 0) &&
			 (post_process_match("MX#r", ctxt->constituent[c1].start_link) == 0) &&
			 (post_process_match("Mr", ctxt->constituent[c1].start_link) == 0) &&
			 (post_process_match("MX#d", ctxt->constituent[c1].start_link) == 0)))
			continue;

		/* If ctype1 is SBAR (clause opener case), it has to be an f domain */
		if ((x==CASE_OPENER) && (ctxt->constituent[c1].domain_type!='f'))
			continue;

		/* If ctype1 is SBAR (pp opener case), it has to be a g domain */
		if ((x==CASE_PPOPEN) && (ctxt->constituent[c1].domain_type!='g'))
			continue;

		/* If ctype1 is NP (paraphrase case), it has to be started by an SI */
		if ((x==CASE_SVINV) && (post_process_match("SI", ctxt->constituent[c1].start_link)==0))
			continue;

		/* If ctype1 is VP (participle modifier case), it has to be
		   started by an Mv or Mg */
		if ((x==CASE_PART_MOD) && (post_process_match("M", ctxt->constituent[c1].start_link)==0))
			continue;

		/* If ctype1 is VP (participle opener case), it has
		   to be started by a COp */
		if ((x==CASE_PART_OPEN) && (post_process_match("COp", ctxt->constituent[c1].start_link)==0))
			continue;

		/* Now start at the bounds of c1, and work outwards until you
		   find a larger constituent of type ctype2 */
		if (!(strcmp(ctxt->constituent[c1].type, ctype1)==0))
			continue;

		if (verbosity >= 2)
			printf("Generating complement constituent for c %d of type %s\n",
				   c1, ctype1);
		done = 0;
		for (w2=ctxt->constituent[c1].left; (done==0) && (w2>=0); w2--) {
			for (w3=ctxt->constituent[c1].right; w3<linkage->num_words; w3++) {
				for (c2=numcon_total; (done==0) &&
						 (c2 < numcon_total + numcon_subl); c2++) {
					if (!((ctxt->constituent[c2].left==w2) &&
						  (ctxt->constituent[c2].right==w3)) || (c2==c1))
						continue;
					if (!(strcmp(ctxt->constituent[c2].type, ctype2)==0))
						continue;

					/* if the new constituent (c) is to the left
					   of c1, its right edge should be adjacent to the
					   left edge of c1 - or as close as possible
					   without going outside the current sublinkage.
					   (Or substituting right and left as necessary.) */

					if ((x==CASE_OPENER) || (x==CASE_PPOPEN) || (x==CASE_PART_OPEN)) {
								/* This is the case where c is to the
								   RIGHT of c1 */
						w = ctxt->constituent[c1].right+1;
						while(1) {
							if (ctxt->word_used[linkage->current][w]==1)
								break;
							w++;
						}
						if (w > ctxt->constituent[c2].right)
						{
							done=1;
							continue;
						}
						ctxt->constituent[c].left = w;
						ctxt->constituent[c].right = ctxt->constituent[c2].right;
					}
					else {
						w = ctxt->constituent[c1].left-1;
						while(1) {
							if (ctxt->word_used[linkage->current][w] == 1)
								break;
							w--;
						}
						if (w < ctxt->constituent[c2].left) {
							done=1;
							continue;
						}
						ctxt->constituent[c].right = w;
						ctxt->constituent[c].left = ctxt->constituent[c2].left;
					}

					adjust_for_left_comma(ctxt, linkage, c1);
					adjust_for_right_comma(ctxt, linkage, c1);

					ctxt->constituent[c].type =
						string_set_add(ctype3, ctxt->phrase_ss);
					ctxt->constituent[c].domain_type = 'x';
					ctxt->constituent[c].start_link =
						string_set_add("XX", ctxt->phrase_ss);
#ifdef AUX_CODE_IS_DEAD
					ctxt->constituent[c].start_num =
						ctxt->constituent[c1].start_num; /* bogus */
#endif /* AUX_CODE_IS_DEAD */
					if (verbosity >= 2)
					{
						printf("Larger c found: c %d (%s); ",
							   c2, ctype2);
						printf("Adding constituent:\n");
						print_constituent(ctxt, linkage, c);
					}
					c++;
					if (MAXCONSTITUENTS <= c)
					{
						err_ctxt ec;
						ec.sent = linkage->sent;
						err_msg(&ec, Error, "Error: Too many constituents (a).\n");
						c--;
					}
					done = 1;
				}
			}
		}
		if (verbosity >= 2)
		{
			if (done == 0)
				printf("No constituent added, because no larger %s " \
					   " was found\n", ctype2);
		}
	}
	numcon_subl = c - numcon_total;
	return numcon_subl;
}

/**
 * Look for a constituent started by an MVs or MVg.
 * Find any VP's or ADJP's that contain it (without going
 * beyond a larger S or NP). Adjust them so that
 * they end right before the m domain starts.
 */
static void adjust_subordinate_clauses(con_context_t *ctxt, Linkage linkage,
                                       int numcon_total,
                                       int numcon_subl)
{
	int c, w, c2, w2, done;

	for (c=numcon_total; c<numcon_total + numcon_subl; c++) {
		if ((post_process_match("MVs", ctxt->constituent[c].start_link) == 1) ||
			 (post_process_match("MVg", ctxt->constituent[c].start_link) == 1)) {
			done=0;
			for (w2=ctxt->constituent[c].left-1; (done==0) && w2>=0; w2--) {
				for (c2=numcon_total; c2<numcon_total + numcon_subl; c2++) {
					if (!((ctxt->constituent[c2].left==w2) &&
						  (ctxt->constituent[c2].right >= ctxt->constituent[c].right)))
						continue;
					if ((strcmp(ctxt->constituent[c2].type, "S") == 0) ||
						(strcmp(ctxt->constituent[c2].type, "NP") == 0)) {
						done=1;
						break;
					}
					if ((ctxt->constituent[c2].domain_type == 'v') ||
						(ctxt->constituent[c2].domain_type == 'a')) {
						w = ctxt->constituent[c].left-1;
						while (1) {
							if (ctxt->word_used[linkage->current][w] == 1) break;
							w--;
						}
						ctxt->constituent[c2].right = w;

						if (verbosity >= 2)
							printf("Adjusting constituent %d:\n", c2);
						print_constituent(ctxt, linkage, c2);
					}
				}
			}
			if (strcmp(linkage->word[ctxt->constituent[c].left], ",") == 0)
				ctxt->constituent[c].left++;
		}	
	}
}

/******************************************************
 * These functions are called once, after constituents
 * for each sublinkage have been generated, to merge them
 * together and fix up some other things.
 *
 ********************************************************/

/**
 * Here we're looking for the next andlist element to add on
 * to a conjectural andlist, stored in the array templist.
 * We go through the constituents, starting at "start".
 */
static int find_next_element(con_context_t *ctxt,
                             Linkage linkage,
                             int start,
                             int numcon_total,
                             int num_elements,
                             int num_lists)
{
	int c, a, ok, c2, c3, addedone=0;

	assert(num_elements <= MAX_ELTS, "Constutent element array overflow!\n");

	for (c=start+1; c<numcon_total; c++)
	{
		constituent_t *cc = &ctxt->constituent[c];

		if (cc->valid == 0)
			continue;
		if (strcmp(ctxt->constituent[ctxt->templist[0]].type, cc->type)!=0)
			continue;
		ok = 1;

		/* We're considering adding constituent c to the andlist.
		   If c is in the same sublinkage as one of the other andlist
		   elements, don't add it. If it overlaps with one of the other
		   constituents, don't add it. If there's a constituent
		   identical to c that occurs in a sublinkage in which one of
		   the other elements occurs, don't add it. */

		for (a=0; a<num_elements; a++)
		{
			int t = ctxt->templist[a];
			constituent_t *ct = &ctxt->constituent[t];

			if (cc->subl == ct->subl)
				ok=0;
			if (((cc->left < ct->left) && (cc->right > ct->left))
				||
				((cc->right > ct->right) && (cc->left < ct->right))
				||
				((cc->right > ct->right) && (cc->left < ct->right))
				||
				((cc->left > ct->left) && (cc->right < ct->right)))
				ok=0;

			for (c2=0; c2<numcon_total; c2++)
			{
				if (ctxt->constituent[c2].canon != cc->canon)
					continue;
				for (c3=0; c3<numcon_total; c3++)
				{
					if ((ctxt->constituent[c3].canon == ct->canon)
						&& (ctxt->constituent[c3].subl == ctxt->constituent[c2].subl))
						ok=0;
				}
			}
		}
		if (ok == 0) continue;

		ctxt->templist[num_elements] = c;
		addedone = 1;
		num_lists = find_next_element(ctxt, linkage, c, numcon_total,
		                              num_elements+1, num_lists);

		/* Test for overlow of the and-list.
		 * With the current parser, the following will cause an
		 * overflow:
		 *
		 * I have not seen the grysbok, or the suni, or the dibitag, or
		 * the lechwi, or the aoul, or the gerenuk, or the blaauwbok,
		 * or the chevrotain, or lots of others, but who in the world
		 * could guess what they were or what they looked like, judging
		 * only from the names?
		 */
		if (MAX_ANDS <= num_lists)
		{
			err_ctxt ec;
			ec.sent = linkage->sent;
			err_msg(&ec, Error, "Error: Constituent overflowed andlist!\n");
			return MAX_ANDS;
		}
	}

	if (addedone == 0 && num_elements > 1)
	{
		for (a=0; a<num_elements; a++) {
			ctxt->andlist[num_lists].e[a] = ctxt->templist[a];
			ctxt->andlist[num_lists].num = num_elements;
		}
		num_lists++;
	}
	return num_lists;
}

/* When fat-links are finally removed, almost all of the code
 * for merge_constituents will evaporate away.
 */
static int merge_constituents(con_context_t *ctxt, Linkage linkage, int numcon_total)
{
	int c1, c2=0, c3, ok, a, n, a2, n2, match, listmatch, a3;
	int num_lists, num_elements;
	int leftend, rightend;

	for (c1=0; c1<numcon_total; c1++)
	{
		ctxt->constituent[c1].valid = 1;

		/* Find and invalidate any constituents with negative length */
		if(ctxt->constituent[c1].right < ctxt->constituent[c1].left)
		{
			if(verbosity >= 2)
			{
				err_ctxt ec;
				ec.sent = linkage->sent;
				err_msg(&ec, Warn, 
					"Warning: Constituent %d has negative length. Deleting it.\n", c1);
			}
			ctxt->constituent[c1].valid = 0;
		}
		ctxt->constituent[c1].canon = c1;
	}

	/* First go through and give each constituent a canonical number
	   (the index number of the lowest-numbered constituent
	   identical to it) */

	for (c1 = 0; c1 < numcon_total; c1++)
	{
		if (ctxt->constituent[c1].canon != c1) continue;
		for (c2 = c1 + 1; c2 < numcon_total; c2++)
		{
			if ((ctxt->constituent[c1].left == ctxt->constituent[c2].left) &&
				(ctxt->constituent[c1].right == ctxt->constituent[c2].right) &&
				(strcmp(ctxt->constituent[c1].type, ctxt->constituent[c2].type) == 0))
			{
				ctxt->constituent[c2].canon = c1;
			}
		}
	}

	/* If constituents A and B in different sublinkages X and Y
	 * have one endpoint in common, but A is larger at the other end,
	 * and B has no duplicate in X, then declare B invalid. (Example:
	 * " [A [B We saw the cat B] and the dog A] "
	 */
	for (c1 = 0; c1 < numcon_total; c1++)
	{
		if (ctxt->constituent[c1].valid == 0) continue;
		for (c2 = 0; c2 < numcon_total; c2++)
		{
			if (ctxt->constituent[c2].subl == ctxt->constituent[c1].subl) continue;
			ok = 1;
			/* Does c2 have a duplicate in the sublinkage containing c1?
			   If so, bag it */
			for (c3 = 0; c3 < numcon_total; c3++)
			{
				if ((ctxt->constituent[c2].canon == ctxt->constituent[c3].canon) &&
					(ctxt->constituent[c3].subl == ctxt->constituent[c1].subl))
					ok = 0;
			}
			for (c3 = 0; c3 < numcon_total; c3++)
			{
				if ((ctxt->constituent[c1].canon == ctxt->constituent[c3].canon) &&
					(ctxt->constituent[c3].subl == ctxt->constituent[c2].subl))
					ok = 0;
			}
			if (ok == 0) continue;
			if ((ctxt->constituent[c1].left == ctxt->constituent[c2].left) &&
				(ctxt->constituent[c1].right > ctxt->constituent[c2].right) &&
				(strcmp(ctxt->constituent[c1].type, ctxt->constituent[c2].type) == 0))
			{
				ctxt->constituent[c2].valid = 0;
			}

			if ((ctxt->constituent[c1].left < ctxt->constituent[c2].left) &&
				(ctxt->constituent[c1].right == ctxt->constituent[c2].right) &&
				(strcmp(ctxt->constituent[c1].type, ctxt->constituent[c2].type) == 0))
			{
				ctxt->constituent[c2].valid = 0;
			}
		}
	}

	/* Now go through and find duplicates; if a pair is found,
	 * mark one as invalid. (It doesn't matter if they're in the
	 * same sublinkage or not)
	 */
	for (c1 = 0; c1 < numcon_total; c1++)
	{
		if (ctxt->constituent[c1].valid == 0) continue;
		for (c2 = c1 + 1; c2 < numcon_total; c2++)
		{
			if (ctxt->constituent[c2].canon == ctxt->constituent[c1].canon)
				ctxt->constituent[c2].valid = 0;
		}
	}

	/* Now we generate the and-lists. An and-list is a set of mutually
	 * exclusive constituents. Each constituent in the list may not
	 * be present in the same sublinkage as any of the others.
	 */
	num_lists = 0;
	for (c1 = 0; c1 < numcon_total; c1++)
	{
		if (ctxt->constituent[c1].valid == 0) continue;
		num_elements = 1;
		ctxt->templist[0] = c1;
		num_lists = find_next_element(ctxt, linkage, c1, numcon_total,
			                           num_elements, num_lists);

		/* If we're overflowing, then punt */
		if (MAX_ANDS <= num_lists)
			break;
	}

	if (verbosity >= 2)
	{
		printf("And-lists:\n");
		for (n=0; n<num_lists; n++)
		{
			printf("  %d: ", n);
			for (a=0; a < ctxt->andlist[n].num; a++)
			{
				printf("%d ", ctxt->andlist[n].e[a]);
			}
			printf("\n");
		}
	}

	/* Now we prune out any andlists that are subsumed by other
	 * andlists--e.g. if andlist X contains constituents A and B,
	 * and Y contains A B and C, we throw out X
	 */
	for (n = 0; n < num_lists; n++)
	{
		ctxt->andlist[n].valid = 1;
		for (n2 = 0; n2 < num_lists; n2++)
		{
			if (n2 == n) continue;
			if (ctxt->andlist[n2].num < ctxt->andlist[n].num)
				continue;

			listmatch = 1;
			for (a = 0; a < ctxt->andlist[n].num; a++)
			{
				match = 0;
				for (a2 = 0; a2 < ctxt->andlist[n2].num; a2++)
				{
					if (ctxt->andlist[n2].e[a2] == ctxt->andlist[n].e[a])
						match = 1;
				}
				if (match == 0) listmatch = 0;
				/* At least one element was not matched by n2 */
			}
			if (listmatch == 1) ctxt->andlist[n].valid = 0;
		}
	}

	/* If an element of an andlist contains an element of another
	 * andlist, it must contain the entire andlist.
	 */
	for (n = 0; n < num_lists; n++)
	{
		if (ctxt->andlist[n].valid == 0)
			continue;
		for (a = 0; (a < ctxt->andlist[n].num) && (ctxt->andlist[n].valid); a++)
		{
			for (n2 = 0; (n2 < num_lists) && (ctxt->andlist[n].valid); n2++)
			{
				if ((n2 == n) || (ctxt->andlist[n2].valid == 0))
					continue;
				for (a2 = 0; (a2 < ctxt->andlist[n2].num) && (ctxt->andlist[n].valid); a2++)
				{
					c1 = ctxt->andlist[n].e[a];
					c2 = ctxt->andlist[n2].e[a2];
					if (c1 == c2)
						continue;
					if (!((ctxt->constituent[c2].left <= ctxt->constituent[c1].left) &&
						  (ctxt->constituent[c2].right >= ctxt->constituent[c1].right)))
						continue;
					if (verbosity >= 2)
						printf("Found that c%d in list %d is bigger " \
							   "than c%d in list %d\n", c2, n2, c1, n);
					ok = 1;

					/* An element of n2 contains an element of n.
					 * Now, we check to see if that element of n2
					 * contains ALL the elements of n.
					 * If not, n is invalid.
					 */
					for (a3 = 0; a3 < ctxt->andlist[n].num; a3++)
					{
						c3 = ctxt->andlist[n].e[a3];
						if ((ctxt->constituent[c2].left>ctxt->constituent[c3].left) ||
							(ctxt->constituent[c2].right<ctxt->constituent[c3].right))
							ok = 0;
					}
					if (ok != 0)
						continue;
					ctxt->andlist[n].valid = 0;
					if (verbosity >= 2)
					{
						printf("Eliminating andlist, " \
							   "n=%d, a=%d, n2=%d, a2=%d: ",
							   n, a, n2, a2);
						for (a3 = 0; a3 < ctxt->andlist[n].num; a3++)
						{
							printf("%d ", ctxt->andlist[n].e[a3]);
						}
						printf("\n");
					}
				}
			}
		}
	}

	if (verbosity >= 2)
	{
		printf("And-lists after pruning:\n");
		for (n=0; n<num_lists; n++) {
			if (ctxt->andlist[n].valid==0)
				continue;
			printf("  %d: ", n);
			for (a=0; a<ctxt->andlist[n].num; a++) {
				printf("%d ", ctxt->andlist[n].e[a]);
			}
			printf("\n");
		}
	}

	c1 = numcon_total;
	for (n = 0; n < num_lists; n++)
	{
		if (ctxt->andlist[n].valid == 0) continue;
		leftend = 256;
		rightend = -1;
		for (a = 0; a < ctxt->andlist[n].num; a++)
		{
			c2 = ctxt->andlist[n].e[a];
			if (ctxt->constituent[c2].left < leftend)
			{
				leftend = ctxt->constituent[c2].left;
			}
			if (ctxt->constituent[c2].right > rightend)
			{
				rightend=ctxt->constituent[c2].right;
			}
		}

		ctxt->constituent[c1].left = leftend;
		ctxt->constituent[c1].right = rightend;
		ctxt->constituent[c1].type = ctxt->constituent[c2].type;
		ctxt->constituent[c1].domain_type = 'x';
		ctxt->constituent[c1].valid = 1;
		ctxt->constituent[c1].start_link = ctxt->constituent[c2].start_link;  /* bogus */

#ifdef AUX_CODE_IS_DEAD /* See comments above */
		ctxt->constituent[c1].start_num = ctxt->constituent[c2].start_num;	/* bogus */

		/* If a constituent within the andlist is an aux (aux==1),
		 * set aux for the whole-list constituent to 2, also set
		 * aux for the smaller constituent to 2, meaning they'll both
		 * be printed (as an "X"). (If aux is 2 for the smaller
		 * constituent going in, the same thing should be done,
		 * though I doubt this ever happens.)
		 */
		for (a = 0; a < ctxt->andlist[n].num; a++)
		{
			c2 = ctxt->andlist[n].e[a];
			if ((ctxt->constituent[c2].aux == 1) || (ctxt->constituent[c2].aux == 2))
			{
				ctxt->constituent[c1].aux = 2;
				ctxt->constituent[c2].aux = 2;
			}
		}
#endif /* AUX_CODE_IS_DEAD */

		if (verbosity >= 2)
			printf("Adding constituent:\n");
		print_constituent(ctxt, linkage, c1);
		c1++;
	}
	numcon_total = c1;
	return numcon_total;
}

/**
 * Go through all the words. If a word is on the right end of
 * an S (or SF or SX), wordtype[w]=STYPE.  If it's also on the left end of a
 * Pg*b, I, PP, or Pv, wordtype[w]=PTYPE. If it's a question-word
 * used in an indirect question, wordtype[w]=QTYPE. If it's a
 * question-word determiner,  wordtype[w]=QDTYPE. Else wordtype[w]=NONE.
 * (This function is called once for each sublinkage.)
 */
static void generate_misc_word_info(con_context_t * ctxt, Linkage linkage)
{
	int l1, l2, w1, w2;
	const char * label1, * label2;

	for (w1=0; w1<linkage->num_words; w1++)
		ctxt->wordtype[w1]=NONE;

	for (l1=0; l1<linkage_get_num_links(linkage); l1++) {	
		w1=linkage_get_link_rword(linkage, l1);
		label1 = linkage_get_link_label(linkage, l1);
		if ((uppercompare(label1, "S")==0) ||
			(uppercompare(label1, "SX")==0) ||
			(uppercompare(label1, "SF")==0)) {
			ctxt->wordtype[w1] = STYPE;
			for (l2=0; l2<linkage_get_num_links(linkage); l2++) {
				w2=linkage_get_link_lword(linkage, l2);
				label2 = linkage_get_link_label(linkage, l2);
				if ((w1==w2) &&
					((post_process_match("Pg#b", label2)==1) ||
					 (uppercompare(label2, "I")==0) ||
					 (uppercompare(label2, "PP")==0) ||
					 (post_process_match("Pv", label2)==1))) {
					/* Pvf, Pgf? */
					ctxt->wordtype[w1] = PTYPE;
				}
			}
		}
		if (post_process_match("QI#d", label1)==1) {
			ctxt->wordtype[w1] = QTYPE;
			for (l2=0; l2<linkage_get_num_links(linkage); l2++) {
				w2=linkage_get_link_lword(linkage, l2);
				label2 = linkage_get_link_label(linkage, l2);
				if ((w1==w2) && (post_process_match("D##w", label2)==1)) {
					ctxt->wordtype[w1] = QDTYPE;
				}
			}
		}
		if (post_process_match("Mr", label1)==1) ctxt->wordtype[w1] = QDTYPE;
		if (post_process_match("MX#d", label1)==1) ctxt->wordtype[w1] = QDTYPE;
	}
}

static int new_style_conjunctions(con_context_t *ctxt, Linkage linkage, int numcon_total)
{
#ifdef DEBUG
	int c;
	for (c = 0; c < numcon_total; c++)
	{
		constituent_t *ct = &ctxt->constituent[c];
		printf("ola %d valid=%d %s start=%s lr=%d %d\n", c, 
			ct->valid, ct->type, ct->start_link, ct->left, ct->right);
	}
#endif
	return numcon_total;
}

static int last_minute_fixes(con_context_t *ctxt, Linkage linkage, int numcon_total)
{
	int c, c2, global_leftend_found, adjustment_made;
	int global_rightend_found, lastword, newcon_total = 0;
	Sentence sent;
	sent = linkage_get_sentence(linkage);

	for (c = 0; c < numcon_total; c++)
	{
		/* In a paraphrase construction ("John ran, he said"),
		   the paraphrasing clause doesn't get
		   an S. (This is true in Treebank II, not Treebank I) */

		if (uppercompare(ctxt->constituent[c].start_link, "CP") == 0)
		{
			ctxt->constituent[c].valid = 0;
		}

		/* If it's a possessive with an "'s", the NP on the left
		   should be extended to include the "'s". */
		if ((uppercompare(ctxt->constituent[c].start_link, "YS") == 0) ||
			(uppercompare(ctxt->constituent[c].start_link, "YP") == 0))
		{
			ctxt->constituent[c].right++;
		}

		/* If a constituent has starting link MVpn, it's a time
		   expression like "last week"; label it as a noun phrase
		   (incorrectly) */

		if (strcmp(ctxt->constituent[c].start_link, "MVpn") == 0)
		{
			ctxt->constituent[c].type = string_set_add("NP", ctxt->phrase_ss);
		}
		if (strcmp(ctxt->constituent[c].start_link, "COn") == 0)
		{
			ctxt->constituent[c].type = string_set_add("NP", ctxt->phrase_ss);
		}
		if (strcmp(ctxt->constituent[c].start_link, "Mpn") == 0)
		{
			ctxt->constituent[c].type = string_set_add("NP", ctxt->phrase_ss);
		}

		/* If the constituent is an S started by "but" or "and" at
		   the beginning of the sentence, it should be ignored. */

		if ((strcmp(ctxt->constituent[c].start_link, "Wdc") == 0) &&
			(ctxt->constituent[c].left == 2))
		{
			ctxt->constituent[c].valid = 0;
		}

		/* For prenominal adjectives, an ADJP constituent is assigned
		   if it's a hyphenated (Ah) or comparative (Am) adjective;
		   otherwise no ADJP is assigned, unless the phrase is more
		   than one word long (e.g. "very big"). The same with certain
		   types of adverbs. */
		/* That was for Treebank I. For Treebank II, the rule only
		   seems to apply to prenominal adjectives (of all kinds).
		   However, it also applies to number expressions ("QP"). */

		if ((post_process_match("A", ctxt->constituent[c].start_link) == 1) ||
			(ctxt->constituent[c].domain_type == 'd') ||
			(ctxt->constituent[c].domain_type == 'h')) {
			if (ctxt->constituent[c].right-ctxt->constituent[c].left == 0)
			{
				ctxt->constituent[c].valid = 0;
			}
		}

		if ((ctxt->constituent[c].domain_type == 'h') &&
			(strcmp(linkage->word[ctxt->constituent[c].left - 1], "$") == 0))
		{
			ctxt->constituent[c].left--;
		}

#ifdef AUX_CODE_IS_DEAD /* See comments at top */
		/* If a constituent has type VP and its aux value is 2,
		   this means it's an aux that should be printed; change its
		   type to "X". If its aux value is 1, set "valid" to 0. (This
		   applies to Treebank I only) */

		if (ctxt->constituent[c].aux == 2)
		{
			ctxt->constituent[c].type = string_set_add("X", ctxt->phrase_ss);
		}
		if (ctxt->constituent[c].aux == 1)
		{
			ctxt->constituent[c].valid = 0;
		}
#endif /* AUX_CODE_IS_DEAD */
	}

	numcon_total = numcon_total + newcon_total;

	/* If there's a global S constituent that includes everything
	   except a final period or question mark, extend it by one word */

	for (c = 0; c < numcon_total; c++)
	{
		if ((ctxt->constituent[c].right == linkage->num_words -3) &&
			(ctxt->constituent[c].left == 1) &&
			(strcmp(ctxt->constituent[c].type, "S") == 0) &&
			(strcmp(sent->word[linkage->num_words -2].string, ".") == 0))
			ctxt->constituent[c].right++;
	}

	/* If there's no S boundary at the very left end of the sentence,
	   or the very right end, create a new S spanning the entire sentence */

	lastword = linkage->num_words - 2;
	global_leftend_found = 0;
	global_rightend_found = 0;
	for (c = 0; c < numcon_total; c++)
	{
		if ((ctxt->constituent[c].left == 1) && (strcmp(ctxt->constituent[c].type, "S") == 0) &&
			(ctxt->constituent[c].valid == 1))
		{
			global_leftend_found = 1;
		}
	}
	for (c = 0; c < numcon_total; c++)
	{
		if ((ctxt->constituent[c].right >= lastword) &&
			(strcmp(ctxt->constituent[c].type, "S") == 0) && (ctxt->constituent[c].valid == 1))
		{
			global_rightend_found = 1;
		}
	}
	if ((global_leftend_found == 0) || (global_rightend_found == 0))
	{
		c = numcon_total;
		ctxt->constituent[c].left = 1;
		ctxt->constituent[c].right = linkage->num_words-1;
		ctxt->constituent[c].type = string_set_add("S", ctxt->phrase_ss);
		ctxt->constituent[c].valid = 1;
		ctxt->constituent[c].domain_type = 'x';
		numcon_total++;
		if (verbosity >= 2)
			printf("Adding global sentence constituent:\n");
		print_constituent(ctxt, linkage, c);
	}

	/* Check once more to see if constituents are nested (checking 
	 * BETWEEN sublinkages this time). This code probably goes away
	 * when fat-links get removed. */
	while (1)
	{
		adjustment_made=0;
		for (c = 0; c < numcon_total; c++)
		{
			if(ctxt->constituent[c].valid == 0) continue;
			for (c2 = 0; c2 < numcon_total; c2++)
			{
				if(ctxt->constituent[c2].valid == 0) continue;
				if ((ctxt->constituent[c].left < ctxt->constituent[c2].left) &&
					(ctxt->constituent[c].right < ctxt->constituent[c2].right) &&
					(ctxt->constituent[c].right >= ctxt->constituent[c2].left))
				{
					if (verbosity >= 2)
					{
						err_ctxt ec;
						ec.sent = linkage->sent;
						err_msg(&ec, Warn, "Warning: the constituents aren't nested! "
						          "Adjusting them. (%d, %d)\n", c, c2);
					}
					ctxt->constituent[c].left = ctxt->constituent[c2].left;
				}
			}
		}
		if (adjustment_made == 0) break;
	}
	return numcon_total;
}

/**
 * This function generates a table, word_used[i][w], showing
 * whether each word w is used in each sublinkage i; if so,
 * the value for that cell of the table is 1.
 */
static void count_words_used(con_context_t *ctxt, Linkage linkage)
{
	int i, w, link, num_subl;

	num_subl = linkage->num_sublinkages;
	if(linkage->unionized == 1 && num_subl > 1) num_subl--;

	if (verbosity >= 2)
		printf("Number of sublinkages = %d\n", num_subl);

	for (i=0; i<num_subl; i++)
	{
		for (w = 0; w < linkage->num_words; w++) ctxt->word_used[i][w] = 0;
		linkage->current = i;
		for (link = 0; link < linkage_get_num_links(linkage); link++)
		{
			ctxt->word_used[i][linkage_get_link_lword(linkage, link)] = 1;
			ctxt->word_used[i][linkage_get_link_rword(linkage, link)] = 1;
		}
		if (verbosity >= 2)
		{
			printf("Sublinkage %d: ", i);
			for (w = 0; w < linkage->num_words; w++)
			{
				if (ctxt->word_used[i][w] == 0) printf("0 ");
				if (ctxt->word_used[i][w] == 1) printf("1 ");
			}
			printf("\n");
		}
	}
}

static int add_constituent(con_context_t *ctxt, int c, const Linkage linkage,
                           const Domain *domain,
                           int l, int r, const char * name)
{
	int nwords = linkage->num_words-2;
	c++;

	/* Avoid running off end, to walls. */
	if (l < 1) l=1;
	if (r > nwords) r = nwords;
	if (l > nwords) l = nwords;
	assert(l <= r, "negative constituent length!" );

	ctxt->constituent[c].type = string_set_add(name, ctxt->phrase_ss);
	ctxt->constituent[c].left = l;
	ctxt->constituent[c].right = r;
	ctxt->constituent[c].domain_type = domain->type;
	ctxt->constituent[c].start_link =
		linkage_get_link_label(linkage, domain->start_link);
#ifdef AUX_CODE_IS_DEAD
	ctxt->constituent[c].start_num = domain->start_link;
#endif /* AUX_CODE_IS_DEAD */
	return c;
}

static const char * cons_of_domain(const Linkage linkage, char domain_type)
{
	switch (domain_type) {
	case 'a':
		return "ADJP";
	case 'b':
		return "SBAR";
	case 'c':
		return "VP";
	case 'd':
		return "QP";
	case 'e':
		return "ADVP";
	case 'f':
		return "SBAR";
	case 'g':
		return "PP";
	case 'h':
		return "QP";
	case 'i':
		return "ADVP";
	case 'k':
		return "PRT";
	case 'n':
		return "NP";
	case 'p':
		return "PP";
	case 'q':
		return "SINV";
	case 's':
		return "S";
	case 't':
		return "VP";
	case 'u':
		return "ADJP";
	case 'v':
		return "VP";
	case 'y':
		return "NP";
	case 'z':
		return "VP";
	default:
		{
			err_ctxt ec;
			ec.sent = linkage->sent;
			err_msg(&ec, Error, "Error: Illegal domain: %c\n", domain_type);
			return "";
		}
	}
}

static int read_constituents_from_domains(con_context_t *ctxt, Linkage linkage,
                                          int numcon_total, int s)
{
	int d, c, leftlimit, l, leftmost, rightmost, w, c2, numcon_subl = 0, w2;
	List_o_links * dlink;
	int rootleft, adjustment_made;
	Sublinkage * subl;
	const char * name;
	Domain domain;

	subl = &linkage->sublinkage[s];

	for (d = 0, c = numcon_total; d < subl->pp_data.N_domains; d++, c++)
	{
		domain = subl->pp_data.domain_array[d];
		// rootright = linkage_get_link_rword(linkage, domain.start_link);
		rootleft =  linkage_get_link_lword(linkage, domain.start_link);

		if ((domain.type=='c') ||
			(domain.type=='d') ||
			(domain.type=='e') ||
			(domain.type=='f') ||
			(domain.type=='g') ||
			(domain.type=='u') ||
			(domain.type=='y'))
		{
			leftlimit = 0;
			leftmost = linkage_get_link_lword(linkage, domain.start_link);
			rightmost = linkage_get_link_lword(linkage, domain.start_link);
		}
		else
		{
			leftlimit = linkage_get_link_lword(linkage, domain.start_link) + 1;
			leftmost = linkage_get_link_rword(linkage, domain.start_link);
			rightmost = linkage_get_link_rword(linkage, domain.start_link);
		}

		/* Start by assigning both left and right limits to the
		 * right word of the start link. This will always be contained
		 * in the constituent. This will also handle the case
		 * where the domain contains no links.
		 */
		for (dlink = domain.lol; dlink != NULL; dlink = dlink->next)
		{
			l = dlink->link;

			if ((linkage_get_link_lword(linkage, l) < leftmost) &&
				(linkage_get_link_lword(linkage, l) >= leftlimit))
			{
				leftmost = linkage_get_link_lword(linkage, l);
			}

			if (linkage_get_link_rword(linkage, l) > rightmost)
			{
				rightmost = linkage_get_link_rword(linkage, l);
			}
		}

		c--;
		c = add_constituent(ctxt, c, linkage, &domain, leftmost, rightmost,
						cons_of_domain(linkage, domain.type));

		if (domain.type == 'z')
		{
			c = add_constituent(ctxt, c, linkage, &domain, leftmost, rightmost, "S");
		}
		if (domain.type=='c')
		{
			c = add_constituent(ctxt, c, linkage, &domain, leftmost, rightmost, "S");
		}
		if ((post_process_match("Ce*", ctxt->constituent[c].start_link)==1) ||
			(post_process_match("Rn", ctxt->constituent[c].start_link)==1))
		{
			c = add_constituent(ctxt, c, linkage, &domain, leftmost, rightmost, "SBAR");
		}
		if ((post_process_match("R*", ctxt->constituent[c].start_link)==1) ||
			(post_process_match("MX#r", ctxt->constituent[c].start_link)==1))
		{
			w = leftmost;
			if (strcmp(linkage->word[w], ",") == 0) w++;
			c = add_constituent(ctxt, c, linkage, &domain, w, w, "WHNP");
		}
		if (post_process_match("Mj", ctxt->constituent[c].start_link) == 1)
		{
			w = leftmost;
			if (strcmp(linkage->word[w], ",") == 0) w++;
			c = add_constituent(ctxt, c, linkage, &domain, w, w+1, "WHPP");
			c = add_constituent(ctxt, c, linkage, &domain, w+1, w+1, "WHNP");
		}
		if ((post_process_match("Ss#d", ctxt->constituent[c].start_link)==1) ||
			(post_process_match("B#d", ctxt->constituent[c].start_link)==1))
		{
			c = add_constituent(ctxt, c, linkage, &domain, rootleft, rootleft, "WHNP");
			c = add_constituent(ctxt, c, linkage, &domain,
							rootleft, ctxt->constituent[c-1].right, "SBAR");
		}
		if (post_process_match("CP", ctxt->constituent[c].start_link)==1)
		{
			if (strcmp(linkage->word[leftmost], ",") == 0)
				ctxt->constituent[c].left++;
			c = add_constituent(ctxt, c, linkage, &domain, 1, linkage->num_words-1, "S");
		}
		if ((post_process_match("MVs", ctxt->constituent[c].start_link)==1) ||
			(domain.type=='f'))
		{
			w = ctxt->constituent[c].left;
			if (strcmp(linkage->word[w], ",") == 0)
				w++;
			if (strcmp(linkage->word[w], "when") == 0)
			{
				c = add_constituent(ctxt, c, linkage, &domain, w, w, "WHADVP");
			}
		}
		if (domain.type=='t')
		{
			c = add_constituent(ctxt, c, linkage, &domain, leftmost, rightmost, "S");
		}
		if ((post_process_match("QI", ctxt->constituent[c].start_link) == 1) ||
			(post_process_match("Mr", ctxt->constituent[c].start_link) == 1) ||
			(post_process_match("MX#d", ctxt->constituent[c].start_link) == 1))
		{
			w = leftmost;
			if (strcmp(linkage->word[w], ",") == 0) w++;
			if (ctxt->wordtype[w] == NONE)
				name = "WHADVP";
			else if (ctxt->wordtype[w] == QTYPE)
				name = "WHNP";
			else if (ctxt->wordtype[w] == QDTYPE)
				name = "WHNP";
			else
				assert(0, "Unexpected word type");
			c = add_constituent(ctxt, c, linkage, &domain, w, w, name);

			if (ctxt->wordtype[w] == QDTYPE)
			{
				/* Now find the finite verb to the right, start an S */
				/* Limit w2 to sentence length. */
				// for( w2=w+1; w2 < ctxt->r_limit-1; w2++ )
				for (w2 = w+1; w2 < rightmost; w2++)
				  if ((ctxt->wordtype[w2] == STYPE) || (ctxt->wordtype[w2] == PTYPE)) break;

				/* Adjust the right boundary of previous constituent */
				ctxt->constituent[c].right = w2 - 1;
				c = add_constituent(ctxt, c, linkage, &domain, w2, rightmost, "S");
			}
		}

		if (ctxt->constituent[c].domain_type == '\0')
		{
			err_ctxt ec;
			ec.sent = linkage->sent;
			err_msg(&ec, Error, "Error: no domain type assigned to constituent\n");
		}
		if (ctxt->constituent[c].start_link == NULL)
		{
			err_ctxt ec;
			ec.sent = linkage->sent;
			err_msg(&ec, Error, "Error: no type assigned to constituent\n");
		}
	}

	numcon_subl = c - numcon_total;
	/* numcon_subl = handle_islands(linkage, numcon_total, numcon_subl);  */

	if (verbosity >= 2)
		printf("Constituents added at first stage for subl %d:\n",
			   linkage->current);
	for (c = numcon_total; c < numcon_total + numcon_subl; c++)
	{
		print_constituent(ctxt, linkage, c);
	}

	/* Opener case - generates S around main clause.
	   (This must be done first; the S generated will be needed for
	   later cases.) */
	numcon_subl =
		gen_comp(ctxt, linkage, numcon_total, numcon_subl, "SBAR", "S", "S", CASE_OPENER);

	/* pp opener case */
	numcon_subl =
		gen_comp(ctxt, linkage, numcon_total, numcon_subl, "PP", "S", "S", CASE_PPOPEN);

	/* participle opener case */
	numcon_subl =
		gen_comp(ctxt, linkage, numcon_total, numcon_subl, "S", "S", "S", CASE_PART_OPEN);

	/* Subject-phrase case; every main VP generates an S */
	numcon_subl =
		gen_comp(ctxt, linkage, numcon_total, numcon_subl, "VP", "S", "NP", CASE_S);

	/* Relative clause case; an SBAR generates a complement NP */
	numcon_subl =
		gen_comp(ctxt, linkage, numcon_total, numcon_subl, "SBAR", "NP", "NP", CASE_REL_CLAUSE);

	/* Participle modifier case */
	numcon_subl =
		gen_comp(ctxt, linkage, numcon_total, numcon_subl, "VP", "NP", "NP", CASE_PART_MOD);

	/* PP modifying NP */
	numcon_subl =
		gen_comp(ctxt, linkage, numcon_total, numcon_subl, "PP", "NP", "NP", CASE_PART_MOD);

	/* Appositive case */
	numcon_subl =
		gen_comp(ctxt, linkage, numcon_total, numcon_subl, "NP", "NP", "NP", CASE_APPOS);

	/* S-V inversion case; an NP generates a complement VP */
	numcon_subl =
		gen_comp(ctxt, linkage, numcon_total, numcon_subl, "NP", "SINV", "VP", CASE_SVINV);

	adjust_subordinate_clauses(ctxt, linkage, numcon_total, numcon_subl);
	for (c = numcon_total; c < numcon_total + numcon_subl; c++)
	{
		if ((ctxt->constituent[c].domain_type=='p') &&
			(strcmp(linkage->word[ctxt->constituent[c].left], ",")==0))
		{
			ctxt->constituent[c].left++;
		}
	}

	/* Make sure the constituents are nested. If two constituents
	 * are not nested: whichever constituent has the furthest left
	 * boundary, shift that boundary rightwards to the left boundary
	 * of the other one.
	 */
	while (1)
	{
		adjustment_made = 0;
		for (c = numcon_total; c < numcon_total + numcon_subl; c++)
		{
			for (c2 = numcon_total; c2 < numcon_total + numcon_subl; c2++)
			{
				if ((ctxt->constituent[c].left < ctxt->constituent[c2].left) &&
					(ctxt->constituent[c].right < ctxt->constituent[c2].right) &&
					(ctxt->constituent[c].right >= ctxt->constituent[c2].left))
				{
					/* We've found two overlapping constituents.
					   If one is larger, except the smaller one
					   includes an extra comma, adjust the smaller one
					   to exclude the comma */

					if ((strcmp(linkage->word[ctxt->constituent[c2].right], ",") == 0) ||
						(strcmp(linkage->word[ctxt->constituent[c2].right],
								"RIGHT-WALL") == 0))
					{
						if (verbosity >= 2)
							printf("Adjusting %d to fix comma overlap\n", c2);
						adjust_for_right_comma(ctxt, linkage, c2);
						adjustment_made = 1;
					}
					else if (strcmp(linkage->word[ctxt->constituent[c].left], ",") == 0)
					{
						if (verbosity >= 2)
							printf("Adjusting c %d to fix comma overlap\n", c);
						adjust_for_left_comma(ctxt, linkage, c);
						adjustment_made = 1;
					}
					else
					{
						if (verbosity >= 2)
						{
							err_ctxt ec;
							ec.sent = linkage->sent;
							err_msg(&ec, Warn, 
							      "Warning: the constituents aren't nested! "
							      "Adjusting them. (%d, %d)\n", c, c2);
					  }
					  ctxt->constituent[c].left = ctxt->constituent[c2].left;
					}
				}
			}
		}
		if (adjustment_made == 0) break;
	}

#ifdef AUX_CODE_IS_DEAD 
/* The code here is ifdef-dead as it appears to be dead, as the computation it does
 * is immediately undone in the very next block.
 */
	/* This labels certain words as auxiliaries (such as forms of "be"
	 * with passives, forms of "have" wth past participles,
	 * "to" with infinitives). These words start VP's which include
	 * them. In Treebank I, these don't get printed unless they're part of an
	 * andlist, in which case they get labeled "X". (this is why we need to
	 * label them as "aux".) In Treebank II, however, they seem to be treated
	 * just like other verbs, so the "aux" stuff isn't needed.
	 */
	for (c = numcon_total; c < numcon_total + numcon_subl; c++)
	{
		ctxt->constituent[c].subl = linkage->current;
		if (((ctxt->constituent[c].domain_type == 'v') &&
			(ctxt->wordtype[linkage_get_link_rword(linkage,
											 ctxt->constituent[c].start_num)] == PTYPE))
		   ||
		   ((ctxt->constituent[c].domain_type == 't') &&
			(strcmp(ctxt->constituent[c].type, "VP") == 0)))
		{
			ctxt->constituent[c].aux = 1;
		}
		else
		{
			ctxt->constituent[c].aux = 0;
		}
	}
#endif /* AUX_CODE_IS_DEAD */

	if (MAXCONSTITUENTS <= numcon_total + numcon_subl)
	{
		err_ctxt ec;
		ec.sent = linkage->sent;
		err_msg(&ec, Error, "Error: Too many constituents (a2).\n");
		numcon_total = MAXCONSTITUENTS - numcon_subl;
	}
	for (c = numcon_total; c < numcon_total + numcon_subl; c++)
	{
		ctxt->constituent[c].subl = linkage->current;
#ifdef AUX_CODE_IS_DEAD /* See comments at top */
		ctxt->constituent[c].aux = 0;
#endif /* AUX_CODE_IS_DEAD */
	}

	return numcon_subl;
}

static char * exprint_constituent_structure(con_context_t *ctxt, Linkage linkage, int numcon_total)
{
	int have_opened = 1;
	int c, w;
	int leftdone[MAXCONSTITUENTS];
	int rightdone[MAXCONSTITUENTS];
	int best, bestright, bestleft;
	Sentence sent;
	char s[100], * p;
	String * cs = string_new();

	assert (numcon_total < MAXCONSTITUENTS, "Too many constituents (b)");
	sent = linkage_get_sentence(linkage);

	for (c = 0; c < numcon_total; c++)
	{
		leftdone[c] = 0;
		rightdone[c] = 0;
	}

	if (verbosity >= 2)
		printf("\n");			

	for (w = 1; w < linkage->num_words; w++)
	{
		/* Skip left wall; don't skip right wall, since it may
		   have constituent boundaries */

		while(1)
		{
			best = -1;
			bestright = -1;
			for (c = 0; c < numcon_total; c++)
			{
				if ((ctxt->constituent[c].left == w) &&
					(leftdone[c] == 0) && (ctxt->constituent[c].valid == 1) &&
					(ctxt->constituent[c].right >= bestright)) {
					best = c;
					bestright = ctxt->constituent[c].right;
				}
			}
			if (best == -1)
				break;

			leftdone[best] = 1;
			/* have_open is a hack to avoid printing anything until
			 * bracket is opened */
			if (w == 1) have_opened = 0;
#ifdef AUX_CODE_IS_DEAD /* See comments at top */
			if (ctxt->constituent[best].aux == 1) continue;
#endif /* AUX_CODE_IS_DEAD */
			have_opened = 1;
			append_string(cs, "%c%s ", OPEN_BRACKET, ctxt->constituent[best].type);
		}

		/* Don't print out right wall */
		if (have_opened && (w < linkage->num_words - 1))
		{
			char *p;
			strcpy(s, sent->word[w].string);

			/* Constituent processing will crash if the sentence contains
			 * square brackets, so we have to do something ... replace
			 * them with curly braces ... will have to do.
			 */
			p = strchr(s, OPEN_BRACKET);
			while(p)
			{
				*p = '{';
				p = strchr(p, OPEN_BRACKET);
			}

			p = strchr(s, CLOSE_BRACKET);
			while(p)
			{
				*p = '}';
				p = strchr(p, CLOSE_BRACKET);
			}

			/* Now, if the first character of the word was
			   originally uppercase, we put it back that way */
			if (sent->word[w].firstupper == 1)
				upcase_utf8_str(s, s, MAX_WORD);
			append_string(cs, "%s ", s);
		}

		while(1)
		{
			best = -1;
			bestleft = -1;
			for(c = 0; c < numcon_total; c++)
			{
				if ((ctxt->constituent[c].right == w) &&
					(rightdone[c] == 0) && (ctxt->constituent[c].valid == 1) &&
					(ctxt->constituent[c].left > bestleft)) {
					best = c;
					bestleft = ctxt->constituent[c].left;
				}
			}
			if (best == -1)
				break;
			rightdone[best] = 1;
#ifdef AUX_CODE_IS_DEAD /* See comments at top */
			if (ctxt->constituent[best].aux == 1)
				continue;
#endif /* AUX_CODE_IS_DEAD */
			append_string(cs, "%s%c ", ctxt->constituent[best].type, CLOSE_BRACKET);
		}
	}

	append_string(cs, "\n");
	p = string_copy(cs);
	string_delete(cs);
	return p;
}

static char * do_print_flat_constituents(con_context_t *ctxt, Linkage linkage)
{
	Postprocessor * pp;
	int s, numcon_total, numcon_subl, num_subl;
	char * q;

	ctxt->phrase_ss = string_set_create();
	pp = linkage->sent->dict->constituent_pp;
	numcon_total = 0;

	count_words_used(ctxt, linkage);

	/* 1 < num_sublinkages only if a parse used fat links. */
	/* A lot of the complixity here could go away once we
	 * eliminate fat links for good ...  */
	num_subl = linkage->num_sublinkages;
	if (num_subl > MAXSUBL)
	{
	  num_subl = MAXSUBL;
	  if (verbosity >= 2)
		printf("Number of sublinkages exceeds maximum: only considering first %d sublinkages\n", MAXSUBL);
	}

	if (linkage->unionized == 1 && num_subl > 1) num_subl--;
	for (s = 0; s < num_subl; s++)
	{
		linkage_set_current_sublinkage(linkage, s);
		linkage_post_process(linkage, pp);
		generate_misc_word_info(ctxt, linkage);
		numcon_subl = read_constituents_from_domains(ctxt, linkage, numcon_total, s);
		numcon_total = numcon_total + numcon_subl;
		if (MAXCONSTITUENTS <= numcon_total)
		{
			err_ctxt ec;
			ec.sent = linkage->sent;
			err_msg(&ec, Error, "Error: Too many constituents (c).\n");
			numcon_total = MAXCONSTITUENTS-1;
			break;
		}
	}
	numcon_total = merge_constituents(ctxt, linkage, numcon_total);
	if (MAXCONSTITUENTS <= numcon_total)
	{
		err_ctxt ec;
		ec.sent = linkage->sent;
		err_msg(&ec, Error, "Error: Too many constituents (d).\n");
		numcon_total = MAXCONSTITUENTS-1;
	}
	numcon_total = new_style_conjunctions(ctxt, linkage, numcon_total);
	if (MAXCONSTITUENTS <= numcon_total)
	{
		err_ctxt ec;
		ec.sent = linkage->sent;
		err_msg(&ec, Error, "Error: Too many constituents (n).\n");
		numcon_total = MAXCONSTITUENTS-1;
	}
	numcon_total = last_minute_fixes(ctxt, linkage, numcon_total);
	if (MAXCONSTITUENTS <= numcon_total)
	{
		err_ctxt ec;
		ec.sent = linkage->sent;
		err_msg(&ec, Error, "Error: Too many constituents (e).\n");
		numcon_total = MAXCONSTITUENTS-1;
	}
	q = exprint_constituent_structure(ctxt, linkage, numcon_total);
	string_set_delete(ctxt->phrase_ss);
	ctxt->phrase_ss = NULL;
	return q;
}

static char * print_flat_constituents(Linkage linkage)
{
	/* In principle, the ctxt could be allocated on stack, instead of
	 * with malloc(). However, The java6 jvm (and MS Windows jvm's)
	 * gives JNI clients only a small amount of stack space. Alloc'ing
	 * this (rather large) structure  on stack will blow up the JVM.
	 * This was discovered only after much work. Bummer.
	 */
	char * p;
	con_context_t *ctxt = (con_context_t *) malloc (sizeof(con_context_t));
	memset(ctxt, 0, sizeof(con_context_t));
	p = do_print_flat_constituents(ctxt, linkage);
	free(ctxt);
	return p;
}

static CType token_type (char *token)
{
	if ((token[0] == OPEN_BRACKET) && (strlen(token) > 1))
		return OPEN_TOK;
	if ((strlen(token) > 1) && (token[strlen(token) - 1] == CLOSE_BRACKET))
		return CLOSE_TOK;
	return WORD_TOK;
}

static CNode * make_CNode(char *q)
{
	CNode * cn;
	cn = (CNode *) exalloc(sizeof(CNode));
	cn->label = (char *) exalloc(sizeof(char)*(strlen(q)+1));
	strcpy(cn->label, q);
	cn->child = cn->next = (CNode *) NULL;
	cn->next = (CNode *) NULL;
	cn->start = cn->end = -1;
	return cn;
}

static CNode * parse_string(CNode * n, char **saveptr)
{
	char *q;
	CNode *m, *last_child=NULL;

	while ((q = strtok_r(NULL, " ", saveptr))) {
		switch (token_type(q)) {
		case CLOSE_TOK :
			q[strlen(q)-1]='\0';
			assert(strcmp(q, n->label)==0,
				   "Constituent tree: Labels do not match.");
			return n;
			break;
		case OPEN_TOK:
			m = make_CNode(q+1);
			m = parse_string(m, saveptr);
			break;
		case WORD_TOK:
			m = make_CNode(q);
			break;
		default:
			assert(0, "Constituent tree: Illegal token type");
		}
		if (n->child == NULL) {
			last_child = n->child = m;
		}
		else {
			last_child->next = m;
			last_child = m;
		}
	}
	assert(0, "Constituent tree: Constituent did not close");
	return NULL;
}

static void print_tree(String * cs, int indent, CNode * n, int o1, int o2)
{
	int i, child_offset;
	CNode * m;

	if (n == NULL) return;

	if (indent)
		for (i = 0; i < o1; ++i)
			append_string(cs, " ");
	append_string(cs, "(%s ", n->label);
	child_offset = o2 + strlen(n->label) + 2;

	for (m = n->child; m != NULL; m = m->next)
	{
		if (m->child == NULL)
		{
			char * p;
			/* If the original string has left or right parens in it,
			 * the printed string will be messed up by these ...
			 * so replace them by curly braces. What else can one do?
			 */
			p = strchr(m->label, '(');
			while(p)
			{
				*p = '{';
				p = strchr(p, '(');
			}

			p = strchr(m->label, ')');
			while(p)
			{
				*p = '}';
				p = strchr(p, ')');
			}

			append_string(cs, "%s", m->label);
			if ((m->next != NULL) && (m->next->child == NULL))
				append_string(cs, " ");
		}
		else
		{
			if (m != n->child)
			{
				if (indent) append_string(cs, "\n");
				else append_string(cs, " ");
				print_tree(cs, indent, m, child_offset, child_offset);
			}
			else
			{
				print_tree(cs, indent, m, 0, child_offset);
			}
			if ((m->next != NULL) && (m->next->child == NULL))
			{
				if (indent)
				{
					append_string(cs, "\n");
					for (i = 0; i < child_offset; ++i)
						append_string(cs, " ");
				}
				else append_string(cs, " ");
			}
		}
	}
	append_string(cs, ")");
}

static int assign_spans(CNode * n, int start)
{
	int num_words=0;
	CNode * m=NULL;
	if (n==NULL) return 0;
	n->start = start;
	if (n->child == NULL) {
		n->end = start;
		return 1;
	}
	else {
		for (m=n->child; m!=NULL; m=m->next) {
			num_words += assign_spans(m, start+num_words);
		}
		n->end = start+num_words-1;
	}
	return num_words;
}

CNode * linkage_constituent_tree(Linkage linkage)
{
	char *p, *q, *saveptr;
	int len;
	CNode * root;

	p = print_flat_constituents(linkage);

	len = strlen(p);
	q = strtok_r(p, " ", &saveptr);
	assert(token_type(q) == OPEN_TOK, "Illegal beginning of string");
	root = make_CNode(q+1);
	root = parse_string(root, &saveptr);
	assign_spans(root, 0);
	exfree(p, sizeof(char)*(len+1));
	return root;
}

void linkage_free_constituent_tree(CNode * n)
{
	CNode *m, *x;
	for (m=n->child; m!=NULL; m=x) {
		x=m->next;
		linkage_free_constituent_tree(m);
	}
	exfree(n->label, sizeof(char)*(strlen(n->label)+1));
	exfree(n, sizeof(CNode));
}

/**
 * Print out the constituent tree.
 * mode 1: treebank-style constituent tree
 * mode 2: flat, bracketed tree [A like [B this B] A]
 * mode 3: flat, treebank-style tree (A like (B this) )
 */
char * linkage_print_constituent_tree(Linkage linkage, ConstituentDisplayStyle mode)
{
	String * cs;
	CNode * root;
	char * p;

	if ((mode == NO_DISPLAY) || (linkage->sent->dict->constituent_pp == NULL))
	{
		return NULL;
	}
	else if (mode == MULTILINE || mode == SINGLE_LINE)
	{
		cs = string_new();
		root = linkage_constituent_tree(linkage);
		print_tree(cs, (mode==1), root, 0, 0);
		linkage_free_constituent_tree(root);
		append_string(cs, "\n");
		p = string_copy(cs);
		string_delete(cs);
		return p;
	}
	else if (mode == BRACKET_TREE)
	{
		return print_flat_constituents(linkage);
	}
	assert(0, "Illegal mode in linkage_print_constituent_tree");
	return NULL;
}

void linkage_free_constituent_tree_str(char * s)
{
	exfree(s, strlen(s)+1);
}

const char * linkage_constituent_node_get_label(const CNode *n)
{
	return n->label;
}


CNode * linkage_constituent_node_get_child(const CNode *n)
{
	return n->child;
}

CNode * linkage_constituent_node_get_next(const CNode *n)
{
	return n->next;
}

int linkage_constituent_node_get_start(const CNode *n)
{
	return n->start;
}

int linkage_constituent_node_get_end(const CNode *n)
{
	return n->end;
}
