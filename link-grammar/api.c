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
#include "tokenize/lookup-exprs.h"
#include "tokenize/tokenize.h"
#include "tokenize/word-structures.h"   // Word_struct
#include "utilities.h"

/***************************************************************
*
* Routines for creating destroying and processing Sentences
*
****************************************************************/

/* From options.c */
extern unsigned int global_rand_state;

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

	Dictionary dict = sent->dict;
	if (!setup_dialect(dict, opts))
		return -4;

	/* Flatten the word graph created by separate_sentence() to a
	 * 2D-word-array which is compatible to the current parsers. */
	flatten_wordgraph(sent, opts);

	/* This may fail if UNKNOWN_WORD is needed, but is not defined in
	 * the dictionary. In that case, warnings for the unknown word were
	 * already printed. */
	if (!build_sentence_expressions(sent, opts))
	{
		err_ctxt ec = { sent };
		err_msgc(&ec, lg_Error,
			"Cannot parse sentence with unknown words!\n");

		return -2;
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
	pool_delete(sent->Table_tracon_pool);
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
	Dictionary dict = sent->dict;
	if (IS_GENERATION(dict))
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
		opts->disjunct_cost = dict->default_max_disjunct_cost;

	sent->num_valid_linkages = 0;

	/* If the sentence has not yet been split, do so now.
	 * This is for backwards compatibility, for existing programs
	 * that do not explicitly call the splitter.
	 */
	if (0 == sent->length)
	{
		int rc = sentence_split(sent, opts);
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
