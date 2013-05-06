/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright 2008, 2009, 2013 Linas Vepstas                              */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <limits.h>
#include <math.h>
#include <string.h>
#include "api.h"
#include "disjuncts.h"
#include "disjunct-utils.h"
#include "error.h"
#include "preparation.h"
#include "read-regex.h"
#include "regex-morph.h"
#include "sat-solver/sat-encoder.h"
#include "corpus/corpus.h"
#include "spellcheck.h"
#include "utilities.h"

/***************************************************************
*
* Routines for setting Parse_Options
*
****************************************************************/

static int VDAL_compare_parse(Linkage_info * p1, Linkage_info * p2)
{
	/* for sorting the linkages in postprocessing */
	if (p1->N_violations != p2->N_violations) {
		return (p1->N_violations - p2->N_violations);
	}
	else if (p1->unused_word_cost != p2->unused_word_cost) {
		return (p1->unused_word_cost - p2->unused_word_cost);
	}
#ifdef USE_FAT_LINKAGES
	else if (p1->fat != p2->fat) {
		return (p1->fat - p2->fat);
	}
#endif /* USE_FAT_LINKAGES */
	else if (p1->disjunct_cost != p2->disjunct_cost) {
		return (p1->disjunct_cost - p2->disjunct_cost);
	}
#ifdef USE_FAT_LINKAGES
	else if (p1->and_cost != p2->and_cost) {
		return (p1->and_cost - p2->and_cost);
	}
#endif /* USE_FAT_LINKAGES */
	else {
		return (p1->link_cost - p2->link_cost);
	}
}

#ifdef USE_CORPUS
static int CORP_compare_parse(Linkage_info * p1, Linkage_info * p2)
{
	double diff = p1->corpus_cost - p2->corpus_cost;
	if (fabs(diff) < 1.0e-5) 
		return VDAL_compare_parse(p1, p2);
	if (diff < 0.0f) return -1;
	return 1;
}
#endif

/**
 * Create and initialize a Parse_Options object
 */
Parse_Options parse_options_create(void)
{
	Parse_Options po;

	init_memusage();
	po = (Parse_Options) xalloc(sizeof(struct Parse_Options_s));

	/* Here's where the values are initialized */
	po->verbosity	 = 1;
	po->linkage_limit = 100;
	po->disjunct_cost = MAX_DISJUNCT_COST;
	po->min_null_count = 0;
	po->max_null_count = 0;
	po->null_block = 1;
	po->islands_ok = FALSE;
	po->use_spell_guess = TRUE;
	po->use_sat_solver = FALSE;

#ifdef XXX_USE_CORPUS
	/* Use the corpus cost model, if available.
	 * It really does a better job at parse ranking.
	 * Err .. sometimes ... 
	 */
	po->cost_model.compare_fn = &CORP_compare_parse;
	po->cost_model.type = CORPUS;
#else /* USE_CORPUS */
	po->cost_model.compare_fn = &VDAL_compare_parse;
	po->cost_model.type = VDAL;
#endif /* USE_CORPUS */
	po->short_length = 6;
	po->all_short = FALSE;
	po->twopass_length = 30;
	po->max_sentence_length = 170;
	po->resources = resources_create();
	po->display_short = TRUE;
	po->display_word_subscripts = TRUE;
	po->display_link_subscripts = TRUE;
	po->display_walls = FALSE;
#ifdef USE_FAT_LINKAGES
	po->use_fat_links = FALSE;
	po->display_union = FALSE;
#endif /* USE_FAT_LINKAGES */
	po->allow_null = TRUE;
	po->use_cluster_disjuncts = FALSE;
	po->echo_on = FALSE;
	po->batch_mode = FALSE;
	po->panic_mode = FALSE;
	po->screen_width = 79;
	po->display_on = TRUE;
	po->display_postscript = FALSE;
	po->display_constituents = NO_DISPLAY;
	po->display_bad = FALSE;
	po->display_disjuncts = FALSE;
	po->display_links = FALSE;
	po->display_senses = FALSE;
	po->display_suffixes = FALSE;

	return po;
}

int parse_options_delete(Parse_Options  opts)
{
	resources_delete(opts->resources);
	xfree(opts, sizeof(struct Parse_Options_s));
	return 0;
}

void parse_options_set_cost_model_type(Parse_Options opts, Cost_Model_type cm)
{
	switch(cm) {
	case VDAL:
		opts->cost_model.type = VDAL;
		opts->cost_model.compare_fn = &VDAL_compare_parse;
		break;
	case CORPUS:
#ifdef USE_CORPUS
		opts->cost_model.type = CORPUS;
		opts->cost_model.compare_fn = &CORP_compare_parse;
#else
		prt_error("Error: Source code compiled with cost model 'CORPUS' disabled.\n");
#endif
		break;
	default:
		prt_error("Error: Illegal cost model: %d\n", cm);
	}
}

Cost_Model_type parse_options_get_cost_model_type(Parse_Options opts)
{
	return opts->cost_model.type;
}

void parse_options_set_verbosity(Parse_Options opts, int dummy)
{
	opts->verbosity = dummy;
	verbosity = opts->verbosity;
	/* this is one of the only global variables. */
}

int parse_options_get_verbosity(Parse_Options opts) {
	return opts->verbosity;
}

void parse_options_set_use_sat_parser(Parse_Options opts, int dummy) {
#ifdef USE_SAT_SOLVER
	opts->use_sat_solver = dummy;
#else
	prt_error("Error: cannot enable the Boolean SAT parser; this "
	          " library was built without SAT solver support.\n");
#endif
}

int parse_options_get_use_sat_parser(Parse_Options opts) {
	return opts->use_sat_solver;
}

void parse_options_set_use_viterbi(Parse_Options opts, int dummy) {
	opts->use_viterbi = dummy;
}

int parse_options_get_use_viterbi(Parse_Options opts) {
	return opts->use_viterbi;
}

#ifdef USE_FAT_LINKAGES
void parse_options_set_use_fat_links(Parse_Options opts, int dummy) {
	opts->use_fat_links = dummy;
}
int parse_options_get_use_fat_links(Parse_Options opts) {
	return opts->use_fat_links;
}
#endif /* USE_FAT_LINKAGES */

void parse_options_set_linkage_limit(Parse_Options opts, int dummy) {
	opts->linkage_limit = dummy;
}
int parse_options_get_linkage_limit(Parse_Options opts) {
	return opts->linkage_limit;
}

void parse_options_set_disjunct_cost(Parse_Options opts, int dummy) {
	opts->disjunct_cost = dummy;
}
void parse_options_set_disjunct_costf(Parse_Options opts, float dummy) {
	opts->disjunct_cost = dummy;
}
int parse_options_get_disjunct_cost(Parse_Options opts) {
	return opts->disjunct_cost;
}
float parse_options_get_disjunct_costf(Parse_Options opts) {
	return opts->disjunct_cost;
}

void parse_options_set_min_null_count(Parse_Options opts, int val) {
	opts->min_null_count = val;
}
int parse_options_get_min_null_count(Parse_Options opts) {
	return opts->min_null_count;
}

void parse_options_set_max_null_count(Parse_Options opts, int val) {
	opts->max_null_count = val;
}
int parse_options_get_max_null_count(Parse_Options opts) {
	return opts->max_null_count;
}


void parse_options_set_null_block(Parse_Options opts, int dummy) {
	opts->null_block = dummy;
}
int parse_options_get_null_block(Parse_Options opts) {
	return opts->null_block;
}

void parse_options_set_islands_ok(Parse_Options opts, int dummy) {
	opts->islands_ok = dummy;
}

int parse_options_get_islands_ok(Parse_Options opts) {
	return opts->islands_ok;
}

void parse_options_set_spell_guess(Parse_Options opts, int dummy) {
	opts->use_spell_guess = dummy;
}

int parse_options_get_spell_guess(Parse_Options opts) {
	return opts->use_spell_guess;
}

void parse_options_set_short_length(Parse_Options opts, int short_length) {
	opts->short_length = short_length;
}

int parse_options_get_short_length(Parse_Options opts) {
	return opts->short_length;
}

void parse_options_set_all_short_connectors(Parse_Options opts, int val) {
	opts->all_short = val;
}

int parse_options_get_all_short_connectors(Parse_Options opts) {
	return opts->all_short;
}

void parse_options_set_max_parse_time(Parse_Options opts, int dummy) {
	opts->resources->max_parse_time = dummy;
}

int parse_options_get_max_parse_time(Parse_Options opts) {
	return opts->resources->max_parse_time;
}

void parse_options_set_max_memory(Parse_Options opts, int dummy) {
	opts->resources->max_memory = dummy;
}

int parse_options_get_max_memory(Parse_Options opts) {
	return opts->resources->max_memory;
}

void parse_options_set_max_sentence_length(Parse_Options opts, int dummy) {
	opts->max_sentence_length = dummy;
}

int parse_options_get_max_sentence_length(Parse_Options opts) {
	return opts->max_sentence_length;
}

void parse_options_set_echo_on(Parse_Options opts, int dummy) {
	opts->echo_on = dummy;
}

int parse_options_get_echo_on(Parse_Options opts) {
	return opts->echo_on;
}

void parse_options_set_batch_mode(Parse_Options opts, int dummy) {
	opts->batch_mode = dummy;
}

int parse_options_get_batch_mode(Parse_Options opts) {
	return opts->batch_mode;
}

void parse_options_set_panic_mode(Parse_Options opts, int dummy) {
	opts->panic_mode = dummy;
}

int parse_options_get_panic_mode(Parse_Options opts) {
	return opts->panic_mode;
}

void parse_options_set_allow_null(Parse_Options opts, int dummy) {
	opts->allow_null = dummy;
}

int parse_options_get_allow_null(Parse_Options opts) {
	return opts->allow_null;
}

void parse_options_set_use_cluster_disjuncts(Parse_Options opts, int dummy) {
	opts->use_cluster_disjuncts = dummy;
}

int parse_options_get_use_cluster_disjuncts(Parse_Options opts) {
	return opts->use_cluster_disjuncts;
}

void parse_options_set_screen_width(Parse_Options opts, int dummy) {
	opts->screen_width = dummy;
}

int parse_options_get_screen_width(Parse_Options opts) {
	return opts->screen_width;
}


void parse_options_set_display_on(Parse_Options opts, int dummy) {
	opts->display_on = dummy;
}

int parse_options_get_display_on(Parse_Options opts) {
	return opts->display_on;
}

void parse_options_set_display_postscript(Parse_Options opts, int dummy) {
	opts->display_postscript = dummy;
}

int parse_options_get_display_postscript(Parse_Options opts)
{
	return opts->display_postscript;
}

void parse_options_set_display_constituents(Parse_Options opts, ConstituentDisplayStyle dummy)
{
	if (dummy > MAX_STYLES)
	{
		prt_error("Possible values for constituents: \n"
	             "   0 (no display)\n"
	             "   1 (treebank style, multi-line indented)\n"
	             "   2 (flat tree, square brackets)\n"
	             "   3 (flat treebank style)\n");
	   opts->display_constituents = NO_DISPLAY;
	}
	else opts->display_constituents = dummy;
}

ConstituentDisplayStyle parse_options_get_display_constituents(Parse_Options opts)
{
	return opts->display_constituents;
}

void parse_options_set_display_bad(Parse_Options opts, int dummy) {
	opts->display_bad = dummy;
}

int parse_options_get_display_bad(Parse_Options opts) {
	return opts->display_bad;
}

void parse_options_set_display_disjuncts(Parse_Options opts, int dummy) {
	opts->display_disjuncts = dummy;
}

int parse_options_get_display_disjuncts(Parse_Options opts) {
	return opts->display_disjuncts;
}

void parse_options_set_display_links(Parse_Options opts, int dummy) {
	opts->display_links = dummy;
}

int parse_options_get_display_links(Parse_Options opts) {
	return opts->display_links;
}

void parse_options_set_display_senses(Parse_Options opts, int dummy) {
	opts->display_senses = dummy;
}

int parse_options_get_display_senses(Parse_Options opts) {
	return opts->display_senses;
}

int parse_options_get_display_suffixes(Parse_Options opts) {
	return opts->display_suffixes;
}

void parse_options_set_display_suffixes(Parse_Options opts, int dummy) {
	opts->display_suffixes = dummy;
}

void parse_options_set_display_walls(Parse_Options opts, int dummy) {
	opts->display_walls = dummy;
}

int parse_options_get_display_walls(Parse_Options opts) {
	return opts->display_walls;
}

int parse_options_get_display_union(Parse_Options opts) {
#ifdef USE_FAT_LINKAGES
	return opts->display_union;
#else
	return 0;
#endif /* USE_FAT_LINKAGES */
}

void parse_options_set_display_union(Parse_Options opts, int dummy) {
#ifdef USE_FAT_LINKAGES
	opts->display_union = dummy;
#endif /* USE_FAT_LINKAGES */
}

int parse_options_timer_expired(Parse_Options opts) {
	return resources_timer_expired(opts->resources);
}

int parse_options_memory_exhausted(Parse_Options opts) {
	return resources_memory_exhausted(opts->resources);
}

int parse_options_resources_exhausted(Parse_Options opts) {
	return (resources_timer_expired(opts->resources) || resources_memory_exhausted(opts->resources));
}

void parse_options_reset_resources(Parse_Options opts) {
	resources_reset(opts->resources);
}

/***************************************************************
*
* Routines for postprocessing
*
****************************************************************/

static Linkage_info * linkage_info_new(int num_to_alloc)
{
	Linkage_info *link_info;
	link_info = (Linkage_info *) xalloc(num_to_alloc * sizeof(Linkage_info));
	memset(link_info, 0, num_to_alloc * sizeof(Linkage_info));
	return link_info;
}

static void linkage_info_delete(Linkage_info *link_info, int sz)
{
	int i,j;

	for (i=0; i<sz; i++)
	{
		Linkage_info *lifo = &link_info[i];
		int nwords = lifo->nwords;
		for (j=0; j<nwords; j++)
		{
			if (lifo->disjunct_list_str[j])
				free(lifo->disjunct_list_str[j]);
		}
		free(lifo->disjunct_list_str);
#ifdef USE_CORPUS
		lg_sense_delete(lifo);
#endif
	}
	xfree(link_info, sz);
}

#ifdef USE_FAT_LINKAGES
static void free_andlists(Sentence sent)
{
	int L;
	Andlist * andlist, * next;
	for(L=0; L<sent->num_linkages_post_processed; L++) {
		/* printf("%d ", sent->link_info[L].canonical);  */
		/* if (sent->link_info[L].canonical==0) continue; */
		andlist = sent->link_info[L].andlist;
		while(1) {
			if(andlist == NULL) break;
			next = andlist->next;
			xfree((char *) andlist, sizeof(Andlist));
			andlist = next;
		}
	}
	/* printf("\n"); */
}
#endif /* USE_FAT_LINKAGES */

static void free_post_processing(Sentence sent)
{
	if (sent->link_info != NULL) {
		/* postprocessing must have been done */
#ifdef USE_FAT_LINKAGES
		free_andlists(sent);
#endif /* USE_FAT_LINKAGES */
		linkage_info_delete(sent->link_info, sent->num_linkages_alloced);
		sent->link_info = NULL;
	}
}

static void post_process_linkages(Sentence sent, Parse_Options opts)
{
	int *indices;
	int in, block_bottom, block_top;
	int N_linkages_found, N_linkages_alloced;
	int N_linkages_post_processed, N_valid_linkages;
	int overflowed;
	Linkage_info *link_info;
#ifdef USE_FAT_LINKAGES
	int N_thin_linkages;
	int only_canonical_allowed;
	int canonical;
#endif /* USE_FAT_LINKAGES */

	free_post_processing(sent);

	overflowed = build_parse_set(sent, sent->null_count, opts);
	print_time(opts, "Built parse set");

	if (overflowed && (1 < opts->verbosity))
	{
		err_ctxt ec;
		ec.sent = sent;
		err_msg(&ec, Warn, "Warning: Count overflow.\n"
		  "Considering a random subset of %d of an unknown and large number of linkages\n",
			opts->linkage_limit);
	}
	N_linkages_found = sent->num_linkages_found;

	if (sent->num_linkages_found == 0)
	{
		sent->num_linkages_alloced = 0;
		sent->num_linkages_post_processed = 0;
		sent->num_valid_linkages = 0;
#ifdef USE_FAT_LINKAGES
		sent->num_thin_linkages = 0;
#endif /* USE_FAT_LINKAGES */
		sent->link_info = NULL;
		return;
	}

	if (N_linkages_found > opts->linkage_limit)
	{
		N_linkages_alloced = opts->linkage_limit;
		if (opts->verbosity > 1)
		{
			err_ctxt ec;
			ec.sent = sent;
			err_msg(&ec, Warn, "Warning: Considering a random subset of %d of %d linkages\n",
				  N_linkages_alloced, N_linkages_found);
		}
	}
	else
	{
		N_linkages_alloced = N_linkages_found;
	}

	link_info = linkage_info_new(N_linkages_alloced);
	N_valid_linkages = 0;

	/* Generate an array of linkage indices to examine */
	indices = (int *) xalloc(N_linkages_alloced * sizeof(int));
	if (overflowed)
	{
		for (in=0; in < N_linkages_alloced; in++)
		{
			indices[in] = -(in+1);
		}
	}
	else
	{
		sent->rand_state = N_linkages_found + sent->length;
		for (in=0; in<N_linkages_alloced; in++)
		{
			double frac = (double) N_linkages_found;
			frac /= (double) N_linkages_alloced;
			block_bottom = (int) (((double) in) * frac);
			block_top = (int) (((double) (in+1)) * frac);
			indices[in] = block_bottom + 
				(rand_r(&sent->rand_state) % (block_top-block_bottom));
		}
	}

#ifdef USE_FAT_LINKAGES
	only_canonical_allowed = !(overflowed || (N_linkages_found > 2*opts->linkage_limit));
#endif /* USE_FAT_LINKAGES */
	/* When we're processing only a small subset of the linkages,
	 * don't worry about restricting the set we consider to be
	 * canonical ones.  In the extreme case where we are only
	 * generating 1 in a million linkages, it's very unlikely
	 * that we'll hit two symmetric variants of the same linkage
	 * anyway.
	 */
	/* (optional) first pass: just visit the linkages */
	/* The purpose of these two passes is to make the post-processing
	 * more efficient.  Because (hopefully) by the time you do the
	 * real work in the 2nd pass you've pruned the relevant rule set
	 * in the first pass.
	 */
	if (sent->length >= opts->twopass_length)
	{
		for (in=0; (in < N_linkages_alloced) &&
				   (!resources_exhausted(opts->resources)); in++)
		{
			extract_links(indices[in], sent->null_count, sent->parse_info);
#ifdef USE_FAT_LINKAGES
			if (set_has_fat_down(sent))
			{
				if (only_canonical_allowed && !is_canonical_linkage(sent)) continue;
				analyze_fat_linkage(sent, opts, PP_FIRST_PASS);
			}
			else
#endif /* USE_FAT_LINKAGES */
			{
				analyze_thin_linkage(sent, opts, PP_FIRST_PASS);
			}
		}
	}

	/* second pass: actually perform post-processing */
	N_linkages_post_processed = 0;
#ifdef USE_FAT_LINKAGES
	N_thin_linkages = 0;
#endif /* USE_FAT_LINKAGES */
	for (in=0; (in < N_linkages_alloced) &&
			   (!resources_exhausted(opts->resources)); in++)
	{
		Linkage_info *lifo = &link_info[N_linkages_post_processed];
		extract_links(indices[in], sent->null_count, sent->parse_info);
#ifdef USE_FAT_LINKAGES
		lifo->fat = FALSE;
		lifo->canonical = TRUE;
		if (set_has_fat_down(sent))
		{
			canonical = is_canonical_linkage(sent);
			if (only_canonical_allowed && !canonical) continue;
			*lifo = analyze_fat_linkage(sent, opts, PP_SECOND_PASS);
			lifo->fat = TRUE;
			lifo->canonical = canonical;
		}
		else
#endif /* USE_FAT_LINKAGES */
		{
			*lifo = analyze_thin_linkage(sent, opts, PP_SECOND_PASS);
		}
		if (0 == lifo->N_violations)
		{
			N_valid_linkages++;
#ifdef USE_FAT_LINKAGES
			if (FALSE == lifo->fat)
				N_thin_linkages++;
#endif /* USE_FAT_LINKAGES */
		}
		lifo->index = indices[in];
		lg_corpus_score(sent, lifo);
		N_linkages_post_processed++;
	}

	print_time(opts, "Postprocessed all linkages");
	qsort((void *)link_info, N_linkages_post_processed, sizeof(Linkage_info),
		  (int (*)(const void *, const void *)) opts->cost_model.compare_fn);

	if (!resources_exhausted(opts->resources))
	{
		if ((N_linkages_post_processed == 0) &&
		    (N_linkages_found > 0) &&
		    (N_linkages_found < opts->linkage_limit))
		{
			/* With the current parser, the following sentence will elicit
			 * this error:
			 *
			 * Well, say, Joe, you can be Friar Tuck or Much the miller's
			 * son, and lam me with a quarter-staff; or I'll be the Sheriff
			 * of Nottingham and you be Robin Hood a little while and kill
			 * me.
			 */
			err_ctxt ec;
			ec.sent = sent;
			err_msg(&ec, Error, "Error: None of the linkages is canonical\n"
			          "\tN_linkages_post_processed=%d "
			          "N_linkages_found=%d\n",
			          N_linkages_post_processed,
			          N_linkages_found);
		}
	}

	if (opts->verbosity > 1)
	{
		err_ctxt ec;
		ec.sent = sent;
		err_msg(&ec, Info, "Info: %d of %d linkages with no P.P. violations\n",
				N_valid_linkages, N_linkages_post_processed);
	}

	print_time(opts, "Sorted all linkages");

	sent->num_linkages_alloced = N_linkages_alloced;
	sent->num_linkages_post_processed = N_linkages_post_processed;
	sent->num_valid_linkages = N_valid_linkages;
#ifdef USE_FAT_LINKAGES
	sent->num_thin_linkages = N_thin_linkages;
#endif /* USE_FAT_LINKAGES */
	sent->link_info = link_info;

	xfree(indices, N_linkages_alloced * sizeof(int));
	/*if(N_valid_linkages == 0) free_andlists(sent); */
}

/***************************************************************
*
* Routines for creating and destroying processing Sentences
*
****************************************************************/

Sentence sentence_create(const char *input_string, Dictionary dict)
{
	Sentence sent;

	sent = (Sentence) xalloc(sizeof(struct Sentence_s));
	memset(sent, 0, sizeof(struct Sentence_s));
	sent->dict = dict;
	sent->length = 0;
	sent->word = NULL;
	sent->post_quote = NULL;
	sent->num_linkages_found = 0;
	sent->num_linkages_alloced = 0;
	sent->num_linkages_post_processed = 0;
	sent->num_valid_linkages = 0;
	sent->link_info = NULL;
	sent->num_valid_linkages = 0;
	sent->null_count = 0;
	sent->parse_info = NULL;
	sent->string_set = string_set_create();

	sent->q_pruned_rules = FALSE;
#ifdef USE_FAT_LINKAGES
	sent->deletable = NULL;
	sent->is_conjunction = NULL;
	sent->dptr = NULL;
	sent->effective_dist = NULL;
#endif /* USE_FAT_LINKAGES */

#ifdef USE_SAT_SOLVER
	sent->hook = NULL;
#endif /* USE_SAT_SOLVER */

	/* Make a copy of the input */
	sent->orig_sentence = string_set_add (input_string, sent->string_set);

	return sent;
}

#ifdef USE_FAT_LINKAGES
/* XXX Extreme hack alert -- English-language words are used
 * completely naked in the C source code!!! FIXME !!!!
 * That's OK, this code is deprecated/obsolete, and will be
 * removed shortly.  We've added the Russian for now, but
 * really, the Russian dictionary needs to be fixed to support
 * conjunctions properly.
 */
static void set_is_conjunction(Sentence sent)
{
	int w;
	const char * s;
	for (w=0; w<sent->length; w++) {
		s = sent->word[w].alternatives[0];
		sent->is_conjunction[w] = 
			/* English, obviously ... */
			(strcmp(s, "and") == 0) ||
			(strcmp(s, "or" ) == 0) ||
			(strcmp(s, "but") == 0) ||
			(strcmp(s, "nor") == 0) ||
			/* Russian */
			(strcmp(s, "и") == 0) ||
			(strcmp(s, "или") == 0) ||
			(strcmp(s, "но") == 0) ||
			(strcmp(s, "ни") == 0);
	}
}
#endif /* USE_FAT_LINKAGES */

int sentence_split(Sentence sent, Parse_Options opts)
{
	Dictionary dict = sent->dict;

	/* Cleanup stuff previously allocated. This is because some free 
	 * routines depend on sent-length, which might change in different
	 * parse-opts settings. 
	 */
#ifdef USE_FAT_LINKAGES
	free_deletable(sent);
#endif /* USE_FAT_LINKAGES */

	/* Tokenize */
	if (!separate_sentence(sent, opts))
	{
		return -1;
	}

	sent->q_pruned_rules = FALSE; /* for post processing */
#ifdef USE_FAT_LINKAGES
	sent->is_conjunction = (char *) xalloc(sizeof(char)*sent->length);
	set_is_conjunction(sent);
	initialize_conjunction_tables(sent);
#endif /* USE_FAT_LINKAGES */

	/* If unknown_word is not defined, then no special processing
	 * will be done for e.g. capitalized words. */
	if (!(dict->unknown_word_defined && dict->use_unknown_word))
	{
		if (!sentence_in_dictionary(sent)) {
			return -2;
		}
	}

	/* Look up each word in the dictionary, collect up all
	 * plausible disjunct expressions for each word.
	 */
	build_sentence_expressions(sent, opts);

	return 0;
}

static void free_sentence_words(Sentence sent)
{
	int i;
	for (i = 0; i < sent->length; i++)
	{
		free_X_nodes(sent->word[i].x);
		free_disjuncts(sent->word[i].d);
		free(sent->word[i].alternatives);
	}
	free(sent->word);
	sent->word = NULL;
	free(sent->post_quote);
}

void sentence_delete(Sentence sent)
{
	if (!sent) return;
	sat_sentence_delete(sent);
#ifdef USE_FAT_LINKAGES
	free_AND_tables(sent);
#endif /* USE_FAT_LINKAGES */
	free_sentence_words(sent);
	string_set_delete(sent->string_set);
	if (sent->parse_info) free_parse_info(sent->parse_info);
	free_post_processing(sent);
	post_process_close_sentence(sent->dict->postprocessor);
#ifdef USE_FAT_LINKAGES
	free_deletable(sent);
	free_effective_dist(sent);
#endif /* USE_FAT_LINKAGES */

	free_count(sent);
	free_analyze(sent);
#ifdef USE_FAT_LINKAGES
	if (sent->is_conjunction) xfree(sent->is_conjunction, sizeof(char)*sent->length);
#endif /* USE_FAT_LINKAGES */
	xfree((char *) sent, sizeof(struct Sentence_s));
}

int sentence_length(Sentence sent)
{
	if (!sent) return 0;
	return sent->length;
}

/**
 * Deprecated; fails to indicate which alternative was
 * actually used in the parse!  Well, OK, this works for
 * ENglish, but is a failure for Russian.
 */
const char * sentence_get_word(Sentence sent, int index)
{
	if (!sent) return NULL;
	if (index >= sent->length) return NULL;
	return sent->word[index].alternatives[0];
}

/**
 * Deprecated; fails to indicate which alternative was
 * actually used in the parse!
 */
const char * sentence_get_nth_word(Sentence sent, int index)
{
	if (!sent) return NULL;
	if (index >= sent->length) return NULL;
	return sent->word[index].alternatives[0];
}

int sentence_null_count(Sentence sent) {
	if (!sent) return 0;
	return sent->null_count;
}

int sentence_num_thin_linkages(Sentence sent) {
	if (!sent) return 0;
#ifdef USE_FAT_LINKAGES
	return sent->num_thin_linkages;
#else
	return sent->num_valid_linkages;
#endif /* USE_FAT_LINKAGES */
}

int sentence_num_linkages_found(Sentence sent) {
	if (!sent) return 0;
	return sent->num_linkages_found;
}

int sentence_num_valid_linkages(Sentence sent) {
	if (!sent) return 0;
	return sent->num_valid_linkages;
}

int sentence_num_linkages_post_processed(Sentence sent) {
	if (!sent) return 0;
	return sent->num_linkages_post_processed;
}

int sentence_num_violations(Sentence sent, int i) {
	if (!sent) return 0;

	/* The sat solver (currently) fails to fill in link_info */
	if (!sent->link_info) return 0;
	return sent->link_info[i].N_violations;
}

int sentence_and_cost(Sentence sent, int i) {
#ifdef USE_FAT_LINKAGES
	if (!sent) return 0;

	/* The sat solver (currently) fails to fill in link_info */
	if (!sent->link_info) return 0;
	return sent->link_info[i].and_cost;
#else
	return 0;
#endif /* USE_FAT_LINKAGES */
}

int sentence_disjunct_cost(Sentence sent, int i) {
	if (!sent) return 0;

	/* The sat solver (currently) fails to fill in link_info */
	if (!sent->link_info) return 0;
	return sent->link_info[i].disjunct_cost;
}

int sentence_link_cost(Sentence sent, int i)
{
	if (!sent) return 0;

	/* The sat solver (currently) fails to fill in link_info */
	if (!sent->link_info) return 0;
	return sent->link_info[i].link_cost;
}

int sentence_nth_word_has_disjunction(Sentence sent, int i)
{
	if (!sent) return 0;
	prt_error("Warning: sentence_nth_word_has_disjunction() is deprecated!\n");
	return (sent->parse_info->chosen_disjuncts[i] != NULL);
}

/* ============================================================== */
/* A kind of morphism post-processing */

#define INFIX_MARK '='

/**
 * This routine solves the problem of mis-link stem + suffix.
 * It checks that the actual stem+suffix, when concatenated, restores
 * the original word.  This is a work-around for somewhat sloppy
 * dictionaries, which aren't entirely careful with making sure
 * that the stem+suffix links are unique. Here's an illustrartion of
 * the problem: The word Russian "тест" can be split into 
 * тест.= =.ndmsi and also тес.= =т.amss The, during linkage, 
 * тес.= and =.ndmsi are linked -- but this is not the original word; 
 * its junk.  This routine marks such linkages as 'bad'.  Really, the
 * dictionary should eb fixed ,, but what the hell. This isn't hard.
 */
static void sane_morphism(Sentence sent, Parse_Options opts)
{
	int lk, i;
	Parse_info pi = sent->parse_info;

	for (lk = 0; lk < sent->num_linkages_alloced; lk++)
	{
		Linkage_info *lifo = &sent->link_info[lk];
		extract_links(lifo->index, sent->null_count, pi);
		for (i=0; i<sent->length; i++)
		{
			const char *djw;
			size_t len;
			const char *unsplit = sent->word[i].unsplit_word;
			size_t unlen;

			/* Ignore island words */
			if (NULL == pi->chosen_disjuncts[i])
				continue;

			/* Ignore suffixes, for now */
			if (NULL == unsplit)
				continue;

			/* If its a perfect match, then keep going */
			unlen = strlen(unsplit);
			djw = pi->chosen_disjuncts[i]->string;
			if (0 == strncmp(djw, unsplit, unlen))
				continue;

			/* Perhaps its a perfect match after capitalization. */
			if (sent->word[i].firstupper)
			{
				char temp_word[MAX_WORD+1];
				downcase_utf8_str(temp_word, unsplit, MAX_WORD);
				if (0 == strncmp(djw, temp_word, strlen(temp_word)))
					continue;
			}

			/* If the next word is a suffix, and, when united with
			 * the stem, it recreates the original word, then we
			 * are very happy, and keep going. */
			len = strlen(djw);
			if ((INFIX_MARK == djw[len-1]) &&
			    ((i+1) < sent->length) && 
			    (NULL != pi->chosen_disjuncts[i+1]) &&
			    (INFIX_MARK == pi->chosen_disjuncts[i+1]->string[0]))
			{
				char *p;
				char newword[MAX_WORD+1];
				strcpy(newword, djw);
				p = strrchr(newword, '.');
				if (p) *p = 0x0;
				strcat(newword, pi->chosen_disjuncts[i+1]->string+1);

				/* OK, we built the concatenation .. does it match? */
				if (0 == strncmp(newword, unsplit, unlen))
					continue;

				/* If we are here, it didn't match. Is that because of
				 * capitalization? Lets check. */
				if (sent->word[i].firstupper)
				{
					char temp_word[MAX_WORD+1];
					downcase_utf8_str(temp_word, unsplit, MAX_WORD);
					if (0 == strncmp(newword, temp_word, strlen(temp_word)))
						continue;
				}
			}

			/* Oh no ... we've joined together the stem and suffix incorrectly! */
			sent->num_valid_linkages --;
			lifo->N_violations ++;
			break;
		}
	}
}

static void chart_parse(Sentence sent, Parse_Options opts)
{
	int nl;

	/* Build lists of disjuncts */
	prepare_to_parse(sent, opts);

	init_fast_matcher(sent);
	init_count(sent);

	/* A parse set may have been already been built for this sentence,
	 * if it was previously parsed.  If so we free it up before
	 * building another.  */
	if (sent->parse_info) free_parse_info(sent->parse_info);
	sent->parse_info = parse_info_new(sent->length);

	for (nl = opts->min_null_count; nl<=opts->max_null_count ; ++nl)
	{
		s64 total;
		if (resources_exhausted(opts->resources)) break;
		sent->null_count = nl;
		total = do_parse(sent, sent->null_count, opts);

		if (verbosity > 1)
		{
			prt_error("Info: Total count with %d null links:   %lld\n",
				sent->null_count, total);
		}

		/* Give up if the parse count is overflowing */
		if (PARSE_NUM_OVERFLOW < total)
		{
			if (verbosity > 0)
			{
				prt_error("WARNING: Combinatorial explosion! nulls=%d cnt=%lld\n"
					"Consider retrying the parse with the max allowed disjunct cost set lower.\n",
					sent->null_count, total);
			}
			total = (total>INT_MAX) ? INT_MAX : total;
		}

		sent->num_linkages_found = (int) total;
		print_time(opts, "Counted parses");

		post_process_linkages(sent, opts);
		sane_morphism(sent, opts);
		if (sent->num_valid_linkages > 0) break;

		/* If we are here, then no valid linakges were found.
		 * If there was a parse overflow, give up now. */
		if (PARSE_NUM_OVERFLOW < total) break;
	}

	free_count(sent);
	free_fast_matcher(sent);
}

static void free_sentence_disjuncts(Sentence sent)
{
	int i;

	for (i = 0; i < sent->length; ++i)
	{
		free_disjuncts(sent->word[i].d);
		sent->word[i].d = NULL;
	}
}

int sentence_parse(Sentence sent, Parse_Options opts)
{
	int rc;

	verbosity = opts->verbosity;

	/* If the sentence has not yet been split, do so now.
	 * This is for backwards compatibility, for existing programs
	 * that do not explicitly call the splitter.
	 */
	if (0 == sent->length)
	{
		rc = sentence_split(sent, opts);
		if (rc) return -1;
	}

	/* Check for bad sentence length */
	if (MAX_SENTENCE <= sent->length)
	{
		prt_error("Error: sentence too long, contains more than %d words\n", 
			MAX_SENTENCE);
		return -2;
	}

	/* Initialize/free any leftover garbage */
	free_sentence_disjuncts(sent);  /* Is this really needed ??? */
#ifdef USE_FAT_LINKAGES
	if (sentence_contains_conjunction(sent)) free_AND_tables(sent);
#endif /* USE_FAT_LINKAGES */
	resources_reset_space(opts->resources);

	if (resources_exhausted(opts->resources)) {
		sent->num_valid_linkages = 0;
		return 0;
	}

	init_analyze(sent);

	/* Expressions were previously set up during the tokenize stage. */
	expression_prune(sent);
	print_time(opts, "Finished expression pruning");
	if (opts->use_sat_solver)
	{
		sat_parse(sent, opts);
	}
	else
	{
		chart_parse(sent, opts);
	}
	print_time(opts, "Finished parse");

	return sent->num_valid_linkages;
}

/***************************************************************
*
* Routines which allow user access to Linkages.
*
****************************************************************/

Linkage linkage_create(int k, Sentence sent, Parse_Options opts)
{
	Linkage linkage;

	if (opts->use_sat_solver)
	{
		return sat_create_linkage(k, sent, opts);
	}

	if ((k >= sent->num_linkages_post_processed) || (k < 0)) return NULL;

	/* Using exalloc since this is external to the parser itself. */
	linkage = (Linkage) exalloc(sizeof(struct Linkage_s));

	linkage->num_words = sent->length;
	linkage->word = (const char **) exalloc(linkage->num_words*sizeof(char *));
	linkage->sent = sent;
	linkage->opts = opts;
	linkage->info = &sent->link_info[k];
#ifdef USE_FAT_LINKAGES
	linkage->sublinkage = NULL;
	linkage->unionized = FALSE;
	linkage->current = 0;
	linkage->num_sublinkages=0;
	linkage->dis_con_tree = NULL;
#endif /* USE_FAT_LINKAGES */

	extract_links(sent->link_info[k].index, sent->null_count, sent->parse_info);
	compute_chosen_words(sent, linkage);

#ifdef USE_FAT_LINKAGES
	if (set_has_fat_down(sent))
	{
		extract_fat_linkage(sent, opts, linkage);
	}
	else
#endif /* USE_FAT_LINKAGES */
	{
		extract_thin_linkage(sent, opts, linkage);
	}

	if (sent->dict->postprocessor != NULL)
	{
	   linkage_post_process(linkage, sent->dict->postprocessor);
	}

	return linkage;
}

int linkage_get_current_sublinkage(const Linkage linkage)
{ 
#ifdef USE_FAT_LINKAGES
	return linkage->current; 
#else
	return 0;
#endif /* USE_FAT_LINKAGES */
} 

int linkage_set_current_sublinkage(Linkage linkage, int index)
{
#ifdef USE_FAT_LINKAGES
	if ((index < 0) ||
		(index >= linkage->num_sublinkages))
	{
		return 0;
	}
	linkage->current = index;
#endif /* USE_FAT_LINKAGES */
	return 1;
}

static void exfree_pp_info(PP_info *ppi)
{
	if (ppi->num_domains > 0)
		exfree(ppi->domain_name, sizeof(const char *)*ppi->num_domains);
	ppi->domain_name = NULL;
	ppi->num_domains = 0;
}

void linkage_delete(Linkage linkage)
{
	int i, j;
	Sublinkage *s;

	/* Can happen on panic timeout or user error */
	if (NULL == linkage) return;

	for (i = 0; i < linkage->num_words; ++i)
	{
		exfree((void *) linkage->word[i], strlen(linkage->word[i])+1);
	}
	exfree(linkage->word, sizeof(char *)*linkage->num_words);

#ifdef USE_FAT_LINKAGES
	for (i = 0; i < linkage->num_sublinkages; ++i)
#endif /* USE_FAT_LINKAGES */
	{
#ifdef USE_FAT_LINKAGES
		s = &(linkage->sublinkage[i]);
#else
		s = &(linkage->sublinkage);
#endif /* USE_FAT_LINKAGES */
		for (j = 0; j < s->num_links; ++j) {
			exfree_link(s->link[j]);
		}
		exfree(s->link, sizeof(Link) * s->num_links);
		if (s->pp_info != NULL) {
			for (j = 0; j < s->num_links; ++j) {
				exfree_pp_info(&s->pp_info[j]);
			}
			exfree(s->pp_info, sizeof(PP_info) * s->num_links);
			s->pp_info = NULL;
			post_process_free_data(&s->pp_data);
		}
		if (s->violation != NULL) {
			exfree((void *) s->violation, sizeof(char) * (strlen(s->violation)+1));
		}
	}
#ifdef USE_FAT_LINKAGES
	exfree(linkage->sublinkage, sizeof(Sublinkage)*linkage->num_sublinkages);
#endif /* USE_FAT_LINKAGES */

#ifdef USE_FAT_LINKAGES
	if (linkage->dis_con_tree)
		free_DIS_tree(linkage->dis_con_tree);
#endif /* USE_FAT_LINKAGES */
	exfree(linkage, sizeof(struct Linkage_s));
}

#ifdef USE_FAT_LINKAGES
static int links_are_equal(Link *l, Link *m)
{
	return ((l->l == m->l) && (l->r == m->r) && (strcmp(l->name, m->name)==0));
}

static int link_already_appears(Linkage linkage, Link *link, int a)
{
	int i, j;

	for (i=0; i<a; ++i) {
		for (j=0; j<linkage->sublinkage[i].num_links; ++j) {
			if (links_are_equal(linkage->sublinkage[i].link[j], link)) return TRUE;
		}
	}
	return FALSE;
}

static PP_info excopy_pp_info(PP_info ppi)
{
	PP_info newppi;
	int i;

	newppi.num_domains = ppi.num_domains;
	newppi.domain_name = (const char **) exalloc(sizeof(const char *)*ppi.num_domains);
	for (i=0; i<newppi.num_domains; ++i)
	{
		newppi.domain_name[i] = ppi.domain_name[i];
	}
	return newppi;
}

static Sublinkage unionize_linkage(Linkage linkage)
{
	int i, j, num_in_union=0;
	Sublinkage u;
	Link *link;
	const char *p;

	for (i=0; i<linkage->num_sublinkages; ++i) {
		for (j=0; j<linkage->sublinkage[i].num_links; ++j) {
			link = linkage->sublinkage[i].link[j];
			if (!link_already_appears(linkage, link, i)) num_in_union++;
		}
	}

	u.link = (Link **) exalloc(sizeof(Link *)*num_in_union);
	u.num_links = num_in_union;
	zero_sublinkage(&u);

	u.pp_info = (PP_info *) exalloc(sizeof(PP_info)*num_in_union);
	u.violation = NULL;
	u.num_links = num_in_union;

	num_in_union = 0;

	for (i=0; i<linkage->num_sublinkages; ++i) {
		for (j=0; j<linkage->sublinkage[i].num_links; ++j) {
			link = linkage->sublinkage[i].link[j];
			if (!link_already_appears(linkage, link, i)) {
				u.link[num_in_union] = excopy_link(link);
				u.pp_info[num_in_union] = excopy_pp_info(linkage->sublinkage[i].pp_info[j]);
				if (((p=linkage->sublinkage[i].violation) != NULL) &&
					(u.violation == NULL)) {
					char *s = (char *) exalloc((strlen(p)+1)*sizeof(char));
					strcpy(s, p);
					u.violation = s;
				}
				num_in_union++;
			}
		}
	}

	return u;
}
#endif /* USE_FAT_LINKAGES */

int linkage_compute_union(Linkage linkage)
{
#ifdef USE_FAT_LINKAGES
	int i, num_subs=linkage->num_sublinkages;
	Sublinkage * new_sublinkage, *s;

	if (linkage->unionized) {
		linkage->current = linkage->num_sublinkages-1;
		return 0;
	}
	if (num_subs == 1) {
		linkage->unionized = TRUE;
		return 1;
	}

	new_sublinkage =
		(Sublinkage *) exalloc(sizeof(Sublinkage)*(num_subs+1));

	for (i=0; i<num_subs; ++i) {
		new_sublinkage[i] = linkage->sublinkage[i];
	}
	exfree(linkage->sublinkage, sizeof(Sublinkage)*num_subs);
	linkage->sublinkage = new_sublinkage;

	/* Zero out the new sublinkage, then unionize it. */
	s = &new_sublinkage[num_subs];
	s->link = NULL;
	s->num_links = 0;
	zero_sublinkage(s);
	linkage->sublinkage[num_subs] = unionize_linkage(linkage);

	linkage->num_sublinkages++;

	linkage->unionized = TRUE;
	linkage->current = linkage->num_sublinkages-1;
	return 1;
#else
	return 0;
#endif /* USE_FAT_LINKAGES */
}

int linkage_get_num_sublinkages(const Linkage linkage)
{
#ifdef USE_FAT_LINKAGES
	return linkage->num_sublinkages;
#else
	return 1;
#endif /* USE_FAT_LINKAGES */
}

int linkage_get_num_words(const Linkage linkage)
{
	return linkage->num_words;
}

int linkage_get_num_links(const Linkage linkage)
{
#ifdef USE_FAT_LINKAGES
	int current = linkage->current;
	return linkage->sublinkage[current].num_links;
#else
	return linkage->sublinkage.num_links;
#endif /* USE_FAT_LINKAGES */
}

static inline int verify_link_index(const Linkage linkage, int index)
{
	if ((index < 0) ||
#ifdef USE_FAT_LINKAGES
		(index >= linkage->sublinkage[linkage->current].num_links))
#else
		(index >= linkage->sublinkage.num_links))
#endif /* USE_FAT_LINKAGES */
	{
		return 0;
	}
	return 1;
}

int linkage_get_link_length(const Linkage linkage, int index)
{
	Link *link;
	int word_has_link[MAX_SENTENCE];
	int i, length;
#ifdef USE_FAT_LINKAGES
	int current = linkage->current;
#endif /* USE_FAT_LINKAGES */

	if (!verify_link_index(linkage, index)) return -1;

	for (i=0; i<linkage->num_words+1; ++i) {
		word_has_link[i] = FALSE;
	}

#ifdef USE_FAT_LINKAGES
	for (i=0; i<linkage->sublinkage[current].num_links; ++i)
	{
		link = linkage->sublinkage[current].link[i];
		word_has_link[link->l] = TRUE;
		word_has_link[link->r] = TRUE;
	}
	link = linkage->sublinkage[current].link[index];
#else
	for (i=0; i<linkage->sublinkage.num_links; ++i)
	{
		link = linkage->sublinkage.link[i];
		word_has_link[link->l] = TRUE;
		word_has_link[link->r] = TRUE;
	}
	link = linkage->sublinkage.link[index];
#endif /* USE_FAT_LINKAGES */

	length = link->r - link->l;
	for (i= link->l+1; i < link->r; ++i) {
		if (!word_has_link[i]) length--;
	}
	return length;
}

int linkage_get_link_lword(const Linkage linkage, int index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return -1;
#ifdef USE_FAT_LINKAGES
	link = linkage->sublinkage[linkage->current].link[index];
#else
	link = linkage->sublinkage.link[index];
#endif /* USE_FAT_LINKAGES */
	return link->l;
}

int linkage_get_link_rword(const Linkage linkage, int index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return -1;
#ifdef USE_FAT_LINKAGES
	link = linkage->sublinkage[linkage->current].link[index];
#else
	link = linkage->sublinkage.link[index];
#endif /* USE_FAT_LINKAGES */
	return link->r;
}

const char * linkage_get_link_label(const Linkage linkage, int index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return NULL;
#ifdef USE_FAT_LINKAGES
	link = linkage->sublinkage[linkage->current].link[index];
#else
	link = linkage->sublinkage.link[index];
#endif /* USE_FAT_LINKAGES */
	return link->name;
}

const char * linkage_get_link_llabel(const Linkage linkage, int index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return NULL;
#ifdef USE_FAT_LINKAGES
	link = linkage->sublinkage[linkage->current].link[index];
#else
	link = linkage->sublinkage.link[index];
#endif /* USE_FAT_LINKAGES */
	return link->lc->string;
}

const char * linkage_get_link_rlabel(const Linkage linkage, int index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return NULL;
#ifdef USE_FAT_LINKAGES
	link = linkage->sublinkage[linkage->current].link[index];
#else
	link = linkage->sublinkage.link[index];
#endif /* USE_FAT_LINKAGES */
	return link->rc->string;
}

const char ** linkage_get_words(const Linkage linkage)
{
	return linkage->word;
}

Sentence linkage_get_sentence(const Linkage linkage)
{
	return linkage->sent;
}

const char * linkage_get_disjunct_str(const Linkage linkage, int w)
{
	Disjunct *dj;

	if (NULL == linkage->info->disjunct_list_str)
	{
		lg_compute_disjunct_strings(linkage->sent, linkage->info);
	}

	/* dj will be null if the word wasn't used in the parse. */
	dj = linkage->sent->parse_info->chosen_disjuncts[w];
	if (NULL == dj) return "";

	return linkage->info->disjunct_list_str[w];
}

double linkage_get_disjunct_cost(const Linkage linkage, int w)
{
	Disjunct *dj = linkage->sent->parse_info->chosen_disjuncts[w];

	/* dj may be null, if the word didn't participate in the parse. */
	if (dj) return dj->cost;
	return 0.0;
}

double linkage_get_disjunct_corpus_score(const Linkage linkage, int w)
{
	Disjunct *dj = linkage->sent->parse_info->chosen_disjuncts[w];

	/* dj may be null, if the word didn't participate in the parse. */
	if (NULL == dj) return 99.999;

	return lg_corpus_disjunct_score(linkage, w); 
}

const char * linkage_get_word(const Linkage linkage, int w)
{
	return linkage->word[w];
}

int linkage_unused_word_cost(const Linkage linkage)
{
	/* The sat solver (currently) fails to fill in info */
	if (!linkage->info) return 0;
	return linkage->info->unused_word_cost;
}

int linkage_disjunct_cost(const Linkage linkage)
{
	/* The sat solver (currently) fails to fill in info */
	if (!linkage->info) return 0;
	return (int) floorf(linkage->info->disjunct_cost);
}

int linkage_is_fat(const Linkage linkage)
{
#ifdef USE_FAT_LINKAGES
	/* The sat solver (currently) fails to fill in info */
	if (!linkage->info) return 0;
	return linkage->info->fat;
#else
	return FALSE;
#endif /* USE_FAT_LINKAGES */
}

int linkage_and_cost(Linkage linkage)
{
#ifdef USE_FAT_LINKAGES
	/* The sat solver (currently) fails to fill in info */
	if (!linkage->info) return 0;
	return linkage->info->and_cost;
#else
	return 0;
#endif /* USE_FAT_LINKAGES */
}

int linkage_link_cost(const Linkage linkage)
{
	/* The sat solver (currently) fails to fill in info */
	if (!linkage->info) return 0;
	return linkage->info->link_cost;
}

double linkage_corpus_cost(const Linkage linkage)
{
	/* The sat solver (currently) fails to fill in info */
	if (!linkage->info) return 0.0;
	return linkage->info->corpus_cost;
}

int linkage_get_link_num_domains(const Linkage linkage, int index)
{
	PP_info *pp_info;
	if (!verify_link_index(linkage, index)) return -1;
#ifdef USE_FAT_LINKAGES
	pp_info = &linkage->sublinkage[linkage->current].pp_info[index];
#else
	pp_info = &linkage->sublinkage.pp_info[index];
#endif /* USE_FAT_LINKAGES */
	return pp_info->num_domains;
}

const char ** linkage_get_link_domain_names(const Linkage linkage, int index)
{
	PP_info *pp_info;
	if (!verify_link_index(linkage, index)) return NULL;
#ifdef USE_FAT_LINKAGES
	pp_info = &linkage->sublinkage[linkage->current].pp_info[index];
#else
	pp_info = &linkage->sublinkage.pp_info[index];
#endif /* USE_FAT_LINKAGES */
	return pp_info->domain_name;
}

const char * linkage_get_violation_name(const Linkage linkage)
{
#ifdef USE_FAT_LINKAGES
	return linkage->sublinkage[linkage->current].violation;
#else
	return linkage->sublinkage.violation;
#endif /* USE_FAT_LINKAGES */
}

int linkage_is_canonical(const Linkage linkage)
{
#ifdef USE_FAT_LINKAGES
	/* The sat solver (currently) fails to fill in info */
	if (!linkage->info) return TRUE;
	return linkage->info->canonical;
#else
	return TRUE;
#endif /* USE_FAT_LINKAGES */
}

int linkage_is_improper(const Linkage linkage)
{
#ifdef USE_FAT_LINKAGES
	/* The sat solver (currently) fails to fill in info */
	if (!linkage->info) return FALSE;
	return linkage->info->improper_fat_linkage;
#else
	return FALSE;
#endif /* USE_FAT_LINKAGES */
}

int linkage_has_inconsistent_domains(const Linkage linkage)
{
#ifdef USE_FAT_LINKAGES
	/* The sat solver (currently) fails to fill in info */
	if (!linkage->info) return FALSE;
	return linkage->info->inconsistent_domains;
#else
	return FALSE;
#endif /* USE_FAT_LINKAGES */
}

void linkage_post_process(Linkage linkage, Postprocessor * postprocessor)
{
#ifdef USE_FAT_LINKAGES
	int N_sublinkages = linkage_get_num_sublinkages(linkage);
#endif /* USE_FAT_LINKAGES */
	Parse_Options opts = linkage->opts;
	Sentence sent = linkage->sent;
	Sublinkage * subl;
	PP_node * pp;
	int j, k;
	D_type_list * d;

#ifdef USE_FAT_LINKAGES
	int i;
	for (i = 0; i < N_sublinkages; ++i)
#endif /* USE_FAT_LINKAGES */
	{
#ifdef USE_FAT_LINKAGES
		subl = &linkage->sublinkage[i];
#else
		subl = &linkage->sublinkage;
#endif /* USE_FAT_LINKAGES */
		if (subl->pp_info != NULL)
		{
			for (j = 0; j < subl->num_links; ++j)
			{
				exfree_pp_info(&subl->pp_info[j]);
			}
			post_process_free_data(&subl->pp_data);
			exfree(subl->pp_info, sizeof(PP_info) * subl->num_links);
		}
		subl->pp_info = (PP_info *) exalloc(sizeof(PP_info) * subl->num_links);
		for (j = 0; j < subl->num_links; ++j)
		{
			subl->pp_info[j].num_domains = 0;
			subl->pp_info[j].domain_name = NULL;
		}
		if (subl->violation != NULL)
		{
			exfree((void *)subl->violation, sizeof(char) * (strlen(subl->violation)+1));
			subl->violation = NULL;
		}

#ifdef USE_FAT_LINKAGES
		if (linkage->info->improper_fat_linkage)
		{
			pp = NULL;
		}
		else
#endif /* USE_FAT_LINKAGES */
		{
			pp = post_process(postprocessor, opts, sent, subl, FALSE);
			/* This can return NULL, for example if there is no
			   post-processor */
		}

		if (pp == NULL)
		{
			for (j = 0; j < subl->num_links; ++j)
			{
				subl->pp_info[j].num_domains = 0;
				subl->pp_info[j].domain_name = NULL;
			}
		}
		else
		{
			for (j = 0; j < subl->num_links; ++j)
			{
				k = 0;
				for (d = pp->d_type_array[j]; d != NULL; d = d->next) k++;
				subl->pp_info[j].num_domains = k;
				if (k > 0)
				{
					subl->pp_info[j].domain_name = (const char **) exalloc(sizeof(const char *)*k);
				}
				k = 0;
				for (d = pp->d_type_array[j]; d != NULL; d = d->next)
				{
					char buff[5];
					sprintf(buff, "%c", d->type);
					subl->pp_info[j].domain_name[k] = string_set_add (buff, sent->string_set);

					k++;
				}
			}
			subl->pp_data = postprocessor->pp_data;
			if (pp->violation != NULL)
			{
				char * s = (char *) exalloc(sizeof(char)*(strlen(pp->violation)+1));
				strcpy(s, pp->violation);
				subl->violation = s;
			}
		}
	}
	post_process_close_sentence(postprocessor);
}

