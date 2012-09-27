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

#ifdef USE_FAT_LINKAGES

#include "api.h"
#include "disjunct-utils.h"

/* This file contains the functions for massaging disjuncts of the
   sentence in special ways having to do with conjunctions.
   The only function called from the outside world is
   install_special_conjunctive_connectors()

   It would be nice if this code was written more transparently.  In
   other words, there should be some fairly general functions that
   manipulate disjuncts, and take words like "neither" etc as input
   parameters, so as to encapsulate the changes being made for special
   words.  This would not be too hard to do, but it's not a high priority.
       -DS 3/98
 */

#define COMMA_LABEL   (-2) /* to hook the comma to the following "and" */
#define EITHER_LABEL  (-3) /* to connect the "either" to the following "or" */
#define NEITHER_LABEL (-4) /* to connect the "neither" to the following "nor"*/
#define NOT_LABEL     (-5) /* to connect the "not" to the following "but"*/
#define NOTONLY_LABEL (-6) /* to connect the "not" to the following "only"*/
#define BOTH_LABEL    (-7) /* to connect the "both" to the following "and"*/

/* There's a problem with installing "...but...", "not only...but...", and
   "not...but...", which is that the current comma mechanism will allow
   a list separated by commas.  "Not only John, Mary but Jim came"
   The best way to prevent this is to make it impossible for the comma
   to attach to the "but", via some sort of additional subscript on commas.

   I can't think of a good way to prevent this.
*/

/* The following functions all do slightly different variants of the
   following thing:

   Catenate to the disjunct list pointed to by d, a new disjunct list.
   The new list is formed by copying the old list, and adding the new
   connector somewhere in the old disjunct, for disjuncts that satisfy
   certain conditions
*/

/**
 * glom_comma_connector() --
 * In this case the connector is to connect to the comma to the
 * left of an "and" or an "or".  Only gets added next to a fat link
 */
static Disjunct * glom_comma_connector(Disjunct * d)
{
	Disjunct * d_list, * d1, * d2;
	Connector * c, * c1;
	d_list = NULL;
	for (d1 = d; d1!=NULL; d1=d1->next) {
		if (d1->left == NULL) continue;
		for (c = d1->left; c->next != NULL; c = c->next)
		  ;
		if (c->label < 0) continue;   /* last one must be a fat link */

		d2 = copy_disjunct(d1);
		d2->next = d_list;
		d_list = d2;

		c1 = connector_new();
		c1->label = COMMA_LABEL;

		c->next = c1;
	}
	return catenate_disjuncts(d, d_list);
}

/**
 * In this case the connector is to connect to the "either", "neither",
 * "not", or some auxilliary d to the current which is a conjunction.
 * Only gets added next to a fat link, but before it (not after it)
 * In the case of "nor", we don't create new disjuncts, we merely modify
 * existing ones.  This forces the fat link uses of "nor" to
 * use a neither.  (Not the case with "or".)  If necessary=FALSE, then
 * duplication is done, otherwise it isn't
 */
static Disjunct * glom_aux_connector(Disjunct * d, int label, int necessary)
{
	Disjunct * d_list, * d1, * d2;
	Connector * c, * c1, *c2;
	d_list = NULL;
	for (d1 = d; d1!=NULL; d1=d1->next) {
		if (d1->left == NULL) continue;
		for (c = d1->left; c->next != NULL; c = c->next)
		  ;
		if (c->label < 0) continue;   /* last one must be a fat link */

		if (!necessary) {
			d2 = copy_disjunct(d1);
			d2->next = d_list;
			d_list = d2;
		}

		c1 = connector_new();
		c1->label = label;
		c1->next = c;

		if (d1->left == c) {
			d1->left = c1;
		} else {
			for (c2 = d1->left; c2->next != c; c2 = c2->next)
			  ;
			c2->next = c1;
		}
	}
	return catenate_disjuncts(d, d_list);
}

/**
 * This adds one connector onto the beginning of the left (or right)
 * connector list of d.  The label and string of the connector are
 * specified
 */
static Disjunct * add_one_connector(int label, int dir, const char *cs, Disjunct * d)
{
	Connector * c;

	c = connector_new();
	c->string = cs;
	c->label = label;

	if (dir == '+') {
		c->next = d->right;
		d->right = c;
	} else {
		c->next = d->left;
		d->left = c;
	}
	return d;
}

/**
 * special_disjunct() --
 * Builds a new disjunct with one connector pointing in direction dir
 * (which is '+' or '-').  The label and string of the connector
 * are specified, as well as the string of the disjunct.
 * The next pointer of the new disjunct set to NULL, so it can be
 * regarded as a list.
 */
static Disjunct * special_disjunct(int label, int dir, const char *cs, const char * ds)
{
	Disjunct * d1;
	Connector * c;
	d1 = (Disjunct *) xalloc(sizeof(Disjunct));
	d1->cost = 0;
	d1->string = ds;
	d1->next = NULL;

	c = connector_new();
	c->string = cs;
	c->label = label;

	if (dir == '+') {
		d1->left = NULL;
		d1->right = c;
	} else {
		d1->right = NULL;
		d1->left = c;
	}
	return d1;
}

/**
 * Finds all places in the sentence where a comma is followed by
 * a conjunction ("and", "or", "but", or "nor").  It modifies these comma
 * disjuncts, and those of the following word, to allow the following
 * word to absorb the comma (if used as a conjunction).
 */
static void construct_comma(Sentence sent)
{
	int w;
	for (w=0; w<sent->length-1; w++) {
		if ((strcmp(sent->word[w].string, ",")==0) && sent->is_conjunction[w+1]) {
			sent->word[w].d = catenate_disjuncts(special_disjunct(COMMA_LABEL,'+',"", ","), sent->word[w].d);
			sent->word[w+1].d = glom_comma_connector(sent->word[w+1].d);
		}
	}
}


/** Returns TRUE if one of the words in the sentence is s */
static int sentence_contains(Sentence sent, const char * s)
{
	int w;
	for (w=0; w<sent->length; w++) {
		if (strcmp(sent->word[w].string, s) == 0) return TRUE;
	}
	return FALSE;
}

/**
 * The functions below put the special connectors on certain auxilliary
   words to be used with conjunctions.  Examples: either, neither,
   both...and..., not only...but...
XXX FIXME: This routine uses "sentence_contains" to test for explicit 
English words, and clearly this fails for other langauges!! XXX FIXME!
*/

static void construct_either(Sentence sent)
{
	int w;
	if (!sentence_contains(sent, "either")) return;
	for (w=0; w<sent->length; w++) {
		if (strcmp(sent->word[w].string, "either") != 0) continue;
		sent->word[w].d = catenate_disjuncts(
				   special_disjunct(EITHER_LABEL,'+',"", "either"),
				   sent->word[w].d);
	}

	for (w=0; w<sent->length; w++) {
		if (strcmp(sent->word[w].string, "or") != 0) continue;
		sent->word[w].d = glom_aux_connector
						  (sent->word[w].d, EITHER_LABEL, FALSE);
	}
}

static void construct_neither(Sentence sent)
{
	int w;
	if (!sentence_contains(sent, "neither")) {
		/* I don't see the point removing disjuncts on "nor".  I
		   Don't know why I did this.  What's the problem keeping the
		   stuff explicitely defined for "nor" in the dictionary?  --DS 3/98 */
#if 0
			for (w=0; w<sent->length; w++) {
			if (strcmp(sent->word[w].string, "nor") != 0) continue;
			free_disjuncts(sent->word[w].d);
			sent->word[w].d = NULL;  /* a nor with no neither is dead */
		}
#endif
		return;
	}
	for (w=0; w<sent->length; w++) {
		if (strcmp(sent->word[w].string, "neither") != 0) continue;
		sent->word[w].d = catenate_disjuncts(
				   special_disjunct(NEITHER_LABEL,'+',"", "neither"),
				   sent->word[w].d);
	}

	for (w=0; w<sent->length; w++) {
		if (strcmp(sent->word[w].string, "nor") != 0) continue;
		sent->word[w].d = glom_aux_connector
						  (sent->word[w].d, NEITHER_LABEL, TRUE);
	}
}

static void construct_notonlybut(Sentence sent)
{
	int w;
	Disjunct *d;
	if (!sentence_contains(sent, "not")) {
		return;
	}
	for (w=0; w<sent->length; w++) {
		if (strcmp(sent->word[w].string, "not") != 0) continue;
		sent->word[w].d = catenate_disjuncts(
			 special_disjunct(NOT_LABEL,'+',"", "not"),
			 sent->word[w].d);
		if (w<sent->length-1 &&  strcmp(sent->word[w+1].string, "only")==0) {
			sent->word[w+1].d = catenate_disjuncts(
						  special_disjunct(NOTONLY_LABEL, '-',"","only"),
						  sent->word[w+1].d);
			d = special_disjunct(NOTONLY_LABEL, '+', "","not");
			d = add_one_connector(NOT_LABEL,'+',"", d);
			sent->word[w].d = catenate_disjuncts(d, sent->word[w].d);
		}
	}
	/* The code below prevents sentences such as the following from
	   parsing:
	   it was not carried out by Serbs but by Croats */


	/* We decided that this is a silly thing to.  Here's the bug report
	   caused by this:

	  Bug with conjunctions.  Some that work with "and" but they don't work
	  with "but".  "He was not hit by John and by Fred".
	  (Try replacing "and" by "but" and it does not work.
	  It's getting confused by the "not".)
	 */
	for (w=0; w<sent->length; w++) {
		if (strcmp(sent->word[w].string, "but") != 0) continue;
		sent->word[w].d = glom_aux_connector
						  (sent->word[w].d, NOT_LABEL, FALSE);
		/* The above line use to have a TRUE in it */
	}
}

static void construct_both(Sentence sent)
{
	int w;
	if (!sentence_contains(sent, "both")) return;
	for (w=0; w<sent->length; w++) {
		if (strcmp(sent->word[w].string, "both") != 0) continue;
		sent->word[w].d = catenate_disjuncts(
				   special_disjunct(BOTH_LABEL,'+',"", "both"),
				   sent->word[w].d);
	}

	for (w=0; w<sent->length; w++) {
		if (strcmp(sent->word[w].string, "and") != 0) continue;
		sent->word[w].d = glom_aux_connector(sent->word[w].d, BOTH_LABEL, FALSE);
	}
}

void install_special_conjunctive_connectors(Sentence sent)
{
	construct_either(sent);      /* special connectors for "either" */
	construct_neither(sent);     /* special connectors for "neither" */
	construct_notonlybut(sent);  /* special connectors for "not..but.." */
	                             /* and               "not only..but.." */
	construct_both(sent);        /* special connectors for "both..and.." */
	construct_comma(sent);       /* special connectors for extra comma */
}

#endif /* USE_FAT_LINKAGES */
