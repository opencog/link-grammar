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
#include <ctype.h>
#include <limits.h>
#include <string.h>

#include "api-structures.h"
#include "count.h"
#include "dict-common/dict-common.h"   // For Dictionary_s
#include "disjunct-utils.h"
#include "extract-links-internal.h"
#include "fast-match.h"
#include "linkage/analyze-linkage.h"
#include "linkage/linkage.h"
#include "linkage/score.h"
#include "linkage/sane.h"
#include "parse.h"
#include "post-process/post-process.h"
#include "preparation.h"
#include "prune.h"
#include "resources.h"
#include "tokenize/word-structures.h"  // For Word_struct

#define D_PARSE 5 /* Debug level for this file. */
#define D_PL 7
#define PP_PARSE_SET_TRACE_LIMIT 20
#define METRIC_MORPH_SORT_LOOKAHEAD_LIMIT 2000

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

static bool metric_extraction_disabled(void)
{
	return NULL != test_enabled("no-metric-extraction");
}

static bool metric_mfc_terminal_state_enabled(void)
{
	return NULL == test_enabled("no-metric-mfc");
}

static bool metric_bounded_domain_state_enabled(void)
{
	return NULL == test_enabled("no-metric-bounded-domain");
}

static bool metric_global_contains_one_state_enabled(void)
{
	return NULL == test_enabled("no-metric-global-contains-one");
}

static bool metric_pp_validate_enabled(void)
{
	return NULL != test_enabled("metric-pp-validate");
}

static bool metric_classic_pp_enabled(void)
{
	return NULL != test_enabled("metric-classic-pp");
}

static bool metric_pp_constraints_enabled(Postprocessor *pp)
{
	return (NULL == test_enabled("no-metric-pp-constraints")) &&
	       post_process_has_parse_constraints(pp);
}

static bool metric_extraction_forced_enabled(void)
{
	return NULL != test_enabled("metric-extraction");
}

static bool metric_extraction_requested(Sentence sent, Parse_Options opts)
{
	if (metric_extraction_disabled()) return false;
	if (IS_GENERATION(sent->dict)) return false;

	return metric_extraction_forced_enabled() ||
	       sent->overflowed ||
	       ((int) opts->linkage_limit < sent->num_linkages_found);
}

static void setup_linkages(Sentence sent, extractor_t* pex,
                          fast_matcher_t* mchxt,
                          count_context_t* ctxt,
                          Parse_Options opts)
{
	sent->overflowed = build_parse_set(pex, sent, mchxt, ctxt, sent->null_count, opts);
	print_time(opts, "Built parse set");

	/* Overflow is known only after the parse-set build.  Keep this
	 * predicate aligned with process_linkages() so metric extraction is
	 * never selected without the corresponding extractor state. */
	extractor_set_metric_enabled(pex, metric_extraction_requested(sent, opts));

	if (sent->overflowed && (1 < opts->verbosity) && !IS_GENERATION(sent->dict))
	{
		err_ctxt ec = { sent };
		err_msgc(&ec, lg_Warn, "Count overflow.\n"
			"Considering up to %zu lowest-metric linkages of an unknown and large number of linkages\n",
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
 * Print linkage signatures in raw extraction order, before final sorting.
 * This is a test-only trace for checking limited extraction strategies.
 */
static void print_extract_order(Sentence sent, Parse_Options opts)
{
	if (!test_enabled("extract-order")) return;

	for (size_t i = 0; i < sent->num_valid_linkages; i++)
	{
		Linkage lkg = &sent->lnkages[i];
		dyn_str *buf = dyn_str_new();
		char tmp[128];

		linkage_score(lkg, opts);
		snprintf(tmp, sizeof(tmp),
		         "extract-order %zu: index=%d dis=%5.2f len=%d links=",
		         i + 1, lkg->lifo.index, lkg->lifo.disjunct_cost,
		         lkg->lifo.link_cost);
		dyn_strcat(buf, tmp);

		for (uint32_t j = 0; j < lkg->num_links; j++)
		{
			const Link *link = &lkg->link_array[j];
			snprintf(tmp, sizeof(tmp), "%s%u-%s-%u",
			         (0 == j) ? "" : " ",
			         (unsigned int)link->lw,
			         (NULL == link->link_name) ? "?" : link->link_name,
			         (unsigned int)link->rw);
			dyn_strcat(buf, tmp);
		}

		err_msg(lg_Debug, "%s\n", dyn_str_value(buf));
		dyn_str_delete(buf);
	}
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

static bool metric_morph_sort_lookahead_enabled(Sentence sent)
{
	return optional_word_exists(sent) &&
	       !sent->overflowed &&
	       (0 < sent->num_linkages_found) &&
	       (sent->num_linkages_found <= METRIC_MORPH_SORT_LOOKAHEAD_LIMIT);
}

typedef enum
{
	EXTRACT_INDEXED,
	EXTRACT_METRIC,
	EXTRACT_RANDOM
} Extract_method;

typedef struct
{
	bool post_processed;
	bool reached_request_cap;
	size_t metric_bounded_feedback_learned;
	size_t raw_extracted;
	size_t invalid_morphism;
	size_t pp_violations;
	size_t parse_constraint_rejections;
} Extraction_stats;

static void discard_linkage(Linkage lkg)
{
	free_linkage(lkg);
	memset(lkg, 0, sizeof(*lkg));
}

typedef enum
{
	EXTRACT_DONE,
	EXTRACT_SKIP,
	EXTRACT_KEEP
} Extract_result;

static Extract_result extract_linkage(Sentence sent, extractor_t *pex,
                                      Linkage lkg, Parse_Options opts,
                                      Extract_method extract_method, int itry,
                                      bool need_sane_morphism,
                                      Extraction_stats *stats)
{
	Linkage_info * lifo = &lkg->lifo;

	/* Negative values tell extract-links to pick randomly; for
	 * reproducible-rand, the actual value is the rand seed. */
	lifo->index = (EXTRACT_RANDOM == extract_method) ? -(itry+1) : itry;

	partial_init_linkage(sent, lkg, sent->length);

	if (EXTRACT_METRIC == extract_method)
	{
		if (!extract_metric_links(pex, lkg))
		{
			discard_linkage(lkg);
			return EXTRACT_DONE;
		}
	}
	else
	{
		extract_links(pex, lkg);
	}
	stats->raw_extracted++;

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
			stats->invalid_morphism++;
			discard_linkage(lkg);
			return EXTRACT_SKIP;
		}
	}

	if (IS_GENERATION(sent->dict))
		compute_generated_words(sent, lkg);

	return EXTRACT_KEEP;
}

static void post_process_batch(Sentence sent, Linkage batch,
                               size_t batch_count, Parse_Options opts,
                               PP_failure *failures)
{
	Linkage saved_lnkages = sent->lnkages;
	size_t saved_alloced = sent->num_linkages_alloced;
	size_t saved_post_processed = sent->num_linkages_post_processed;
	size_t saved_valid = sent->num_valid_linkages;

	post_process_reset(sent->postprocessor);
	sent->lnkages = batch;
	sent->num_linkages_alloced = batch_count;
	sent->num_valid_linkages = batch_count;
	sent->num_linkages_post_processed = 0;

	if (NULL == failures)
		post_process_lkgs(sent, opts);
	else
		post_process_lkgs_with_failures(sent, opts, failures,
		                                batch_count);

	sent->lnkages = saved_lnkages;
	sent->num_linkages_alloced = saved_alloced;
	sent->num_linkages_post_processed = saved_post_processed;
	sent->num_valid_linkages = saved_valid;
}

static void trace_postprocessed_batch(Sentence sent, extractor_t *pex,
                                      Linkage batch, size_t batch_count,
                                      Metric_candidate **trace_roots,
                                      size_t *num_traced,
                                      Parse_Options opts)
{
	if (NULL == trace_roots) return;
	if (*num_traced >= PP_PARSE_SET_TRACE_LIMIT) return;

	for (size_t i = 0; i < batch_count; i++)
	{
		const PP_failure *failure;

		if (0 == batch[i].lifo.N_violations) continue;
		if (*num_traced >= PP_PARSE_SET_TRACE_LIMIT) break;

		failure = post_process_find_failure(sent, &batch[i], opts);
		(*num_traced)++;
		extractor_trace_metric_candidate(pex, &batch[i], failure,
		                                 trace_roots[i], *num_traced);
	}
}

static void keep_postprocessed_batch(Linkage output, size_t *kept,
                                     size_t output_limit, Linkage batch,
                                     size_t batch_count,
                                     Extraction_stats *stats,
                                     size_t *batch_good,
                                     size_t *batch_bad)
{
	for (size_t i = 0; i < batch_count; i++)
	{
		if (0 == batch[i].lifo.N_violations)
		{
			(*batch_good)++;
			if (*kept < output_limit)
			{
				if (&output[*kept] != &batch[i])
				{
					output[*kept] = batch[i];
					memset(&batch[i], 0, sizeof(batch[i]));
				}
				(*kept)++;
			}
			else
			{
				discard_linkage(&batch[i]);
			}
		}
		else
		{
			(*batch_bad)++;
			stats->pp_violations++;
			discard_linkage(&batch[i]);
		}
	}
}

static void sort_postprocessed_metric_batch(Linkage batch, size_t batch_count,
                                            Parse_Options opts)
{
	for (size_t i = 0; i < batch_count; i++)
		batch[i].dupe = false;

	qsort((void *)batch, batch_count, sizeof(struct Linkage_s),
	      (int (*)(const void *, const void *))opts->cost_model.compare_fn);
}

static const char *metric_trace_failure_type_name(PP_failure_type type)
{
	switch (type)
	{
	case PP_FAILURE_NONE: return "none";
	case PP_FAILURE_CONTAINS_ONE: return "contains_one";
	case PP_FAILURE_CONTAINS_NONE: return "contains_none";
	case PP_FAILURE_CONTAINS_ONE_GLOBAL: return "contains_one_global";
	case PP_FAILURE_MUST_FORM_CYCLE: return "must_form_cycle";
	case PP_FAILURE_BOUNDED: return "bounded";
	}
	return "unknown";
}

static bool metric_trace_enabled(void)
{
	/* Keep verbosity 5 usable for other parse.c debug messages; this
	 * detailed extraction trace requires an explicit parse.c filter. */
	return (D_PARSE <= verbosity) && ('\0' != debug[0]) &&
	       (NULL != feature_enabled(debug, "parse.c",
	                                "process_metric_linkages",
	                                NULL));
}

/* The parse.c trace describes batch-level progress: extraction requests,
 * PP failures, learned feedback, and final kept counts.  Lower-level
 * candidate/ranker counters live in extract-links.c. */
static void metric_trace_start(Sentence sent, Parse_Options opts,
                               int maxtries, size_t output_limit)
{
	err_msg(lg_Debug, "metric-extraction: start null_count=%u found=%d "
	        "overflowed=%s output_limit=%zu maxtries=%d verbosity=%d\n",
	        sent->null_count, sent->num_linkages_found,
	        sent->overflowed ? "true" : "false", output_limit, maxtries,
	        opts->verbosity);
	if (NULL != sent->orig_sentence)
		err_msg(lg_Debug, "metric-extraction: sentence=\"%s\"\n",
		        sent->orig_sentence);
}

static void metric_trace_link(const char *label, Linkage lkg,
                              unsigned int idx)
{
	if ((NULL == lkg) || (lkg->num_links <= idx)) return;

	const Link *link = &lkg->link_array[idx];
	err_msg(lg_Debug, " %s=%u:%u-%s-%u", label, idx,
	        (unsigned int)link->lw,
	        (NULL == link->link_name) ? "?" : link->link_name,
	        (unsigned int)link->rw);
}

static void metric_trace_link_list(const char *label, Linkage lkg,
                                   const unsigned int *links,
                                   size_t num_links)
{
	if (0 == num_links) return;

	err_msg(lg_Debug, " %s=[", label);
	for (size_t i = 0; i < num_links; i++)
	{
		if (0 < i) err_msg(lg_Debug, ",");
		if ((NULL != lkg) && (links[i] < lkg->num_links))
		{
			const Link *link = &lkg->link_array[links[i]];
			err_msg(lg_Debug, "%u:%u-%s-%u", links[i],
			        (unsigned int)link->lw,
			        (NULL == link->link_name) ? "?" : link->link_name,
			        (unsigned int)link->rw);
		}
		else
		{
			err_msg(lg_Debug, "%u", links[i]);
		}
	}
	err_msg(lg_Debug, "]");
}

static void metric_trace_pp_failure(bool trace, size_t extracted_idx,
                                    const PP_failure *failure, Linkage lkg,
                                    const char *source)
{
	if (!trace || (NULL == failure) ||
	    (PP_FAILURE_NONE == failure->type))
		return;

	/* Include the PP failure's link roles so a short trace can explain
	 * which extracted links taught the next feedback mark. */
	err_msg(lg_Debug, "metric-extraction: detected PP-violation "
	        "extracted=%zu source=%s type=%s message=%s "
	        "selector=%s domain=%d truncated=%s",
	        extracted_idx, source,
	        metric_trace_failure_type_name(failure->type),
	        (NULL == failure->message) ? "" : failure->message,
	        (NULL == failure->selector) ? "" : failure->selector,
	        failure->domain, failure->truncated ? "true" : "false");
	if (failure->has_domain_start_link)
		metric_trace_link("domain-start", lkg,
		                  failure->domain_start_link);
	metric_trace_link_list("domain-links", lkg, failure->domain_links,
	                       failure->num_domain_links);
	metric_trace_link_list("selector-links", lkg, failure->selector_links,
	                       failure->num_selector_links);
	metric_trace_link_list("criterion-links", lkg,
	                       failure->criterion_links,
	                       failure->num_criterion_links);
	metric_trace_link_list("offending-links", lkg,
	                       failure->offending_links,
	                       failure->num_offending_links);
	err_msg(lg_Debug, "\n");
}

static void metric_trace_batch(bool trace, Linkage batch,
                               size_t batch_count,
                               const PP_failure *failures,
                               const Extraction_stats *stats,
                               size_t kept)
{
	size_t batch_bad = 0;
	size_t batch_good = 0;

	if (!trace) return;

	for (size_t i = 0; i < batch_count; i++)
	{
		if (0 == batch[i].lifo.N_violations)
			batch_good++;
		else
			batch_bad++;
	}

	err_msg(lg_Debug, "metric-extraction: extracted %zu linkages, "
	        "%zu with PP-violations, kept %zu; batch count=%zu "
	        "good=%zu bad=%zu\n",
	        stats->raw_extracted, stats->pp_violations + batch_bad,
	        kept, batch_count, batch_good, batch_bad);
	for (size_t i = 0; i < batch_count; i++)
	{
		if (0 == batch[i].lifo.N_violations) continue;
		if (NULL != failures)
			metric_trace_pp_failure(trace, stats->raw_extracted -
			                        batch_count + i + 1,
			                        &failures[i], &batch[i],
			                        "batch-pp");
		else
			err_msg(lg_Debug, "metric-extraction: detected PP-violation "
			        "extracted=%zu source=batch-pp message=%s\n",
			        stats->raw_extracted - batch_count + i + 1,
			        (NULL == batch[i].lifo.pp_violation_msg) ? "" :
			        batch[i].lifo.pp_violation_msg);
	}
}

static void metric_trace_feedback(bool trace, size_t learned_bounded,
                                  size_t learned_global,
                                  size_t learned_contains_one,
                                  size_t learned_contains_none)
{
	if (!trace) return;
	if ((0 == learned_bounded) && (0 == learned_global) &&
	    (0 == learned_contains_one) && (0 == learned_contains_none))
		return;

	err_msg(lg_Debug, "metric-extraction: learned feedback bounded=%zu "
	        "global-contains-one=%zu domain-contains-one=%zu "
	        "domain-contains-none=%zu\n",
	        learned_bounded, learned_global, learned_contains_one,
	        learned_contains_none);
}

static void metric_trace_summary(bool trace, const Extraction_stats *stats,
                                 size_t kept, int itry, int maxtries,
                                 bool done, Parse_Options opts)
{
	if (!trace) return;

	err_msg(lg_Debug, "metric-extraction: summary extracted=%zu kept=%zu "
	        "PP-violations=%zu invalid-morphism=%zu itry=%d maxtries=%d "
	        "done=%s timer-expired=%s memory-exhausted=%s\n",
	        stats->raw_extracted, kept, stats->pp_violations,
	        stats->invalid_morphism, itry, maxtries,
	        done ? "true" : "false",
	        opts->resources->timer_expired ? "true" : "false",
	        opts->resources->memory_exhausted ? "true" : "false");
}

typedef struct
{
	size_t checked;
	size_t true_accept;
	size_t true_reject;
	size_t false_positive;
	size_t false_negative;
	size_t mismatch;
	size_t unrelated_reject;
	size_t examples_reported;
} Metric_pp_validate_stats;

/* PP messages often differ between an active legacy row and a parser-side
 * replacement row.  The numeric rule suffix is the stable identity used for
 * validation comparison; fall back to text only when a suffix is unavailable. */
static bool metric_pp_message_id(const char *message, unsigned int *id)
{
	const char *end;
	const char *start;
	unsigned int value = 0;

	if ((NULL == message) || ('\0' == message[0])) return false;

	end = message + strlen(message);
	while ((message < end) && !isdigit((unsigned char)end[-1]))
		end--;
	if (message == end) return false;

	start = end;
	while ((message < start) && isdigit((unsigned char)start[-1]))
		start--;

	for (const char *p = start; p < end; p++)
		value = 10 * value + (unsigned int)(*p - '0');

	*id = value;
	return true;
}

static bool metric_pp_same_rule_message(const char *left, const char *right)
{
	unsigned int left_id;
	unsigned int right_id;

	if (metric_pp_message_id(left, &left_id) &&
	    metric_pp_message_id(right, &right_id))
		return left_id == right_id;

	if ((NULL == left) || (NULL == right)) return false;
	return 0 == strcmp(left, right);
}

static bool metric_pp_message_in_parse_rules(Postprocessor *pp,
                                             const char *message)
{
	size_t count = post_process_parse_contains_one_rule_count(pp);

	for (size_t i = 0; i < count; i++)
		if (metric_pp_same_rule_message(
		    message, post_process_parse_contains_one_rule_message(pp, i)))
			return true;

	count = post_process_parse_contains_one_global_rule_count(pp);
	for (size_t i = 0; i < count; i++)
		if (metric_pp_same_rule_message(
		    message,
		    post_process_parse_contains_one_global_rule_message(pp, i)))
			return true;

	count = post_process_parse_contains_none_rule_count(pp);
	for (size_t i = 0; i < count; i++)
		if (metric_pp_same_rule_message(
		    message, post_process_parse_contains_none_rule_message(pp, i)))
			return true;

	return false;
}

static bool metric_pp_validate_failure_handled(Postprocessor *pp,
                                             const PP_failure *failure)
{
	if ((NULL == failure) || (PP_FAILURE_NONE == failure->type))
		return false;
	if ((PP_FAILURE_CONTAINS_ONE != failure->type) &&
	    (PP_FAILURE_CONTAINS_ONE_GLOBAL != failure->type) &&
	    (PP_FAILURE_CONTAINS_NONE != failure->type))
		return false;

	return metric_pp_message_in_parse_rules(pp, failure->message);
}

static bool metric_pp_validate_failure_same(const PP_failure *predicted,
                                          const PP_failure *actual)
{
	if ((NULL == predicted) || (NULL == actual))
		return false;
	if (PP_FAILURE_NONE == predicted->type) return false;
	if (PP_FAILURE_NONE == actual->type) return false;
	if (metric_pp_same_rule_message(predicted->message, actual->message))
		return true;

	return (predicted->type == actual->type) &&
	       (NULL != predicted->message) &&
	       (NULL != actual->message) &&
	       (0 == strcmp(predicted->message, actual->message));
}

static void metric_pp_validate_report_example(Sentence sent, size_t index,
                                            const PP_failure *predicted,
                                            const PP_failure *actual,
                                            const char *kind,
                                            Metric_pp_validate_stats *stats)
{
	if (5 <= stats->examples_reported) return;
	stats->examples_reported++;

	prt_error("Error: metric-pp-validate %s at candidate %zu: "
	          "predicted=%s/%s actual=%s/%s\n",
	          kind, index + 1,
	          metric_trace_failure_type_name(predicted->type),
	          (NULL == predicted->message) ? "" : predicted->message,
	          metric_trace_failure_type_name(actual->type),
	          (NULL == actual->message) ? "" : actual->message);
	if (NULL != sent->orig_sentence)
		prt_error("Error: metric-pp-validate sentence: %s\n",
		          sent->orig_sentence);
}

/* PP validation leaves extractor candidates alive, then compares the
 * extractor's would-reject prediction with batched PP.  This makes the old
 * PP rows a tripwire instead of a silent duplicate filter. */
static void metric_pp_validate_validate_batch(
	Sentence sent, size_t batch_count, const PP_failure *predictions,
	const PP_failure *failures, Metric_pp_validate_stats *stats)
{
	for (size_t i = 0; i < batch_count; i++)
	{
		PP_failure none = { .type = PP_FAILURE_NONE, .domain = -1 };
		const PP_failure *predicted = &predictions[i];
		const PP_failure *actual = &failures[i];
		bool predicted_bad = PP_FAILURE_NONE != predicted->type;
		bool actual_bad = PP_FAILURE_NONE != actual->type;
		bool actual_handled =
			metric_pp_validate_failure_handled(
				sent->postprocessor, actual);

		stats->checked++;
		if (!predicted_bad && !actual_bad)
		{
			stats->true_accept++;
			continue;
		}
		if (predicted_bad && actual_handled &&
		    metric_pp_validate_failure_same(predicted, actual))
		{
			stats->true_reject++;
			continue;
		}
		if (!predicted_bad && actual_handled)
		{
			stats->false_negative++;
			metric_pp_validate_report_example(
				sent, i, &none, actual, "false-negative", stats);
			continue;
		}
		if (predicted_bad && !actual_bad)
		{
			stats->false_positive++;
			metric_pp_validate_report_example(
				sent, i, predicted, &none, "false-positive", stats);
			continue;
		}
		if (!predicted_bad)
		{
			stats->unrelated_reject++;
			continue;
		}

		stats->mismatch++;
		metric_pp_validate_report_example(
			sent, i, predicted, actual, "mismatch", stats);
	}
}

static void metric_pp_validate_summary(const Metric_pp_validate_stats *stats)
{
	if ((0 == stats->checked) ||
	    ((verbosity < D_USER_INFO) &&
	     (0 == stats->false_positive) &&
	     (0 == stats->false_negative) &&
	     (0 == stats->mismatch)))
		return;

	prt_error("Info: metric-pp-validate: checked=%zu true-accept=%zu "
	          "true-reject=%zu false-positive=%zu false-negative=%zu "
	          "mismatch=%zu unrelated=%zu\n",
	          stats->checked, stats->true_accept, stats->true_reject,
	          stats->false_positive, stats->false_negative,
	          stats->mismatch, stats->unrelated_reject);
}

static Extraction_stats process_metric_linkages(Sentence sent,
                                                extractor_t *pex,
                                                Parse_Options opts,
                                                int maxtries,
                                                bool need_sane_morphism)
{
	/* Metric extraction still postprocesses in batches.  A batch can
	 * teach feedback marks, but those marks are applied only after every
	 * learner has inspected the same PP result array. */
	Extraction_stats stats = { .post_processed = true };
	size_t output_limit = sent->num_linkages_alloced;
	bool morph_sort_lookahead =
		metric_morph_sort_lookahead_enabled(sent);
	size_t batch_capacity = morph_sort_lookahead ?
		(size_t)sent->num_linkages_found : output_limit;
	Linkage output = sent->lnkages;
	Linkage batch = linkage_array_new(batch_capacity);
	bool global_contains_one_state =
		metric_global_contains_one_state_enabled();
	bool mfc_terminal_state = metric_mfc_terminal_state_enabled();
	bool bounded_domain_state =
		metric_bounded_domain_state_enabled();
	bool metric_trace = metric_trace_enabled();
	bool metric_pp_validate = metric_pp_validate_enabled();
	bool metric_pp_constraints =
		metric_pp_constraints_enabled(sent->postprocessor);
	bool metric_classic_pp =
		metric_classic_pp_enabled() || metric_pp_validate;
	bool suppress_classic_mfc =
		!metric_classic_pp && mfc_terminal_state;
	bool suppress_classic_parse_constraints =
		!metric_classic_pp && metric_pp_constraints;
	bool any_feedback =
		bounded_domain_state || metric_trace || metric_pp_validate;
	bool trace_pp_parse_set =
		(NULL != test_enabled("pp-parse-set-trace")) ||
		(NULL != test_enabled("pp-parse-set-trace-all-links"));
	bool need_metric_roots =
		trace_pp_parse_set || bounded_domain_state;
	PP_failure *failures = any_feedback ?
		calloc(batch_capacity, sizeof(*failures)) : NULL;
	PP_failure *validation_predictions = metric_pp_validate ?
		calloc(batch_capacity, sizeof(*validation_predictions)) : NULL;
	Metric_candidate **metric_roots = need_metric_roots ?
		malloc(batch_capacity * sizeof(*metric_roots)) : NULL;
	if (any_feedback)
		assert(NULL != failures,
		       "Out of memory allocating PP failure batch");
	if (metric_pp_validate)
		assert(NULL != validation_predictions,
		       "Out of memory allocating metric PP validation predictions");
	if (need_metric_roots)
		assert(NULL != metric_roots,
		       "Out of memory allocating metric trace roots");
	Metric_pp_validate_stats validation_stats = { 0 };
	size_t num_traced = 0;
	size_t kept = 0;
	bool done = false;
	int itry = 0;

	if (metric_trace)
		metric_trace_start(sent, opts, maxtries, output_limit);

	pex->metric.resources = opts->resources;
	pex->metric.pp.mfc_enabled = mfc_terminal_state;
	pex->metric.pp.bounded_enabled = bounded_domain_state;
	pex->metric.pp.global_enabled = global_contains_one_state;
	post_process_set_metric_rule_suppression(
		sent->postprocessor, suppress_classic_mfc,
		suppress_classic_parse_constraints,
		suppress_classic_parse_constraints);

	while ((itry < maxtries) && (kept < output_limit))
	{
		/* Request one output-sized block at a time, capped by the
		 * remaining request budget.  This preserves batched PP while
		 * allowing feedback to affect the next block. */
		size_t block_limit = morph_sort_lookahead ?
			MIN(batch_capacity, (size_t)(maxtries - itry)) :
			MIN(output_limit, (size_t)(maxtries - itry));
		size_t batch_count = 0;

		for (size_t iblk = 0;
		     (iblk < block_limit) && (itry < maxtries); iblk++)
		{
			size_t reject_before =
				pex->metric.pp.parse_constraint_rejected;
			size_t remaining = (size_t)(maxtries - itry);
			pex->metric.pp.parse_constraint_reject_limit =
				reject_before + remaining;
			pex->metric.pp.parse_constraint_reject_limit_hit = false;

			Extract_result er = extract_linkage(sent, pex,
				&batch[batch_count], opts, EXTRACT_METRIC, itry,
				need_sane_morphism, &stats);
			size_t rejected =
				pex->metric.pp.parse_constraint_rejected - reject_before;

			stats.parse_constraint_rejections += rejected;
			if (remaining <= rejected)
				itry = maxtries;
			else
				itry += (int)rejected;

			if (EXTRACT_DONE == er)
			{
				done = true;
				break;
			}
			if (EXTRACT_KEEP == er)
			{
				if (need_metric_roots)
					metric_roots[batch_count] =
						pex->metric.trace_root;
				if (metric_pp_validate)
					validation_predictions[batch_count] =
						pex->metric.pp.prediction;
				batch_count++;
			}
			itry++;
		}

		if (0 < batch_count)
		{
			size_t batch_good = 0;
			size_t batch_bad = 0;
			size_t learned_bounded = 0;

			if (any_feedback)
				memset(failures, 0, batch_count * sizeof(*failures));
			post_process_batch(sent, batch, batch_count, opts,
			                    any_feedback ? failures : NULL);
			if (metric_pp_validate)
				metric_pp_validate_validate_batch(
					sent, batch_count, validation_predictions,
					failures, &validation_stats);
			metric_trace_batch(metric_trace, batch, batch_count,
			                   any_feedback ? failures : NULL,
			                   &stats, kept);
			if (trace_pp_parse_set)
				trace_postprocessed_batch(sent, pex, batch,
				                          batch_count, metric_roots,
				                          &num_traced, opts);
			if (bounded_domain_state)
			{
				/* Bounded-domain feedback is learned only from
				 * normal batch PP failures.  This preserves the
				 * important property that metric extraction does
				 * not call PP one linkage at a time. */
				for (size_t i = 0; i < batch_count; i++)
					learned_bounded +=
						extractor_finish_metric_bounded_domain_feedback_state(
							pex, metric_roots[i], &batch[i],
							&failures[i]);
				stats.metric_bounded_feedback_learned +=
					learned_bounded;
			}
			if (0 < learned_bounded)
				extractor_apply_metric_bounded_domain_feedback(pex);
			metric_trace_feedback(metric_trace, learned_bounded,
			                      0, 0, 0);
			if (morph_sort_lookahead)
				sort_postprocessed_metric_batch(batch, batch_count, opts);

			keep_postprocessed_batch(output, &kept, output_limit,
			                         batch, batch_count, &stats,
			                         &batch_good, &batch_bad);
		}

		if (done || resources_exhausted(opts->resources)) break;
	}

	free(failures);
	free(validation_predictions);
	free(metric_roots);
	linkage_array_free(batch);
	post_process_set_metric_rule_suppression(sent->postprocessor,
	                                         false, false, false);

	stats.reached_request_cap = (kept < output_limit) &&
	                            (maxtries <= itry) &&
	                            (maxtries < sent->num_linkages_found);
	metric_trace_summary(metric_trace, &stats, kept, itry, maxtries,
	                     done, opts);
	if (metric_pp_validate)
		metric_pp_validate_summary(&validation_stats);

	sent->num_valid_linkages = kept;
	sent->num_linkages_post_processed = kept;

	/* The remainder of the array is garbage; we never filled it in.
	 * So just pretend that it's shorter than it is */
	sent->num_linkages_alloced = sent->num_valid_linkages;
	print_extract_order(sent, opts);

	if (verbosity >= D_USER_INFO)
	{
		lgdebug(0, "Info: sane_morphism(): %zu of %zu linkages had "
		        "invalid morphology construction\n", stats.invalid_morphism,
		        stats.raw_extracted);
	}

	if (verbosity_level(D_PARSE))
	{
		if (stats.post_processed)
			lgdebug(0, "Info: metric extraction examined %zu "
			        "linkages: %zu kept, %zu P.P. violations, "
			        "%zu parse-constraint rejections\n",
			        stats.raw_extracted +
			        stats.parse_constraint_rejections,
			        sent->num_valid_linkages,
			        stats.pp_violations,
			        stats.parse_constraint_rejections);
		if (bounded_domain_state &&
		    (0 < pex->metric.trace.state_assignments_considered))
			lgdebug(0, "Info: metric state ranking considered %zu "
			        "assignments, pushed %zu candidates\n",
			        pex->metric.trace.state_assignments_considered,
			        pex->metric.trace.state_assignments_pushed);
		if (bounded_domain_state)
			lgdebug(0, "Info: metric bounded-domain feedback learned "
			        "%zu blockers (%zu encoded marks, %zu ignored), "
			        "rejected %zu candidates, skipped %zu repeated "
			        "candidates\n",
			        stats.metric_bounded_feedback_learned,
			        pex->metric.pp.bounded.num_feedbacks,
			        pex->metric.pp.bounded.duplicates +
			        pex->metric.pp.bounded.ignored,
			        pex->metric.pp.bounded.rejected,
			        pex->metric.seen.duplicate_skipped);

	}

	return stats;
}

/**
 * This fills the linkage array with morphologically-acceptable
 * linkages.
 */
static Extraction_stats process_linkages(Sentence sent, extractor_t* pex,
                                         Parse_Options opts)
{
	Extraction_stats stats = { 0 };

	if (0 == sent->num_linkages_found) return stats;
	if (0 == sent->num_linkages_alloced) return stats; /* Avoid a later crash. */

	/* Use metric extraction when requested explicitly by a metric test, or
	 * when we get more linkages than what was asked for. */
	bool limited_extraction = sent->overflowed ||
	    (sent->num_linkages_found > (int) opts->linkage_limit);
	Extract_method extract_method = EXTRACT_INDEXED;
	if (metric_extraction_requested(sent, opts))
		extract_method = EXTRACT_METRIC;
	else if (limited_extraction)
		extract_method = EXTRACT_RANDOM;

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

	if (limited_extraction)
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

	if (EXTRACT_METRIC == extract_method)
		return process_metric_linkages(sent, pex, opts, maxtries,
		                               need_sane_morphism);

	sent->num_valid_linkages = 0;
	size_t N_invalid_morphism = 0;

	int itry = 0;
	size_t in = 0;
	size_t linkage_array_limit = sent->num_linkages_alloced;
	bool need_init = true;
	for (itry=0; itry<maxtries; itry++)
	{
		Linkage lkg = &sent->lnkages[in];
		Linkage_info * lifo = &lkg->lifo;

		/* Negative values tell extract-links to pick randomly; for
		 * reproducible-rand, the actual value is the rand seed. */
		lifo->index = (EXTRACT_RANDOM == extract_method) ? -(itry+1) : itry;

		if (need_init)
		{
			partial_init_linkage(sent, lkg, sent->length);
			need_init = false;
		}

		extract_links(pex, lkg);
		stats.raw_extracted++;

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
				memset(lkg->chosen_disjuncts, 0,
				       sent->length * sizeof(Disjunct *));

				continue;
			}
		}

		if (IS_GENERATION(sent->dict))
			compute_generated_words(sent, lkg);

		need_init = true;
		in++;
		if (in >= linkage_array_limit) break;
	}

	/* The last one was alloced, but never actually used. Free it. */
	if (!need_init) free_linkage(&sent->lnkages[in]);

	sent->num_valid_linkages = in;

	/* The remainder of the array is garbage; we never filled it in.
	 * So just pretend that it's shorter than it is */
	sent->num_linkages_alloced = sent->num_valid_linkages;
	print_extract_order(sent, opts);

	if (verbosity >= D_USER_INFO)
	{
		stats.invalid_morphism = N_invalid_morphism;
		lgdebug(0, "Info: sane_morphism(): %zu of %zu linkages had "
		        "invalid morphology construction\n", stats.invalid_morphism,
		        stats.raw_extracted);
	}

	return stats;
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
	// Compare link endpoints
	for (uint32_t li=0; li<lpv->num_links; li++)
	{
		Link * plk = &lpv->link_array[li];
		Link * nlk = &lnx->link_array[li];

		// Compare word-endpoints first. Most differences are likely
		// to be noticeable here. This is an inexpensive check.
		int lwd = plk->lw - nlk->lw;
		if (lwd) return lwd;

		int rwd = plk->rw - nlk->rw;
		if (rwd) return rwd;
	}

	// Compare link names. This is slightly more expensive than the
	// check above, so we defer this check.
	for (uint32_t li=0; li<lpv->num_links; li++)
	{
		Link * plk = &lpv->link_array[li];
		Link * nlk = &lnx->link_array[li];

		// Note (see intersect_strings()):
		// link_name is not always in the same string set, so inequality
		// test cannot be done here.
		if (plk->link_name == nlk->link_name) continue;
		int lncmp = strcmp(plk->link_name, nlk->link_name);
		if (lncmp) return lncmp;
	}

	// Compare words. The chosen_disjuncts->word_string is the
	// dictionary word. It can happen that two different dictionary
	// words can have the same disjunct, and thus result in the same
	// linkage. For backwards compat, we will report these as being
	// different, as printing will reveal the differences in words.
	for (uint32_t wi=0; wi<lpv->num_words; wi++)
	{
		Disjunct * pdj = lpv->chosen_disjuncts[wi];
		Disjunct * ndj = lnx->chosen_disjuncts[wi];

		// Parses with non-zero null count will have null words,
		// i.e. word without chosen_disjuncts. Avoid a null-pointer
		// deref in this case.
		if (NULL == pdj)
		{
			// If one is null, both should be null. (I think this
			// will always be true, but I'm not sure.)
			if (NULL == ndj) continue;
			return 1;
		}

		// Note (see build_word_expressions()):
		// word_string is not always in the same string set, so inequality
		// test cannot be done here.
		if (pdj->word_string == ndj->word_string) continue;
		int wscmp = strcmp(pdj->word_string, ndj->word_string);
		if (wscmp) return wscmp;
	}

	// Compare connector types at the link endpoints. If we are here,
	// then the link endpoints landed on the same words, and the link
	// names were the same. The connector types might still differ,
	// due to intersection. The multi-connector flag might differ.
	// However, neither of these are likely. It is plausible to skip
	// this check entirely, it's mostly a CPU-time-waster that will
	// never find any differences for the almost any situation.
	for (uint32_t li=0; li<lpv->num_links; li++)
	{
		Link * plk = &lpv->link_array[li];
		Link * nlk = &lnx->link_array[li];

		if (plk->lc != nlk->lc)
		{
			if (plk->lc->desc != nlk->lc->desc)
				return strcmp(connector_string(plk->lc), connector_string(nlk->lc));

			int md = plk->lc->multi - nlk->lc->multi;
			if (md) return md;
		}
		if (plk->rc != nlk->rc)
		{
			if (plk->rc->desc != nlk->rc->desc)
				return strcmp(connector_string(plk->rc), connector_string(nlk->rc));

			int md = plk->rc->multi - nlk->rc->multi;
			if (md) return md;
		}
	}

#if DOUBLE_CHECK
	// We also expect the chosen disjuncts to be identical. But after
	// the above checks, it should be impossible that they differ.
	for (uint32_t wi=0; wi<lpv->num_words; wi++)
	{
		if (lpv->chosen_disjuncts[wi] != lnx->chosen_disjuncts[wi])
			return strcmp(
				linkage_get_disjunct_str(lpv, wi),
				linkage_get_disjunct_str(lnx, wi));
	}
#endif

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
 * larger than the linkage array. In this case, a limited subset of
 * linkages will be selected, and duplicates can be present. When the
 * alloc array is slightly less than the number of linkages found, then
 * as many as half(!) of the linkages can be duplicates.
 *
 * This assumes that the duplicates have already been detected and
 * marked by setting `linkage->dupe=true` during linkage sorting.
 */
static void deduplicate_linkages(Sentence sent, int linkage_limit)
{
	int linkage_dedup = -1;
	const char *test_linkage_dedup = test_enabled("linkage-dedup");
	/* Never dedup: linkage-dedup:0; Always dedup: linkage-dedup:1 . */

	if (test_linkage_dedup != NULL)
	{
		if ((test_linkage_dedup[0] != ':') || (test_linkage_dedup[1] == '\0'))
			linkage_dedup = 1; /* just linkage-dedup w/o value defaults to 1 */
		else
			linkage_dedup = atoi(test_linkage_dedup + 1);
	}

	/* No need for deduplication, if limited extraction wasn't done. */
	if ((linkage_dedup == 0) || ((linkage_dedup < 0) &&
	    !sent->overflowed && (sent->num_linkages_found <= linkage_limit)))
		return;

	// Deduplicate the valid linkages only; it's not worth wasting
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

	/* Sorting will also mark some of them as being duplicates */
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
			extractor_t * pex = extractor_new(sent);
			Extraction_stats extraction_stats;
			setup_linkages(sent, pex, mchxt, ctxt, opts);
			extraction_stats = process_linkages(sent, pex, opts);
			if (IS_GENERATION(sent->dict))
			    find_unused_disjuncts(sent, pex);
#ifdef PC_DISPLAY
			display_parse_choice(pex);
#endif
			free_extractor(pex);

			if (!extraction_stats.post_processed)
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
				size_t attempted =
					extraction_stats.raw_extracted +
					extraction_stats.parse_constraint_rejections;
				if (extraction_stats.post_processed &&
				    extraction_stats.reached_request_cap &&
				    (0 < attempted))
					prt_error("Info: All examined linkages (%zu) "
					          "were rejected (%zu P.P. violations, "
					          "%zu parse-constraint rejections, "
					          "%zu invalid morphology).\n"
					          "Consider increasing the linkage limit.\n"
					          "At the command line, use !limit\n",
					          attempted,
					          extraction_stats.pp_violations,
					          extraction_stats.parse_constraint_rejections,
					          extraction_stats.invalid_morphism);
				else if ((sent->num_linkages_post_processed > 0) &&
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
