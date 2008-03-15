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
#include <link-grammar/api.h>
#include "constituents.h"

#define MAXCONSTITUENTS 1024
#define MAXSUBL 16
#define OPEN_BRACKET '['
#define CLOSE_BRACKET ']'

typedef enum {OPEN, CLOSE, WORD} CType;
typedef enum {NONE, STYPE, PTYPE, QTYPE, QDTYPE} WType;
static String_set * phrase_ss;

struct {
  int left;
  int right;
  const char * type;
  char domain_type;
  const char * start_link;
  int start_num;
  int subl;
  int canon;
  int valid;
  int aux;	
  /* 0: it's an ordinary VP (or other type);
	 1: it's an AUX, don't print it;
	 2: it's an AUX, and print it */
} constituent[MAXCONSTITUENTS];

/*PV* made global vars static */
static WType wordtype[MAX_SENTENCE];
static int word_used[MAXSUBL][MAX_SENTENCE];
static int templist[100];
static struct {
  int num;
  int e[10];
  int valid;
} andlist[1024];

/* forward declarations */
static void print_constituent(Linkage linkage, int c);
static void adjust_for_left_comma(Linkage linkage, int c);
static void adjust_for_right_comma(Linkage linkage, int c);
static int  find_next_element(Linkage linkage,
							  int start,
							  int numcon_total,
							  int num_elements,
							  int num_lists);

/******************************************************
 *        These functions do the bulk of the actual
 * constituent-generating; they're called once for each
 *                      sublinkage
 *********************************************************/

static inline int uppercompare(const char * s, const char * t)
{
	return (FALSE == utf8_upper_match(s,t));
}

static int gen_comp(Linkage linkage, int numcon_total, int numcon_subl,
					const char * ctype1, const char * ctype2, const char * ctype3, int x) {

	/* This function looks for constituents of type ctype1. Say it finds
	   one, call it c1. It searches for the next larger constituent of
	   type ctype2, call it c2. It then generates a new constituent of
	   ctype3, containing all the words in c2 but not c1. */

	int w, w2, w3, c, c1, c2, done;
	c = numcon_total + numcon_subl;

	for (c1=numcon_total; c1<numcon_total + numcon_subl; c1++) {

		/* If ctype1 is NP, it has to be an appositive to continue */
		if ((x==4) && (post_process_match("MX#*", constituent[c1].start_link)==0))
			continue;

		/* If ctype1 is X, and domain_type is t, it's an infinitive - skip it */
		if ((x==2) && (constituent[c1].domain_type=='t'))
			continue;

		/* If it's domain-type z, it's a subject-relative clause;
		   the VP doesn't need an NP */
		if (constituent[c1].domain_type=='z')
			continue;

		/* If ctype1 is X or VP, and it's not started by an S, don't generate an NP
		 (Neither of the two previous checks are necessary now, right?) */
		if ((x==1 || x==2) &&
			(((post_process_match("S", constituent[c1].start_link)==0) &&
			  (post_process_match("SX", constituent[c1].start_link)==0) &&
			  (post_process_match("SF", constituent[c1].start_link)==0)) ||
			 (post_process_match("S##w", constituent[c1].start_link)!=0)))
			continue;

		/* If it's an SBAR (relative clause case), it has to be a relative clause */
		if ((x==3) &&
			((post_process_match("Rn", constituent[c1].start_link)==0) &&
			 (post_process_match("R*", constituent[c1].start_link)==0) &&
			 (post_process_match("MX#r", constituent[c1].start_link)==0) &&
			 (post_process_match("Mr", constituent[c1].start_link)==0) &&
			 (post_process_match("MX#d", constituent[c1].start_link)==0)))
			continue;

		/* If ctype1 is SBAR (clause opener case), it has to be an f domain */
		if ((x==5) && (constituent[c1].domain_type!='f'))
			continue;

		/* If ctype1 is SBAR (pp opener case), it has to be a g domain */
		if ((x==6) && (constituent[c1].domain_type!='g'))
			continue;

		/* If ctype1 is NP (paraphrase case), it has to be started by an SI */
		if ((x==7) && (post_process_match("SI", constituent[c1].start_link)==0))
			continue;

		/* If ctype1 is VP (participle modifier case), it has to be
		   started by an Mv or Mg */
		if ((x==8) && (post_process_match("M", constituent[c1].start_link)==0))
			continue;

		/* If ctype1 is VP (participle opener case), it has
		   to be started by a COp */
		if ((x==9) && (post_process_match("COp", constituent[c1].start_link)==0))
			continue;

		/* Now start at the bounds of c1, and work outwards until you
		   find a larger constituent of type ctype2 */
		if (!(strcmp(constituent[c1].type, ctype1)==0))
			continue;

		if (verbosity>=2)
			printf("Generating complement constituent for c %d of type %s\n",
				   c1, ctype1);
		done=0;
		for (w2=constituent[c1].left; (done==0) && (w2>=0); w2--) {
			for (w3=constituent[c1].right; w3<linkage->num_words; w3++) {
				for (c2=numcon_total; (done==0) &&
						 (c2 < numcon_total + numcon_subl); c2++) {
					if (!((constituent[c2].left==w2) &&
						  (constituent[c2].right==w3)) || (c2==c1))
						continue;
					if (!(strcmp(constituent[c2].type, ctype2)==0))
						continue;

					/* if the new constituent (c) is to the left
					   of c1, its right edge should be adjacent to the
					   left edge of c1 - or as close as possible
					   without going outside the current sublinkage.
					   (Or substituting right and left as necessary.) */

					if ((x==5) || (x==6) || (x==9)) {
								/* This is the case where c is to the
								   RIGHT of c1 */
						w=constituent[c1].right+1;
						while(1) {
							if (word_used[linkage->current][w]==1)
								break;
							w++;
						}
						if (w>constituent[c2].right) {
							done=1;
							continue;
						}
						constituent[c].left=w;
						constituent[c].right=constituent[c2].right;
					}
					else {
						w=constituent[c1].left-1;
						while(1) {
							if (word_used[linkage->current][w]==1)
								break;
							w--;
						}
						if (w<constituent[c2].left) {
							done=1;
							continue;
						}
						constituent[c].right=w;
						constituent[c].left=constituent[c2].left;
					}

					adjust_for_left_comma(linkage, c1);
					adjust_for_right_comma(linkage, c1);

					constituent[c].type =
						string_set_add(ctype3, phrase_ss);
					constituent[c].domain_type = 'x';
					constituent[c].start_link =
						string_set_add("XX", phrase_ss);
					constituent[c].start_num =
						constituent[c1].start_num; /* bogus */
					if (verbosity>=2) {
						printf("Larger c found: c %d (%s); ",
							   c2, ctype2);
						printf("Adding constituent:\n");
						print_constituent(linkage, c);
					}
					c++;
					assert(c < MAXCONSTITUENTS, "Too many constituents");
					done=1;
				}
			}
		}
		if (verbosity>=2) {
		  if (done==0)
			printf("No constituent added, because no larger %s " \
				   " was found\n", ctype2);
		}
	}
	numcon_subl = c - numcon_total;
	return numcon_subl;
}

static void adjust_subordinate_clauses(Linkage linkage,
									   int numcon_total,
									   int numcon_subl) {

  /* Look for a constituent started by an MVs or MVg.
	 Find any VP's or ADJP's that contain it (without going
	 beyond a larger S or NP). Adjust them so that
	 they end right before the m domain starts. */

	int c, w, c2, w2, done;

	for (c=numcon_total; c<numcon_total + numcon_subl; c++) {
		if ((post_process_match("MVs", constituent[c].start_link)==1) ||
			(post_process_match("MVg", constituent[c].start_link)==1)) {
			done=0;
			for (w2=constituent[c].left-1; (done==0) && w2>=0; w2--) {
				for (c2=numcon_total; c2<numcon_total + numcon_subl; c2++) {
					if (!((constituent[c2].left==w2) &&
						  (constituent[c2].right >= constituent[c].right)))
						continue;
					if ((strcmp(constituent[c2].type, "S")==0) ||
						(strcmp(constituent[c2].type, "NP")==0)) {
						done=1;
						break;
					}
					if ((constituent[c2].domain_type=='v') ||
						(constituent[c2].domain_type=='a')) {
						w = constituent[c].left-1;
						while (1) {
							if (word_used[linkage->current][w]==1) break;
							w--;
						}
						constituent[c2].right = w;

						if (verbosity>=2)
							printf("Adjusting constituent %d:\n", c2);
						print_constituent(linkage, c2);
					}
				}
			}
			if (strcmp(linkage->word[constituent[c].left], ",")==0)
				constituent[c].left++;
		}	
	}
}

static void print_constituent(Linkage linkage, int c) {
	int w;
	/* Sentence sent;
	   sent = linkage_get_sentence(linkage); **PV* using linkage->word not sent->word */
	if (verbosity<2) return;
	printf("  c %2d %4s [%c] (%2d-%2d): ",
		   c, constituent[c].type, constituent[c].domain_type,
		   constituent[c].left, constituent[c].right);
	for (w=constituent[c].left; w<=constituent[c].right; w++) {
		printf("%s ", linkage->word[w]); /**PV**/
	}
	printf("\n");
}

static void adjust_for_left_comma(Linkage linkage, int c) {

	/* If a constituent c has a comma at either end, we exclude the
	   comma. (We continue to shift the boundary until we get to
	   something inside the current sublinkage) */
	int w;

	w=constituent[c].left;
	if (strcmp(linkage->word[constituent[c].left], ",")==0) {
		w++;
		while (1) {
			if (word_used[linkage->current][w]==1) break;
			w++;
		}
	}
	constituent[c].left = w;
}

static void adjust_for_right_comma(Linkage linkage, int c) {

	int w;
	w=constituent[c].right;
	if ((strcmp(linkage->word[constituent[c].right], ",")==0) ||
		(strcmp(linkage->word[constituent[c].right], "RIGHT-WALL")==0)) {
		w--;
		while (1) {
			if (word_used[linkage->current][w]==1) break;
			w--;
		}
	}
	constituent[c].right = w;
}

/**********************************************************/
/* These functions are called once, after constituents	*/
/* for each sublinkage have been generated, to merge them */
/*		together and fix up some other things		   */
/*														*/
/**********************************************************/

static int merge_constituents(Linkage linkage, int numcon_total) {

	int c1, c2=0, c3, ok, a, n, a2, n2, match, listmatch, a3;
	int num_lists, num_elements;
	int leftend, rightend;


	for (c1=0; c1<numcon_total; c1++) {
		constituent[c1].valid = 1;
		/* Find and invalidate any constituents with negative length */
		if(constituent[c1].right<constituent[c1].left) {
		  if(verbosity>=2) printf("WARNING: Constituent %d has negative length. Deleting it.\n", c1);
		  constituent[c1].valid=0;
		}
		constituent[c1].canon=c1;
	}

	/* First go through and give each constituent a canonical number
	   (the index number of the lowest-numbered constituent
	   identical to it) */

	for (c1=0; c1<numcon_total; c1++) {
		if (constituent[c1].canon!=c1) continue;
		for (c2=c1+1; c2<numcon_total; c2++) {
			if ((constituent[c1].left == constituent[c2].left) &&
				(constituent[c1].right == constituent[c2].right) &&
				(strcmp(constituent[c1].type, constituent[c2].type)==0)) {
				constituent[c2].canon=c1;
			}
		}
	}

	/* If constituents A and B in different sublinkages X and Y
	   have one endpoint in common, but A is larger at the other end,
	   and B has no duplicate in X, then declare B invalid. (Example:
	   " [A [B We saw the cat B] and the dog A] " */

	for (c1=0; c1<numcon_total; c1++) {
		if (constituent[c1].valid==0) continue;
		for (c2=0; c2<numcon_total; c2++) {
			if (constituent[c2].subl == constituent[c1].subl) continue;
			ok=1;
		/* Does c2 have a duplicate in the sublinkage containing c1?
		   If so, bag it */
			for (c3=0; c3<numcon_total; c3++) {
				if ((constituent[c2].canon == constituent[c3].canon) &&
					(constituent[c3].subl == constituent[c1].subl))
					ok=0;
			}
			for (c3=0; c3<numcon_total; c3++) {
				if ((constituent[c1].canon == constituent[c3].canon) &&
					(constituent[c3].subl == constituent[c2].subl))
					ok=0;
			}
			if (ok==0) continue;
			if ((constituent[c1].left == constituent[c2].left) &&
				(constituent[c1].right > constituent[c2].right) &&
				(strcmp(constituent[c1].type, constituent[c2].type)==0)) {
				constituent[c2].valid=0;
			}

			if ((constituent[c1].left < constituent[c2].left) &&
				(constituent[c1].right == constituent[c2].right) &&
				(strcmp(constituent[c1].type, constituent[c2].type)==0)) {
				constituent[c2].valid=0;
			}
		}
	}

	/* Now go through and find duplicates; if a pair is found,
	   mark one as invalid. (It doesn't matter if they're in the
	   same sublinkage or not) */

	for (c1=0; c1<numcon_total; c1++) {
		if (constituent[c1].valid==0) continue;
		for (c2=c1+1; c2<numcon_total; c2++) {
			if (constituent[c2].canon == constituent[c1].canon)
				constituent[c2].valid=0;
		}
	}

	/* Now we generate the and-lists. An and-list is a set of mutually
	   exclusive constituents. Each constituent in the list may not
	   be present in the same sublinkage as any of the others. */

	num_lists=0;
	for (c1=0; c1<numcon_total; c1++) {
		if (constituent[c1].valid==0) continue;
		num_elements=1;
		templist[0]=c1;
		num_lists =
			find_next_element(linkage, c1, numcon_total,
							  num_elements, num_lists);
	}

	if (verbosity>=2) {
		printf("And-lists:\n");
		for (n=0; n<num_lists; n++) {
			printf("  %d: ", n);
			for (a=0; a<andlist[n].num; a++) {
				printf("%d ", andlist[n].e[a]);
			}
			printf("\n");
		}
	}

	/* Now we prune out any andlists that are subsumed by other
	   andlists--e.g. if andlist X contains constituents A and B,
	   and Y contains A B and C, we throw out X */

	for (n=0; n<num_lists; n++) {
		andlist[n].valid=1;
		for (n2=0; n2<num_lists; n2++) {
			if (n2==n) continue;
			if (andlist[n2].num < andlist[n].num)
				continue;
			listmatch=1;
			for (a=0; a<andlist[n].num; a++) {
				match=0;
				for (a2=0; a2<andlist[n2].num; a2++) {
					if (andlist[n2].e[a2]==andlist[n].e[a])
						match=1;
				}
				if (match==0) listmatch=0;
				/* At least one element was not matched by n2 */
			}
			if (listmatch==1) andlist[n].valid=0;
		}
	}

	/* If an element of an andlist contains an element of another
	   andlist, it must contain the entire andlist. */

	for (n=0; n<num_lists; n++) {
		if (andlist[n].valid==0)
			continue;
		for (a=0; (a<andlist[n].num) && (andlist[n].valid); a++) {
			for (n2=0; (n2<num_lists) && (andlist[n].valid); n2++) {
				if ((n2==n) || (andlist[n2].valid==0))
					continue;
				for (a2=0; (a2<andlist[n2].num) && (andlist[n].valid); a2++) {
					c1=andlist[n].e[a];
					c2=andlist[n2].e[a2];
					if (c1==c2)
						continue;
					if (!((constituent[c2].left<=constituent[c1].left) &&
						  (constituent[c2].right>=constituent[c1].right)))
						continue;
					if (verbosity>=2)
						printf("Found that c%d in list %d is bigger " \
							   "than c%d in list %d\n", c2, n2, c1, n);
					ok=1;

					/* An element of n2 contains an element of n.
					   Now, we check to see if that element of n2
					   contains ALL the elements of n.
					   If not, n is invalid. */

					for (a3=0; a3<andlist[n].num; a3++) {
						c3=andlist[n].e[a3];
						if ((constituent[c2].left>constituent[c3].left) ||
							(constituent[c2].right<constituent[c3].right))
							ok=0;
					}
					if (ok != 0)
						continue;
					andlist[n].valid=0;
					if (verbosity>=2) {
						printf("Eliminating andlist, " \
							   "n=%d, a=%d, n2=%d, a2=%d: ",
							   n, a, n2, a2);
						for (a3=0; a3<andlist[n].num; a3++) {
							printf("%d ", andlist[n].e[a3]);
						}
						printf("\n");
					}
				}
			}
		}
	}


	if (verbosity>=2) {
		printf("And-lists after pruning:\n");
		for (n=0; n<num_lists; n++) {
			if (andlist[n].valid==0)
				continue;
			printf("  %d: ", n);
			for (a=0; a<andlist[n].num; a++) {
				printf("%d ", andlist[n].e[a]);
			}
			printf("\n");
		}
	}

	c1=numcon_total;
	for (n=0; n<num_lists; n++) {
		if (andlist[n].valid==0) continue;
		leftend=256;
		rightend=-1;
		for (a=0; a<andlist[n].num; a++) {
			c2=andlist[n].e[a];
			if (constituent[c2].left<leftend) {
				leftend=constituent[c2].left;
			}
			if (constituent[c2].right>rightend) {
				rightend=constituent[c2].right;
			}
		}

		constituent[c1].left=leftend;
		constituent[c1].right=rightend;
		constituent[c1].type = constituent[c2].type;
		constituent[c1].domain_type = 'x';
		constituent[c1].valid=1;
		constituent[c1].start_link = constituent[c2].start_link;  /* bogus */
		constituent[c1].start_num = constituent[c2].start_num;	/* bogus */

		/* If a constituent within the andlist is an aux (aux==1),
		   set aux for the whole-list constituent to 2, also set
		   aux for the smaller constituent to 2, meaning they'll both
		   be printed (as an "X"). (If aux is 2 for the smaller
		   constituent going in, the same thing should be done,
		   though I doubt this ever happens.) */

		for (a=0; a<andlist[n].num; a++) {
			c2=andlist[n].e[a];
			if ((constituent[c2].aux==1) || (constituent[c2].aux==2)) {
				constituent[c1].aux=2;
				constituent[c2].aux=2;
			}
		}

		if (verbosity>=2)
			printf("Adding constituent:\n");
		print_constituent(linkage, c1);
		c1++;
	}
	numcon_total=c1;
	return numcon_total;
}

static int find_next_element(Linkage linkage,
							 int start,
							 int numcon_total,
							 int num_elements,
							 int num_lists) {
	/* Here we're looking for the next andlist element to add on
	   to a conjectural andlist, stored in the array templist.
	   We go through the constituents, starting at "start". */

	int c, a, ok, c2, c3, addedone=0, n;

	n = num_lists;
	for (c=start+1; c<numcon_total; c++) {
		if (constituent[c].valid==0)
			continue;
		if (strcmp(constituent[templist[0]].type, constituent[c].type)!=0)
			continue;
		ok=1;

		/* We're considering adding constituent c to the andlist.
		   If c is in the same sublinkage as one of the other andlist
		   elements, don't add it. If it overlaps with one of the other
		   constituents, don't add it. If there's a constituent
		   identical to c that occurs in a sublinkage in which one of
		   the other elements occurs, don't add it. */

		for (a=0; a<num_elements; a++) {
			if (constituent[c].subl==constituent[templist[a]].subl)
				ok=0;
			if (((constituent[c].left<constituent[templist[a]].left) &&
				 (constituent[c].right>constituent[templist[a]].left))
				||
				((constituent[c].right>constituent[templist[a]].right) &&
				 (constituent[c].left<constituent[templist[a]].right))
				||
				((constituent[c].right>constituent[templist[a]].right) &&
				 (constituent[c].left<constituent[templist[a]].right))
				||
				((constituent[c].left>constituent[templist[a]].left) &&
				 (constituent[c].right<constituent[templist[a]].right)))
				ok=0;
			for (c2=0; c2<numcon_total; c2++) {
				if (constituent[c2].canon != constituent[c].canon)
					continue;
				for (c3=0; c3<numcon_total; c3++) {
					if ((constituent[c3].canon==constituent[templist[a]].canon)
						&& (constituent[c3].subl==constituent[c2].subl))
						ok=0;
				}
			}
		}
		if (ok==0) continue;
		templist[num_elements]=c;
		addedone=1;
		num_lists = find_next_element(linkage, c, numcon_total,
									  num_elements+1, num_lists);
	}
	if (addedone==0 && num_elements>1) {
		for (a=0; a<num_elements; a++) {
			andlist[num_lists].e[a]=templist[a];
			andlist[num_lists].num=num_elements;
		}
		num_lists++;
	}
	return num_lists;
}

static void generate_misc_word_info(Linkage linkage) {

	/* Go through all the words. If a word is on the right end of
	   an S (or SF or SX), wordtype[w]=STYPE.  If it's also on the left end of a
	   Pg*b, I, PP, or Pv, wordtype[w]=PTYPE. If it's a question-word
	   used in an indirect question, wordtype[w]=QTYPE. If it's a
	   question-word determiner,  wordtype[w]=QDTYPE. Else wordtype[w]=NONE.
	   (This function is called once for each sublinkage.) */
	
	int l1, l2, w1, w2;
	const char * label1, * label2;

	for (w1=0; w1<linkage->num_words; w1++)
		wordtype[w1]=NONE;

	for (l1=0; l1<linkage_get_num_links(linkage); l1++) {	
		w1=linkage_get_link_rword(linkage, l1);
		label1 = linkage_get_link_label(linkage, l1);
		if ((uppercompare(label1, "S")==0) ||
			(uppercompare(label1, "SX")==0) ||
			(uppercompare(label1, "SF")==0)) {
			wordtype[w1] = STYPE;
			for (l2=0; l2<linkage_get_num_links(linkage); l2++) {
				w2=linkage_get_link_lword(linkage, l2);
				label2 = linkage_get_link_label(linkage, l2);
				if ((w1==w2) &&
					((post_process_match("Pg#b", label2)==1) ||
					 (uppercompare(label2, "I")==0) ||
					 (uppercompare(label2, "PP")==0) ||
					 (post_process_match("Pv", label2)==1))) {
					/* Pvf, Pgf? */
					wordtype[w1] = PTYPE;
				}
			}
		}
		if (post_process_match("QI#d", label1)==1) {
			wordtype[w1] = QTYPE;
			for (l2=0; l2<linkage_get_num_links(linkage); l2++) {
				w2=linkage_get_link_lword(linkage, l2);
				label2 = linkage_get_link_label(linkage, l2);
				if ((w1==w2) && (post_process_match("D##w", label2)==1)) {
					wordtype[w1] = QDTYPE;
				}
			}
		}
		if (post_process_match("Mr", label1)==1) wordtype[w1] = QDTYPE;
		if (post_process_match("MX#d", label1)==1) wordtype[w1] = QDTYPE;
	}
}

static int last_minute_fixes(Linkage linkage, int numcon_total) {

	int c, c2, global_leftend_found, adjustment_made,
		global_rightend_found, lastword, newcon_total=0;
	Sentence sent;
	sent = linkage_get_sentence(linkage);

	for (c=0; c<numcon_total; c++) {

		/* In a paraphrase construction ("John ran, he said"),
		   the paraphrasing clause doesn't get
		   an S. (This is true in Treebank II, not Treebank I) */

		if (uppercompare(constituent[c].start_link, "CP")==0) {
			constituent[c].valid = 0;
		}

		/* If it's a possessive with an "'s", the NP on the left
		   should be extended to include the "'s". */
		if ((uppercompare(constituent[c].start_link, "YS")==0) ||
			(uppercompare(constituent[c].start_link, "YP")==0)) {
			constituent[c].right++;
		}

		/* If a constituent has starting link MVpn, it's a time
		   expression like "last week"; label it as a noun phrase
		   (incorrectly) */

		if (strcmp(constituent[c].start_link, "MVpn")==0) {
			constituent[c].type = string_set_add("NP", phrase_ss);
		}
		if (strcmp(constituent[c].start_link, "COn")==0) {
			constituent[c].type = string_set_add("NP", phrase_ss);
		}
		if (strcmp(constituent[c].start_link, "Mpn")==0) {
			constituent[c].type = string_set_add("NP", phrase_ss);
		}

		/* If the constituent is an S started by "but" or "and" at
		   the beginning of the sentence, it should be ignored. */

		if ((strcmp(constituent[c].start_link, "Wdc")==0) &&
			(constituent[c].left==2)) {
			constituent[c].valid = 0;
		}

		/* For prenominal adjectives, an ADJP constituent is assigned
		   if it's a hyphenated (Ah) or comparative (Am) adjective;
		   otherwise no ADJP is assigned, unless the phrase is more
		   than one word long (e.g. "very big"). The same with certain
		   types of adverbs. */
		/* That was for Treebank I. For Treebank II, the rule only
		   seems to apply to prenominal adjectives (of all kinds).
		   However, it also applies to number expressions ("QP"). */

		if ((post_process_match("A", constituent[c].start_link)==1) ||
			(constituent[c].domain_type=='d') ||
			(constituent[c].domain_type=='h')) {
			if (constituent[c].right-constituent[c].left==0) {
				constituent[c].valid=0;
			}
		}

		if ((constituent[c].domain_type=='h') &&
			(strcmp(linkage->word[constituent[c].left-1], "$")==0)) {
			constituent[c].left--;
		}

		/* If a constituent has type VP and its aux value is 2,
		   this means it's an aux that should be printed; change its
		   type to "X". If its aux value is 1, set "valid" to 0. (This
		   applies to Treebank I only) */

		if (constituent[c].aux==2) {
			constituent[c].type = string_set_add("X", phrase_ss);
		}
		if (constituent[c].aux==1) {
			constituent[c].valid=0;
		}
	}

	numcon_total = numcon_total + newcon_total;

	/* If there's a global S constituent that includes everything
	   except a final period or question mark, extend it by one word */

	for (c=0; c<numcon_total; c++) {
		if ((constituent[c].right==(linkage->num_words)-3) &&
			(constituent[c].left==1) &&
			(strcmp(constituent[c].type, "S")==0) &&
			(strcmp(sent->word[(linkage->num_words)-2].string, ".")==0))
			constituent[c].right++;
	}

	/* If there's no S boundary at the very left end of the sentence,
	   or the very right end, create a new S spanning the entire sentence */

	lastword=(linkage->num_words)-2;
	global_leftend_found = 0;
	global_rightend_found = 0;
	for (c=0; c<numcon_total; c++) {
		if ((constituent[c].left==1) && (strcmp(constituent[c].type, "S")==0) &&
			(constituent[c].valid==1))
			global_leftend_found=1;
	}
	for (c=0; c<numcon_total; c++) {
		if ((constituent[c].right>=lastword) &&
			(strcmp(constituent[c].type, "S")==0) && (constituent[c].valid==1))
			global_rightend_found=1;
	}
	if ((global_leftend_found==0) || (global_rightend_found==0)) {
		c=numcon_total;
		constituent[c].left=1;
		constituent[c].right=linkage->num_words-1;
		constituent[c].type = string_set_add("S", phrase_ss);
		constituent[c].valid=1;
		constituent[c].domain_type = 'x';
		numcon_total++;
		if (verbosity>=2)
			printf("Adding global sentence constituent:\n");
		print_constituent(linkage, c);
	}

	/* Check once more to see if constituents are nested (checking BETWEEN sublinkages
	   this time) */

	while (1) {
		adjustment_made=0;
		for (c=0; c<numcon_total; c++) {
			if(constituent[c].valid==0) continue;
			for (c2=0; c2<numcon_total; c2++) {
				if(constituent[c2].valid==0) continue;
				if ((constituent[c].left < constituent[c2].left) &&
					(constituent[c].right < constituent[c2].right) &&
					(constituent[c].right >= constituent[c2].left)) {

					if (verbosity>=2) {
					  printf("WARNING: the constituents aren't nested! Adjusting them." \
							   "(%d, %d)\n", c, c2);
					  }
					constituent[c].left = constituent[c2].left;
				}
			}
		}
		if (adjustment_made==0) break;
	}
	return numcon_total;
}

static void count_words_used(Linkage linkage) {

	/*This function generates a table, word_used[i][w], showing
	  whether each word w is used in each sublinkage i; if so,
	  the value for that cell of the table is 1 */

	int i, w, link, num_subl;

	num_subl = linkage->num_sublinkages;
	if(linkage->unionized==1 && num_subl>1) num_subl--;

	if (verbosity>=2)
		printf("Number of sublinkages = %d\n", num_subl);
	for (i=0; i<num_subl; i++) {
		for (w=0; w<linkage->num_words; w++) word_used[i][w]=0;
		linkage->current=i;
		for (link=0; link<linkage_get_num_links(linkage); link++) {
			word_used[i][linkage_get_link_lword(linkage, link)]=1;
			word_used[i][linkage_get_link_rword(linkage, link)]=1;
		}
		if (verbosity>=2) {
			printf("Sublinkage %d: ", i);
			for (w=0; w<linkage->num_words; w++) {
				if (word_used[i][w]==0) printf("0 ");
				if (word_used[i][w]==1) printf("1 ");
			}
			printf("\n");
		}
	}
}

static int r_limit=0;

static void add_constituent(int * cons, Linkage linkage, Domain domain,
							int l, int r, const char * name) {
	int c = *cons;
	c++;

	/* Avoid running off end to walls **PV**/
	if( l<1 ) l=1;
	if( r>r_limit ) r=r_limit;
	assert( l<=r, "negative constituent length!" );

	constituent[c].left = l;
	constituent[c].right = r;
	constituent[c].domain_type = domain.type;
	constituent[c].start_link =
		linkage_get_link_label(linkage, domain.start_link);
	constituent[c].start_num = domain.start_link;
	constituent[c].type = string_set_add(name, phrase_ss);
	*cons = c;
}

static const char * cons_of_domain(char domain_type)
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
		printf("Illegal domain: %c\n", domain_type);
		assert(0, "Illegal domain");
	}
}

static int read_constituents_from_domains(Linkage linkage,
										  int numcon_total, int s) {

	int d, c, leftlimit, l, leftmost, rightmost, w, c2, numcon_subl=0, w2;
	List_o_links * dlink;
	int rootright, rootleft, adjustment_made;
	Sublinkage * subl;
	const char * name;
	Domain domain;

	r_limit = linkage->num_words-2; /**PV**/

	subl = &linkage->sublinkage[s];

	for (d=0, c=numcon_total; d<subl->pp_data.N_domains; d++, c++) {
		domain = subl->pp_data.domain_array[d];
		rootright = linkage_get_link_rword(linkage, domain.start_link);
		rootleft =  linkage_get_link_lword(linkage, domain.start_link);

		if ((domain.type=='c') ||
			(domain.type=='d') ||
			(domain.type=='e') ||
			(domain.type=='f') ||
			(domain.type=='g') ||
			(domain.type=='u') ||
			(domain.type=='y')) {
			leftlimit = 0;
			leftmost = linkage_get_link_lword(linkage, domain.start_link);
			rightmost = linkage_get_link_lword(linkage, domain.start_link);
		}
		else {
			leftlimit = linkage_get_link_lword(linkage, domain.start_link)+1;
			leftmost = linkage_get_link_rword(linkage, domain.start_link);
			rightmost = linkage_get_link_rword(linkage, domain.start_link);
		}

		/* Start by assigning both left and right limits to the
		   right word of the start link. This will always be contained
		   in the constituent. This will also handle the case
		   where the domain contains no links. */

		for (dlink = domain.lol; dlink!=NULL; dlink=dlink->next) {
			l=dlink->link;

			if ((linkage_get_link_lword(linkage, l) < leftmost) &&
				(linkage_get_link_lword(linkage, l) >= leftlimit))
				leftmost = linkage_get_link_lword(linkage, l);

			if (linkage_get_link_rword(linkage, l) > rightmost)
				rightmost = linkage_get_link_rword(linkage, l);
		}

		c--;
		add_constituent(&c, linkage, domain, leftmost, rightmost,
						cons_of_domain(domain.type));

		if (domain.type=='z') {
			add_constituent(&c, linkage, domain, leftmost, rightmost, "S");
		}
		if (domain.type=='c') {
			add_constituent(&c, linkage, domain, leftmost, rightmost, "S");
		}
		if ((post_process_match("Ce*", constituent[c].start_link)==1) ||
			(post_process_match("Rn", constituent[c].start_link)==1)) {
			add_constituent(&c, linkage, domain, leftmost, rightmost, "SBAR");
		}
		if ((post_process_match("R*", constituent[c].start_link)==1) ||
			(post_process_match("MX#r", constituent[c].start_link)==1)) {
			w=leftmost;
			if (strcmp(linkage->word[w], ",")==0) w++;
			add_constituent(&c, linkage, domain, w, w, "WHNP");
		}
		if (post_process_match("Mj", constituent[c].start_link)==1) {
			w=leftmost;
			if (strcmp(linkage->word[w], ",")==0) w++;
			add_constituent(&c, linkage, domain, w, w+1, "WHPP");
			add_constituent(&c, linkage, domain, w+1, w+1, "WHNP");
		}
		if ((post_process_match("Ss#d", constituent[c].start_link)==1) ||
			(post_process_match("B#d", constituent[c].start_link)==1)) {
			add_constituent(&c, linkage, domain, rootleft, rootleft, "WHNP");
			add_constituent(&c, linkage, domain,
							rootleft, constituent[c-1].right, "SBAR");
		}
		if (post_process_match("CP", constituent[c].start_link)==1) {
			if (strcmp(linkage->word[leftmost], ",")==0)
				constituent[c].left++;
			add_constituent(&c, linkage, domain, 1, linkage->num_words-1, "S");
		}
		if ((post_process_match("MVs", constituent[c].start_link)==1) ||
			(domain.type=='f')) {
			w=constituent[c].left;
			if (strcmp(linkage->word[w], ",")==0)
				w++;
			if (strcmp(linkage->word[w], "when")==0) {
				add_constituent(&c, linkage, domain, w, w, "WHADVP");
			}
		}
		if (domain.type=='t') {
			add_constituent(&c, linkage, domain, leftmost, rightmost, "S");
		}
		if ((post_process_match("QI", constituent[c].start_link)==1) ||
			(post_process_match("Mr", constituent[c].start_link)==1) ||
			(post_process_match("MX#d", constituent[c].start_link)==1)) {
			w=leftmost;
			if (strcmp(linkage->word[w], ",")==0) w++;
			if (wordtype[w] == NONE)
				name = "WHADVP";
			else if (wordtype[w] == QTYPE)
				name = "WHNP";
			else if (wordtype[w] == QDTYPE)
				name = "WHNP";
			else
				assert(0, "Unexpected word type");
			add_constituent(&c, linkage, domain, w, w, name);

			if (wordtype[w] == QDTYPE) {
				/* Now find the finite verb to the right, start an S */
				/*PV* limited w2 to sentence len*/
				for( w2=w+1; w2 < r_limit-1; w2++ )
				  if ((wordtype[w2] == STYPE) || (wordtype[w2] == PTYPE)) break;
				/* adjust the right boundary of previous constituent */
				constituent[c].right = w2-1;
				add_constituent(&c, linkage, domain, w2, rightmost, "S");
			  }
		}

		if (constituent[c].domain_type=='\0') {
			error("Error: no domain type assigned to constituent\n");
		}
		if (constituent[c].start_link==NULL) {
			error("Error: no type assigned to constituent\n");
		}
	}

	numcon_subl = c - numcon_total;
	/* numcon_subl = handle_islands(linkage, numcon_total, numcon_subl);  */

	if (verbosity>=2)
		printf("Constituents added at first stage for subl %d:\n",
			   linkage->current);
	for (c=numcon_total; c<numcon_total + numcon_subl; c++) {
		print_constituent(linkage, c);
	}

	/* Opener case - generates S around main clause.
	   (This must be done first; the S generated will be needed for
	   later cases.) */
	numcon_subl =
		gen_comp(linkage, numcon_total, numcon_subl, "SBAR", "S", "S", 5);

	/* pp opener case */
	numcon_subl =
		gen_comp(linkage, numcon_total, numcon_subl, "PP", "S", "S", 6);

	/* participle opener case */
	numcon_subl =
		gen_comp(linkage, numcon_total, numcon_subl, "S", "S", "S", 9);

	/* Subject-phrase case; every main VP generates an S */
	numcon_subl =
		gen_comp(linkage, numcon_total, numcon_subl, "VP", "S", "NP", 1);

	/* Relative clause case; an SBAR generates a complement NP */
	numcon_subl =
		gen_comp(linkage, numcon_total, numcon_subl, "SBAR", "NP", "NP", 3);

	/* Participle modifier case */
	numcon_subl =
		gen_comp(linkage, numcon_total, numcon_subl, "VP", "NP", "NP", 8);

	/* PP modifying NP */
	numcon_subl =
		gen_comp(linkage, numcon_total, numcon_subl, "PP", "NP", "NP", 8);

	/* Appositive case */
	numcon_subl =
		gen_comp(linkage, numcon_total, numcon_subl, "NP", "NP", "NP", 4);

	/* S-V inversion case; an NP generates a complement VP */
	numcon_subl =
		gen_comp(linkage, numcon_total, numcon_subl, "NP", "SINV", "VP", 7);

	adjust_subordinate_clauses(linkage, numcon_total, numcon_subl);
	for (c=numcon_total; c<numcon_total + numcon_subl; c++) {
		if ((constituent[c].domain_type=='p') &&
			(strcmp(linkage->word[constituent[c].left], ",")==0)) {
			constituent[c].left++;
		}
	}

	/* Make sure the constituents are nested. If two constituents are not nested: whichever
	   constituent has the furthest left boundary, shift that boundary rightwards to the left
	   boundary of the other one */

	while (1) {
		adjustment_made=0;
		for (c=numcon_total; c<numcon_total + numcon_subl; c++) {
			for (c2=numcon_total; c2<numcon_total + numcon_subl; c2++) {
				if ((constituent[c].left < constituent[c2].left) &&
					(constituent[c].right < constituent[c2].right) &&
					(constituent[c].right >= constituent[c2].left)) {

					/* We've found two overlapping constituents.
					   If one is larger, except the smaller one
					   includes an extra comma, adjust the smaller one
					   to exclude the comma */

					if ((strcmp(linkage->word[constituent[c2].right], ",")==0) ||
						(strcmp(linkage->word[constituent[c2].right],
								"RIGHT-WALL")==0)) {
						if (verbosity>=2)
							printf("Adjusting %d to fix comma overlap\n", c2);
						adjust_for_right_comma(linkage, c2);
						adjustment_made=1;
					}
					else if (strcmp(linkage->word[constituent[c].left], ",")==0) {
						if (verbosity>=2)
							printf("Adjusting c %d to fix comma overlap\n", c);
						adjust_for_left_comma(linkage, c);
						adjustment_made=1;
					}
					else {
					  if (verbosity>=2) {
						printf("WARNING: the constituents aren't nested! Adjusting them." \
							   "(%d, %d)\n", c, c2);
					  }
					  constituent[c].left = constituent[c2].left;
					}
				}
			}
		}
		if (adjustment_made==0) break;
	}

	/* This labels certain words as auxiliaries (such as forms of "be"
	   with passives, forms of "have" wth past participles,
	   "to" with infinitives). These words start VP's which include
	   them. In Treebank I, these don't get printed unless they're part of an
	   andlist, in which case they get labeled "X". (this is why we need to
	   label them as "aux".) In Treebank II, however, they seem to be treated
	   just like other verbs, so the "aux" stuff isn't needed. */


	for (c=numcon_total; c<numcon_total + numcon_subl; c++) {
		constituent[c].subl = linkage->current;
		if (((constituent[c].domain_type == 'v') &&
			(wordtype[linkage_get_link_rword(linkage,
											 constituent[c].start_num)]==PTYPE))
		   ||
		   ((constituent[c].domain_type == 't') &&
			(strcmp(constituent[c].type, "VP")==0))) {
			constituent[c].aux=1;
		}
		else constituent[c].aux=0;
	}

	for (c=numcon_total; c<numcon_total + numcon_subl; c++) {
		constituent[c].subl = linkage->current;
		constituent[c].aux=0;
	}

	return numcon_subl;
}

static char * exprint_constituent_structure(Linkage linkage, int numcon_total) {
	int c, w;
	int leftdone[MAXCONSTITUENTS];
	int rightdone[MAXCONSTITUENTS];
	int best, bestright, bestleft;
	Sentence sent;
	char s[100], * p;
	String * cs = String_create();

	assert (numcon_total < MAXCONSTITUENTS, "Too many constituents");
	sent = linkage_get_sentence(linkage);

	for(c=0; c<numcon_total; c++) {
		leftdone[c]=0;
		rightdone[c]=0;
	}

	if(verbosity>=2)
		printf("\n");			

	for(w=1; w<linkage->num_words; w++) {	
		/* Skip left wall; don't skip right wall, since it may
		   have constituent boundaries */

		while(1) {
			best = -1;
			bestright = -1;
			for(c=0; c<numcon_total; c++) {
				if ((constituent[c].left==w) &&
					(leftdone[c]==0) && (constituent[c].valid==1) &&
					(constituent[c].right >= bestright)) {
					best = c;
					bestright = constituent[c].right;
				}
			}
			if (best==-1)
				break;
			leftdone[best]=1;
			if(constituent[best].aux==1) continue;
			append_string(cs, "%c%s ", OPEN_BRACKET, constituent[best].type);
		}

		if (w<linkage->num_words-1) {
			/* Don't print out right wall */
			strcpy(s, sent->word[w].string);

			/* Now, if the first character of the word was
			   originally uppercase, we put it back that way */
			if (sent->word[w].firstupper ==1 )
				s[0]=toupper(s[0]);
			append_string(cs, "%s ", s);
		}

		while(1) {
			best = -1;
			bestleft = -1;
			for(c=0; c<numcon_total; c++) {
				if ((constituent[c].right==w) &&
					(rightdone[c]==0) && (constituent[c].valid==1) &&
					(constituent[c].left > bestleft)) {
					best = c;
					bestleft = constituent[c].left;
				}
			}
			if (best==-1)
				break;
			rightdone[best]=1;
			if (constituent[best].aux==1)
				continue;
			append_string(cs, "%s%c ", constituent[best].type, CLOSE_BRACKET);
		}
	}

	append_string(cs, "\n");
	p = exalloc(strlen(cs->p)+1);
	strcpy(p, cs->p);
	exfree(cs->p, sizeof(char)*cs->allocated);
	exfree(cs, sizeof(String));
	return p;
}

static char * print_flat_constituents(Linkage linkage) {
	int num_words;
	Sentence sent;
	Postprocessor * pp;
	int s, numcon_total, numcon_subl, num_subl;
	char * q;

	sent = linkage_get_sentence(linkage);
	phrase_ss = string_set_create();
	pp = linkage->sent->dict->constituent_pp;
	numcon_total = 0;

	count_words_used(linkage);

	num_subl = linkage->num_sublinkages;
	if(num_subl > MAXSUBL) {
	  num_subl=MAXSUBL;
	  if(verbosity>=2) printf("Number of sublinkages exceeds maximum: only considering first %d sublinkages\n", MAXSUBL);
	}
	if(linkage->unionized==1 && num_subl>1) num_subl--;
	for (s=0; s<num_subl; s++) {
		linkage_set_current_sublinkage(linkage, s);
		linkage_post_process(linkage, pp);
		num_words = linkage_get_num_words(linkage);
		generate_misc_word_info(linkage);
		numcon_subl = read_constituents_from_domains(linkage, numcon_total, s);
		numcon_total = numcon_total + numcon_subl;
	}
	numcon_total = merge_constituents(linkage, numcon_total);
	numcon_total = last_minute_fixes(linkage, numcon_total);
	q = exprint_constituent_structure(linkage, numcon_total);
	string_set_delete(phrase_ss);
	return q;
}

static CType token_type (char *token) {
	if ((token[0]==OPEN_BRACKET) && (strlen(token)>1))
		return OPEN;
	if ((strlen(token)>1) && (token[strlen(token)-1]==CLOSE_BRACKET))
		return CLOSE;
	return WORD;
}

static CNode * make_CNode(char *q) {
	CNode * cn;
	cn = exalloc(sizeof(CNode));
	cn->label = (char *) exalloc(sizeof(char)*(strlen(q)+1));
	strcpy(cn->label, q);
	cn->child = cn->next = (CNode *) NULL;
	cn->next = (CNode *) NULL;
	cn->start = cn->end = -1;
	return cn;
}

static CNode * parse_string(CNode * n) {
	char *q;
	CNode *m, *last_child=NULL;

	while ((q=strtok(NULL, " "))) {
		switch (token_type(q)) {
		case CLOSE :
			q[strlen(q)-1]='\0';
			assert(strcmp(q, n->label)==0,
				   "Constituent tree: Labels do not match.");
			return n;
			break;
		case OPEN:
			m = make_CNode(q+1);
			m = parse_string(m);
			break;
		case WORD:
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

static void print_tree(String * cs, int indent, CNode * n, int o1, int o2) {
	int i, child_offset;
	CNode * m;

	if (n==NULL) return;

	if (indent)
		for (i=0; i<o1; ++i)
			append_string(cs, " ");
	append_string(cs, "(%s ", n->label);
	child_offset = o2 + strlen(n->label)+2;

	for (m=n->child; m!=NULL; m=m->next) {
		if (m->child == NULL) {
			append_string(cs, "%s", m->label);
			if ((m->next != NULL) && (m->next->child == NULL))
				append_string(cs, " ");
		}
		else {
			if (m != n->child) {
				if (indent) append_string(cs, "\n");
				else append_string(cs, " ");
				print_tree(cs, indent, m, child_offset, child_offset);
			}
			else {
				print_tree(cs, indent, m, 0, child_offset);
			}
			if ((m->next != NULL) && (m->next->child == NULL)) {
				if (indent) {
					append_string(cs, "\n");
					for (i=0; i<child_offset; ++i)
						append_string(cs, " ");
				}
				else append_string(cs, " ");
			}
		}
	}
	append_string(cs, ")");
}

static int assign_spans(CNode * n, int start) {
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

CNode * linkage_constituent_tree(Linkage linkage) {
	static char *p, *q;
	int len;
	CNode * root;
	p = print_flat_constituents(linkage);
	len = strlen(p);
	q = strtok(p, " ");
	assert(token_type(q) == OPEN, "Illegal beginning of string");
	root = make_CNode(q+1);
	root = parse_string(root);
	assign_spans(root, 0);
	exfree(p, sizeof(char)*(len+1));
	return root;
}

void linkage_free_constituent_tree(CNode * n) {
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

char * linkage_print_constituent_tree(Linkage linkage, int mode)
{
	String * cs;
	CNode * root;
	char * p;

	if ((mode == 0) || (linkage->sent->dict->constituent_pp == NULL)) {
		return NULL;
	}
	else if (mode==1 || mode==3) {
		cs = String_create();
		root = linkage_constituent_tree(linkage);
		print_tree(cs, (mode==1), root, 0, 0);
		linkage_free_constituent_tree(root);
		append_string(cs, "\n");
		p = exalloc(strlen(cs->p)+1);
		strcpy(p, cs->p);
		exfree(cs->p, sizeof(char)*cs->allocated);
		exfree(cs, sizeof(String));
		return p;
	}
	else if (mode == 2) {
		return print_flat_constituents(linkage);
	}
	assert(0, "Illegal mode in linkage_print_constituent_tree");
	return NULL;
}

void linkage_free_constituent_tree_str(char * s)
{
	exfree(s, strlen(s)+1);
}

char * linkage_constituent_node_get_label(const CNode *n)
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
