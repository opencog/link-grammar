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

static void notify_no_complete_linkages(unsigned int null_count,
                                        unsigned int max_null_count)
{
		if ((0 == null_count) && (0 < max_null_count) && verbosity > 0)
			prt_error("No complete linkages found.\n");
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
 * To increase the parsing speed, before invoking do_parse(), invoke
 * pp_and_power_prune() to remove connectors which have no possibility to
 * connect. Since power_prune() includes a significant optimization if it
 * assumes that the linkage has no more than a specific number of null
 * links (aka null_count), call it with the current number of null_count
 * for which do_count() is invoked.
 *
 * In order to be able to so repeat the pruning step, we need to keep
 * the original disjunct/connectors in order to prune them again.
 * This is done only when needed, i.e. when we are invoked with
 * min_null_count != max_null_count (a typical case is that they are
 * both 0).
 *
 * So in case this optimization has been done and a parse (e.g.
 * a parse when null_count==0) is not found, we are left with sentence
 * disjuncts which are not appropriate to continue do_parse() tries with
 * a greater null_count. To solve that, we need to restore the original
 * disjuncts of the sentence and call pp_and_power_prune() once again.
 */
void classic_parse(Sentence sent, Parse_Options opts)
{
	fast_matcher_t * mchxt = NULL;
	count_context_t * ctxt = NULL;
	Tracon_sharing *ts_parsing = NULL;
	void *saved_memblock = NULL;
	int current_prune_level = -1; /* -1: No pruning has been done yet. */
	int needed_prune_level = opts->min_null_count;
	bool more_pruning_possible = false;

	unsigned int max_null_count = opts->max_null_count;
	max_null_count = (unsigned int)MIN(max_null_count, sent->length);
	bool one_step_parse = (unsigned int)opts->min_null_count != max_null_count;
	int max_prune_level = (int)max_null_count;
	bool optimize_pruning = true; /* Perform pruning null count optimization. */

	unsigned int *ncu[2];
	ncu[0] = alloca(sent->length * sizeof(*ncu[0]));
	ncu[1] = alloca(sent->length * sizeof(*ncu[1]));

	/* Null-count optimization not implemented for islands_ok==true. */
	if (opts->islands_ok)
		optimize_pruning = false;

	/* Pruning per null-count and one-step-parse are costly for sentences
	 * whose parsing takes tens of milliseconds or so. Disable them for
	 * short-enough sentences. */
	if (sent->length < sent->min_len_multi_pruning)
		optimize_pruning = false;

	if (!optimize_pruning)
	{
		/* Turn-off null-count optimization. */
		if (opts->min_null_count == 0)
			max_prune_level = 0;
		else
		{
			needed_prune_level = MAX_SENTENCE;
			one_step_parse = false;
		}
	}

	/* Build lists of disjuncts */
	prepare_to_parse(sent, opts);
	if (resources_exhausted(opts->resources)) return; /* Nothing to free yet. */

	Tracon_sharing *ts_pruning = pack_sentence_for_pruning(sent);
	free_sentence_disjuncts(sent);

	if (one_step_parse)
	{
		/* Save the disjuncts for possible parse w/ an increased null count. */
		saved_memblock = save_disjuncts(sent, ts_pruning);
	}

	print_time(opts, "Encoded for pruning%s%s",
	           (NULL == ts_pruning->tracon_list) ? " (skipped)" : "",
	           (one_step_parse) ? " (one-step)" : "");

	for (unsigned int nl = opts->min_null_count; nl <= max_null_count; nl++)
	{
		sent->null_count = nl;

		/* We may be here again for parsing with a higher null_count since
		 * num_valid_linkages of the previous parse was 0 because all the
		 * linkages had P.P. violations. Ensure that in case of a timeout we
		 * will not end up with the previous num_linkages_found. */
		sent->num_linkages_found = 0;
		sent->num_valid_linkages = 0;
		sent->num_linkages_post_processed = 0;

		if (needed_prune_level > current_prune_level)
		{
			current_prune_level = needed_prune_level;
			if (needed_prune_level < max_prune_level)
				needed_prune_level++;
			else
				needed_prune_level = MAX_SENTENCE;

			if (more_pruning_possible)
				restore_disjuncts(sent, saved_memblock, ts_pruning);

			more_pruning_possible =
				one_step_parse && (current_prune_level != MAX_SENTENCE);

			unsigned int expected_null_count =
				pp_and_power_prune(sent, ts_pruning, current_prune_level, opts,
				                   ncu);
			if (expected_null_count > nl)
			{
				if (opts->verbosity >= D_USER_TIMES)
				{
					prt_error("#### Skip parsing (w/%u ", nl);
					if (expected_null_count-1 > nl)
						prt_error("to %u nulls)\n", expected_null_count-1);
					else
						prt_error("null%s)\n", (nl != 1) ? "s" : "");
				}
				notify_no_complete_linkages(nl, max_null_count);
				nl = expected_null_count-1;
				/* To get a result, parse w/null count which is at most one less
				 * than the number of tokens (w/all nulls there is no linkage). */
				if (nl == sent->length-1) nl--;
				continue;
			}
		}

		if (NULL != ts_pruning)
		{

			free_tracon_sharing(ts_parsing);
			ts_parsing = pack_sentence_for_parsing(sent);
			print_time(opts, "Encoded for parsing");

			if (!more_pruning_possible)
			{
				/* At this point no further pruning will be done. Free the
				 * pruning tracon stuff here instead of at the end. */
				if (NULL != ts_pruning)
				{
					free(ts_pruning->memblock);
					free_tracon_sharing(ts_pruning);
					ts_pruning = NULL;
					if (NULL != saved_memblock)
						free(saved_memblock);
				}
			}

			gword_record_in_connector(sent);

			free_fast_matcher(sent, mchxt);
			mchxt = alloc_fast_matcher(sent, ncu);
			print_time(opts, "Initialized fast matcher");
			if (resources_exhausted(opts->resources)) goto parse_end_cleanup;
		}

		free_linkages(sent);

		free_count_context(ctxt, sent);
		ctxt = alloc_count_context(sent, ts_parsing);

		sent->num_linkages_found = do_parse(sent, mchxt, ctxt, opts);

		print_time(opts, "Counted parses (%d w/%u null%s)",
		           sent->num_linkages_found, sent->null_count,
		           (sent->null_count != 1) ? "s" : "");

		/* In case of a timeout, the linkage is partial and may be
		 * inconsistent. It is also usually different on each run.
		 * So in that case, pretend that the linkage count is 0. */
		if (resources_exhausted(opts->resources))
		{
			sent->num_linkages_found = 0;
			goto parse_end_cleanup;
		}

		if (sent->num_linkages_found > 0)
		{
			extractor_t * pex = extractor_new(sent->length, sent->rand_state);
			bool ovfl = setup_linkages(sent, pex, mchxt, ctxt, opts);
			process_linkages(sent, pex, ovfl, opts);
			free_extractor(pex);

			post_process_lkgs(sent, opts);
			if (resources_exhausted(opts->resources))
			{
				sent->num_linkages_found = 0;
				sent->num_valid_linkages = 0;
				sent->num_linkages_post_processed = 0;
				goto parse_end_cleanup;
			}

			if (sent->num_valid_linkages > 0) break;

			if (verbosity >= D_USER_INFO)
			{
				if ((sent->num_valid_linkages == 0) &&
				    (sent->num_linkages_post_processed > 0) &&
				    ((int)opts->linkage_limit < sent->num_linkages_found) &&
				    (PARSE_NUM_OVERFLOW >= sent->num_linkages_found))
					prt_error("Info: All examined linkages (%zu) had P.P. violations.\n"
					          "Consider increasing the linkage limit.\n"
					          "At the command line, use !limit\n",
					          sent->num_linkages_post_processed);
			}
		}

		notify_no_complete_linkages(nl, max_null_count);
	}
	sort_linkages(sent, opts);

parse_end_cleanup:
	if (NULL != ts_pruning)
	{
		free(ts_pruning->memblock);
		free_tracon_sharing(ts_pruning);
		free(saved_memblock);
	}
	free_tracon_sharing(ts_parsing);
	free_count_context(ctxt, sent);
	free_fast_matcher(sent, mchxt);
}
