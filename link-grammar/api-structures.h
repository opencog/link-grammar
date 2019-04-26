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
#include "memory-pool.h"
#include "string-set.h"
#include "string-id.h"

/* Performance tuning.
 * For short sentences, setting suffix IDs and packing takes more
 * resources than it saves. If this overhead is improved, these
 * limit can be set lower. */
#define SENTENCE_MIN_LENGTH_TRAILING_HASH 10

typedef struct Cost_Model_s Cost_Model;
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
	char * debug;          /* Comma-separated function names to debug "" */
	char * test;           /* Comma-separated features to test "" */
	Resources resources;   /* For deciding when to abort the parsing */

	/* Options governing the tokenizer (sentence-splitter) */
	short use_spell_guess; /* Up to this many spell-guesses per unknown word 7 */

	/* Choice of the parser to use */
	bool use_sat_solver;   /* Use the Boolean SAT based parser */

	/* Options governing the parser internals operation */
	double disjunct_cost;  /* Max disjunct cost to allow */
	short min_null_count;  /* The minimum number of null links to allow */
	short max_null_count;  /* The maximum number of null links to allow */
	bool islands_ok;       /* If TRUE, then linkages with islands
	                          (separate component of the link graph)
	                          will be generated (default=FALSE) */
	bool use_cluster_disjuncts; /* Attempt using a broader list of disjuncts */
	size_t short_length;   /* Links that are limited in length can be
	                          no longer than this.  Default = 16 */
	bool all_short;        /* If true, there can be no connectors that are exempt */
	bool repeatable_rand;  /* Reset rand number gen after every parse. */

	/* Options governing post-processing */
	bool perform_pp_prune; /* Perform post-processing-based pruning TRUE */
	size_t twopass_length; /* Min sent length for two-pass post processing */
	Cost_Model cost_model; /* For sorting linkages after parsing. */

	/* Options governing the generation of linkages. */
	size_t linkage_limit;  /* The maximum number of linkages processed 100 */
	bool display_morphology;/* If true, print morpho analysis of words FALSE */
};

typedef struct word_queue_s word_queue_t;
struct word_queue_s
{
	Gword *word;
	word_queue_t *next;
};

/* A jet is an ordered set of connectors all pointing in the same
 * direction (left, or right). Every disjunct can be split into two jets;
 * that is, a disjunct is a pair of jets, and so each word consists of a
 * collection of pairs of jets. The array num_cnctrs_per_word holds the
 * number of the connectors on the disjuncts of each word; it is used for
 * sizing the power table in power_prune().
 * On one-step-parse (automatic parsing with null words if the is no
 * solution without 0 nulls) the table is preserved, but currently its
 * jet pointers are recalculated. This is the reason the table is here and
 * not private to power_prune().
 * Possible FIXME: merge with the power table, and allocate it in
 * do_parse (like allocate_count_context()).
 */
typedef struct
{
Connector *c;
} JT_entry;

/* Jet sharing: [0] - left side; [1] - right side. */
typedef struct
{
	JT_entry *table[2];           /* Indexed by jet ID */
	unsigned int entries[2];      /* Number of table entries */
	unsigned int *num_cnctrs_per_word[2]; /* Indexed by word number */
	String_id *csid[2];           /* For generating unique jet IDs */
} jet_sharing_t;

struct Sentence_s
{
	Dictionary  dict;           /* Words are defined from this dictionary */
	const char *orig_sentence;  /* Copy of original sentence */
	size_t length;              /* Number of words */
	Word  *word;                /* Array of words after tokenization */
	String_set *   string_set;  /* Used for assorted strings */
	Pool_desc * fm_Match_node;  /* Fast-matcher Match_node memory pool */
	Pool_desc * Table_connector_pool; /* Count memoizing memory pool */
	Pool_desc * E_list_pool;
	Pool_desc * Exp_pool;
	Pool_desc * X_node_pool;
	Pool_desc * Disjunct_pool;
	Pool_desc * Connector_pool;

	/* Trailing connector encoding stuff (tracon_id), used for speeding up
	 * parsing (the classic one for now) of long sentences. */
	String_id *connector_tracon_id; /* For connector trailing sequence IDs */
	unsigned int num_tracon_id;     /* Currently unused. */

	jet_sharing_t jet_sharing;   /* Disjunct l/r duplication sharing */
	size_t min_len_sharing;      /* Do trailing encoding + jet-sharing.
	                                Disjunct/connector packing is also done
	                                in that case (only). */

	/* Wordgraph stuff. FIXME: create stand-alone struct for these. */
	Gword *wordgraph;            /* Tokenization wordgraph */
	Gword *last_word;            /* FIXME Last issued word */
	word_queue_t *word_queue;    /* Element in queue of words to tokenize */
	word_queue_t *word_queue_last;
	size_t gword_node_num;       /* Debug - for differentiating between
	                                wordgraph nodes with identical subwords. */

	/* Parse results */
	int    num_linkages_found;  /* Total number before postprocessing.  This
	                               is returned by the do_count() function */
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

	/* thread-safe random number state */
	unsigned int rand_state;

#ifdef USE_SAT_SOLVER
	void *hook;                 /* Hook for the SAT solver */
#endif /* USE_SAT_SOLVER */
	void *disjuncts_connectors_memblock;
};

#endif
