/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright 2008, 2009, 2013, 2014 Linas Vepstas                        */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

#include "analyze-linkage.h"
#include "corpus/corpus.h"
#include "count.h"
#include "dict-common.h"
#include "disjunct-utils.h"
#include "error.h"
#include "externs.h"
#include "extract-links.h"
#include "fast-match.h"
#include "linkage.h"
#include "post-process.h"
#include "preparation.h"
#include "print.h"
#include "prune.h"
#include "regex-morph.h"
#include "resources.h"
#include "score.h"
#include "sat-solver/sat-encoder.h"
#include "spellcheck.h"
#include "string-set.h"
#include "structures.h"
#include "tokenize.h"
#include "utilities.h"
#include "wordgraph.h"
#include "word-utils.h"

/***************************************************************
*
* Routines for setting Parse_Options
*
****************************************************************/

/**
 * For sorting the linkages in postprocessing
 */

static int VDAL_compare_parse(Linkage l1, Linkage l2)
{
	Linkage_info * p1 = &l1->lifo;
	Linkage_info * p2 = &l2->lifo;

	/* Move the discarded entries to the end of the list */
	if (p1->discarded || p2->discarded) return (p1->discarded - p2->discarded);

	if (p1->N_violations != p2->N_violations) {
		return (p1->N_violations - p2->N_violations);
	}
	else if (p1->unused_word_cost != p2->unused_word_cost) {
		return (p1->unused_word_cost - p2->unused_word_cost);
	}
	else if (p1->disjunct_cost > p2->disjunct_cost) return 1;
	else if (p1->disjunct_cost < p2->disjunct_cost) return -1;
	else {
		return (p1->link_cost - p2->link_cost);
	}
}

#ifdef USE_CORPUS
static int CORP_compare_parse(Linkage l1, Linkage l2)
{
	Linkage_info * p1 = &l1->lifo;
	Linkage_info * p2 = &l2->lifo;

	double diff = p1->corpus_cost - p2->corpus_cost;

	/* Move the discarded entries to the end of the list */
	if (p1->discarded || p2->discarded) return (p1->discarded - p2->discarded);

	if (fabs(diff) < 1.0e-5)
		return VDAL_compare_parse(p1, p2);
	if (diff < 0.0) return -1;
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

	/* The parse_options_set_(verbosity|debug|test) functions set also the
	 * corresponding global variables. So these globals are initialized
	 * here too. */
	verbosity = po->verbosity = 1;
	debug = po->debug = (char *)"";
	test = po->test = (char *)"";

	/* A cost of 2.7 allows the usual cost-2 connectors, plus the
	 * assorted fractional costs, without going to cost 3.0, which
	 * is used only during panic-parsing.
	 */
	po->disjunct_cost = 2.7;
	po->min_null_count = 0;
	po->max_null_count = 0;
	po->islands_ok = false;
	po->use_spell_guess = 7;
	po->use_sat_solver = false;
	po->use_viterbi = false;
	po->linkage_limit = 100;

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
	po->short_length = 16;
	po->all_short = false;
	po->twopass_length = 30;
	po->repeatable_rand = true;
	po->resources = resources_create();
	po->use_cluster_disjuncts = false;
	po->display_morphology = false;

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

void parse_options_set_debug(Parse_Options opts, const char * dummy)
{
	/* The comma-separated list of functions is limited to this size.
	 * Can be easily dynamically allocated. In any case it is not reentrant
	 * because the "debug" variable is static. */
	static char buff[256];
	size_t len = strlen(dummy);

	if (0 == strcmp(dummy, opts->debug)) return;

	if (0 == len)
	{
		buff[0] = '\0';
	}
	else
	{
		buff[0] = ',';
		strncpy(buff+1, dummy, sizeof(buff)-2);
		if (len < sizeof(buff)-2)
		{
			buff[len+1] = ',';
			buff[len+2] = '\0';
		}
		else
		{
			buff[sizeof(buff)-1] = '\0';
		}
	}
	opts->debug = buff;
	debug = opts->debug;
	/* this is one of the only global variables. */
}

char * parse_options_get_debug(Parse_Options opts) {
	return opts->debug;
}

void parse_options_set_test(Parse_Options opts, const char * dummy)
{
	/* The comma-separated test features is limited to this size.
	 * Can be easily dynamically allocated. In any case it is not reentrant
	 * because the "test" variable is static. */
	static char buff[256];
	size_t len = strlen(dummy);

	if (0 == strcmp(dummy, opts->test)) return;

	if (0 == len)
	{
		buff[0] = '\0';
	}
	else
	{
		buff[0] = ',';
		strncpy(buff+1, dummy, sizeof(buff)-2);
		if (len < sizeof(buff)-2)
		{
			buff[len+1] = ',';
			buff[len+2] = '\0';
		}
		else
		{
			buff[sizeof(buff)-1] = '\0';
		}
	}
	opts->test = buff;
	test = opts->test;
	/* this is one of the only global variables. */
}

char * parse_options_get_test(Parse_Options opts) {
	return opts->test;
}

void parse_options_set_use_sat_parser(Parse_Options opts, bool dummy) {
#ifdef USE_SAT_SOLVER
	opts->use_sat_solver = dummy;
#else
	if (dummy)
		prt_error("Error: cannot enable the Boolean SAT parser; this"
					 " library was built without SAT solver support.");
#endif
}

bool parse_options_get_use_sat_parser(Parse_Options opts) {
	return opts->use_sat_solver;
}

void parse_options_set_use_viterbi(Parse_Options opts, bool dummy) {
	opts->use_viterbi = dummy;
}

bool parse_options_get_use_viterbi(Parse_Options opts) {
	return opts->use_viterbi;
}

void parse_options_set_linkage_limit(Parse_Options opts, int dummy)
{
	opts->linkage_limit = dummy;
}
int parse_options_get_linkage_limit(Parse_Options opts)
{
	return opts->linkage_limit;
}

void parse_options_set_disjunct_cost(Parse_Options opts, double dummy)
{
	opts->disjunct_cost = dummy;
}
double parse_options_get_disjunct_cost(Parse_Options opts)
{
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

void parse_options_set_islands_ok(Parse_Options opts, bool dummy) {
	opts->islands_ok = dummy;
}

bool parse_options_get_islands_ok(Parse_Options opts) {
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

void parse_options_set_all_short_connectors(Parse_Options opts, bool val) {
	opts->all_short = val;
}

bool parse_options_get_all_short_connectors(Parse_Options opts) {
	return opts->all_short;
}

void parse_options_set_repeatable_rand(Parse_Options opts, bool val) {
	opts->repeatable_rand = val;
}

bool parse_options_get_repeatable_rand(Parse_Options opts) {
	return opts->repeatable_rand;
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

void parse_options_set_use_cluster_disjuncts(Parse_Options opts, bool dummy) {
	opts->use_cluster_disjuncts = dummy;
}

bool parse_options_get_use_cluster_disjuncts(Parse_Options opts) {
	return opts->use_cluster_disjuncts;
}

int parse_options_get_display_morphology(Parse_Options opts) {
	return opts->display_morphology;
}

void parse_options_set_display_morphology(Parse_Options opts, int dummy) {
	opts->display_morphology = dummy;
}

bool parse_options_timer_expired(Parse_Options opts) {
	return resources_timer_expired(opts->resources);
}

bool parse_options_memory_exhausted(Parse_Options opts) {
	return resources_memory_exhausted(opts->resources);
}

bool parse_options_resources_exhausted(Parse_Options opts) {
	return (resources_exhausted(opts->resources));
}

void parse_options_reset_resources(Parse_Options opts) {
	resources_reset(opts->resources);
}

/***************************************************************
*
* Routines for postprocessing
*
****************************************************************/

static Linkage linkage_array_new(int num_to_alloc)
{
	Linkage lkgs = (Linkage) exalloc(num_to_alloc * sizeof(struct Linkage_s));
	memset(lkgs, 0, num_to_alloc * sizeof(struct Linkage_s));
	return lkgs;
}

void free_linkage(Linkage linkage)
{
		size_t j;

		exfree((void *) linkage->word, sizeof(const char *) * linkage->num_words);
		exfree(linkage->chosen_disjuncts, linkage->num_words * sizeof(Disjunct *));
		free(linkage->link_array);

		/* Q: Why isn't this in a string set ?? A: Because there is no
		 * string-set handy when we compute this. */
		if (linkage->disjunct_list_str)
		{
			for (j=0; j<linkage->num_words; j++)
			{
				if (linkage->disjunct_list_str[j])
					free(linkage->disjunct_list_str[j]);
			}
			free(linkage->disjunct_list_str);
		}
#ifdef USE_CORPUS
		lg_sense_delete(linkage);
#endif

		linkage_free_pp_info(linkage);

		/* XXX FIXME */
		free(linkage->wg_path);
		free(linkage->wg_path_display);
}

static void free_linkages(Sentence sent)
{
	size_t in;
	Linkage lkgs = sent->lnkages;
	if (!lkgs) return;

	for (in=0; in<sent->num_linkages_alloced; in++)
	{
		free_linkage(&lkgs[in]);
	}

	exfree(lkgs, sent->num_linkages_alloced * sizeof(struct Linkage_s));
	sent->num_linkages_alloced = 0;
	sent->num_linkages_found = 0;
	sent->num_linkages_post_processed = 0;
	sent->num_valid_linkages = 0;
	sent->lnkages = NULL;
}

static void select_linkages(Sentence sent, fast_matcher_t* mchxt,
                            count_context_t* ctxt,
                            Parse_Options opts)
{
	size_t in;
	size_t N_linkages_found, N_linkages_alloced;

	bool overflowed = build_parse_set(sent, mchxt, ctxt, sent->null_count, opts);
	print_time(opts, "Built parse set");

	if (overflowed && (1 < opts->verbosity))
	{
		err_ctxt ec;
		ec.sent = sent;
		err_msg(&ec, Warn, "Warning: Count overflow.\n"
		  "Considering a random subset of %zu of an unknown and large number of linkages\n",
			opts->linkage_limit);
	}
	N_linkages_found = sent->num_linkages_found;

	if (sent->num_linkages_found == 0)
	{
		sent->num_linkages_alloced = 0;
		sent->num_linkages_post_processed = 0;
		sent->num_valid_linkages = 0;
		sent->lnkages = NULL;
		return;
	}

	if (N_linkages_found > opts->linkage_limit)
	{
		N_linkages_alloced = opts->linkage_limit;
		if (opts->verbosity > 1)
		{
			err_ctxt ec;
			ec.sent = sent;
			err_msg(&ec, Warn,
			    "Warning: Considering a random subset of %zu of %zu linkages\n",
			    N_linkages_alloced, N_linkages_found);
		}
	}
	else
	{
		N_linkages_alloced = N_linkages_found;
	}

	/* Now actually malloc the array in which we will process linkages. */
	/* We may have been called before, e.g this might be a panic parse,
	 * and the linkages array may still be there from last time.
	 * XXX free_linkages() zeros sent->num_linkages_found. */
	if (sent->lnkages) free_linkages(sent);
	sent->num_linkages_found = N_linkages_found;
	sent->lnkages = linkage_array_new(N_linkages_alloced);

	/* Generate an array of linkage indices to examine */
	if (overflowed)
	{
		/* The negative index means that a random subset of links
		 * will be picked later on, in extract_links(). */
		for (in=0; in < N_linkages_alloced; in++)
		{
			sent->lnkages[in].lifo.index = -(in+1);
		}
	}
	else if (N_linkages_found == N_linkages_alloced)
	{
		for (in=0; in<N_linkages_alloced; in++)
			sent->lnkages[in].lifo.index = in;
	}
	else
	{
		/* There are more linkages found than we can handle */
		/* Pick a (quasi-)uniformly distributed random subset. */
		if (opts->repeatable_rand)
			sent->rand_state = N_linkages_found + sent->length;

		for (in=0; in<N_linkages_alloced; in++)
		{
			size_t block_bottom, block_top;
			double frac = (double) N_linkages_found;

			frac /= (double) N_linkages_alloced;
			block_bottom = (int) (((double) in) * frac);
			block_top = (int) (((double) (in+1)) * frac);
			sent->lnkages[in].lifo.index = block_bottom +
				(rand_r(&sent->rand_state) % (block_top-block_bottom));
		}
	}

	sent->num_linkages_alloced = N_linkages_alloced;
	/* Later, we subtract the number of invalid linkages */
	sent->num_valid_linkages = N_linkages_alloced;
}

/* Partial, but not full initialization of the linakge struct ... */
void partial_init_linkage(Linkage lkg, unsigned int N_words)
{
	lkg->num_links = 0;
	lkg->lasz = 2 * N_words;
	lkg->link_array = (Link *) malloc(lkg->lasz * sizeof(Link));
	memset(lkg->link_array, 0, lkg->lasz * sizeof(Link));

	lkg->num_words = N_words;
	lkg->cdsz =  N_words;
	lkg->chosen_disjuncts = (Disjunct **) exalloc(lkg->cdsz * sizeof(Disjunct *));
	memset(lkg->chosen_disjuncts, 0, N_words * sizeof(Disjunct *));

	lkg->disjunct_list_str = NULL;
#ifdef USE_CORPUS
	lkg->sense_list = NULL;
#endif

	lkg->pp_info = NULL;
}

void check_link_size(Linkage lkg)
{
	if (lkg->lasz <= lkg->num_links)
	{
		lkg->lasz = 2 * lkg->lasz + 10;
		lkg->link_array = realloc(lkg->link_array, lkg->lasz * sizeof(Link));
	}
}

/** The extract_links() call sets the chosen_disjuncts array */
static void compute_chosen_disjuncts(Sentence sent)
{
	size_t in;
	size_t N_linkages_alloced = sent->num_linkages_alloced;
	Parse_info pi = sent->parse_info;

	for (in=0; in < N_linkages_alloced; in++)
	{
		Linkage lkg = &sent->lnkages[in];
		Linkage_info *lifo = &lkg->lifo;

		if (lifo->discarded || lifo->N_violations) continue;

		partial_init_linkage(lkg, pi->N_words);
		extract_links(lkg, pi);
		compute_link_names(lkg, sent->string_set);
		/* Because the empty words are used only in the parsing stage, they are
		 * removed here along with their links, so from now on we will not need to
		 * consider them. */
		remove_empty_words(lkg);
	}
}

/** This does basic post-processing for all linkages.
 */
static void post_process_linkages(Sentence sent, Parse_Options opts)
{
	size_t in;
	size_t N_linkages_post_processed = 0;
	size_t N_valid_linkages = sent->num_valid_linkages;
	size_t N_linkages_alloced = sent->num_linkages_alloced;
	bool twopass = sent->length >= opts->twopass_length;

	/* (optional) First pass: just visit the linkages */
	/* The purpose of the first pass is to make the post-processing
	 * more efficient.  Because (hopefully) by the time the real work
	 * is done in the 2nd pass, the relevant rule set has been pruned
	 * in the first pass.
	 */
	if (twopass)
	{
		for (in=0; in < N_linkages_alloced; in++)
		{
			Linkage lkg = &sent->lnkages[in];
			Linkage_info *lifo = &lkg->lifo;

			if (lifo->discarded || lifo->N_violations) continue;

			post_process_scan_linkage(sent->postprocessor, lkg);

			if ((49 == in%50) && resources_exhausted(opts->resources)) break;
		}
	}

	/* Second pass: actually perform post-processing */
	for (in=0; in < N_linkages_alloced; in++)
	{
		PP_node *ppn;
		Linkage lkg = &sent->lnkages[in];
		Linkage_info *lifo = &lkg->lifo;

		if (lifo->discarded || lifo->N_violations) continue;

		ppn = do_post_process(sent->postprocessor, lkg, twopass);

		/* XXX There is no need to set the domain names if we are not
		 * printing them. However, deferring this until later requires
		 * a huge code re-org, because pp_data is needed to get the
		 * domain type array, and pp_data is deleted immediately below.
		 * Basically, pp_data and pp_node should be a part of the linkage,
		 * and not part of the Postprocessor struct.
		 * This costs about 1% performance penalty. */
		build_type_array(sent->postprocessor);
		linkage_set_domain_names(sent->postprocessor, lkg);

	   post_process_free_data(&sent->postprocessor->pp_data);

		if (NULL != ppn->violation)
		{
			N_valid_linkages--;
			lifo->N_violations++;

			/* Set the message, only if not set (e.g. by sane_morphism) */
			if (NULL == lifo->pp_violation_msg)
				lifo->pp_violation_msg = ppn->violation;
		}
		N_linkages_post_processed++;

		linkage_score(lkg, opts);
		if ((9 == in%10) && resources_exhausted(opts->resources)) break;
	}

	/* If the timer expired, then we never finished post-processing.
	 * Mark the remaining sentences as bad, as otherwise strange
	 * results get reported.  At any rate, need to compute the link
	 * names, as otherwise linkage_create() will crash and burn
	 * trying to touch them. */
	for (; in < N_linkages_alloced; in++)
	{
		Linkage lkg = &sent->lnkages[in];
		Linkage_info *lifo = &lkg->lifo;

		if (lifo->discarded || lifo->N_violations) continue;

		N_valid_linkages--;
		lifo->N_violations++;

		/* Set the message, only if not set (e.g. by sane_morphism) */
		if (NULL == lifo->pp_violation_msg)
			lifo->pp_violation_msg = "Timeout during postprocessing";
	}

	print_time(opts, "Postprocessed all linkages");

	if (debug_level(6))
	{
		err_ctxt ec;
		ec.sent = sent;
		err_msg(&ec, Info, "Info: %zu of %zu linkages with no P.P. violations\n",
		        N_valid_linkages, N_linkages_post_processed);
	}

	sent->num_linkages_post_processed = N_linkages_post_processed;
	sent->num_valid_linkages = N_valid_linkages;
}

static void sort_linkages(Sentence sent, Parse_Options opts)
{
	if (0 == sent->num_linkages_found) return;
	qsort((void *)sent->lnkages, sent->num_linkages_alloced,
	      sizeof(struct Linkage_s),
	      (int (*)(const void *, const void *))opts->cost_model.compare_fn);

#ifdef DEBUG
	/* num_linkages_post_processed sanity check (ONLY). */
	{
		size_t in;
		size_t N_linkages_post_processed = 0;
		for (in=0; in < sent->num_linkages_alloced; in++)
		{
			Linkage_info *lifo = &sent->lnkages[in].lifo;
			if (lifo->discarded) break;
			N_linkages_post_processed++;
		}
		assert(sent->num_linkages_post_processed==N_linkages_post_processed,
		       "Bad num_linkages_post_processed (%zu!=%zu)",
		       sent->num_linkages_post_processed, N_linkages_post_processed);
	}
#endif

	print_time(opts, "Sorted all linkages");
}

/***************************************************************
*
* Routines for creating and destroying processing Sentences
*
****************************************************************/

/* Its OK if this is racey across threads.  Any mild shuffling is enough. */
static unsigned int global_rand_state;

Sentence sentence_create(const char *input_string, Dictionary dict)
{
	Sentence sent;

	sent = (Sentence) xalloc(sizeof(struct Sentence_s));
	memset(sent, 0, sizeof(struct Sentence_s));

	sent->dict = dict;
	sent->string_set = string_set_create();
	sent->rand_state = global_rand_state;

	sent->postprocessor = post_process_new(dict->base_knowledge);

	/* Make a copy of the input */
	sent->orig_sentence = string_set_add (input_string, sent->string_set);

	return sent;
}

int sentence_split(Sentence sent, Parse_Options opts)
{
	Dictionary dict = sent->dict;
	bool fw_failed = false;

	/* Tokenize */
	if (!separate_sentence(sent, opts))
	{
		return -1;
	}

	/* Flatten the word graph created by separate_sentence() to a 2D-word-array
	 * which is compatible to the current parsers.
	 * This may fail if the EMPTY_WORD_DOT or UNKNOWN_WORD words are needed but
	 * are not defined in the dictionary, or an internal error happens. */
	fw_failed = !flatten_wordgraph(sent, opts);

	/* If unknown_word is not defined, then no special processing
	 * will be done for e.g. capitalized words. */
	if (!(dict->unknown_word_defined && dict->use_unknown_word))
	{
		if (!sentence_in_dictionary(sent)) {
			return -2;
		}
	}

	if (fw_failed)
	{
		/* Make sure an error message is always printed.
		 * So it may be redundant. */
		prt_error("Error: sentence_split(): Internal error detected");
		return -3;
	}

	return 0;
}

static void free_sentence_words(Sentence sent)
{
	size_t i;

	for (i = 0; i < sent->length; i++)
	{
		free_X_nodes(sent->word[i].x);
		free_disjuncts(sent->word[i].d);
		free(sent->word[i].alternatives);
	}
	free((void *) sent->word);
	sent->word = NULL;
}

static void wordgraph_delete(Sentence sent)
{
	Gword *w = sent->wordgraph;

	while(NULL != w)
	{
		Gword *w_tofree = w;

		free(w->prev);
		free(w->next);
		free(w->hier_position);
		free(w->null_subwords);
		w = w->chain_next;
		free(w_tofree);
	}
	sent->wordgraph = sent->last_word = NULL;
}

static void word_queue_delete(Sentence sent)
{
	struct word_queue *wq = sent->word_queue;
	while (NULL != wq)
	{
		struct word_queue *wq_tofree = wq;
		wq = wq->next;
		free(wq_tofree);
	};
	sent->word_queue = NULL;
}

void sentence_delete(Sentence sent)
{
	if (!sent) return;
	sat_sentence_delete(sent);
	free_sentence_words(sent);
	wordgraph_delete(sent);
	word_queue_delete(sent);
	string_set_delete(sent->string_set);
	free_parse_info(sent->parse_info);
	free_linkages(sent);
	post_process_free(sent->postprocessor);
	post_process_free(sent->constituent_pp);

	global_rand_state = sent->rand_state;
	xfree((char *) sent, sizeof(struct Sentence_s));
}

int sentence_length(Sentence sent)
{
	if (!sent) return 0;
	return sent->length;
}

int sentence_null_count(Sentence sent)
{
	if (!sent) return 0;
	return sent->null_count;
}

int sentence_num_linkages_found(Sentence sent)
{
	if (!sent) return 0;
	return sent->num_linkages_found;
}

int sentence_num_valid_linkages(Sentence sent)
{
	if (!sent) return 0;
	return sent->num_valid_linkages;
}

int sentence_num_linkages_post_processed(Sentence sent)
{
	if (!sent) return 0;
	return sent->num_linkages_post_processed;
}

int sentence_num_violations(Sentence sent, LinkageIdx i)
{
	if (!sent) return 0;

	if (!sent->lnkages) return 0;
	if (sent->num_linkages_alloced <= i) return 0; /* bounds check */
	return sent->lnkages[i].lifo.N_violations;
}

double sentence_disjunct_cost(Sentence sent, LinkageIdx i)
{
	if (!sent) return 0.0;

	/* The sat solver (currently) fails to fill in link_info */
	if (!sent->lnkages) return 0.0;
	if (sent->num_linkages_alloced <= i) return 0.0; /* bounds check */
	return sent->lnkages[i].lifo.disjunct_cost;
}

int sentence_link_cost(Sentence sent, LinkageIdx i)
{
	if (!sent) return 0;

	/* The sat solver (currently) fails to fill in link_info */
	if (!sent->lnkages) return 0;
	if (sent->num_linkages_alloced <= i) return 0; /* bounds check */
	return sent->lnkages[i].lifo.link_cost;
}

/**
 * Construct word paths (one or more) through the Wordgraph.
 *
 * Add 'current_word" to the potential path.
 * Add "p" to the path queue, which defines the start of the next potential
 * paths to be checked.
 *
 * Each path is up to the current word (not including). It doesn't actually
 * construct a full path if there are null words - they break it. The final path
 * is constructed when the Wordgraph termination word is encountered.
 *
 * Note: The final path doesn't match the linkage word indexing if the linkage
 * contains empty words, at least until empty words are eliminated from the
 * linkage (in compute_chosen_words()). Further processing of the path is done
 * there in case morphology splits are to be hidden or there are morphemes with
 * null linkage.
 */
static void wordgraph_path_append(Wordgraph_pathpos **nwp, const Gword **path,
                                  Gword *current_word, /* add to the path */
                                  Gword *p)      /* add to the path queue */
{
	size_t n = wordgraph_pathpos_len(*nwp);

	assert(NULL != p, "Tried to add a NULL word to the word queue");

	/* Check if the path queue already contains the word to be added to it. */
	if (NULL != *nwp)
	{
		const Wordgraph_pathpos *wpt;

		for (wpt = *nwp; NULL != wpt->word; wpt++)
		{
			if (p == wpt->word)
			{
				/* If we are here, there are 2 or more paths leading to this word
				 * (p) that end with the same number of consecutive null words that
				 * consist an entire alternative. These null words represent
				 * different ways to split the subword upward in the hierarchy, but
				 * since they don't have linkage we don't care which of these
				 * paths is used. */
				return; /* The word is already in the queue */
			}
		}
	}

	/* Not already in the path queue - add it. */
	*nwp = wordgraph_pathpos_resize(*nwp, n);
	(*nwp)[n].word = p;

	if (MT_INFRASTRUCTURE == p->prev[0]->morpheme_type)
	{
			/* Previous word is the Wordgraph dummy word. Initialize the path. */
			(*nwp)[n].path = NULL;
	}
	else
	{
		/* We branch to another path. Duplicate it from the current path and add
		 * the current word to it. */
		size_t path_arr_size = (gwordlist_len(path)+1)*sizeof(*path);

		(*nwp)[n].path = malloc(path_arr_size);
		memcpy((*nwp)[n].path, path, path_arr_size);
	}
   /* FIXME (cast) but anyway gwordlist_append() doesn't modify Gword. */
	gwordlist_append((Gword ***)&(*nwp)[n].path, current_word);
}

/**
 * Free the Wordgraph paths and the Wordgraph_pathpos array.
 * In case of a match, the final path is still needed so this function is
 * then invoked with free_final_path=false.
 */
static void wordgraph_path_free(Wordgraph_pathpos *wp, bool free_final_path)
{
	Wordgraph_pathpos *twp;

	if (NULL == wp) return;
	for (twp = wp; NULL != twp->word; twp++)
	{
		if (free_final_path || (MT_INFRASTRUCTURE != twp->word->morpheme_type))
			free(twp->path);
	}
	free(wp);
}

/* ============================================================== */
/* A kind of morphism post-processing */

/* These letters create a string that should be matched by a SANEMORPHISM regex,
 * given in the affix file. The empty word doesn't have a letter. E.g. for the
 * Russian dictionary: "w|ts". It is converted here to: "^((w|ts)b)+$".
 * It matches "wbtsbwbtsbwb" but not "wbtsbwsbtsb".
 * FIXME? In this version of the function, 'b' is not yet supported,
 * so "w|ts" is converted to "^(w|ts)+$" for now. */

#define AFFIXTYPE_PREFIX   'p'   /* prefix */
#define AFFIXTYPE_STEM     't'   /* stem */
#define AFFIXTYPE_SUFFIX   's'   /* suffix */
#define AFFIXTYPE_MIDDLE   'm'   /* middle morpheme */
#define AFFIXTYPE_WORD     'w'   /* regular word */
#ifdef WORD_BOUNDARIES
#define AFFIXTYPE_END      'b'   /* end of input word */
#endif

/**
 * This routine solves the problem of mis-linked alternatives,
 * i.e a morpheme in one alternative that is linked to a morpheme in
 * another alternative. This can happen due to the way in which word
 * alternatives are implemented.
 *
 * It does so by checking that all the chosen disjuncts in a linkage (including
 * null words) match, in the same order, a path in the Wordgraph.
 *
 * An important side effect of this check is that if the linkage is good,
 * its Wordgraph path is found.
 *
 * Optionally (if SANEMORPHISM regex is defined in the affix file), it
 * also validates that the morpheme-type sequence is permitted for the
 * language. This is a sanity check of the program and the dictionary.
 *
 * Return true if the linkage is good, else return false.
 */
#define D_SLM 7
bool sane_linkage_morphism(Sentence sent, Linkage lkg, Parse_Options opts)
{
	Wordgraph_pathpos *wp_new = NULL;
	Wordgraph_pathpos *wp_old = NULL;
	Wordgraph_pathpos *wpp;
	Gword **next; /* next Wordgraph words of the current word */

	size_t i;
	Linkage_info * const lifo = &lkg->lifo;

	bool match_found = true; /* if all the words are null - it's still a match */
	Gword **lwg_path;

	Dictionary afdict = sent->dict->affix_table;       /* for SANEMORPHISM */
	char *const affix_types = alloca(sent->length*2 + 1);   /* affix types */

	affix_types[0] = '\0';

	/* Populate the path word queue, initializing the path to NULL. */
	for (next = sent->wordgraph->next; *next; next++)
	{
		wordgraph_path_append(&wp_new, /*path*/NULL, /*add_word*/NULL, *next);
	}
	assert(NULL != wp_new, "Path word queue is empty");

	for (i = 0; i < lkg->num_words; i++)
	{
		Disjunct *cdj;            /* chosen disjunct */

		lgdebug(D_SLM, "%p Word %zu: ", lkg, i);

		if (NULL == wp_new)
		{
			lgdebug(+D_SLM, "- No more words in the wordgraph\n");
			match_found = false;
			break;
		}

		if (wp_old != wp_new)
		{
			wordgraph_path_free(wp_old, true);
			wp_old = wp_new;
		}
		wp_new = NULL;
		//wordgraph_pathpos_print(wp_old);

		cdj = lkg->chosen_disjuncts[i];
		/* Handle null words */
		if (NULL == cdj)
		{
			lgdebug(D_SLM, "- Null word\n");
			/* A null word matches any word in the Wordgraph -
			 * so, unconditionally proceed in all paths in parallel. */
			match_found = false;
			for (wpp = wp_old; NULL != wpp->word; wpp++)
			{
				if (NULL == wpp->word->next)
					continue; /* This path encountered the Wordgraph end */

				/* The null words cannot be marked here because wpp->path consists
				 * of pointers to the Wordgraph words, and these words are common to
				 * all the linkages, with potentially different null words in each
				 * of them. However, the position of the null words can be inferred
				 * from the null words in the word array of the Linkage structure.
				 */
				for (next = wpp->word->next; NULL != *next; next++)
				{
					match_found = true;
					wordgraph_path_append(&wp_new, wpp->path, wpp->word, *next);
				}
			}
			continue;
		}

		if (!match_found)
		{
			const char *e = "Internal error: Too many words in the linkage\n";
			lgdebug(D_SLM, "- %s", e);
			prt_error("Error: %s.", e);
			break;
		}

		assert(MT_EMPTY != cdj->word[0]->morpheme_type); /* already discarded */

		if (debug_level(D_SLM)) print_with_subscript_dot(cdj->string);

		match_found = false;
		/* Proceed in all the paths in which the word is found. */
		for (wpp = wp_old; NULL != wpp->word; wpp++)
		{
			const Gword **wlp; /* disjunct word list */

			for (wlp = cdj->word; *wlp; wlp++)
			{
				if (*wlp == wpp->word)
				{
					match_found = true;
					for (next = wpp->word->next; NULL != *next; next++)
					{
						wordgraph_path_append(&wp_new, wpp->path, wpp->word, *next);
					}
					break;
				}
			}
		}

		if (!match_found)
		{
			/* FIXME? A message can be added here if there are too many words
			 * in the linkage (can happen only if there is an internal error). */
			lgdebug(D_SLM, "- No Wordgraph match\n");
			break;
		}
		lgdebug(D_SLM, "\n");
	}

	if (match_found)
	{
		match_found = false;
		/* Validate that there are no missing words in the linkage. It is so if
		 * the dummy termination word is found in the new pathpos queue. */
		if (NULL != wp_new)
		{
			for (wpp = wp_new; NULL != wpp->word; wpp++)
			{
				if (MT_INFRASTRUCTURE == wpp->word->morpheme_type) {
					match_found = true;
					/* Exit the loop with with wpp of the termination word. */
					break;
				}
			}
		}
		if (!match_found)
		    lgdebug(D_SLM, "%p Missing word(s) at the end of the linkage.\n", lkg);
	}

#define DEBUG_morpheme_type 0
	/* Check the morpheme type combination.
	 * If null_count > 0, the morpheme type combination may be invalid
	 * due to null subwords, so skip this check. */
	if (match_found && (0 == sent->null_count) &&
		(NULL != afdict) && (NULL != afdict->regex_root))
	{
		const Gword **w;
		char *affix_types_p = affix_types;

		/* Construct the affix_types string. */
#if DEBUG_morpheme_type
		print_lwg_path(wpp->path);
#endif
		i = 0;
		for (w = wpp->path; *w; w++)
		{
			i++;
			if (MT_EMPTY == (*w)->morpheme_type) continue; /* really a null word */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
			switch ((*w)->morpheme_type)
			{
#pragma GCC diagnostic pop
				default:
					/* What to do with the rest? */
				case MT_WORD:
					*affix_types_p = AFFIXTYPE_WORD;
					break;
				case MT_PREFIX:
					*affix_types_p = AFFIXTYPE_PREFIX;
					break;
				case MT_STEM:
					*affix_types_p = AFFIXTYPE_STEM;
					break;
				case MT_MIDDLE:
					*affix_types_p = AFFIXTYPE_MIDDLE;
					break;
				case MT_SUFFIX:
					*affix_types_p = AFFIXTYPE_SUFFIX;
					break;
			}

#if DEBUG_morpheme_type
			lgdebug(D_SLM, "Word %zu: %s affixtype=%c\n",
			     i, (*w)->subword,  *affix_types_p);
#endif

			affix_types_p++;
		}
		*affix_types_p = '\0';

#ifdef WORD_BOUNDARIES /* not yet implemented */
		{
			const Gword *uw;

			/* If w is an "end subword", return its unsplit word, else NULL. */
			uw = word_boundary(w); /* word_boundary() unimplemented */

			if (NULL != uw)
			{
				*affix_types_p++ = AFFIXTYPE_END;
				lgdebug(D_SLM, "%p End of Gword %s\n", lkg, uw->subword);
			}
		}
#endif

		/* Check if affix_types is valid according to SANEMORPHISM. */
		if (('\0' != affix_types[0]) &&
		    (NULL == match_regex(afdict->regex_root, affix_types)))
		{
			/* Morpheme type combination is invalid */
			match_found = false;
			/* Notify to stdout, so it will be shown along with the result.
			 * XXX We should have a better way to notify. */
			if (0 < opts->verbosity)
				printf("Warning: Invalid morpheme type combination '%s'.\n"
				       "Run with !bad and !verbosity>"STRINGIFY(D_USER_MAX)
				       " to debug\n", affix_types);
		}
	}

	if (match_found) lwg_path = (Gword **)wpp->path; /* OK to modify */
	wordgraph_path_free(wp_old, true);
	wordgraph_path_free(wp_new, !match_found);

	if (match_found)
	{
		if ('\0' != affix_types[0])
		{
			lgdebug(D_SLM, "%p Morpheme type combination '%s'\n", lkg, affix_types);
		}
		lgdebug(+D_SLM, "%p SUCCEEDED\n", lkg);
		lkg->wg_path = lwg_path;
		return true;
	}

	/* Oh no ... invalid morpheme combination! */
	sent->num_valid_linkages --;
	lifo->N_violations++;
	lifo->pp_violation_msg = "Invalid morphism construction.";
	lkg->wg_path = NULL;
	lifo->discarded = true;
	lgdebug(D_SLM, "%p FAILED\n", lkg);
	return false;
}
#undef D_SLM

static void sane_morphism(Sentence sent, Parse_Options opts)
{
	size_t N_invalid_morphism = 0;
	size_t lk;

	for (lk = 0; lk < sent->num_linkages_alloced; lk++)
	{
		Linkage lkg = &sent->lnkages[lk];
		/* Don't bother with linkages that already failed post-processing... */
		if (0 != lkg->lifo.N_violations) continue;

		if (!sane_linkage_morphism(sent, lkg, opts))
			N_invalid_morphism ++;
	}

	if (debug_level(5))
	{
		prt_error("Info: sane_morphism(): %zu of %zu linkages had "
		          "invalid morphology construction\n",
		          N_invalid_morphism, sent->num_linkages_alloced);
	}
}

/** Misnamed, this has nothing to do with chart parsing */
static void chart_parse(Sentence sent, Parse_Options opts)
{
	int nl;
	fast_matcher_t * mchxt;
	count_context_t * ctxt;

	/* Build lists of disjuncts */
	prepare_to_parse(sent, opts);
	if (resources_exhausted(opts->resources)) return;

	mchxt = alloc_fast_matcher(sent);
	ctxt = alloc_count_context(sent->length);
	print_time(opts, "Initialized fast matcher");
	if (resources_exhausted(opts->resources))
	{
		free_count_context(ctxt);
		free_fast_matcher(mchxt);
		return;
	}

	/* A parse set may have been already been built for this sentence,
	 * if it was previously parsed.  If so we free it up before
	 * building another.  Huh ?? How could that happen? */
	free_parse_info(sent->parse_info);
	sent->parse_info = parse_info_new(sent->length);

	nl = opts->min_null_count;
	while (true)
	{
		Count_bin hist;
		s64 total;
		if (resources_exhausted(opts->resources)) break;
		sent->null_count = nl;
		hist = do_parse(sent, mchxt, ctxt, sent->null_count, opts);
		total = hist_total(&hist);

		if (debug_level(5))
		{
			prt_error("Info: Total count with %zu null links:   %lld\n",
			          sent->null_count, total);
		}

		/* total is 64-bit, num_linkages_found is 32-bit. Clamp */
		total = (total > INT_MAX) ? INT_MAX : total;
		total = (total < 0) ? INT_MAX : total;

		sent->num_linkages_found = (int) total;
		print_time(opts, "Counted parses");

		select_linkages(sent, mchxt, ctxt, opts);
		compute_chosen_disjuncts(sent);
		sane_morphism(sent, opts);
		post_process_linkages(sent, opts);
		if (sent->num_valid_linkages > 0) break;

		/* If we are here, then no valid linkages were found.
		 * If there was a parse overflow, give up now. */
		if (PARSE_NUM_OVERFLOW < total) break;

		/* loop termination */
		if (nl == opts->max_null_count) break;

		/* If we are here, we are going round again. Free stuff. */
		free_linkages(sent);
		nl++;
	}
	sort_linkages(sent, opts);

	free_count_context(ctxt);
	free_fast_matcher(mchxt);
}

static void free_sentence_disjuncts(Sentence sent)
{
	size_t i;

	for (i = 0; i < sent->length; ++i)
	{
		free_disjuncts(sent->word[i].d);
		sent->word[i].d = NULL;
	}
}

int sentence_parse(Sentence sent, Parse_Options opts)
{
	int rc;

	sent->num_valid_linkages = 0;

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
	resources_reset_space(opts->resources);

	if (resources_exhausted(opts->resources))
		return 0;

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

	if ((verbosity > 0) &&
	   (PARSE_NUM_OVERFLOW < sent->num_linkages_found))
	{
		prt_error("WARNING: Combinatorial explosion! nulls=%zu cnt=%d\n"
			"Consider retrying the parse with the max allowed disjunct cost set lower.\n"
			"At the command line, use !cost-max\n",
			sent->null_count, sent->num_linkages_found);
	}
	return sent->num_valid_linkages;
}
