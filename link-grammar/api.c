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

#include <string.h>

#include "api-structures.h"
#include "dict-common/dict-defines.h"
#include "dict-common/dialect.h"
#include "dict-common/dict-utils.h"
#include "disjunct-utils.h"             // free_sentence_disjuncts
#include "linkage/linkage.h"
#include "memory-pool.h"
#include "parse/histogram.h"            // PARSE_NUM_OVERFLOW
#include "parse/parse.h"
#include "post-process/post-process.h"  // post_process_new
#include "prepare/exprune.h"
#include "string-set.h"
#include "tokenize/wordgraph.h"         // wordgraph_delete
#include "resources.h"
#include "sat-solver/sat-encoder.h"
#include "tokenize/tokenize.h"
#include "tokenize/word-structures.h"   // Word_struct
#include "utilities.h"

/* Its OK if this is racey across threads.  Any mild shuffling is enough. */
static unsigned int global_rand_state = 0;

int verbosity;
/* debug and test should not be NULL since they can be used before they
 * are assigned a value by parse_options_get_...() */
char * debug = (char *)"";
char * test = (char *)"";

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

/**
 * Create and initialize a Parse_Options object
 */
Parse_Options parse_options_create(void)
{
	Parse_Options po;

	init_memusage();
	po = (Parse_Options) malloc(sizeof(struct Parse_Options_s));

	/* Here's where the values are initialized */

	/* The parse_options_set_(verbosity|debug|test) functions set also the
	 * corresponding global variables. So these globals are initialized
	 * here too. */
	verbosity = po->verbosity = 1;
	debug = po->debug = (char *)"";
	test = po->test = (char *)"";

	/* A cost of 2.7 allows the usual cost-2 connectors, plus the
	 * assorted fractional costs, without going to cost 3.0, which
	 * is used only during panic-parsing. */
	po->disjunct_cost = UNINITIALIZED_MAX_DISJUNCT_COST;
	po->min_null_count = 0;
	po->max_null_count = 0;
	po->islands_ok = false;
#if USE_SAT_SOLVER
	po->use_sat_solver = false;
#endif
	po->linkage_limit = 100;
#if defined HAVE_HUNSPELL || defined HAVE_ASPELL
	po->use_spell_guess = 7;
#else
	po->use_spell_guess = 0;
#endif /* defined HAVE_HUNSPELL || defined HAVE_ASPELL */

	po->cost_model.compare_fn = &VDAL_compare_parse;
	po->cost_model.type = VDAL;
	po->short_length = 16;
	po->all_short = false;
	po->perform_pp_prune = true;
	po->twopass_length = 30;
	po->repeatable_rand = true;
	po->resources = resources_create();
	po->display_morphology = true;
	po->dialect = (dialect_info){ .conf = strdup("") };

	return po;
}

static void free_dialect_info(dialect_info *);
int parse_options_delete(Parse_Options opts)
{
	resources_delete(opts->resources);
	free_dialect_info(&opts->dialect);
	free(opts);
	return 0;
}

void parse_options_set_cost_model_type(Parse_Options opts, Cost_Model_type cm)
{
	switch(cm) {
	case VDAL:
		opts->cost_model.type = VDAL;
		opts->cost_model.compare_fn = &VDAL_compare_parse;
		break;
	default:
		prt_error("Error: Illegal cost model: %d\n", (int)cm);
	}
}

Cost_Model_type parse_options_get_cost_model_type(Parse_Options opts)
{
	return opts->cost_model.type;
}

void parse_options_set_perform_pp_prune(Parse_Options opts, bool dummy)
{
	opts->perform_pp_prune = dummy;
}

bool parse_options_get_perform_pp_prune(Parse_Options opts) {
	return opts->perform_pp_prune;
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
	static char buff[256]; /* see parse_options_set_debug() */
	char *buffp = buff;

	/* Remove the added commas. */
	strcpy(buff, opts->debug);
	if (buffp[0] == ',')
		buffp++;
	if ((buffp[0] != '\0') && buffp[strlen(buffp)-1] == ',')
		buffp[strlen(buffp)-1] = '\0';

	return buffp;
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
	static char buff[256]; /* see parse_options_set_test() */
	char *buffp = buff;

	/* Remove the added commas. */
	strcpy(buff, opts->test);
	if (buffp[0] == ',')
		buffp++;
	if ((buffp[0] != '\0') && buffp[strlen(buffp)-1] == ',')
		buffp[strlen(buffp)-1] = '\0';

	return buffp;
}

void parse_options_set_use_sat_parser(Parse_Options opts, bool dummy) {
#ifdef USE_SAT_SOLVER
	opts->use_sat_solver = dummy;
#else
	if (dummy && (verbosity > D_USER_BASIC))
	{
		prt_error("Error: Cannot enable the Boolean SAT parser; "
		          "this library was built without SAT solver support.\n");
	}
#endif
}

bool parse_options_get_use_sat_parser(Parse_Options opts) {
#if USE_SAT_SOLVER
	return opts->use_sat_solver;
#else
	return false;
#endif
}

void parse_options_set_linkage_limit(Parse_Options opts, int dummy)
{
	opts->linkage_limit = dummy;
}
int parse_options_get_linkage_limit(Parse_Options opts)
{
	return opts->linkage_limit;
}

void parse_options_set_disjunct_cost(Parse_Options opts, float dummy)
{
	opts->disjunct_cost = dummy;
}
float parse_options_get_disjunct_cost(Parse_Options opts)
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
#if defined HAVE_HUNSPELL || defined HAVE_ASPELL
	opts->use_spell_guess = dummy;
#else
	if (dummy && (verbosity > D_USER_BASIC))
	{
		prt_error("Error: Cannot enable spell guess; "
		        "this library was built without spell guess support.\n");
	}

#endif /* defined HAVE_HUNSPELL || defined HAVE_ASPELL */
}

int parse_options_get_spell_guess(Parse_Options opts) {
	return opts->use_spell_guess;
}

void parse_options_set_short_length(Parse_Options opts, int short_length) {
	opts->short_length = MIN(short_length, UNLIMITED_LEN);
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

/** True means "make it repeatable.". False means "make it random". */
void parse_options_set_repeatable_rand(Parse_Options opts, bool val)
{
	opts->repeatable_rand = val;

	/* Zero is used to indicate repeatability. */
	if (val) global_rand_state = 0;
	else if (0 == global_rand_state) global_rand_state = 42;
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

/* Dialect. */

static void free_dialect_info(dialect_info *dinfo)
{
	if (dinfo == NULL) return;

	free(dinfo->cost_table);
	dinfo->cost_table = NULL;
	free((void *)dinfo->conf);
}

char * parse_options_get_dialect(Parse_Options opts)
{
	return opts->dialect.conf;
}

void parse_options_set_dialect(Parse_Options opts, const char *dconf)
{
	if (0 == strcmp(dconf, opts->dialect.conf)) return;
	free_dialect_info(&opts->dialect);
	opts->dialect.conf = strdup(dconf);
}
/***************************************************************
*
* Routines for creating destroying and processing Sentences
*
****************************************************************/

Sentence sentence_create(const char *input_string, Dictionary dict)
{
	Sentence sent;

	sent = (Sentence) malloc(sizeof(struct Sentence_s));
	memset(sent, 0, sizeof(struct Sentence_s));

	sent->dict = dict;
	sent->string_set = string_set_create();
	sent->rand_state = global_rand_state;
	sent->Exp_pool = pool_new(__func__, "Exp", /*num_elements*/4096,
	                             sizeof(Exp), /*zero_out*/false,
	                             /*align*/false, /*exact*/false);
	sent->X_node_pool = pool_new(__func__, "X_node", /*num_elements*/256,
	                             sizeof(X_node), /*zero_out*/false,
	                             /*align*/false, /*exact*/false);

	sent->postprocessor = post_process_new(dict->base_knowledge);

	/* Make a copy of the input */
	sent->orig_sentence = string_set_add(input_string, sent->string_set);

	/* Set the minimum length for tracon sharing.
	 * In that case a tracon list is produced for the pruning step,
	 * and disjunct/connector packing is done too. */
	if (IS_GENERATION(dict))
		sent->min_len_encoding = 0;
	else
		sent->min_len_encoding = SENTENCE_MIN_LENGTH_TRAILING_HASH;
	const char *min_len_encoding = test_enabled("min-len-encoding");
	if (NULL != min_len_encoding)
		sent->min_len_encoding = atoi(min_len_encoding+1);

	/* Set the minimum length for pruning per null-count. */
	sent->min_len_multi_pruning = SENTENCE_MIN_LENGTH_MULTI_PRUNING;
	const char *min_len_multi_pruning = test_enabled("len-multi-pruning");
	if (NULL != min_len_multi_pruning)
		sent->min_len_multi_pruning = atoi(min_len_multi_pruning+1);

	return sent;
}

int sentence_split(Sentence sent, Parse_Options opts)
{
	Dictionary dict = sent->dict;
	bool fw_failed = false;

	/* 0 == global_rand_state denotes "repeatable rand".
	 * If non-zero, set it here so that anysplit can use it.
	 */
	if (false == opts->repeatable_rand && 0 == sent->rand_state)
	{
		if (0 == global_rand_state) global_rand_state = 42;
		sent->rand_state = global_rand_state;
	}

	/* Tokenize */
	if (!separate_sentence(sent, opts))
	{
		return -1;
	}

	if (!setup_dialect(dict, opts))
		return -4;

	/* Flatten the word graph created by separate_sentence() to a 2D-word-array
	 * which is compatible to the current parsers.
	 * This may fail if UNKNOWN_WORD is needed but
	 * is not defined in the dictionary, or an internal error happens. */
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
		prt_error("Error: sentence_split(): Internal error detected\n");
		return -3;
	}

	if (verbosity >= D_USER_TIMES)
		prt_error("#### Finished tokenizing (%zu tokens)\n", sent->length);
	return 0;
}

void sentence_delete(Sentence sent)
{
	if (!sent) return;
	sat_sentence_delete(sent);
	free_sentence_disjuncts(sent, /*categories_too*/true);
	free_words(sent);
	wordgraph_delete(sent);
	string_set_delete(sent->string_set);
	free_linkages(sent);
	post_process_free(sent->postprocessor);
	post_process_free(sent->constituent_pp);
	exp_stringify(NULL);
	free(sent->disjunct_used);

	global_rand_state = sent->rand_state;
	pool_delete(sent->Match_node_pool);
	pool_delete(sent->Table_connector_pool);
	pool_delete(sent->wordvec_pool);
	pool_delete(sent->Exp_pool);
	pool_delete(sent->X_node_pool);

	/* Usually the memory pools created in build_disjuncts_for_exp() are
	 * deleted in build_sentence_disjuncts(). Delete them here in case
	 * build_disjuncts_for_exp() is directly called. */
	if (sent->Clause_pool != NULL)
	{
		pool_delete(sent->Clause_pool);
		pool_delete(sent->Tconnector_pool);
	}

	// This is a hack. Should just ask the backend to "do the right
	// thing".
	if (IS_SQL_DICT(sent->dict))
	{
#if 0 /* Cannot reuse in case a previous sentence is not deleted yet. */
		We could fix this by putting a use-count in the dict.
		condesc_reuse(sent->dict);
#endif
		pool_reuse(sent->dict->Exp_pool);
	}

	if (NULL != sent->wildcard_word_dc_memblock)
	{
		free_categories_from_disjunct_array(sent->wildcard_word_dc_memblock,
		                                    sent->wildcard_word_num_disjuncts);
		free(sent->wildcard_word_dc_memblock);
	}

	free(sent);
}

int sentence_length(Sentence sent)
{
	if (!sent) return 0;
	return sent->length;
}

int sentence_null_count(Sentence sent)
{
	if (!sent) return 0;
	return (int)sent->null_count;
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

float sentence_disjunct_cost(Sentence sent, LinkageIdx i)
{
	if (!sent) return 0.0;

	if (!sent->lnkages) return 0.0;
	if (sent->num_linkages_alloced <= i) return 0.0; /* bounds check */
	return sent->lnkages[i].lifo.disjunct_cost;
}

int sentence_link_cost(Sentence sent, LinkageIdx i)
{
	if (!sent) return 0;

	if (!sent->lnkages) return 0;
	if (sent->num_linkages_alloced <= i) return 0; /* bounds check */
	return sent->lnkages[i].lifo.link_cost;
}

int sentence_parse(Sentence sent, Parse_Options opts)
{
	int rc;

	if (IS_GENERATION(sent->dict))
	{
#if USE_SAT_SOLVER
		if (opts->use_sat_solver)
		{
			prt_error("Error: Cannot use the SAT parser in generation mode\n");
			return -3;
		}
#endif
		if (opts->max_null_count > 0)
		{
			prt_error("Error: Cannot parse with nulls in generation mode\n");
			return -3;
		}
	}

	if (opts->disjunct_cost == UNINITIALIZED_MAX_DISJUNCT_COST)
		opts->disjunct_cost = sent->dict->default_max_disjunct_cost;

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
	else
	{
		/* During a panic parse, we enter here a second time, with leftover
		 * garbage. Free it. We really should make the code that is panicking
		 * do this free, but right now, they have no API for it, so we do it
		 * as a favor. XXX FIXME someday. */
		free_sentence_disjuncts(sent, /*categories_too*/true);
	}

	/* Check for bad sentence length */
	if (MAX_SENTENCE <= sent->length)
	{
		prt_error("Error: sentence too long, contains more than %d words\n",
			MAX_SENTENCE);
		return -2;
	}

	resources_reset(opts->resources);

	/* When a dynamic dictionary is used, expressions are read on demand,
	 * so the connector descriptor table is not yet ready at this point. */
	if (IS_DYNAMIC_DICT(sent->dict))
		condesc_setup(sent->dict);

	for (WordIdx w = 0; w < sent->length; w++)
	{
		for (X_node *x = sent->word[w].x; x != NULL; x = x->next)
			set_connector_farthest_word(x->exp, (int)w, (int)sent->length, opts);
	}

	/* Expressions were set up during the tokenize stage.
	 * Prune them, and then parse.
	 */
	expression_prune(sent, opts);
	print_time(opts, "Finished expression pruning");
#if USE_SAT_SOLVER
	if (opts->use_sat_solver)
	{
		sat_parse(sent, opts);
	}
	else
#endif
	{
		classic_parse(sent, opts);
	}
	print_time(opts, "Finished parse");

	if ((verbosity > 0) && !IS_GENERATION(sent->dict) &&
	   (PARSE_NUM_OVERFLOW < sent->num_linkages_found))
	{
		prt_error("Warning: Combinatorial explosion! nulls=%u cnt=%d\n"
			"Consider retrying the parse with the max allowed disjunct cost set lower.\n"
			"At the command line, use !cost-max\n",
			sent->null_count, sent->num_linkages_found);
	}
	return sent->num_valid_linkages;
}

/*
 * Definitions for linkgrammar_get_configuration().
 */

#define MACVAL(macro) #macro lg_str(=macro) " "

#ifdef __STDC_VERSION__
#define LG_S1 MACVAL(__STDC_VERSION__)
#else
#define LG_S1
#endif

#ifdef _POSIX_C_SOURCE
#define LG_S2 MACVAL(_POSIX_C_SOURCE)
#else
#define LG_S2
#endif

#if !defined _POSIX_C_SOURCE || _POSIX_C_SOURCE == 0
 #ifdef _POSIX_SOURCE
 #define LG_S3 MACVAL(_POSIX_SOURCE)
 #else
 #define LG_S3
 #endif
#else
#define LG_S3
#endif

/* -DCC=$(CC) is added in the Makefile. */
#ifdef CC
#define LG_CC CC
#elif _MSC_VER
#define LG_CC "lc"
#else
#define LG_CC "(unknown)"
#endif

#ifdef __VERSION__
#define LG_V1 MACVAL(__VERSION__)
#else
#define LG_V1
#endif

#ifdef _MSC_FULL_VER
#define LG_V2 MACVAL(_MSC_FULL_VER)
#else
#define LG_V2
#endif

#define LG_COMP LG_CC " " LG_V1 " " LG_V2
#define LG_STD LG_S1 LG_S2 LG_S3

#ifdef __unix__
#define LG_unix "__unix__ "
#else
#define LG_unix
#endif

#ifdef _WIN32
#define LG_WIN32 "_WIN32 "
#else
#define LG_WIN32
#endif

#ifdef _WIN64
#define LG_WIN64 "_WIN64 "
#else
#define LG_WIN64
#endif

#ifdef __CYGWIN__
#define LG_CYGWIN "__CYGWIN__ "
#else
#define LG_CYGWIN
#endif

#ifdef __MINGW32__
#define LG_MINGW32 "__MINGW32__ "
#else
#define LG_MINGW32
#endif

#ifdef __MINGW64__
#define LG_MINGW64 "__MINGW64__ "
#else
#define LG_MINGW64
#endif

#ifdef __APPLE__
#define LG_APPLE "__APPLE__ "
#else
#define LG_APPLE
#endif

#ifdef __MACH__
#define LG_MACH "__MACH__ "
#else
#define LG_MACH
#endif

#ifndef DICTIONARY_DIR
#define DICTIONARY_DIR "None"
#endif

#define LG_windows LG_WIN32 LG_WIN64 LG_CYGWIN LG_MINGW32 LG_MINGW64
#define LG_mac LG_APPLE LG_MACH

/**
 * Return information about the configuration as a static string.
 */
const char *linkgrammar_get_configuration(void)
{
	return "Compiled with: " LG_COMP "\n"
		"OS: " LG_HOST_OS " " LG_unix LG_windows LG_mac "\n"
		"Standards: " LG_STD "\n"
		"Configuration (source code):\n\t"
			LG_CPPFLAGS "\n\t"
			LG_CFLAGS "\n"
	   "Configuration (features):\n\t"
			"DICTIONARY_DIR=" DICTIONARY_DIR "\n\t"
			LG_DEFS
		;
}
