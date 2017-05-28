/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2014 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

/* This file is somewhat misnamed, as everything here defines the
 * link-private, internal-use-only "api", which is subject to change
 * from revision to revision. No external code should link to this
 * stuff.
 */

/*****************************************************************************
*
* NOTE: There are five basic "types" used within the link parser.  These are:
*
*       Dictionary, Parse_Options, Sentence, Linkage, PostProcessor
*
* To make the use of the API simpler, each of these is typedef'ed as a pointer
* to a data structure.  As a result, some of the code may look a little funny,
* since it uses pointers in a way that is syntactically inconsistent.  After
* working a bit with these basic types enough, this should not be confusing.
*
******************************************************************************/

#ifndef _API_STRUCTURESH_
#define _API_STRUCTURESH_

/* For locale_t */
#ifdef HAVE_LOCALE_T_IN_LOCALE_H
#include <locale.h>
#endif /* HAVE_LOCALE_T_IN_LOCALE_H */
#ifdef HAVE_LOCALE_T_IN_XLOCALE_H
#include <xlocale.h>
#endif /* HAVE_LOCALE_T_IN_XLOCALE_H */

#include "api-types.h"
#include "corpus/corpus.h"
#include "error.h"
#include "utilities.h"

struct Cost_Model_s
{
	Cost_Model_type type;
	int (*compare_fn)(Linkage, Linkage);
};

struct Resources_s
{
	int    max_parse_time;  /* in seconds */
	size_t max_memory;      /* in bytes */
	double time_when_parse_started;
	size_t space_when_parse_started;
	double when_created;
	double when_last_called;
	double cumulative_time;
	bool   memory_exhausted;
	bool   timer_expired;
};

struct Parse_Options_s
{
	/* General options */
	short verbosity;       /* Level of detail to give about the computation 0 */
	char * debug;          /* comma-separated function names to debug "" */
	char * test;           /* comma-separated features to test "" */
	Resources resources;   /* For deciding when to abort the parsing */

	/* Options governing the tokenizer (sentence-splitter) */
	short use_spell_guess;  /* Perform spell-guessing of unknown words. */

	/* Choice of the parser to use */
	bool use_sat_solver;   /* Use the Boolean SAT based parser */
	bool use_viterbi;      /* Use the Viterbi decoder-based parser */

	/* Options governing the parser internals operation */
	double disjunct_cost;  /* Max disjunct cost to allow */
	short min_null_count;  /* The minimum number of null links to allow */
	short max_null_count;  /* The maximum number of null links to allow */
	bool islands_ok;       /* If TRUE, then linkages with islands
	                          (separate component of the link graph)
	                          will be generated (default=FALSE) */
	bool use_cluster_disjuncts; /* Attempt using a broader list of disjuncts */
	size_t short_length;   /* Links that are limited in length can be
	                          no longer than this.  Default = 6 */
	bool all_short;        /* If true, there can be no connectors that are exempt */
	bool repeatable_rand;  /* Reset rand number gen after every parse. */

	/* Options governing post-processing */
	bool perform_pp_prune; /* Perform post-processing-based pruning */
	size_t twopass_length; /* min sent length for two-pass post processing */
	Cost_Model cost_model; /* For sorting linkages in post_processing */

	/* Options governing the generation of linkages. */
	size_t linkage_limit;  /* The maximum number of linkages processed 100 */
	bool display_morphology;/* if true, print morpho analysis of words */
};

struct Connector_set_s
{
	Connector ** hash_table;
	unsigned int table_size;
};

struct Link_s
{
	size_t lw;              /* Offset into Linkage->word NOT Sentence->word */
	size_t rw;              /* Offset into Linkage->word NOT Sentence->word */
	Connector * lc;
	Connector * rc;
	const char * link_name; /* Spelling of full link name */
};

struct Parse_info_struct
{
	unsigned int   x_table_size;
	unsigned int   log2_x_table_size;
	X_table_connector ** x_table;  /* Hash table */
	Parse_set *    parse_set;
	int            N_words; /* Number of words in current sentence;
	                           Computed by separate_sentence() */

	/* thread-safe random number state */
	unsigned int rand_state;
};


struct Sentence_s
{
	Dictionary  dict;           /* Words are defined from this dictionary */
	const char *orig_sentence;  /* Copy of original sentence */
	size_t length;              /* Number of words */
	Word  *word;                /* Array of words after tokenization */
	String_set *   string_set;  /* Used for assorted strings */

	/* Wordgraph stuff. FIXME: typedef for structs. */
	Gword *wordgraph;            /* Tokenization wordgraph */
	Gword *last_word;            /* FIXME Last issued word */
	struct word_queue            /* Element in queue of words to tokenize */
	{
		Gword *word;
		struct word_queue *next;
	} *word_queue;
	struct word_queue *word_queue_last;
	size_t gword_node_num;       /* Debug - for differentiating between
	                                wordgraph nodes with identical subwords. */

	/* Parse results */
	int    num_linkages_found;  /* Total number before postprocessing.  This
	                               is returned by the count() function */
	size_t num_linkages_alloced;/* Total number of linkages allocated.
	                               the number post-processed might be fewer
	                               because some are non-canonical */
	size_t num_linkages_post_processed;
	                            /* The number of linkages that are actually
	                               put into the array that was alloced.
	                               This is not the same as num alloced
	                               because some may be non-canonical. */
	size_t num_valid_linkages;  /* Number with no pp violations */
	size_t null_count;          /* Number of null links in linkages */
	Linkage        lnkages;     /* Sorted array of valid & invalid linkages */
	Postprocessor * postprocessor;
	Postprocessor * constituent_pp;

	/* parse_info not used by SAT solver */
	Parse_info     parse_info;  /* Set of parses for the sentence */

	/* thread-safe random number state */
	unsigned int rand_state;

#ifdef USE_SAT_SOLVER
	void *hook;                 /* Hook for the SAT solver */
#endif /* USE_SAT_SOLVER */
};

/*********************************************************
 *
 * Linkages
 *
 **********************************************************/

/**
 * This summarizes the linkage status.
 */
struct Linkage_info_struct
{
	int index;            /* Index into the parse_set */
	bool discarded;
	short N_violations;
	short unused_word_cost;
	short link_cost;

	double disjunct_cost;
	double corpus_cost;
	const char *pp_violation_msg;
};

/**
 * num_links:
 *   The number of links in the current linkage.  Computed by
 *   extract_linkage().
 *
 * chosen_disjuncts[]
 *   This is an array pointers to disjuncts, one for each word, that is
 *   computed by extract_links().  It represents the chosen disjuncts
 *   for the current linkage.  It is used to compute the cost of the
 *   linkage, and also by compute_chosen_words() to compute the
 *   chosen_words[].
 *
 * link_array[]
 *   This is an array of links.  These links define the current linkage.
 *   It is computed by extract_links().  It is used by analyze_linkage().
 */
struct Linkage_s
{
	WordIdx         num_words;    /* Number of (tokenized) words */
	bool            is_sent_long; /* num_words >= twopass_length */
	const char *  * word;         /* Array of word spellings */

	size_t          num_links;    /* Number of links in array */
	Link *          link_array;   /* Array of links */
	size_t          lasz;         /* Alloc'ed length of link_array */

	Disjunct **     chosen_disjuncts; /* Disjuncts used, one per word */
	size_t          cdsz;         /* Alloc'ed length of chosen_disjuncts */
	char **         disjunct_list_str; /* Stringified version of above */
#ifdef USE_CORPUS
	Sense **        sense_list;   /* Word senses, inferred from disjuncts */
#endif

	Gword **wg_path;              /* Linkage Wordgraph path */
	Gword **wg_path_display;      /* ... for !morphology=0. Experimental. */
	//size_t *wg_path_index;      /* Displayed-word indices in wg_path (FIXME?)*/

	Linkage_info    lifo;         /* Parse_set index and cost information */
	PP_info *       pp_info;      /* PP domain info, one for each link */

	Sentence        sent;         /* Used for common linkage data */
};

// XXX FIXME
// This needs a home.
bool sane_linkage_morphism(Sentence, Linkage, Parse_Options);

#endif
