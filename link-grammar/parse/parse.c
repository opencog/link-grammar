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

void linkage_array_free(Linkage lkgs)
{
	free(lkgs);
}

static void find_unused_disjuncts(Sentence sent, extractor_t *pex)
{
	const size_t disjunct_used_sz =
		sizeof(bool) * sent->wildcard_word_num_disjuncts;
	sent->disjunct_used = malloc(disjunct_used_sz);

	memset(sent->disjunct_used, 0, disjunct_used_sz);

	if (pex != NULL)
		mark_used_disjuncts(pex, sent->disjunct_used);

	if (verbosity_level(+D_PARSE))
	{
		unsigned int num_unused = 0;
		for (unsigned int i = 0; i < sent->wildcard_word_num_disjuncts; i++)
			if (!sent->disjunct_used[i]) num_unused++;
		prt_error("Info: Unused disjuncts %u/%u\n", num_unused,
		          sent->wildcard_word_num_disjuncts);
	}
}

static void setup_linkages(Sentence sent, extractor_t* pex,
                          fast_matcher_t* mchxt,
                          count_context_t* ctxt,
                          Parse_Options opts)
{
	sent->overflowed = build_parse_set(pex, sent, mchxt, ctxt, sent->null_count, opts);
	print_time(opts, "Built parse set");

	if (sent->overflowed && (1 < opts->verbosity) && !IS_GENERATION(sent->dict))
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
		return;
	}

	sent->num_linkages_alloced =
		MIN(sent->num_linkages_found, (int) opts->linkage_limit);

	/* Now actually malloc the array in which we will process linkages. */
	/* We may have been called before, e.g. this might be a panic parse,
	 * and the linkages array may still be there from last time.
	 * XXX free_linkages() zeros sent->num_linkages_found. */
	if (sent->lnkages) free_linkages(sent);
	sent->lnkages = linkage_array_new(sent->num_linkages_alloced);
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
		{
			djw = (prt_optword && lkg->sent->word[i].optional) ? "{}" : "[]";
		}
		else if (0 == cdj->is_category)
		{
			if ('\0' == cdj->word_string[0])
				djw = "\\0"; /* null string - something is wrong */
			else
				djw = cdj->word_string;
		}
		else
		{
			if ((NULL == cdj->category))
			{
				djw = "\\0"; /* something is wrong */
			}
			else
			{
				char *cbuf = alloca(32); /* much more space than needed */
				snprintf(cbuf, 32, "Category[0]:%u", cdj->category[0].num);
				djw = cbuf;
			}
		}

		dyn_strcat(djwbuf, djw);
		dyn_strcat(djwbuf, " ");
	}
	err_msg(lg_Debug, "%s\n", djwbuf->str);
	dyn_str_delete(djwbuf);
}

/**
 * Return \c true iff \p sent has an optional word.
 */
static bool optional_word_exists(Sentence sent)
{
	for (WordIdx w = 0; w < sent->length; w++)
		if (sent->word[w].optional) return true;

	return false;
}

#define D_PL 7
/**
 * This fills the linkage array with morphologically-acceptable
 * linkages.
 */
static void process_linkages(Sentence sent, extractor_t* pex,
                             Parse_Options opts)
{
	if (0 == sent->num_linkages_found) return;
	if (0 == sent->num_linkages_alloced) return; /* Avoid a later crash. */

	/* Pick random linkages if we get more than what was asked for. */
	bool pick_randomly = sent->overflowed ||
	    (sent->num_linkages_found > (int) opts->linkage_limit);

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

	bool need_sane_morphism = !IS_GENERATION(sent->dict) ||
	                          optional_word_exists(sent);
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

		if (need_sane_morphism)
		{
			if (sane_linkage_morphism(sent, lkg, opts))
			{
				remove_empty_words(lkg);

				if (verbosity_level(+D_PL))
				{
					err_msg(lg_Debug, "chosen_disjuncts after:\n\\");
					print_chosen_disjuncts_words(lkg, /*prt_opt*/false);
				}
			}
			else
			{
				N_invalid_morphism++;
				lkg->num_links = 0;
				lkg->num_words = sent->length;
				// memset(lkg->link_array, 0, lkg->lasz * sizeof(Link));
				memset(lkg->chosen_disjuncts, 0, sent->length * sizeof(Disjunct *));

				continue;
			}
		}

		if (IS_GENERATION(sent->dict))
			compute_generated_words(sent, lkg);

		need_init = true;
		in++;
		if (in >= sent->num_linkages_alloced) break;
	}

	/* The last one was alloced, but never actually used. Free it. */
	if (!need_init) free_linkage(&sent->lnkages[in]);

	sent->num_valid_linkages = in;

	/* The remainder of the array is garbage; we never filled it in.
	 * So just pretend that it's shorter than it is */
	sent->num_linkages_alloced = sent->num_valid_linkages;

	if (verbosity >= D_USER_INFO)
	{
		lgdebug(0, "Info: sane_morphism(): %zu of %d linkages had "
		        "invalid morphology construction\n", N_invalid_morphism,
		        itry + (itry != maxtries));
	}
}

/**
 * Linkage-equivalent predicate. Return zero if they are equivalent,
 * else return +1 or -1. This does provide a stable sort; inequivalent
 * linkages are always sorted the same way.
 *
 * This assumes that the more basic inequivalence compares have already
 * been done. This only disambiguates the final little bit of
 * nearly-identical linkages.
 */
static int linkage_equiv_p(Linkage lpv, Linkage lnx)
{
	// Compare links
	for (uint32_t li=0; li<lpv->num_links; li++)
	{
		// Compare word-endpoints first. Most differences are likely
		// to be noticeable here.
		int lwd = lpv->link_array[li].lw - lnx->link_array[li].lw;
		if (lwd) return lwd;

		int rwd = lpv->link_array[li].rw - lnx->link_array[li].rw;
		if (rwd) return rwd;
	}

	// Compare words. The chosen_disjuncts->word_string is the
	// dictionary word. It can happen that two different dictionary
	// words can have the same disjunct, and thus result in the same
	// linkage. For backwards compat, we will report these as being
	// different, as printing will reveal the differences in words.
	for (uint32_t wi=0; wi<lpv->num_words; wi++)
	{
		// Parses with non-zero null count will have null words,
		// i.e. word without chosen_disjuncts. Avoid a null-pointer
		// deref in this case.
		if (NULL == lpv->chosen_disjuncts[wi])
		{
			// If one is null, both should be null. (I think this
			// will always be true, but I'm not sure.)
			if (NULL == lnx->chosen_disjuncts[wi]) continue;
			return 1;
		}
		if (lpv->chosen_disjuncts[wi]->word_string !=
		    lnx->chosen_disjuncts[wi]->word_string)
			return strcmp(lpv->chosen_disjuncts[wi]->word_string,
			              lnx->chosen_disjuncts[wi]->word_string);
	}

	// Compare connector types at the link endpoints. If we are here,
	// then the link endpoints landed on the same words. It would be
	// unusual if the link types differed, but we have to check.
	for (uint32_t li=0; li<lpv->num_links; li++)
	{
		if (lpv->link_array[li].lc != lnx->link_array[li].lc)
		{
			int diff = strcmp(
				lpv->link_array[li].lc->desc->string,
				lnx->link_array[li].lc->desc->string);
			if (diff) return diff;
		}
		if (lpv->link_array[li].rc != lnx->link_array[li].rc)
		{
			int diff = strcmp(
				lpv->link_array[li].rc->desc->string,
				lnx->link_array[li].rc->desc->string);
			if (diff) return diff;
		}
	}

	// Since the above performed a stable compare, we can safely mark
	// the second linkage as a duplicate of the first.
	lnx->dupe = true;
	return 0;
}

/**
 * VDAL == Compare by Violations, Disjunct, Link length.
 */
int VDAL_compare_linkages(Linkage l1, Linkage l2)
{
	Linkage_info * p1 = &l1->lifo;
	Linkage_info * p2 = &l2->lifo;

	if (p1->N_violations != p2->N_violations)
		return (p1->N_violations - p2->N_violations);

	if (p1->unused_word_cost != p2->unused_word_cost)
		return (p1->unused_word_cost - p2->unused_word_cost);

	float diff = p1->disjunct_cost - p2->disjunct_cost;

#define COST_EPSILON 1.0e-6
	if (COST_EPSILON < diff) return 1;
	if (diff < -COST_EPSILON) return -1;

	if (p1->link_cost != p2->link_cost)
		return (p1->link_cost - p2->link_cost);

	if (l1->num_words != l2->num_words)
		return l1->num_words - l2->num_words;

	// Don't bother sorting bad linkages any further.
	if (0 < p1->N_violations) return 0;

	return linkage_equiv_p(l1, l2);
}

/**
 * Remove duplicate linkages in the link array. Duplicates can appear
 * if the number of parses overflowed, or if the number of parses is
 * larger than the linkage array. In this case, random linkages will
 * be selected, and, by random chance, duplicate linkages can be
 * selected. When the alloc array is slightly less than the number of
 * linkages found, then as many as half(!) of the linkages can be
 * duplicates.
 *
 * This assumes that the duplicates have already been detected and
 * marked by setting `linkage->dupe=true` during linkage sorting.
 */
static void deduplicate_linkages(Sentence sent, int linkage_limit)
{
	/* No need for deduplication, if random selection wasn't done. */
	if (!sent->overflowed && (sent->num_linkages_found <= linkage_limit))
		return;

	// Deduplicate the valid linkages only; its not worth wasting
	// CPU time on the rest.  Sorting guarantees that the valid
	// linkages come first.
	uint32_t nl = sent->num_valid_linkages;
	if (2 > nl) return;

	// Sweep away duplicates
	uint32_t tgt = 0;
	uint32_t blkstart = 0;
	uint32_t blklen = 1; // Initial block, already skipped
	uint32_t num_dupes = 0;
	for (uint32_t i=1; i<nl; i++)
	{
		Linkage lnx = &sent->lnkages[i];
		if (false == lnx->dupe) { blklen++; continue; }
		free_linkage(lnx);
		num_dupes ++;

		// If there's a block of good linkages to copy, then copy.
		if (0 < blklen)
		{
			// Skip initial block; it is already in place.
			if (0 < tgt)
			{
				Linkage ltgt = &sent->lnkages[tgt];
				Linkage lsrc = &sent->lnkages[blkstart];
				memmove(ltgt, lsrc, blklen * sizeof(struct Linkage_s));
			}
			tgt += blklen;
			blklen = 0;
		}

		// The next good linkage comes after this bad one.
		blkstart = i+1;
	}

	// Copy the final block. This will copy the rest of the valid
	// linkages, as well as the bad ones. (We need to copy the bad
	// ones, because users can still examine them with the UI.)
	if (0 < tgt)
	{
		Linkage ltgt = &sent->lnkages[tgt];
		Linkage lsrc = &sent->lnkages[blkstart];
		blklen += sent->num_linkages_alloced - sent->num_valid_linkages;
		memmove(ltgt, lsrc, blklen * sizeof(struct Linkage_s));
	}

	assert(num_dupes < sent->num_valid_linkages, "Too many duplicates found!");

	// Adjust the totals.
	sent->num_linkages_alloced -= num_dupes;
	sent->num_valid_linkages -= num_dupes;
	sent->num_linkages_post_processed -= num_dupes;
}

static void sort_linkages(Sentence sent, Parse_Options opts)
{
	if (0 == sent->num_linkages_found) return;

	/* It they're randomized, don't bother sorting */
	if (0 != sent->rand_state && sent->dict->shuffle_linkages) return;

	/* Initialize all linkages as unique */
	for (uint32_t i=0; i<sent->num_linkages_alloced; i++)
		sent->lnkages[i].dupe = false;

	/* Sorting will also mark some of hem as being duplicates */
	qsort((void *)sent->lnkages, sent->num_linkages_alloced,
	      sizeof(struct Linkage_s),
	      (int (*)(const void *, const void *))opts->cost_model.compare_fn);

	/* Remove the duplicates. */
	deduplicate_linkages(sent, opts->linkage_limit);
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
	free_sentence_disjuncts(sent, /*category_too*/false);

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
		sent->overflowed = false;
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
				free_tracon_memblock(ts_pruning);
				ts_pruning = NULL;
				if (NULL != saved_memblock)
					free_saved_memblock(saved_memblock);
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
			extractor_t * pex =
				extractor_new(sent->length, sent->rand_state, IS_GENERATION(sent->dict));
			setup_linkages(sent, pex, mchxt, ctxt, opts);
			process_linkages(sent, pex, opts);
			if (IS_GENERATION(sent->dict))
			    find_unused_disjuncts(sent, pex);
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
				/* FIXME:
				 * 1. Issue this message if verbosity != 0.
				 * 2. Don't continue parsing with higher null counts. */
				if ((sent->num_linkages_post_processed > 0) &&
				    (sent->num_linkages_post_processed == sent->num_linkages_alloced) &&
				    ((int)opts->linkage_limit < sent->num_linkages_found) &&
				    !IS_GENERATION(sent->dict))
					prt_error("Info: All examined linkages (%zu) had P.P. violations.\n"
					          "Consider increasing the linkage limit.\n"
					          "At the command line, use !limit\n",
					          sent->num_linkages_post_processed);
			}
		}

		notify_no_complete_linkages(nl, max_null_count);
	}
	if ((sent->num_linkages_found == 0) && IS_GENERATION(sent->dict))
		find_unused_disjuncts(sent, NULL);

	sort_linkages(sent, opts);

parse_end_cleanup:
	if (NULL != ts_pruning)
	{
		free_categories(sent);
		free_tracon_memblock(ts_pruning);
		free_saved_memblock(saved_memblock);
	}
	free_tracon_sharing(ts_parsing);
	free_count_context(ctxt, sent);
	free_fast_matcher(sent, mchxt);
}
