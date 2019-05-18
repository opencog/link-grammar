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
#include "api-structures.h"
#include "count.h"
#include "dict-common/dict-common.h"   // For Dictionary_s
#include "disjunct-utils.h"
#include "extract-links.h"
#include "fast-match.h"
#include "linkage/analyze-linkage.h"
#include "linkage/linkage.h"
#include "linkage/sane.h"
#include "parse.h"
#include "post-process/post-process.h"
#include "preparation.h"
#include "prune.h"
#include "resources.h"
#include "tokenize/word-structures.h"  // For Word_struct

#define D_PARSE 5 /* Debug level for this file. */

static Linkage linkage_array_new(int num_to_alloc)
{
	Linkage lkgs = (Linkage) malloc(num_to_alloc * sizeof(struct Linkage_s));
	memset(lkgs, 0, num_to_alloc * sizeof(struct Linkage_s));
	return lkgs;
}

static bool setup_linkages(Sentence sent, extractor_t* pex,
                          fast_matcher_t* mchxt,
                          count_context_t* ctxt,
                          Parse_Options opts)
{
	bool overflowed = build_parse_set(pex, sent, mchxt, ctxt, sent->null_count, opts);
	print_time(opts, "Built parse set");

	if (overflowed && (1 < opts->verbosity))
	{
		err_ctxt ec = { sent };
		err_msgc(&ec, lg_Warn, "Count overflow.\n"
			"Considering a random subset of %zu of an unknown and large number of linkages\n",
			opts->linkage_limit);
	}

	if (sent->num_linkages_found == 0)
	{
		sent->num_linkages_alloced = 0;
		sent->num_linkages_post_processed = 0;
		sent->num_valid_linkages = 0;
		sent->lnkages = NULL;
		return overflowed;
	}

	sent->num_linkages_alloced =
		MIN(sent->num_linkages_found, (int) opts->linkage_limit);

	/* Now actually malloc the array in which we will process linkages. */
	/* We may have been called before, e.g. this might be a panic parse,
	 * and the linkages array may still be there from last time.
	 * XXX free_linkages() zeros sent->num_linkages_found. */
	if (sent->lnkages) free_linkages(sent);
	sent->lnkages = linkage_array_new(sent->num_linkages_alloced);

	return overflowed;
}

/**
 *  Print the chosen_disjuncts words.
 *  This is used for debug, e.g. for tracking them in the Wordgraph display.
 */
static void print_chosen_disjuncts_words(const Linkage lkg, bool prt_optword)
{
	size_t i;
	dyn_str *djwbuf = dyn_str_new();

	err_msg(lg_Debug, "Linkage %p (%zu words): ", lkg, lkg->num_words);
	for (i = 0; i < lkg->num_words; i++)
	{
		Disjunct *cdj = lkg->chosen_disjuncts[i];
		const char *djw; /* disjunct word - the chosen word */

		if (NULL == cdj)
			djw = (prt_optword && lkg->sent->word[i].optional) ? "{}" : "[]";
		else if ('\0' == cdj->word_string[0])
			djw = "\\0"; /* null string - something is wrong */
		else
			djw = cdj->word_string;

		dyn_strcat(djwbuf, djw);
		dyn_strcat(djwbuf, " ");
	}
	err_msg(lg_Debug, "%s\n", djwbuf->str);
	dyn_str_delete(djwbuf);
}

#define D_PL 7
/**
 * This fills the linkage array with morphologically-acceptable
 * linkages.
 */
static void process_linkages(Sentence sent, extractor_t* pex,
                             bool overflowed, Parse_Options opts)
{
	if (0 == sent->num_linkages_found) return;
	if (0 == sent->num_linkages_alloced) return; /* Avoid a later crash. */

	/* Pick random linkages if we get more than what was asked for. */
	bool pick_randomly = overflowed ||
	    (sent->num_linkages_found > (int) sent->num_linkages_alloced);

	sent->num_valid_linkages = 0;
	size_t N_invalid_morphism = 0;

	int itry = 0;
	size_t in = 0;
	int maxtries;

	/* In the case of overflow, which will happen for some long
	 * sentences, but is particularly common for the amy/ady random
	 * splitters, we want to find as many morpho-acceptable linkages
	 * as possible, but keep the CPU usage down, as these might be
	 * very rare. This is due to a bug/feature in the interaction
	 * between the word-graph and the parser: valid morph linkages
	 * can be one-in-a-thousand.. or worse.  Search for them, but
	 * don't over-do it.
	 * Note: This problem has recently been alleviated by an
	 * alternatives-compatibility check in the fast matcher - see
	 * alt_connection_possible().
	 */
#define MAX_TRIES 250000

	if (pick_randomly)
	{
		/* Try picking many more linkages, but not more than possible. */
		maxtries = MIN((int) sent->num_linkages_alloced + MAX_TRIES,
		               sent->num_linkages_found);
	}
	else
	{
		maxtries = sent->num_linkages_alloced;
	}

	bool need_init = true;
	for (itry=0; itry<maxtries; itry++)
	{
		Linkage lkg = &sent->lnkages[in];
		Linkage_info * lifo = &lkg->lifo;

		/* Negative values tell extract-links to pick randomly; for
		 * reproducible-rand, the actual value is the rand seed. */
		lifo->index = pick_randomly ? -(itry+1) : itry;

		if (need_init)
		{
			partial_init_linkage(sent, lkg, sent->length);
			need_init = false;
		}
		extract_links(pex, lkg);
		compute_link_names(lkg, sent->string_set);

		if (verbosity_level(+D_PL))
		{
			err_msg(lg_Debug, "chosen_disjuncts before:\n\\");
			print_chosen_disjuncts_words(lkg, /*prt_opt*/true);
		}

		if (sane_linkage_morphism(sent, lkg, opts))
		{
			remove_empty_words(lkg);

			if (verbosity_level(+D_PL))
			{
				err_msg(lg_Debug, "chosen_disjuncts after:\n\\");
				print_chosen_disjuncts_words(lkg, /*prt_opt*/false);
			}

			need_init = true;
			in++;
			if (in >= sent->num_linkages_alloced) break;
		}
		else
		{
			N_invalid_morphism++;
			lkg->num_links = 0;
			lkg->num_words = sent->length;
			// memset(lkg->link_array, 0, lkg->lasz * sizeof(Link));
			memset(lkg->chosen_disjuncts, 0, sent->length * sizeof(Disjunct *));
		}
	}

	/* The last one was alloced, but never actually used. Free it. */
	if (!need_init) free_linkage(&sent->lnkages[in]);

	sent->num_valid_linkages = in;

	/* The remainder of the array is garbage; we never filled it in.
	 * So just pretend that it's shorter than it is */
	sent->num_linkages_alloced = sent->num_valid_linkages;

	if (verbosity >= D_USER_INFO)
	{
		prt_error("Info: sane_morphism(): %zu of %d linkages had "
		        "invalid morphology construction\n", N_invalid_morphism,
		        itry + (itry != maxtries));
	}
}

static void sort_linkages(Sentence sent, Parse_Options opts)
{
	if (0 == sent->num_linkages_found) return;

	/* It they're randomized, don't bother sorting */
	if (0 != sent->rand_state && sent->dict->shuffle_linkages) return;

	qsort((void *)sent->lnkages, sent->num_linkages_alloced,
	      sizeof(struct Linkage_s),
	      (int (*)(const void *, const void *))opts->cost_model.compare_fn);

	print_time(opts, "Sorted all linkages");
}

/**
 * classic_parse() -- parse the given sentence.
 * Perform parsing, using the original link-grammar parsing algorithm
 * given in the original link-grammar papers.
 *
 * Do the parse with the minimum number of null-links within the range
 * specified by opts->min_null_count and opts->max_null_count.
 *
 * To that end, call do_parse() with an increasing null_count, from
 * opts->min_null_count up to (including) opts->max_null_count, until a
 * parse is found.
 *
 * A note about the disjuncts save/restore that is done here:
 * To increase the parsing speed, before invoking do_parse(),
 * pp_and_power_prune() is invoked to remove connectors which have no
 * possibility to connect. It includes a significant optimization when
 * null_count==0 that makes a more aggressive removal, but this
 * optimization is not appropriate when null_count>0.
 *
 * So in case this optimization has been done and a complete parse (i.e.
 * a parse when null_count==0) is not found, we are left with sentence
 * disjuncts which are not appropriate to continue do_parse() tries with
 * null_count>0. To solve that, we need to restore the original
 * disjuncts of the sentence and call pp_and_power_prune() once again.
 */
void classic_parse(Sentence sent, Parse_Options opts)
{
	fast_matcher_t * mchxt = NULL;
	count_context_t * ctxt = NULL;
	void *saved_memblock = NULL;
	int min_null_count = opts->min_null_count;
	int current_prune_level = -1; /* -1: No pruning has been done yet. */
	int needed_prune_level = opts->min_null_count;

	unsigned int max_null_count = opts->max_null_count;
	max_null_count = (unsigned int)MIN(max_null_count, sent->length);
	bool one_step_parse = (0 == opts->min_null_count) && (0 < max_null_count);
	const int max_prune_level = 1;

	/* Build lists of disjuncts */
	prepare_to_parse(sent, opts);
	if (resources_exhausted(opts->resources)) return;

	Tracon_sharing *ts_pruning = pack_sentence_for_pruning(sent);
	print_time(opts, "Encode connectors for pruning");
	free_sentence_disjuncts(sent);

	if (one_step_parse)
	{
		/* Save the disjuncts for possible parse w/ an increased null count. */
		saved_memblock = save_disjuncts(sent, ts_pruning);
	}

	for (unsigned int nl = opts->min_null_count; nl <= max_null_count; nl++)
	{
		Count_bin hist;
		s64 total;

		if ((needed_prune_level <= max_prune_level) &&
		    (needed_prune_level > current_prune_level))
		{
			if (one_step_parse &&
			   (needed_prune_level > 0) && (-1 != current_prune_level))
			{
				/* We need to prune *again*. Restore the original disjuncts. */
				restore_disjuncts(sent, saved_memblock, ts_pruning);
				/* Free the memory of the previous pack_sentence_for_parsing(). */
				free(sent->dc_memblock);
				sent->dc_memblock = NULL;
			}

			current_prune_level = needed_prune_level;
			opts->min_null_count = needed_prune_level;

			pp_and_power_prune(sent, ts_pruning, opts);
			if (one_step_parse)
				needed_prune_level++; /* In case we need to prune again. */

			Tracon_sharing *ts_parsing = pack_sentence_for_parsing(sent);
			print_time(opts, "Encode connectors for parsing");
			if (NULL != ts_parsing)
			{
				sent->dc_memblock = ts_parsing->memblock;
				free_tracon_sharing(ts_parsing);
			}

			if (!one_step_parse || (max_prune_level == opts->min_null_count))
			{
				/* At this point no further pruning will be done. Free the
				 * pruning tracon stuff here instead of at the end. */
				if (NULL != ts_pruning)
				{
					free(ts_pruning->memblock);
					free_tracon_sharing(ts_pruning);
					ts_pruning = NULL;
					free(saved_memblock);
				}
			}

			gword_record_in_connector(sent);
			if (resources_exhausted(opts->resources)) break;

			free_count_context(ctxt, sent);
			ctxt = alloc_count_context(sent);
			free_fast_matcher(sent, mchxt);
			mchxt = alloc_fast_matcher(sent);
			print_time(opts, "Initialized fast matcher");
		}

		if (resources_exhausted(opts->resources)) break;
		free_linkages(sent);

		sent->null_count = nl;
		hist = do_parse(sent, mchxt, ctxt, sent->null_count, opts);
		total = hist_total(&hist);

		/* total is 64-bit, num_linkages_found is 32-bit. Clamp */
		total = (total > INT_MAX) ? INT_MAX : total;
		total = (total < 0) ? INT_MAX : total;

		sent->num_linkages_found = (int) total;
		print_time(opts, "Counted parses");

		if (verbosity >= D_USER_INFO)
		{
			prt_error("Info: Total count with %zu null links: %lld\n",
			        sent->null_count, total);
		}

		extractor_t * pex = extractor_new(sent->length, sent->rand_state);
		bool ovfl = setup_linkages(sent, pex, mchxt, ctxt, opts);
		process_linkages(sent, pex, ovfl, opts);
		free_extractor(pex);

		post_process_lkgs(sent, opts);

		if (sent->num_valid_linkages > 0) break;

		if (verbosity >= D_USER_INFO)
		{
			if ((sent->num_valid_linkages == 0) &&
				 (sent->num_linkages_post_processed > 0) &&
				 ((int)opts->linkage_limit < sent->num_linkages_found))
				prt_error("Info: All examined linkages (%zu) had P.P. violations.\n"
						  "Consider increasing the linkage limit.\n"
						  "At the command line, use !limit\n",
						  sent->num_linkages_post_processed);
		}

		if ((0 == nl) && (0 < max_null_count) && verbosity > 0)
			prt_error("No complete linkages found.\n");
	}
	opts->min_null_count = min_null_count;
	sort_linkages(sent, opts);

	if (NULL != ts_pruning)
	{
		free(ts_pruning->memblock);
		free_tracon_sharing(ts_pruning);
		free(saved_memblock);
	}
	free_count_context(ctxt, sent);
	free_fast_matcher(sent, mchxt);
}
