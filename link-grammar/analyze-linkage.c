/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2012, 2014 Linas Vepstas                                */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/


#include <stdarg.h>
#include "analyze-linkage.h"
#include "api-structures.h"
#include "post-process.h"
#include "string-set.h"
#include "structures.h"
#include "word-utils.h"

/**
 * This function defines the cost of a link as a function of its length.
 */
static inline int cost_for_length(int length)
{
	return length-1;
}

/**
 * Computes the cost of the current parse of the current sentence,
 * due to the length of the links.
 */
static size_t compute_link_cost(Linkage lkg)
{
	size_t lcost, i;
	lcost =  0;
	for (i = 0; i < lkg->num_links; i++)
	{
		lcost += cost_for_length(lkg->link_array[i].rw - lkg->link_array[i].lw);
	}
	return lcost;
}

static int unused_word_cost(Linkage lkg)
{
	int lcost;
	size_t i;
	lcost =  0;
	for (i = 0; i < lkg->num_words; i++)
		lcost += (lkg->chosen_disjuncts[i] == NULL);
	return lcost;
}

/**
 * Computes the cost of the current parse of the current sentence
 * due to the cost of the chosen disjuncts.
 */
static double compute_disjunct_cost(Linkage lkg)
{
	size_t i;
	double lcost;
	lcost =  0.0;
	for (i = 0; i < lkg->num_words; i++)
	{
		if (lkg->chosen_disjuncts[i] != NULL)
			lcost += lkg->chosen_disjuncts[i]->cost;
	}
	return lcost;
}

/**
 * This returns a string that is the the GCD of the two given strings.
 * If the GCD is equal to one of them, a pointer to it is returned.
 * Otherwise a new string for the GCD is xalloced and put on the
 * "free later" list.
 */
const char * intersect_strings(Sentence sent, const char * s, const char * t)
{
	int i, j, d;
	const char *w, *s0;
	char u0[MAX_TOKEN_LENGTH]; /* Links are *always* less than 10 chars long */
	char *u;

	if (islower((int) *s)) s++;  /* skip the head-dependent indicator */
	if (islower((int) *t)) t++;  /* skip the head-dependent indicator */

	if (strcmp(s,t) == 0) return s;  /* would work without this */
	i = strlen(s);
	j = strlen(t);
	if (j > i) {
		w = s; s = t; t = w;
	}
	/* s is now the longer (at least not the shorter) string */
	u = u0;
	d = 0;
	s0 = s;
	while (*t != '\0') {
		if ((*s == *t) || (*t == '*')) {
			*u = *s;
		} else {
			d++;
			if (*s == '*') *u = *t;
			else *u = '^';
		}
		s++; t++; u++;
	}
	if (d == 0) {
		return s0;
	} else {
		strcpy(u, s);   /* get the remainder of s */
		return string_set_add(u0, sent->string_set);
	}
}

/**
 * The name of the link is set to be the GCD of the names of
 * its two endpoints. Must be called after each extract_links(),
 * etc. since that call issues a brand-new set of links into
 * parse_info.
 */
static void compute_link_names(Sentence sent, Linkage lkg)
{
	size_t i;
	for (i = 0; i < lkg->num_links; i++)
	{
		lkg->link_array[i].link_name = intersect_strings(sent,
			connector_get_string(lkg->link_array[i].lc),
			connector_get_string(lkg->link_array[i].rc));
	}
}

/**
 * This does a minimal post-processing step, using the 'standard'
 * post-processor.  It also computes some of the linkage costs.
 */
void analyze_thin_linkage(Sentence sent, Linkage lkg, Parse_Options opts, int analyze_pass)
{
	PP_node * pp;
	Postprocessor * postprocessor = sent->postprocessor;

	compute_link_names(sent, lkg);
	if (analyze_pass == PP_FIRST_PASS)
	{
		post_process_scan_linkage(postprocessor, opts, sent, lkg);
		return;
	}

	pp = do_post_process(postprocessor, opts, sent, lkg);
	post_process_free_data(&postprocessor->pp_data);

	lkg->lifo.N_violations = 0;
	lkg->lifo.unused_word_cost = unused_word_cost(lkg);
	if (opts->use_sat_solver)
	{
		lkg->lifo.disjunct_cost = 0.0;
	}
	else
	{
		lkg->lifo.disjunct_cost = compute_disjunct_cost(lkg);
	}
	lkg->lifo.link_cost = compute_link_cost(lkg);
	lkg->lifo.corpus_cost = -1.0;

	if (pp == NULL)
	{
		if (postprocessor != NULL) lkg->lifo.N_violations = 1;
	}
	else if (pp->violation != NULL)
	{
		lkg->lifo.N_violations++;
		lkg->lifo.pp_violation_msg = pp->violation;
	}
}
