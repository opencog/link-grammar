/*************************************************************************/
/* Copyright (c) 2026 Amir Plivatsky                                     */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

/* This file implements the parser's Viterbi-style linkage extractor.  It is
 * not an HMM decoder; it adapts the same dynamic-programming idea to the
 * Parse_set DAG.  Each Parse_set/state stream is ranked by local
 * Parse_choice cost plus child ranks, and small heaps lazily merge the next
 * cheapest candidates so linkages can be emitted in ascending raw metric
 * order without materializing the full linkage set. */

#include "extract-links-internal.h"

#define METRIC_TRACE_PROGRESS_INTERVAL 100000
/* Metric candidates are allocated one at a time with pool_alloc(), not
 * pool_alloc_vec().  Keep the block size independent of the potentially huge
 * Parse_choice estimate so metric extraction does not allocate a second
 * worst-case parse-set-sized block. */
#define METRIC_CANDIDATE_POOL_BLOCK_SIZE 4096

static inline int link_cost_for_length(int length)
{
	return length - 1;
}

/* A Parse_choice pays disjunct cost only when it actually
 * commits the middle disjunct to the emitted linkage. */
static bool choice_uses_disjunct(const Parse_choice *pc,
                                 const Parse_set *set)
{
	if (NULL == pc->md) return false;
	if (!is_zero_tracon(set->le)) return true;
	return pc->r_id >= NULL_TRACON_BLOCK;
}

/* Compute the metric contribution that belongs to one
 * Parse_choice edge.  Child subranges add their own costs
 * through the ranker state. */
static Parse_metric choice_local_metric(const Parse_choice *pc,
                                        const Parse_set *set)
{
	Parse_metric metric = { 0 };

	if (choice_uses_disjunct(pc, set) && (0 == pc->md->is_category))
		metric.disjunct_cost = pc->md->cost;

	if (!is_zero_tracon(set->le) && (pc->l_id >= NULL_TRACON_BLOCK))
		metric.link_cost +=
			link_cost_for_length(pc->set[0]->rw - pc->set[0]->lw);

	if ((pc->r_id >= NULL_TRACON_BLOCK) && !is_zero_tracon(set->re))
		metric.link_cost +=
			link_cost_for_length(pc->set[1]->rw - pc->set[1]->lw);

	return metric;
}

/* Enable metric extraction after build_parse_set() has made
 * overflow and parse-constraint availability known.  Test
 * flags used in hot paths are sampled here once. */
void extractor_set_metric_enabled(extractor_t *pex, bool enabled)
{
	pex->metric.enabled = enabled;
	pex->metric.pp.validate_enabled =
		enabled && (NULL != test_enabled("metric-pp-validate"));
	pex->metric.pp.constraints_enabled =
		enabled &&
		(NULL == test_enabled("no-metric-pp-constraints")) &&
		post_process_has_parse_constraints(pex->postprocessor);
	pex->metric.pp.feedback_trace_enabled =
		(NULL != test_enabled("pp-parse-set-trace")) ||
		(NULL != test_enabled("pp-parse-set-trace-all-links"));
	pex->metric.pp.trace_all_links_enabled =
		NULL != test_enabled("pp-parse-set-trace-all-links");
	if (enabled && (NULL == pex->metric.ranker_pool))
	{
		pex->metric.ranker_pool =
			pool_new(__func__, "Metric_ranker",
			         /*num_elements*/128, sizeof(Metric_ranker),
			         /*zero_out*/false, /*align*/false, /*exact*/false);
	}
}

static void metric_heap_delete(Metric_heap *heap)
{
	if (NULL == heap) return;
	free(heap->array);
	free(heap);
}

static void metric_state_stream_free(Metric_state_stream *stream)
{
	if (NULL == stream) return;

	metric_heap_delete(stream->heap);
	free(stream->ranked);
	memset(stream, 0, sizeof(*stream));
}

static void metric_set_cache_free(Metric_set_cache *cache)
{
	if (NULL == cache) return;

	for (size_t i = 0; i < cache->num_streams; i++)
		metric_state_stream_free(&cache->streams[i]);
	free(cache->streams);
	free(cache->ranked);
}

static void metric_ranker_release(Metric_ranker *ranker)
{
	if ((NULL == ranker) || (NULL == ranker->cache_table)) return;

	for (size_t i = 0; i < ranker->cache_table_size; i++)
	{
		Metric_set_cache *cache = ranker->cache_table[i];
		while (NULL != cache)
		{
			Metric_set_cache *next = cache->next;
			metric_set_cache_free(cache);
			cache = next;
		}
	}

	free(ranker->cache_table);
	free(ranker->root_states);
	pool_delete(ranker->candidate_pool);
	pool_delete(ranker->set_cache_pool);
	ranker->candidate_pool = NULL;
	ranker->set_cache_pool = NULL;
	ranker->root_states = NULL;
	ranker->num_root_states = 0;
	ranker->cache_table = NULL;
	ranker->cache_table_size = 0;
	ranker->cache_table_limit = 0;
	ranker->num_set_caches = 0;
}

static void metric_ranker_free_all(extractor_t *pex)
{
	Metric_ranker *ranker = pex->metric.rankers;

	while (NULL != ranker)
	{
		metric_ranker_release(ranker);
		ranker = ranker->next;
	}
	pex->metric.rankers = NULL;
	pex->metric.ranker = NULL;
}

/* Rebuilding rankers is the safe way to change exact-state or feedback
 * semantics: cached streams contain candidates ranked under the previous
 * state interpretation. */
static void metric_ranker_reset(extractor_t *pex)
{
	metric_ranker_free_all(pex);
	if (NULL != pex->metric.ranker_pool)
		pool_reuse(pex->metric.ranker_pool);

	pex->metric.next_rank = 0;
	pex->metric.trace_root = NULL;
}

static bool metric_trace_selected(void)
{
	return (D_EXTRACT <= verbosity) && ('\0' != debug[0]) &&
	       (NULL != feature_enabled(debug, "extract-links.c",
	                                "extract_metric_links", NULL));
}

/* Compress many hot counters into one monotonic progress value.  The
 * trace intentionally reports by work done, not by wall time, so slow
 * candidate populations show where the extractor is spending effort. */
static size_t metric_trace_work(const extractor_t *pex)
{
	return pex->metric.trace.choices_scanned +
	       pex->metric.trace.streams_started +
	       pex->metric.trace.stream_candidates_emitted +
	       pex->metric.trace.root_candidates_emitted +
	       pex->metric.trace.state_assignments_considered +
	       pex->metric.trace.state_assignments_pushed +
	       pex->metric.seen.duplicate_skipped +
	       pex->metric.pp.bounded.rejected +
	       pex->metric.pp.global.rejected;
}

/* Emit a sparse extractor progress line.  The force flag is used at
 * phase boundaries where a trace line is useful even if the work
 * interval has not advanced. */
static void metric_trace_report(extractor_t *pex, const char *phase,
                                bool force)
{
	if (!pex->metric.trace.enabled) return;

	size_t work = metric_trace_work(pex);
	if (!force && (work < pex->metric.trace.next_work))
		return;

	while (pex->metric.trace.next_work <= work)
		pex->metric.trace.next_work += METRIC_TRACE_PROGRESS_INTERVAL;

	err_msg(lg_Debug, "metric-extraction: extractor phase=%s work=%zu "
	        "calls=%zu rank=%zu streams=%zu choices=%zu stream-out=%zu "
	        "root-out=%zu assignments=%zu/%zu reject dup=%zu "
	        "bounded=%zu global=%zu\n",
	        phase, work, pex->metric.trace.extract_calls,
	        pex->metric.next_rank, pex->metric.trace.streams_started,
	        pex->metric.trace.choices_scanned,
	        pex->metric.trace.stream_candidates_emitted,
	        pex->metric.trace.root_candidates_emitted,
	        pex->metric.trace.state_assignments_considered,
	        pex->metric.trace.state_assignments_pushed,
	        pex->metric.seen.duplicate_skipped,
	        pex->metric.pp.bounded.rejected,
	        pex->metric.pp.global.rejected);
}

/* Report the concrete metric and rejection counters at a point where a
 * root candidate is either emitted or the extractor reaches a terminal
 * condition. */
static void metric_trace_candidate(extractor_t *pex, const char *phase,
                                   size_t rank,
                                   const Metric_candidate *candidate)
{
	if (!pex->metric.trace.enabled) return;

	if (NULL == candidate)
	{
		metric_trace_report(pex, phase, true);
		return;
	}

	err_msg(lg_Debug, "metric-extraction: extractor phase=%s rank=%zu "
	        "metric=(DIS=%.2f LEN=%d) work=%zu stream-out=%zu "
	        "root-out=%zu reject dup=%zu bounded=%zu global=%zu\n",
	        phase, rank, candidate->metric.disjunct_cost,
	        candidate->metric.link_cost, metric_trace_work(pex),
	        pex->metric.trace.stream_candidates_emitted,
	        pex->metric.trace.root_candidates_emitted,
	        pex->metric.seen.duplicate_skipped,
	        pex->metric.pp.bounded.rejected,
	        pex->metric.pp.global.rejected);
}

static void metric_trace_begin(extractor_t *pex)
{
	if (pex->metric.trace.started) return;

	pex->metric.trace.started = true;
	pex->metric.trace.enabled = metric_trace_selected();
	pex->metric.trace.next_work = METRIC_TRACE_PROGRESS_INTERVAL;
}

/* Domain feedback changes root-candidate rejection semantics, so all
 * ranker caches derived under the old marks must be discarded together. */
#define METRIC_COST_EPSILON 1.0e-6

/* The metric extractor is a lazy K-best dynamic program over the
 * Parse_set forest.  Each Parse_set/state stream is sorted by the sum
 * of the local Parse_choice metric and the already-ranked child metrics.
 * This keeps extraction ordered without materializing all linkages first.
 */
static Parse_metric metric_add(Parse_metric left, Parse_metric right)
{
	left.disjunct_cost += right.disjunct_cost;
	left.link_cost += right.link_cost;
	return left;
}

static int metric_compare(Parse_metric left, Parse_metric right)
{
	float diff = left.disjunct_cost - right.disjunct_cost;
	if (METRIC_COST_EPSILON < diff) return 1;
	if (diff < -METRIC_COST_EPSILON) return -1;

	if (left.link_cost != right.link_cost)
		return left.link_cost - right.link_cost;

	return 0;
}

static bool metric_candidate_less(Metric_candidate *left,
                                  Metric_candidate *right)
{
	int cmp = metric_compare(left->metric, right->metric);
	if (cmp != 0) return cmp < 0;
	return left->serial < right->serial;
}

/* Candidate heaps order the frontier inside one Parse_set/state stream.
 * Compare the metric tuple and use serial numbers only as stable
 * tie-breakers. */
static Metric_heap *metric_heap_new(size_t capacity)
{
	Metric_heap *heap = malloc(sizeof(*heap));
	assert(NULL != heap, "Out of memory allocating metric heap");

	if (capacity < 16) capacity = 16;
	heap->array = malloc(capacity * sizeof(*heap->array));
	assert(NULL != heap->array, "Out of memory allocating metric heap array");
	heap->size = 0;
	heap->capacity = capacity;
	return heap;
}

static void metric_heap_swap(Metric_candidate **left,
                             Metric_candidate **right)
{
	Metric_candidate *tmp = *left;
	*left = *right;
	*right = tmp;
}

static void metric_heapify_up(Metric_heap *heap, size_t idx)
{
	while (idx > 0)
	{
		size_t parent = (idx - 1) / 2;
		if (!metric_candidate_less(heap->array[idx], heap->array[parent]))
			break;

		metric_heap_swap(&heap->array[idx], &heap->array[parent]);
		idx = parent;
	}
}

static void metric_heapify_down(Metric_heap *heap, size_t idx)
{
	for (;;)
	{
		size_t smallest = idx;
		size_t left = 2 * idx + 1;
		size_t right = left + 1;

		if ((left < heap->size) &&
		    metric_candidate_less(heap->array[left], heap->array[smallest]))
			smallest = left;

		if ((right < heap->size) &&
		    metric_candidate_less(heap->array[right], heap->array[smallest]))
			smallest = right;

		if (smallest == idx) break;
		metric_heap_swap(&heap->array[idx], &heap->array[smallest]);
		idx = smallest;
	}
}

static void metric_heap_push(Metric_heap *heap, Metric_candidate *candidate)
{
	if (heap->size == heap->capacity)
	{
		heap->capacity *= 2;
		heap->array = realloc(heap->array, heap->capacity * sizeof(*heap->array));
		assert(NULL != heap->array, "Out of memory growing metric heap array");
	}

	heap->array[heap->size] = candidate;
	metric_heapify_up(heap, heap->size);
	heap->size++;
}

static Metric_candidate *metric_heap_pop(Metric_heap *heap)
{
	if ((NULL == heap) || (0 == heap->size)) return NULL;

	Metric_candidate *candidate = heap->array[0];
	heap->size--;
	if (heap->size > 0)
	{
		heap->array[0] = heap->array[heap->size];
		metric_heapify_down(heap, 0);
	}
	return candidate;
}

static size_t metric_ranker_cache_table_size(const extractor_t *pex)
{
	return MAX((size_t)64, MIN((size_t)pex->x_table_size, (size_t)4096));
}

static size_t metric_ranker_cache_table_limit(const extractor_t *pex)
{
	return MAX((size_t)64, (size_t)pex->x_table_size);
}

/* Exact metric-ranker states summarize a chosen subgraph.  They differ
 * from feedback summaries: MFC and small global contains-one facts are
 * exact stream states, while bounded/domain feedback remains summarized on
 * Metric_candidate. */
#define METRIC_MFC_TERMINAL_CONN ((Metric_state)1)
#define METRIC_MFC_TERMINAL_NEED ((Metric_state)2)
#define METRIC_MFC_TERMINAL_NUM_STATES 4
#define METRIC_MFC_TERMINAL_NUM_BITS 2
#define METRIC_GLOBAL_CONTAINS_ONE_EXACT_BITS 2
#define METRIC_GLOBAL_CONTAINS_ONE_PROMOTE_REJECTS 4096

static Metric_state metric_exact_state_mask(Metric_state state)
{
	return (Metric_state)1 << state;
}

static bool metric_mfc_terminal_exact_enabled(const extractor_t *pex)
{
	return pex->metric.pp.mfc_enabled;
}

static void metric_pp_prediction_clear(extractor_t *pex)
{
	memset(&pex->metric.pp.prediction, 0,
	       sizeof(pex->metric.pp.prediction));
	pex->metric.pp.prediction.type = PP_FAILURE_NONE;
	pex->metric.pp.prediction.domain = -1;
}

/* Validation mode records the first production constraint that would have
 * rejected a candidate, but keeps it alive so batched PP can be the oracle. */
static void metric_pp_predict(extractor_t *pex, PP_failure_type type,
                              const char *message)
{
	if (!pex->metric.pp.validate_enabled) return;
	if (PP_FAILURE_NONE != pex->metric.pp.prediction.type) return;

	pex->metric.pp.prediction.type = type;
	pex->metric.pp.prediction.message = message;
}

/* Static global contains-one constraints occupy global bit positions used
 * by exact root states and fallback root-candidate scans. */
static size_t metric_global_contains_one_static_count(const extractor_t *pex)
{
	if (!pex->metric.pp.global_enabled) return 0;

	return MIN(post_process_parse_contains_one_global_rule_count(
		           pex->postprocessor),
	           (size_t)METRIC_GLOBAL_CONTAINS_ONE_MAX_MARKS);
}

static const char *metric_global_contains_one_message(
	const extractor_t *pex, size_t idx)
{
	if (idx < metric_global_contains_one_static_count(pex))
		return post_process_parse_contains_one_global_rule_message(
			pex->postprocessor, idx);
	return NULL;
}

static size_t metric_global_contains_one_exact_count(const extractor_t *pex)
{
	if (!pex->metric.pp.global_enabled) return 0;

	return pex->metric.pp.global.num_exact;
}

static size_t metric_global_contains_one_exact_offset(const extractor_t *pex)
{
	return metric_mfc_terminal_exact_enabled(pex) ?
	       METRIC_MFC_TERMINAL_NUM_BITS : 0;
}

static size_t metric_global_contains_one_exact_rule_idx(
	const extractor_t *pex, size_t slot)
{
	assert(slot < pex->metric.pp.global.num_exact,
	       "Invalid global contains-one exact slot");
	return pex->metric.pp.global.exact_rule_idx[slot];
}

static Metric_state metric_global_contains_one_exact_selector_state(
	const extractor_t *pex, size_t slot)
{
	size_t bit = metric_global_contains_one_exact_offset(pex) +
	             METRIC_GLOBAL_CONTAINS_ONE_EXACT_BITS * slot;

	return (Metric_state)1 << bit;
}

static Metric_state metric_global_contains_one_exact_criterion_state(
	const extractor_t *pex, size_t slot)
{
	size_t bit = metric_global_contains_one_exact_offset(pex) +
	             METRIC_GLOBAL_CONTAINS_ONE_EXACT_BITS * slot + 1;

	return (Metric_state)1 << bit;
}

static Metric_state metric_global_contains_one_exact_mask(
	const extractor_t *pex)
{
	size_t count = metric_global_contains_one_exact_count(pex);
	size_t offset;
	size_t bits;

	if (0 == count) return 0;

	offset = metric_global_contains_one_exact_offset(pex);
	bits = METRIC_GLOBAL_CONTAINS_ONE_EXACT_BITS * count;
	return (((Metric_state)1 << bits) - 1) << offset;
}

static size_t metric_exact_state_num_bits(const extractor_t *pex)
{
	size_t bits = metric_mfc_terminal_exact_enabled(pex) ?
	              METRIC_MFC_TERMINAL_NUM_BITS : 0;

	bits += METRIC_GLOBAL_CONTAINS_ONE_EXACT_BITS *
	        metric_global_contains_one_exact_count(pex);
	assert(bits < CHAR_BIT * sizeof(size_t),
	       "Too many metric exact-state bits");
	assert(((size_t)1 << bits) <= CHAR_BIT * sizeof(Metric_state),
	       "Too many metric exact states");
	return bits;
}

static bool metric_exact_state_enabled(const extractor_t *pex)
{
	return 0 < metric_exact_state_num_bits(pex);
}

static size_t metric_exact_state_count(const extractor_t *pex)
{
	return (size_t)1 << metric_exact_state_num_bits(pex);
}

static Metric_state metric_state_mfc_part(const extractor_t *pex,
                                          Metric_state state)
{
	return metric_mfc_terminal_exact_enabled(pex) ?
		state & (METRIC_MFC_TERMINAL_NUM_STATES - 1) : 0;
}

static void metric_state_set_mfc_part(const extractor_t *pex,
                                      Metric_state *state,
                                      Metric_state part)
{
	if (!metric_mfc_terminal_exact_enabled(pex)) return;

	*state |= part & (METRIC_MFC_TERMINAL_NUM_STATES - 1);
}

static Metric_state metric_state_global_contains_one_part(
	const extractor_t *pex, Metric_state state)
{
	return state & metric_global_contains_one_exact_mask(pex);
}

static bool metric_global_contains_one_select_exact_slots(extractor_t *);
static void metric_ranker_init_root_states(extractor_t *, Metric_ranker *);

/* A Metric_ranker owns all memoized streams for one interpretation of
 * Metric_state.  Learning new feedback creates a new ranker instead of
 * mutating old stream caches whose states were computed under different
 * rules. */
static Metric_ranker *metric_ranker_new(extractor_t *pex)
{
	Metric_ranker *ranker = pool_alloc(pex->metric.ranker_pool);
	memset(ranker, 0, sizeof(*ranker));

	ranker->candidate_pool =
		pool_new(__func__, "Metric_candidate",
		         METRIC_CANDIDATE_POOL_BLOCK_SIZE, sizeof(Metric_candidate),
		         /*zero_out*/false, /*align*/false, /*exact*/false);
	ranker->set_cache_pool =
		pool_new(__func__, "Metric_set_cache",
		         /*num_elements*/1024, sizeof(Metric_set_cache),
		         /*zero_out*/false, /*align*/false, /*exact*/false);
	if (metric_exact_state_enabled(pex))
		ranker->state_count = metric_exact_state_count(pex);
	else
		ranker->state_count = 1;
	ranker->cache_table_size = metric_ranker_cache_table_size(pex);
	ranker->cache_table_limit = metric_ranker_cache_table_limit(pex);
	ranker->cache_table = calloc(ranker->cache_table_size,
	                             sizeof(*ranker->cache_table));
	assert(NULL != ranker->cache_table,
	       "Out of memory allocating metric ranker cache table");
	metric_ranker_init_root_states(pex, ranker);

	ranker->next = pex->metric.rankers;
	pex->metric.rankers = ranker;
	return ranker;
}

static Metric_ranker *metric_root_ranker(extractor_t *pex)
{
	if (NULL == pex->metric.ranker)
	{
		metric_global_contains_one_select_exact_slots(pex);
		pex->metric.ranker = metric_ranker_new(pex);
	}
	return pex->metric.ranker;
}

static size_t metric_set_cache_hash_size(const Parse_set *set, size_t size)
{
	uintptr_t key = (uintptr_t)set >> 4;

	key *= (uintptr_t)11400714819323198485ull;
	return key & (size - 1);
}

static size_t metric_set_cache_hash(const Metric_ranker *ranker,
                                    const Parse_set *set)
{
	return metric_set_cache_hash_size(set, ranker->cache_table_size);
}

/* Long parse sets can create many memoized set streams.  Keep the initial
 * table conservative, but grow it when chains become too dense so repeated
 * rank lookups do not spend avoidable time walking cache chains. */
static void metric_ranker_cache_table_maybe_grow(Metric_ranker *ranker)
{
	size_t new_size;
	Metric_set_cache **new_table;

	if (ranker->cache_table_size >= ranker->cache_table_limit)
		return;
	if (ranker->num_set_caches < 2 * ranker->cache_table_size)
		return;

	new_size = MIN(2 * ranker->cache_table_size,
	               ranker->cache_table_limit);
	new_table = calloc(new_size, sizeof(*new_table));
	assert(NULL != new_table,
	       "Out of memory growing metric ranker cache table");

	for (size_t i = 0; i < ranker->cache_table_size; i++)
	{
		Metric_set_cache *cache = ranker->cache_table[i];

		while (NULL != cache)
		{
			Metric_set_cache *next = cache->next;
			size_t idx = metric_set_cache_hash_size(cache->set,
			                                        new_size);

			cache->next = new_table[idx];
			new_table[idx] = cache;
			cache = next;
		}
	}

	free(ranker->cache_table);
	ranker->cache_table = new_table;
	ranker->cache_table_size = new_size;
}

static Metric_set_cache *metric_get_set_cache(extractor_t *pex,
                                              Metric_ranker *ranker,
                                              Parse_set *set)
{
	size_t idx = metric_set_cache_hash(ranker, set);

	for (Metric_set_cache *cache = ranker->cache_table[idx];
	     NULL != cache; cache = cache->next)
	{
		if (cache->set == set)
			return cache;
	}

	metric_ranker_cache_table_maybe_grow(ranker);
	idx = metric_set_cache_hash(ranker, set);
	Metric_set_cache *cache = pool_alloc(ranker->set_cache_pool);
	memset(cache, 0, sizeof(*cache));
	cache->set = set;
	cache->next = ranker->cache_table[idx];
	ranker->cache_table[idx] = cache;
	ranker->num_set_caches++;
	return cache;
}

static Metric_candidate *metric_get_state_rank(extractor_t *, Metric_ranker *,
                                               Parse_set *, Metric_state,
                                               size_t);
static uint64_t metric_candidate_signature(extractor_t *, Metric_ranker *,
                                           Parse_set *, Metric_candidate *);
static bool metric_seen_insert(extractor_t *, uint64_t);
static void metric_choice_link_words(Parse_choice *, const Parse_set *, int,
                                     WordIdx *, WordIdx *);
static void list_metric_links(extractor_t *, Metric_ranker *, Linkage,
                              Parse_set *, Metric_candidate *);
static bool metric_resources_exhausted(extractor_t *);
static uint64_t metric_global_contains_one_bit(size_t);
static bool metric_link_key_equal(Metric_link_key left,
                                  Metric_link_key right)
{
	return (left.uc_num == right.uc_num) &&
	       (left.lc_label == right.lc_label);
}

static uint64_t metric_link_key_hash(Metric_link_key key)
{
	uint64_t h = key.lc_label;

	h ^= (uint64_t)key.uc_num + 0x9e3779b97f4a7c15ULL +
	     (h << 6) + (h >> 2);
	return h;
}

static bool metric_link_key_from_connectors(const Connector *left,
                                            const Connector *right,
                                            Metric_link_key *key)
{
	const condesc_t *ld;
	const condesc_t *rd;

	if ((NULL == left) || (NULL == right)) return false;
	ld = left->desc;
	rd = right->desc;
	if ((NULL == ld) || (NULL == rd)) return false;

	/* Connector matching already requires the uppercase class to agree.
	 * Keep the guard so a malformed call cannot merge unrelated links. */
	if (ld->uc_num != rd->uc_num) return false;

	key->uc_num = ld->uc_num;
	key->lc_label = (ld->lc_letters >> 1) | (rd->lc_letters >> 1);
	return true;
}

static void metric_link_classifier_init(extractor_t *pex)
{
	if (NULL != pex->metric.links.classes) return;

	pex->metric.links.classes_size = 32;
	pex->metric.links.classes = calloc(
		pex->metric.links.classes_size,
		sizeof(*pex->metric.links.classes));
	assert(NULL != pex->metric.links.classes,
	       "Out of memory allocating metric link classes");
	pex->metric.links.num_classes = 1; /* ID 0 is the zero/irrelevant link. */

	pex->metric.links.id_table_size = 64;
	pex->metric.links.id_table = calloc(
		pex->metric.links.id_table_size,
		sizeof(*pex->metric.links.id_table));
	assert(NULL != pex->metric.links.id_table,
	       "Out of memory allocating metric link-id table");
}

static void metric_link_id_table_insert(extractor_t *pex,
                                        Metric_link_key key,
                                        Metric_link_id id)
{
	size_t mask = pex->metric.links.id_table_size - 1;

	for (size_t idx = metric_link_key_hash(key) & mask;;
	     idx = (idx + 1) & mask)
	{
		Metric_link_id_entry *entry = &pex->metric.links.id_table[idx];

		if (METRIC_LINK_ID_NONE != entry->id) continue;
		entry->key = key;
		entry->id = id;
		pex->metric.links.id_table_used++;
		return;
	}
}

static void metric_link_id_table_grow(extractor_t *pex)
{
	Metric_link_id_entry *old_table = pex->metric.links.id_table;
	size_t old_size = pex->metric.links.id_table_size;

	pex->metric.links.id_table_size *= 2;
	pex->metric.links.id_table = calloc(
		pex->metric.links.id_table_size,
		sizeof(*pex->metric.links.id_table));
	assert(NULL != pex->metric.links.id_table,
	       "Out of memory growing metric link-id table");
	pex->metric.links.id_table_used = 0;

	for (size_t i = 0; i < old_size; i++)
		if (METRIC_LINK_ID_NONE != old_table[i].id)
			metric_link_id_table_insert(
				pex, old_table[i].key, old_table[i].id);

	free(old_table);
}

static Metric_link_id metric_link_id_lookup(extractor_t *pex,
                                            Metric_link_key key)
{
	size_t mask = pex->metric.links.id_table_size - 1;

	for (size_t idx = metric_link_key_hash(key) & mask;;
	     idx = (idx + 1) & mask)
	{
		Metric_link_id_entry *entry = &pex->metric.links.id_table[idx];

		if (METRIC_LINK_ID_NONE == entry->id) return METRIC_LINK_ID_NONE;
		if (metric_link_key_equal(entry->key, key)) return entry->id;
	}
}

static Metric_link_class *metric_link_class_by_id(extractor_t *pex,
                                                  Metric_link_id id)
{
	assert(id < pex->metric.links.num_classes, "Invalid metric link ID");
	return &pex->metric.links.classes[id];
}

/* Refresh one link class against the current feedback epoch.  Static PP
 * link-set facts and dynamic selector/criterion masks are cached together
 * so hot candidate scans can use integer masks instead of PP string
 * matching. */
static void metric_link_class_refresh(extractor_t *pex,
                                      Metric_link_class *cls)
{
	const char *name = cls->name;

	cls->flags = 0;
	cls->domain_kind = PP_DOMAIN_NONE;
	cls->global_contains_one_state = 0;
	cls->global_contains_one_selector = 0;
	cls->global_contains_one_criterion = 0;
	cls->parse_contains_one_selector = 0;
	cls->parse_contains_one_criterion = 0;
	cls->parse_contains_none_selector = 0;
	cls->parse_contains_none_forbidden = 0;

	if (NULL != name)
	{
		if (post_process_link_ignored(pex->postprocessor, name))
			cls->flags |= METRIC_LINK_IGNORED;
		if (post_process_link_restricted(pex->postprocessor, name))
			cls->flags |= METRIC_LINK_RESTRICTED;
		if (post_process_link_must_form_cycle(pex->postprocessor, name))
			cls->flags |= METRIC_LINK_MUST_FORM_CYCLE;
		if (post_process_link_domain_contains(pex->postprocessor, name))
			cls->flags |= METRIC_LINK_DOMAIN_CONTAINS_START;
		cls->domain_kind =
			post_process_link_domain_kind(pex->postprocessor, name);
		cls->parse_contains_one_selector =
			post_process_parse_contains_one_selector_mask(
				pex->postprocessor, name);
		cls->parse_contains_one_criterion =
			post_process_parse_contains_one_criterion_mask(
				pex->postprocessor, name);
		cls->parse_contains_none_selector =
			post_process_parse_contains_none_selector_mask(
				pex->postprocessor, name);
		cls->parse_contains_none_forbidden =
			post_process_parse_contains_none_forbidden_mask(
				pex->postprocessor, name);
		cls->global_contains_one_selector =
			post_process_parse_contains_one_global_selector_mask(
				pex->postprocessor, name);
		cls->global_contains_one_criterion =
			post_process_parse_contains_one_global_criterion_mask(
				pex->postprocessor, name);
	}

	if (NULL == name) return;

	/* Exact state uses sentence-local slots.  The full selector and
	 * criterion masks keep the original rule indexes so fallback scans and
	 * adaptive promotion can still refer to the parsed knowledge rows. */
	for (size_t slot = 0;
	     slot < metric_global_contains_one_exact_count(pex); slot++)
	{
		size_t rule_idx =
			metric_global_contains_one_exact_rule_idx(pex, slot);
		uint64_t bit = metric_global_contains_one_bit(rule_idx);

		if (cls->global_contains_one_selector & bit)
			cls->global_contains_one_state |=
				metric_global_contains_one_exact_selector_state(
					pex, slot);
		if (cls->global_contains_one_criterion & bit)
			cls->global_contains_one_state |=
				metric_global_contains_one_exact_criterion_state(
					pex, slot);
	}

	cls->epoch = pex->metric.links.class_epoch;
}

static const Metric_link_class *metric_link_class(extractor_t *pex,
                                                  Metric_link_id id)
{
	Metric_link_class *cls;

	metric_link_classifier_init(pex);
	cls = metric_link_class_by_id(pex, id);
	if (cls->epoch != pex->metric.links.class_epoch)
		metric_link_class_refresh(pex, cls);
	return cls;
}

static bool metric_link_class_parse_constraint_relevant(
	extractor_t *pex, const Metric_link_class *cls)
{
	(void)pex;
	return (0 != cls->parse_contains_one_selector) ||
	       (0 != cls->parse_contains_none_selector);
}

static Metric_link_id metric_link_id_for_connectors(extractor_t *pex,
                                                    const Connector *left,
                                                    const Connector *right)
{
	Metric_link_key key;
	Metric_link_id id;

	if (!metric_link_key_from_connectors(left, right, &key))
		return METRIC_LINK_ID_NONE;

	metric_link_classifier_init(pex);
	id = metric_link_id_lookup(pex, key);
	if (METRIC_LINK_ID_NONE != id) return id;

	if (pex->metric.links.num_classes == pex->metric.links.classes_size)
	{
		pex->metric.links.classes_size *= 2;
		pex->metric.links.classes = realloc(
			pex->metric.links.classes,
			pex->metric.links.classes_size *
			sizeof(*pex->metric.links.classes));
		assert(NULL != pex->metric.links.classes,
		       "Out of memory growing metric link classes");
	}
	if (2 * (pex->metric.links.id_table_used + 1) >=
	    pex->metric.links.id_table_size)
		metric_link_id_table_grow(pex);

	id = (Metric_link_id)pex->metric.links.num_classes++;
	Metric_link_class *cls = metric_link_class_by_id(pex, id);
	memset(cls, 0, sizeof(*cls));
	cls->key = key;
	cls->name = intersect_strings(pex->string_set, left, right);
	metric_link_id_table_insert(pex, key, id);
	return id;
}

static Metric_link_id choice_link_id(extractor_t *pex,
                                     Parse_choice *pc,
                                     const Parse_set *set,
                                     int side)
{
	uint8_t mask = (uint8_t)1 << side;
	Connector *lc = side ? get_tracon_by_id(pc->md, pc->r_id, 1) : set->le;
	Connector *rc;

	if (pc->metric_link_id_done & mask)
		return pc->metric_link_id[side];

	pc->metric_link_id_done |= mask;
	if (is_zero_tracon(lc)) return METRIC_LINK_ID_NONE;

	rc = side ? set->re : get_tracon_by_id(pc->md, pc->l_id, 0);
	if (is_zero_tracon(rc)) return METRIC_LINK_ID_NONE;

	pc->metric_link_id[side] =
		metric_link_id_for_connectors(pex, lc, rc);
	return pc->metric_link_id[side];
}

/* Link names and PP link properties are expensive enough to matter in
 * metric extraction.  Cache a dense link ID on Parse_choice because the
 * same choice can be inspected many times while advancing K-best stream
 * frontiers.  Diagnostics can recover the canonical string from the shared
 * link-class table. */
static const char *choice_link_name(extractor_t *pex,
                                    Parse_choice *pc,
                                    const Parse_set *set,
                                    int side)
{
	Metric_link_id id = choice_link_id(pex, pc, set, side);

	if (METRIC_LINK_ID_NONE == id) return NULL;
	return metric_link_class(pex, id)->name;
}

static bool metric_choice_parse_constraint_relevant(
	extractor_t *pex, Parse_choice *choice, const Parse_set *set)
{
	for (int side = 0; side < 2; side++)
	{
		Metric_link_id id = choice_link_id(pex, choice, set, side);
		const Metric_link_class *cls;

		if (METRIC_LINK_ID_NONE == id) continue;
		cls = metric_link_class(pex, id);
		if (metric_link_class_parse_constraint_relevant(pex, cls))
			return true;
	}
	return false;
}

static bool choice_link_ignored(extractor_t *pex, Parse_choice *pc,
                                int side, const char *name)
{
	uint8_t mask = (uint8_t)1 << side;

	if (!(pc->metric_link_id_done & mask))
		return post_process_link_ignored(pex->postprocessor, name);

	Metric_link_id id = pc->metric_link_id[side];
	return 0 != (metric_link_class(pex, id)->flags & METRIC_LINK_IGNORED);
}

static bool choice_link_must_form_cycle(extractor_t *pex, Parse_choice *pc,
                                        int side, const char *name)
{
	uint8_t mask = (uint8_t)1 << side;

	if (!(pc->metric_link_id_done & mask))
		return post_process_link_must_form_cycle(pex->postprocessor, name);

	Metric_link_id id = pc->metric_link_id[side];
	return 0 != (metric_link_class(pex, id)->flags &
	             METRIC_LINK_MUST_FORM_CYCLE);
}

/* Compact graph summary for one Parse_choice.  The parent can only
 * interact with this subgraph through its left and right terminals; an
 * MFC need that cannot be satisfied inside this graph is propagated only
 * when an outside left-right path could satisfy it. */
typedef struct
{
	unsigned char left;
	unsigned char right;
} Metric_mfc_terminal_edge;

typedef struct
{
	unsigned char left;
	unsigned char right;
	int skip_edge;
} Metric_mfc_terminal_need;

typedef struct
{
	Metric_mfc_terminal_edge edge[4];
	Metric_mfc_terminal_need need[4];
	size_t num_edges;
	size_t num_needs;
} Metric_mfc_terminal_graph;

#define METRIC_MFC_NODE_LEFT 0
#define METRIC_MFC_NODE_MIDDLE 1
#define METRIC_MFC_NODE_RIGHT 2

static int metric_mfc_terminal_add_edge(Metric_mfc_terminal_graph *graph,
                                        unsigned char left,
                                        unsigned char right)
{
	assert(graph->num_edges < ARRAY_SIZE(graph->edge),
	       "Too many terminal MFC edges");

	size_t idx = graph->num_edges++;
	graph->edge[idx] = (Metric_mfc_terminal_edge){
		.left = left,
		.right = right
	};
	return (int)idx;
}

static void metric_mfc_terminal_add_need(Metric_mfc_terminal_graph *graph,
                                         unsigned char left,
                                         unsigned char right,
                                         int skip_edge)
{
	assert(graph->num_needs < ARRAY_SIZE(graph->need),
	       "Too many terminal MFC needs");

	graph->need[graph->num_needs++] = (Metric_mfc_terminal_need){
		.left = left,
		.right = right,
		.skip_edge = skip_edge
	};
}

static unsigned char metric_mfc_find(unsigned char parent[3],
                                     unsigned char node)
{
	while (parent[node] != node)
	{
		parent[node] = parent[parent[node]];
		node = parent[node];
	}
	return node;
}

static void metric_mfc_union(unsigned char parent[3],
                             unsigned char left, unsigned char right)
{
	left = metric_mfc_find(parent, left);
	right = metric_mfc_find(parent, right);
	if (left != right) parent[right] = left;
}

static void metric_mfc_terminal_components(
	const Metric_mfc_terminal_graph *graph, int skip_edge,
	unsigned char parent[3])
{
	for (unsigned char i = 0; i < 3; i++)
		parent[i] = i;

	for (size_t i = 0; i < graph->num_edges; i++)
	{
		if ((int)i == skip_edge) continue;
		metric_mfc_union(parent, graph->edge[i].left,
		                 graph->edge[i].right);
	}
}

static bool metric_mfc_terminal_connected(
	const Metric_mfc_terminal_graph *graph, int skip_edge,
	unsigned char left, unsigned char right)
{
	unsigned char parent[3];

	metric_mfc_terminal_components(graph, skip_edge, parent);
	return metric_mfc_find(parent, left) ==
	       metric_mfc_find(parent, right);
}

static bool metric_mfc_terminal_can_escape(
	const Metric_mfc_terminal_graph *graph, int skip_edge,
	unsigned char left, unsigned char right)
{
	unsigned char parent[3];
	unsigned char lcomp, rcomp, left_comp, right_comp;

	metric_mfc_terminal_components(graph, skip_edge, parent);
	lcomp = metric_mfc_find(parent, METRIC_MFC_NODE_LEFT);
	rcomp = metric_mfc_find(parent, METRIC_MFC_NODE_RIGHT);
	left_comp = metric_mfc_find(parent, left);
	right_comp = metric_mfc_find(parent, right);

	return ((left_comp == lcomp) && (right_comp == rcomp)) ||
	       ((left_comp == rcomp) && (right_comp == lcomp));
}

static void metric_mfc_terminal_choice_link(
	extractor_t *pex, Parse_choice *choice, const Parse_set *set,
	int side, Metric_mfc_terminal_graph *graph,
	unsigned char left, unsigned char right)
{
	const char *name = choice_link_name(pex, choice, set, side);
	int edge_idx = -1;

	if (NULL == name) return;

	if (!choice_link_ignored(pex, choice, side, name))
		edge_idx = metric_mfc_terminal_add_edge(graph, left, right);

	if (choice_link_must_form_cycle(pex, choice, side, name))
		metric_mfc_terminal_add_need(graph, left, right, edge_idx);
}

/* Exact terminal-state MFC handling summarizes each subtree by two facts:
 * whether its terminals are connected, and whether an unresolved
 * must-form-cycle need still has to escape through the subtree terminals.
 * If a need is disconnected and cannot escape to an outer context, this
 * Parse_choice cannot participate in any PP-valid root candidate. */
static bool metric_mfc_terminal_choice_state(
	extractor_t *pex, Parse_choice *choice, const Parse_set *set,
	Metric_state left_state, Metric_state right_state,
	Metric_state *parent_state)
{
	Metric_mfc_terminal_graph graph = { 0 };
	bool need = false;
	int left_edge = -1;
	int right_edge = -1;

	if (left_state & METRIC_MFC_TERMINAL_CONN)
		left_edge = metric_mfc_terminal_add_edge(
			&graph, METRIC_MFC_NODE_LEFT, METRIC_MFC_NODE_MIDDLE);
	if (right_state & METRIC_MFC_TERMINAL_CONN)
		right_edge = metric_mfc_terminal_add_edge(
			&graph, METRIC_MFC_NODE_MIDDLE, METRIC_MFC_NODE_RIGHT);

	if (left_state & METRIC_MFC_TERMINAL_NEED)
		metric_mfc_terminal_add_need(
			&graph, METRIC_MFC_NODE_LEFT, METRIC_MFC_NODE_MIDDLE,
			left_edge);
	if (right_state & METRIC_MFC_TERMINAL_NEED)
		metric_mfc_terminal_add_need(
			&graph, METRIC_MFC_NODE_MIDDLE, METRIC_MFC_NODE_RIGHT,
			right_edge);

	metric_mfc_terminal_choice_link(
		pex, choice, set, 0, &graph,
		METRIC_MFC_NODE_LEFT, METRIC_MFC_NODE_MIDDLE);
	metric_mfc_terminal_choice_link(
		pex, choice, set, 1, &graph,
		METRIC_MFC_NODE_MIDDLE, METRIC_MFC_NODE_RIGHT);

	for (size_t i = 0; i < graph.num_needs; i++)
	{
		const Metric_mfc_terminal_need *cur = &graph.need[i];

		if (metric_mfc_terminal_connected(
		    &graph, cur->skip_edge, cur->left, cur->right))
			continue;
		if (!metric_mfc_terminal_can_escape(
		    &graph, cur->skip_edge, cur->left, cur->right))
			return false;

		need = true;
	}

	/* The parent inherits terminal connectivity and any cycle need that
	 * is still satisfiable only by links outside this local choice. */
	*parent_state =
		(metric_mfc_terminal_connected(
			&graph, -1, METRIC_MFC_NODE_LEFT,
			METRIC_MFC_NODE_RIGHT) ?
			METRIC_MFC_TERMINAL_CONN : 0) |
		(need ? METRIC_MFC_TERMINAL_NEED : 0);
	return true;
}

static bool metric_mfc_terminal_root_state_allowed(Metric_state state)
{
	return 0 == (state & METRIC_MFC_TERMINAL_NEED);
}

typedef struct
{
	uint8_t edge[3];
	uint8_t source;
} Metric_bounded_domain_graph;

#define METRIC_BOUNDED_REACH_LEFT ((uint8_t)1)
#define METRIC_BOUNDED_REACH_RIGHT ((uint8_t)2)
#define METRIC_BOUNDED_REL_LR ((uint8_t)4)
#define METRIC_BOUNDED_REL_RL ((uint8_t)8)

static size_t metric_bounded_domain_active_count(const extractor_t *pex)
{
	if (!pex->metric.pp.bounded_enabled) return 0;

	return MIN(pex->metric.pp.bounded.num_feedbacks,
	           (size_t)METRIC_BOUNDED_DOMAIN_MAX_MARKS);
}

static size_t metric_global_contains_one_active_count(const extractor_t *pex)
{
	if (!pex->metric.pp.global_enabled) return 0;

	return metric_global_contains_one_static_count(pex);
}

static uint64_t metric_global_contains_one_bit(size_t idx)
{
	return (uint64_t)1 << idx;
}

static uint64_t metric_global_contains_one_active_rule_mask(
	const extractor_t *pex)
{
	size_t count = metric_global_contains_one_active_count(pex);

	if (0 == count) return 0;
	if (METRIC_GLOBAL_CONTAINS_ONE_MAX_MARKS <= count)
		return UINT64_MAX;
	return ((uint64_t)1 << count) - 1;
}

typedef struct
{
	/* Pointer identity set used while scanning a Parse_set DAG.  The
	 * sentence-local scan must count each Parse_set node once even when
	 * several parent choices share the same subproblem. */
	Parse_set **table;
	size_t size;
	size_t used;
} Metric_set_seen;

static size_t metric_parse_set_hash(const Parse_set *set, size_t size)
{
	uintptr_t key = (uintptr_t)set >> 4;

	key *= (uintptr_t)11400714819323198485ull;
	return key & (size - 1);
}

static void metric_set_seen_init(Metric_set_seen *seen)
{
	seen->size = 1024;
	seen->used = 0;
	seen->table = calloc(seen->size, sizeof(*seen->table));
	assert(NULL != seen->table,
	       "Out of memory allocating metric Parse_set seen table");
}

static void metric_set_seen_free(Metric_set_seen *seen)
{
	free(seen->table);
	*seen = (Metric_set_seen){ 0 };
}

static bool metric_set_seen_insert(Metric_set_seen *seen, Parse_set *set)
{
	if (NULL == set) return false;

	if (2 * (seen->used + 1) >= seen->size)
	{
		Parse_set **old_table = seen->table;
		size_t old_size = seen->size;

		seen->size *= 2;
		seen->table = calloc(seen->size, sizeof(*seen->table));
		assert(NULL != seen->table,
		       "Out of memory growing metric Parse_set seen table");
		seen->used = 0;

		for (size_t i = 0; i < old_size; i++)
		{
			Parse_set *old_set = old_table[i];
			if (NULL == old_set) continue;
			size_t idx = metric_parse_set_hash(old_set, seen->size);
			while (NULL != seen->table[idx])
				idx = (idx + 1) & (seen->size - 1);
			seen->table[idx] = old_set;
			seen->used++;
		}
		free(old_table);
	}

	size_t idx = metric_parse_set_hash(set, seen->size);
	while (NULL != seen->table[idx])
	{
		if (seen->table[idx] == set) return false;
		idx = (idx + 1) & (seen->size - 1);
	}

	seen->table[idx] = set;
	seen->used++;
	return true;
}

static void metric_global_contains_one_score_link(
	extractor_t *pex, Metric_link_id id, uint64_t active_mask,
	size_t active_count,
	size_t selector_score[METRIC_GLOBAL_CONTAINS_ONE_MAX_MARKS])
{
	if (METRIC_LINK_ID_NONE == id) return;

	const Metric_link_class *cls = metric_link_class(pex, id);
	uint64_t selector = cls->global_contains_one_selector & active_mask;

	pex->metric.pp.global.active_selector_mask |= selector;
	for (size_t idx = 0; idx < active_count; idx++)
	{
		uint64_t bit = metric_global_contains_one_bit(idx);

		if (selector & bit) selector_score[idx]++;
	}
}

/* Score selector pressure in the root Parse_set.  Exact slots are assigned
 * to rules whose selector can actually occur in this sentence, with hot
 * fallback-rejection counters breaking ties before raw occurrence counts. */
static void metric_global_contains_one_score_set(
	extractor_t *pex, Parse_set *set, Metric_set_seen *seen,
	uint64_t active_mask, size_t active_count,
	size_t selector_score[METRIC_GLOBAL_CONTAINS_ONE_MAX_MARKS])
{
	if ((NULL == set) || (NULL == set->first)) return;
	if (!metric_set_seen_insert(seen, set)) return;

	for (Parse_choice *pc = set->first; NULL != pc; pc = pc->next)
	{
		for (int side = 0; side < 2; side++)
			metric_global_contains_one_score_link(
				pex, choice_link_id(pex, pc, set, side),
				active_mask, active_count, selector_score);
		for (int side = 0; side < 2; side++)
			metric_global_contains_one_score_set(
				pex, pc->set[side], seen, active_mask,
				active_count, selector_score);
	}
}

static bool metric_global_contains_one_rule_better(
	const extractor_t *pex, size_t rule_idx, size_t best_idx,
	const size_t selector_score[METRIC_GLOBAL_CONTAINS_ONE_MAX_MARKS])
{
	bool hot = METRIC_GLOBAL_CONTAINS_ONE_PROMOTE_REJECTS <=
	           pex->metric.pp.global.fallback_rejects[rule_idx];
	bool best_hot = METRIC_GLOBAL_CONTAINS_ONE_PROMOTE_REJECTS <=
	           pex->metric.pp.global.fallback_rejects[best_idx];

	if (hot != best_hot) return hot;
	if (pex->metric.pp.global.fallback_rejects[rule_idx] !=
	    pex->metric.pp.global.fallback_rejects[best_idx])
		return pex->metric.pp.global.fallback_rejects[rule_idx] >
		       pex->metric.pp.global.fallback_rejects[best_idx];
	if (selector_score[rule_idx] != selector_score[best_idx])
		return selector_score[rule_idx] > selector_score[best_idx];
	return rule_idx < best_idx;
}

static bool metric_global_contains_one_exact_slots_changed(
	const extractor_t *pex, size_t old_count,
	const size_t old_idx[METRIC_GLOBAL_CONTAINS_ONE_EXACT_MAX_MARKS])
{
	if (old_count != pex->metric.pp.global.num_exact)
		return true;

	for (size_t i = 0; i < old_count; i++)
		if (old_idx[i] != pex->metric.pp.global.exact_rule_idx[i])
			return true;

	return false;
}

/* Pick the global contains-one rules that deserve exact ranker state for
 * this sentence.  The knowledge file remains dictionary-level data, but the
 * exact-state budget is sentence-local and should not be consumed by rules
 * whose selector cannot occur in the current root Parse_set. */
static bool metric_global_contains_one_select_exact_slots(extractor_t *pex)
{
	if (pex->metric.pp.global.exact_ready) return false;

	size_t old_count = pex->metric.pp.global.num_exact;
	size_t old_idx[METRIC_GLOBAL_CONTAINS_ONE_EXACT_MAX_MARKS];
	memcpy(old_idx, pex->metric.pp.global.exact_rule_idx,
	       sizeof(old_idx));

	pex->metric.pp.global.num_exact = 0;
	pex->metric.pp.global.exact_rule_mask = 0;
	pex->metric.pp.global.active_selector_mask = 0;

	if (!pex->metric.pp.global_enabled)
	{
		pex->metric.pp.global.exact_ready = true;
		bool changed = metric_global_contains_one_exact_slots_changed(
			pex, old_count, old_idx);
		if (changed) pex->metric.links.class_epoch++;
		return changed;
	}

	size_t active_count = metric_global_contains_one_active_count(pex);
	uint64_t active_mask = metric_global_contains_one_active_rule_mask(pex);
	if ((0 == active_mask) || (NULL == pex->parse_set))
	{
		pex->metric.pp.global.exact_ready = true;
		bool changed = metric_global_contains_one_exact_slots_changed(
			pex, old_count, old_idx);
		if (changed) pex->metric.links.class_epoch++;
		return changed;
	}

	size_t selector_score[METRIC_GLOBAL_CONTAINS_ONE_MAX_MARKS] = { 0 };
	Metric_set_seen seen;

	metric_set_seen_init(&seen);
	metric_global_contains_one_score_set(
		pex, pex->parse_set, &seen, active_mask,
		active_count, selector_score);
	metric_set_seen_free(&seen);

	uint64_t selected = 0;
	for (size_t slot = 0;
	     slot < METRIC_GLOBAL_CONTAINS_ONE_EXACT_MAX_MARKS; slot++)
	{
		size_t best = (size_t)-1;

		for (size_t idx = 0; idx < active_count; idx++)
		{
			uint64_t bit = metric_global_contains_one_bit(idx);

			if (0 == (pex->metric.pp.global.active_selector_mask &
			          bit))
				continue;
			if (selected & bit) continue;
			if ((size_t)-1 == best ||
			    metric_global_contains_one_rule_better(
				    pex, idx, best, selector_score))
				best = idx;
		}
		if ((size_t)-1 == best) break;

		selected |= metric_global_contains_one_bit(best);
		pex->metric.pp.global.exact_rule_idx[
			pex->metric.pp.global.num_exact++] = best;
	}

	pex->metric.pp.global.exact_rule_mask = selected;
	pex->metric.pp.global.exact_ready = true;
	bool changed = metric_global_contains_one_exact_slots_changed(
		pex, old_count, old_idx);
	if (changed)
		pex->metric.links.class_epoch++;
	return changed;
}

static Metric_state metric_global_contains_one_exact_link_state(
	extractor_t *pex, Metric_link_id id)
{
	return metric_link_class(pex, id)->global_contains_one_state;
}

static Metric_state metric_global_contains_one_exact_choice_state(
	extractor_t *pex, Parse_choice *choice, const Parse_set *set)
{
	Metric_state state = 0;

	for (int side = 0; side < 2; side++)
		state |= metric_global_contains_one_exact_link_state(
			pex, choice_link_id(pex, choice, set, side));

	return state;
}

static size_t metric_global_contains_one_exact_root_bad_index(
	const extractor_t *pex, Metric_state state)
{
	size_t count = metric_global_contains_one_exact_count(pex);

	for (size_t slot = 0; slot < count; slot++)
	{
		Metric_state selector =
			metric_global_contains_one_exact_selector_state(
				pex, slot);
		Metric_state criterion =
			metric_global_contains_one_exact_criterion_state(
				pex, slot);

		if ((state & selector) && !(state & criterion))
			return metric_global_contains_one_exact_rule_idx(
				pex, slot);
	}

	return (size_t)-1;
}

static bool metric_global_contains_one_exact_root_state_allowed(
	const extractor_t *pex, Metric_state state)
{
	return (size_t)-1 ==
	       metric_global_contains_one_exact_root_bad_index(pex, state);
}

static void metric_pp_predict_global_contains_one(
	extractor_t *pex, Metric_state state)
{
	size_t idx;

	if (!pex->metric.pp.validate_enabled) return;

	idx = metric_global_contains_one_exact_root_bad_index(pex, state);
	if ((size_t)-1 == idx) return;

	metric_pp_predict(
		pex, PP_FAILURE_CONTAINS_ONE_GLOBAL,
		metric_global_contains_one_message(pex, idx));
}

static size_t metric_global_contains_one_mask_bad_index(uint64_t selector,
                                                        uint64_t criterion,
                                                        uint64_t active,
                                                        size_t active_count)
{
	uint64_t missing = (selector & active) & ~(criterion & active);

	for (size_t i = 0; i < active_count; i++)
		if (missing & metric_global_contains_one_bit(i))
			return i;

	return (size_t)-1;
}

static void metric_choice_link_words(Parse_choice *choice,
                                     const Parse_set *set, int side,
                                     WordIdx *lw, WordIdx *rw)
{
	if (0 == side)
	{
		*lw = choice->set[0]->lw;
		*rw = choice->set[0]->rw;
	}
	else
	{
		*lw = choice->set[1]->lw;
		*rw = choice->set[1]->rw;
	}

	if (*rw < *lw)
	{
		WordIdx tmp = *lw;
		*lw = *rw;
		*rw = tmp;
	}
	(void)set;
}

static bool metric_bounded_feedback_matches_choice(
	const Metric_bounded_domain_feedback *feedback,
	const Parse_choice *choice, const Parse_set *set, int side,
	const char *link_name)
{
	return (feedback->set == set) &&
	       (feedback->l_id == choice->l_id) &&
	       (feedback->r_id == choice->r_id) &&
	       (feedback->side == side) &&
	       (0 == strcmp(feedback->link_name, link_name));
}

static uint8_t metric_bounded_vertex_bit(uint8_t vertex)
{
	return (uint8_t)1 << vertex;
}

static void metric_bounded_add_edge(Metric_bounded_domain_graph *graph,
                                    uint8_t from, uint8_t to)
{
	graph->edge[from] |= metric_bounded_vertex_bit(to);
}

static uint8_t metric_bounded_reachable(
	const Metric_bounded_domain_graph *graph, uint8_t source)
{
	uint8_t reach = source;
	bool changed;

	do
	{
		changed = false;
		for (uint8_t v = 0; v < 3; v++)
		{
			uint8_t next;

			if (!(reach & metric_bounded_vertex_bit(v))) continue;
			next = reach | graph->edge[v];
			if (next == reach) continue;

			reach = next;
			changed = true;
		}
	} while (changed);

	return reach;
}

static bool metric_domain_can_traverse(extractor_t *pex, WordIdx root,
                                       WordIdx from, WordIdx to,
                                       const char *name)
{
	if ((from == root) || (to == root))
		return false;

	/* This mirrors the pruning condition in depth_first_search():
	 * a restricted link may be put in the domain, but it is not traced
	 * further when it points back to a lower word before the root. */
	if ((to < root) && (to < from) &&
	    post_process_link_restricted(pex->postprocessor, name))
		return false;

	return true;
}

static bool metric_bounded_can_traverse(
	extractor_t *pex, const Metric_bounded_domain_feedback *feedback,
	WordIdx from, WordIdx to, const char *name)
{
	return metric_domain_can_traverse(pex, feedback->root, from, to,
	                                  name);
}

static void metric_bounded_add_child_state(
	Metric_bounded_domain_graph *graph, uint8_t state,
	uint8_t left, uint8_t right)
{
	if (state & METRIC_BOUNDED_REACH_LEFT)
		graph->source |= metric_bounded_vertex_bit(left);
	if (state & METRIC_BOUNDED_REACH_RIGHT)
		graph->source |= metric_bounded_vertex_bit(right);
	if (state & METRIC_BOUNDED_REL_LR)
		metric_bounded_add_edge(graph, left, right);
	if (state & METRIC_BOUNDED_REL_RL)
		metric_bounded_add_edge(graph, right, left);
}

static bool metric_bounded_local_link_bad(
	const Metric_bounded_domain_feedback *feedback,
	WordIdx lw, WordIdx rw, uint8_t high_vertex, uint8_t reach)
{
	/* BOUNDED_RULES require every link in an s/r domain to stay on or
	 * after the domain root.  Once the learned domain start can reach a
	 * local link whose left word is before the root, the candidate would
	 * reproduce the same PP-bounded violation. */
	if (lw >= feedback->root) return false;
	if (rw == feedback->root) return false;

	return 0 != (reach & metric_bounded_vertex_bit(high_vertex));
}

static bool metric_bounded_choice_summary(
	extractor_t *pex, const Metric_bounded_domain_feedback *feedback,
	Parse_choice *choice, const Parse_set *set,
	const Metric_candidate *left_child, const Metric_candidate *right_child,
	size_t feedback_idx, uint8_t *parent_state)
{
	/* Reconstruct only the reachability facts needed by one learned
	 * bounded-domain failure.  The three vertices are the left terminal,
	 * the middle word where this Parse_choice joins its children, and
	 * the right terminal. */
	typedef struct
	{
		WordIdx lw;
		WordIdx rw;
		uint8_t left;
		uint8_t right;
		bool active;
		bool start;
	} Local_link;

	Metric_bounded_domain_graph graph = { 0 };
	Local_link link[2] = { 0 };
	uint8_t left_state = (NULL == left_child) ? 0 :
		left_child->bounded_domain_state[feedback_idx];
	uint8_t right_state = (NULL == right_child) ? 0 :
		right_child->bounded_domain_state[feedback_idx];

	metric_bounded_add_child_state(
		&graph, left_state,
		METRIC_MFC_NODE_LEFT, METRIC_MFC_NODE_MIDDLE);
	metric_bounded_add_child_state(
		&graph, right_state,
		METRIC_MFC_NODE_MIDDLE, METRIC_MFC_NODE_RIGHT);

	for (int side = 0; side < 2; side++)
	{
		const char *name = choice_link_name(pex, choice, set, side);
		WordIdx lw, rw;
		uint8_t left = (0 == side) ?
			METRIC_MFC_NODE_LEFT : METRIC_MFC_NODE_MIDDLE;
		uint8_t right = (0 == side) ?
			METRIC_MFC_NODE_MIDDLE : METRIC_MFC_NODE_RIGHT;

		if (NULL == name) continue;
		if (choice_link_ignored(pex, choice, side, name)) continue;

		metric_choice_link_words(choice, set, side, &lw, &rw);
		link[side] = (Local_link){
			.lw = lw,
			.rw = rw,
			.left = left,
			.right = right,
			.active = true,
			.start = metric_bounded_feedback_matches_choice(
				feedback, choice, set, side, name)
		};

		if (link[side].start)
		{
			/* The PP domain search starts at the non-root endpoint
			 * of the recorded start link.  Seed the compact
			 * three-node summary at the corresponding boundary. */
			if (feedback->seed == lw)
				graph.source |= metric_bounded_vertex_bit(left);
			if (feedback->seed == rw)
				graph.source |= metric_bounded_vertex_bit(right);
			continue;
		}

		if (metric_bounded_can_traverse(pex, feedback, lw, rw, name))
			metric_bounded_add_edge(&graph, left, right);
		if (metric_bounded_can_traverse(pex, feedback, rw, lw, name))
			metric_bounded_add_edge(&graph, right, left);
	}

	uint8_t reach = metric_bounded_reachable(&graph, graph.source);
	for (int side = 0; side < 2; side++)
	{
		if (!link[side].active || link[side].start) continue;
		if (metric_bounded_local_link_bad(
		    feedback, link[side].lw, link[side].rw, link[side].right,
		    reach))
		{
			pex->metric.pp.bounded.rejected++;
			return false;
		}
	}

	uint8_t state = 0;
	if (reach & metric_bounded_vertex_bit(METRIC_MFC_NODE_LEFT))
		state |= METRIC_BOUNDED_REACH_LEFT;
	if (reach & metric_bounded_vertex_bit(METRIC_MFC_NODE_RIGHT))
		state |= METRIC_BOUNDED_REACH_RIGHT;

	if (metric_bounded_reachable(
	    &graph, metric_bounded_vertex_bit(METRIC_MFC_NODE_LEFT)) &
	    metric_bounded_vertex_bit(METRIC_MFC_NODE_RIGHT))
		state |= METRIC_BOUNDED_REL_LR;
	if (metric_bounded_reachable(
	    &graph, metric_bounded_vertex_bit(METRIC_MFC_NODE_RIGHT)) &
	    metric_bounded_vertex_bit(METRIC_MFC_NODE_LEFT))
		state |= METRIC_BOUNDED_REL_RL;

	*parent_state = state;
	return true;
}

static bool metric_candidate_bounded_summary(
	extractor_t *pex, Parse_choice *choice, const Parse_set *set,
	Metric_candidate *child[2],
	uint8_t state[METRIC_BOUNDED_DOMAIN_MAX_MARKS])
{
	size_t count = metric_bounded_domain_active_count(pex);

	if (0 == count) return true;
	memset(state, 0, METRIC_BOUNDED_DOMAIN_MAX_MARKS * sizeof(*state));
	for (size_t i = 0; i < count; i++)
	{
		if (!metric_bounded_choice_summary(
		    pex, &pex->metric.pp.bounded.feedbacks[i], choice, set,
		    child[0], child[1], i, &state[i]))
			return false;
	}
	return true;
}

typedef struct
{
	const char *name;
	Metric_link_id id;
	WordIdx lw;
	WordIdx rw;
	PP_domain_kind domain_kind;
	uint64_t global_contains_one_selector;
	uint64_t global_contains_one_criterion;
	uint64_t parse_contains_one_selector;
	uint64_t parse_contains_one_criterion;
	uint64_t parse_contains_none_selector;
	uint64_t parse_contains_none_forbidden;
	bool ignored;
	bool restricted;
	bool domain_contains_start;
} Metric_domain_link;

typedef struct
{
	size_t start_link;
	size_t size;
	bool *raw_links;
} Metric_domain_summary;

typedef struct
{
	size_t link_idx;
	WordIdx from;
	WordIdx to;
	size_t next;
} Metric_domain_graph_edge;

typedef struct
{
	size_t *head;
	Metric_domain_graph_edge *edge;
	size_t num_edges;
	size_t num_words;
} Metric_domain_graph;

static size_t metric_domain_link_capacity(const extractor_t *pex)
{
	/* Match the Linkage link_array capacity used by free_linkages().
	 * This is still sentence-bounded, so alloca() scratch stays small. */
	return 2 * ((size_t)pex->parse_set->rw + 1);
}

#define METRIC_DOMAIN_GRAPH_NONE ((size_t)-1)

static void metric_domain_graph_add_edge(Metric_domain_graph *graph,
                                         size_t link_idx,
                                         WordIdx from, WordIdx to)
{
	size_t edge_idx = graph->num_edges++;

	graph->edge[edge_idx] = (Metric_domain_graph_edge){
		.link_idx = link_idx,
		.from = from,
		.to = to,
		.next = graph->head[from]
	};
	graph->head[from] = edge_idx;
}

/* Build a word adjacency view of the candidate links once per exact domain
 * replay.  PP's raw domain search is a graph walk; using adjacency avoids
 * repeatedly scanning every link until the visited set stops changing. */
static void metric_domain_graph_build(
	const Metric_domain_link *links, size_t num_links,
	Metric_domain_graph *graph, size_t *head,
	Metric_domain_graph_edge *edge, size_t num_words)
{
	graph->head = head;
	graph->edge = edge;
	graph->num_edges = 0;
	graph->num_words = num_words;

	for (size_t i = 0; i < num_words; i++)
		graph->head[i] = METRIC_DOMAIN_GRAPH_NONE;

	for (size_t i = 0; i < num_links; i++)
	{
		const Metric_domain_link *link = &links[i];

		if (link->ignored || (NULL == link->name)) continue;
		if ((num_words <= link->lw) || (num_words <= link->rw))
			continue;

		metric_domain_graph_add_edge(graph, i, link->lw, link->rw);
		metric_domain_graph_add_edge(graph, i, link->rw, link->lw);
	}
}

static bool metric_collect_candidate_domain_links(
	extractor_t *pex, Metric_ranker *ranker, Parse_set *set,
	Metric_candidate *candidate, Metric_domain_link *links,
	size_t *num_links, size_t max_links)
{
	if ((NULL == set) || (NULL == candidate)) return true;

	Parse_choice *pc = candidate->choice;
	for (int side = 0; side < 2; side++)
	{
		Metric_link_id id = choice_link_id(pex, pc, set, side);
		const Metric_link_class *cls = metric_link_class(pex, id);
		const char *name = cls->name;
		if (NULL == name) continue;

		if (*num_links == max_links) return false;
		Metric_domain_link *link = &links[(*num_links)++];

		link->name = name;
		link->id = id;
		link->domain_kind = cls->domain_kind;
		link->global_contains_one_selector =
			cls->global_contains_one_selector;
		link->global_contains_one_criterion =
			cls->global_contains_one_criterion;
		link->parse_contains_one_selector =
			cls->parse_contains_one_selector;
		link->parse_contains_one_criterion =
			cls->parse_contains_one_criterion;
		link->parse_contains_none_selector =
			cls->parse_contains_none_selector;
		link->parse_contains_none_forbidden =
			cls->parse_contains_none_forbidden;
		link->ignored = 0 != (cls->flags & METRIC_LINK_IGNORED);
		link->restricted = 0 != (cls->flags & METRIC_LINK_RESTRICTED);
		link->domain_contains_start =
			0 != (cls->flags & METRIC_LINK_DOMAIN_CONTAINS_START);
		metric_choice_link_words(pc, set, side, &link->lw, &link->rw);
	}

	for (int side = 0; side < 2; side++)
	{
		Metric_candidate *child =
			metric_get_state_rank(pex, ranker, pc->set[side],
			                      candidate->child_state[side],
			                      candidate->rank[side]);
		if (!metric_collect_candidate_domain_links(
		    pex, ranker, pc->set[side], child, links, num_links,
		    max_links))
			return false;
	}

	return true;
}

static bool metric_candidate_global_contains_one_masks(
	extractor_t *pex, Metric_ranker *ranker, Parse_set *set,
	Metric_candidate *candidate, uint64_t *selector, uint64_t *criterion)
{
	if ((NULL == set) || (NULL == candidate)) return true;

	Parse_choice *pc = candidate->choice;
	for (int side = 0; side < 2; side++)
	{
		Metric_link_id id = choice_link_id(pex, pc, set, side);
		const Metric_link_class *cls = metric_link_class(pex, id);

		*selector |= cls->global_contains_one_selector;
		*criterion |= cls->global_contains_one_criterion;
	}

	for (int side = 0; side < 2; side++)
	{
		Metric_candidate *child =
			metric_get_state_rank(pex, ranker, pc->set[side],
			                      candidate->child_state[side],
			                      candidate->rank[side]);
		if (!metric_candidate_global_contains_one_masks(
		    pex, ranker, pc->set[side], child, selector, criterion))
			return false;
	}

	return true;
}

static void metric_global_contains_one_note_fallback_reject(
	extractor_t *pex, size_t rule_idx)
{
	if (METRIC_GLOBAL_CONTAINS_ONE_MAX_MARKS <= rule_idx) return;

	size_t rejects =
		++pex->metric.pp.global.fallback_rejects[rule_idx];
	if (rejects != METRIC_GLOBAL_CONTAINS_ONE_PROMOTE_REJECTS)
		return;

	pex->metric.pp.global.exact_ready = false;
	if (metric_global_contains_one_select_exact_slots(pex))
	{
		pex->metric.pp.global.promotions++;
		metric_ranker_reset(pex);
		metric_trace_report(pex, "promote-global", true);
	}
}

/* CONTAINS_ONE_GLOBAL rules are sentence-wide.  Dynamically selected rules
 * are represented in exact metric state, so impossible root states are never
 * generated.  Active rules outside that budget use this lightweight root
 * candidate mask scan, and can promote themselves if they become hot. */
static bool metric_candidate_global_contains_one_ok(
	extractor_t *pex, Metric_ranker *ranker, Parse_set *set,
	Metric_candidate *candidate)
{
	uint64_t fallback =
		pex->metric.pp.global.active_selector_mask &
		~pex->metric.pp.global.exact_rule_mask;

	if ((0 == fallback) || (set != pex->parse_set)) return true;

	uint64_t selector = 0;
	uint64_t criterion = 0;

	if (!metric_candidate_global_contains_one_masks(
	    pex, ranker, set, candidate, &selector, &criterion))
		return true;

	size_t bad_idx = metric_global_contains_one_mask_bad_index(
		selector, criterion, fallback,
		metric_global_contains_one_active_count(pex));
	if ((size_t)-1 != bad_idx)
	{
		metric_pp_predict(
			pex, PP_FAILURE_CONTAINS_ONE_GLOBAL,
			metric_global_contains_one_message(pex, bad_idx));
		if (pex->metric.pp.validate_enabled) return true;

		pex->metric.pp.global.rejected++;
		metric_global_contains_one_note_fallback_reject(pex, bad_idx);
		return false;
	}

	return true;
}

static const char *metric_domain_kind_name(PP_domain_kind kind)
{
	switch (kind)
	{
	case PP_DOMAIN_NONE: return "none";
	case PP_DOMAIN_REGULAR: return "regular";
	case PP_DOMAIN_URFL: return "urfl";
	case PP_DOMAIN_URFL_ONLY: return "urfl-only";
	case PP_DOMAIN_LEFT: return "left";
	}
	return "unknown";
}

static bool metric_domain_restricted_blocks(WordIdx root, WordIdx from,
                                            WordIdx to, bool restricted)
{
	return (to < root) && (to < from) && restricted;
}

static bool metric_domain_should_add(PP_domain_kind kind, size_t link_idx,
                                     size_t start_link, WordIdx root,
                                     WordIdx from, WordIdx to)
{
	/* PP domains contain only links reached while walking toward the
	 * left, with special handling for the domain start link and URFL
	 * variants. */
	if ((to >= from) || (link_idx == start_link)) return false;

	switch (kind)
	{
	case PP_DOMAIN_URFL:
	case PP_DOMAIN_URFL_ONLY:
		return from != root;
	case PP_DOMAIN_REGULAR:
	case PP_DOMAIN_LEFT:
		return true;
	case PP_DOMAIN_NONE:
		return false;
	}
	return false;
}

static bool metric_domain_should_traverse(PP_domain_kind kind,
                                          WordIdx start_word,
                                          WordIdx root, WordIdx right,
                                          WordIdx from, WordIdx to,
                                          bool restricted)
{
	/* Keep this switch aligned with the PP domain DFS variants.  A mark
	 * is learned only after this replay exactly matches PP on the
	 * teaching linkage, so unsupported shapes are safer as no-feedback
	 * than as approximate pruning. */
	switch (kind)
	{
	case PP_DOMAIN_REGULAR:
		if (to == root) return false;
		return !metric_domain_restricted_blocks(
			root, from, to, restricted);
	case PP_DOMAIN_URFL:
		if ((from == root) && (to < from)) return false;
		return !metric_domain_restricted_blocks(
			root, from, to, restricted);
	case PP_DOMAIN_URFL_ONLY:
		if ((from == root) && (to >= right)) return false;
		if ((from == root) && (to < root)) return false;
		return !metric_domain_restricted_blocks(
			root, from, to, restricted);
	case PP_DOMAIN_LEFT:
		if (from == start_word) return to != right;
		if (to == right) return false;
		return !metric_domain_restricted_blocks(
			right, from, to, restricted);
	case PP_DOMAIN_NONE:
		return false;
	}
	return false;
}

static bool metric_domain_raw_flags_graph(
	const Metric_domain_link *links, size_t num_links,
	const Metric_domain_graph *graph, size_t start_link,
	PP_domain_kind kind, bool *raw_links, size_t *domain_size)
{
	const Metric_domain_link *start = &links[start_link];
	WordIdx root = start->lw;
	WordIdx right = start->rw;
	WordIdx start_word = start->rw;
	bool *visited;
	WordIdx *queue;
	size_t qread = 0;
	size_t qwrite = 0;

	memset(raw_links, 0, num_links * sizeof(*raw_links));
	*domain_size = 0;

	if (start->ignored || (NULL == start->name)) return false;
	if (PP_DOMAIN_NONE == kind) return false;

	if ((PP_DOMAIN_URFL_ONLY == kind) || (PP_DOMAIN_LEFT == kind))
		start_word = start->lw;
	if ((graph->num_words <= start->lw) || (graph->num_words <= start->rw))
		return false;

	if ((PP_DOMAIN_URFL == kind) ||
	    ((PP_DOMAIN_REGULAR == kind) &&
	     start->domain_contains_start))
	{
		raw_links[start_link] = true;
		(*domain_size)++;
	}

	visited = alloca(graph->num_words * sizeof(*visited));
	queue = alloca(graph->num_words * sizeof(*queue));
	memset(visited, 0, graph->num_words * sizeof(*visited));
	visited[start_word] = true;
	queue[qwrite++] = start_word;

	while (qread < qwrite)
	{
		WordIdx from = queue[qread++];

		for (size_t edge_idx = graph->head[from];
		     METRIC_DOMAIN_GRAPH_NONE != edge_idx;
		     edge_idx = graph->edge[edge_idx].next)
		{
			const Metric_domain_graph_edge *edge =
				&graph->edge[edge_idx];
			size_t link_idx = edge->link_idx;
			const Metric_domain_link *link = &links[link_idx];
			WordIdx to = edge->to;

			if (!raw_links[link_idx] &&
			    metric_domain_should_add(kind, link_idx, start_link,
			                             root, from, to))
			{
				raw_links[link_idx] = true;
				(*domain_size)++;
			}

			if (!visited[to] &&
			    metric_domain_should_traverse(
			    kind, start_word, root, right, from, to,
			    link->restricted))
			{
				visited[to] = true;
				queue[qwrite++] = to;
			}
		}
	}

	return true;
}

static bool metric_domain_before(const Metric_domain_summary *domains,
                                 size_t a, size_t b)
{
	return (domains[a].size != domains[b].size) ?
		(domains[a].size < domains[b].size) :
		(domains[a].start_link < domains[b].start_link);
}

static void metric_sort_domain_order(const Metric_domain_summary *domains,
                                     size_t *order, size_t num_domains)
{
	for (size_t i = 0; i < num_domains; i++)
		order[i] = i;

	for (size_t i = 1; i < num_domains; i++)
	{
		size_t current = order[i];
		size_t j = i;

		while ((0 < j) &&
		       metric_domain_before(domains, current, order[j - 1]))
		{
			order[j] = order[j - 1];
			j--;
		}
		order[j] = current;
	}
}

static bool metric_pp_constraints_enabled(extractor_t *pex)
{
	return pex->metric.pp.constraints_enabled;
}

static size_t metric_parse_constraint_bad_index(uint64_t selector,
                                                uint64_t criterion,
                                                uint64_t active_mask)
{
	uint64_t missing = (selector & active_mask) & ~(criterion & active_mask);

	for (size_t i = 0; i < 8 * sizeof(uint64_t); i++)
		if (missing & ((uint64_t)1 << i))
			return i;

	return (size_t)-1;
}

static size_t metric_parse_contains_none_bad_index(uint64_t selector,
                                                   uint64_t forbidden,
                                                   uint64_t active_mask)
{
	uint64_t present = selector & forbidden & active_mask;

	for (size_t i = 0; i < 8 * sizeof(uint64_t); i++)
		if (present & ((uint64_t)1 << i))
			return i;

	return (size_t)-1;
}

typedef struct
{
	/* First knowledge-declared PP row that this candidate would violate.
	 * The type selects the PARSE_CONTAINS_* rule array for `rule`. */
	PP_failure_type type;
	size_t rule;
} Metric_parse_constraint_failure;

static void metric_parse_constraint_failure_clear(
	Metric_parse_constraint_failure *failure)
{
	failure->type = PP_FAILURE_NONE;
	failure->rule = (size_t)-1;
}

static bool metric_pp_constraints_check_domains(
	extractor_t *pex, const Metric_domain_link *links, size_t num_links,
	Metric_parse_constraint_failure *failure)
{
	size_t num_words = (size_t)pex->parse_set->rw + 1;
	size_t num_domains = 0;
	Metric_domain_graph graph;
	size_t *graph_head;
	Metric_domain_graph_edge *graph_edge;
	Metric_domain_summary *domains;
	size_t *order;
	bool *raw_matrix;
	uint64_t active_contains_one =
		post_process_parse_contains_one_active_mask(pex->postprocessor);
	uint64_t active_contains_none =
		post_process_parse_contains_none_active_mask(pex->postprocessor);

	for (size_t i = 0; i < num_links; i++)
	{
		if (!links[i].ignored && (PP_DOMAIN_NONE != links[i].domain_kind))
			num_domains++;
	}

	if (0 == num_domains) return true;

	graph_head = alloca(num_words * sizeof(*graph_head));
	graph_edge = alloca(MAX((size_t)1, 2 * num_links) *
	                    sizeof(*graph_edge));
	metric_domain_graph_build(
		links, num_links, &graph, graph_head, graph_edge, num_words);

	domains = alloca(num_domains * sizeof(*domains));
	order = alloca(num_domains * sizeof(*order));
	raw_matrix = alloca(num_domains * num_links * sizeof(*raw_matrix));

	num_domains = 0;
	for (size_t i = 0; i < num_links; i++)
	{
		if (links[i].ignored || (PP_DOMAIN_NONE == links[i].domain_kind))
			continue;

		bool *raw_links = &raw_matrix[num_domains * num_links];
		size_t domain_size;

		if (!metric_domain_raw_flags_graph(
		    links, num_links, &graph, i, links[i].domain_kind,
		    raw_links, &domain_size))
			continue;

		domains[num_domains++] = (Metric_domain_summary){
			.start_link = i,
			.size = domain_size,
			.raw_links = raw_links
		};
	}
	if (0 == num_domains) return true;

	if ((0 == active_contains_one) && (0 == active_contains_none))
		return true;
	metric_sort_domain_order(domains, order, num_domains);

	uint64_t *one_selector = alloca(num_domains * sizeof(*one_selector));
	uint64_t *one_criterion = alloca(num_domains * sizeof(*one_criterion));
	uint64_t *none_selector = alloca(num_domains * sizeof(*none_selector));
	uint64_t *none_forbidden =
		alloca(num_domains * sizeof(*none_forbidden));
	memset(one_selector, 0, num_domains * sizeof(*one_selector));
	memset(one_criterion, 0, num_domains * sizeof(*one_criterion));
	memset(none_selector, 0, num_domains * sizeof(*none_selector));
	memset(none_forbidden, 0, num_domains * sizeof(*none_forbidden));

	/* Match build_domain_forest(): each link belongs to the first sorted
	 * raw domain that contains it.  CONTAINS_ONE and CONTAINS_NONE see
	 * that child-link set, not every raw domain that could reach the link. */
	for (size_t link = 0; link < num_links; link++)
	{
		for (size_t idx = 0; idx < num_domains; idx++)
		{
			size_t domain = order[idx];

			if (!domains[domain].raw_links[link]) continue;
			one_selector[domain] |=
				links[link].parse_contains_one_selector;
			one_criterion[domain] |=
				links[link].parse_contains_one_criterion;
			none_selector[domain] |=
				links[link].parse_contains_none_selector;
			none_forbidden[domain] |=
				links[link].parse_contains_none_forbidden;
			break;
		}
	}

	/* Classic PP runs all active CONTAINS_ONE rows before CONTAINS_NONE,
	 * so keep the same priority for validation diagnostics. */
	for (size_t d = 0; d < num_domains; d++)
	{
		size_t bad = metric_parse_constraint_bad_index(
			one_selector[d], one_criterion[d], active_contains_one);
		if ((size_t)-1 != bad)
		{
			if (NULL != failure)
			{
				failure->type = PP_FAILURE_CONTAINS_ONE;
				failure->rule = bad;
			}
			return false;
		}
	}

	for (size_t d = 0; d < num_domains; d++)
	{
		size_t bad = metric_parse_contains_none_bad_index(
			none_selector[d], none_forbidden[d],
			active_contains_none);
		if ((size_t)-1 != bad)
		{
			if (NULL != failure)
			{
				failure->type = PP_FAILURE_CONTAINS_NONE;
				failure->rule = bad;
			}
			return false;
		}
	}

	return true;
}

static bool metric_pp_constraints_candidate_ok(
	extractor_t *pex, Metric_ranker *ranker, Metric_candidate *candidate)
{
	if (!metric_pp_constraints_enabled(pex)) return true;
	if (!candidate->parse_constraint_relevant) return true;

	size_t max_links = metric_domain_link_capacity(pex);
	Metric_domain_link *links = alloca(max_links * sizeof(*links));
	size_t num_links = 0;
	Metric_parse_constraint_failure failure;
	metric_parse_constraint_failure_clear(&failure);

	if (!metric_collect_candidate_domain_links(
	    pex, ranker, pex->parse_set, candidate, links, &num_links, max_links))
		return true;

	if (metric_pp_constraints_check_domains(
	    pex, links, num_links, &failure))
		return true;

	if (PP_FAILURE_CONTAINS_ONE == failure.type)
		metric_pp_predict(
			pex, failure.type,
			post_process_parse_contains_one_rule_message(
				pex->postprocessor, failure.rule));
	else
		metric_pp_predict(
			pex, failure.type,
			post_process_parse_contains_none_rule_message(
				pex->postprocessor, failure.rule));
	if (pex->metric.pp.validate_enabled) return true;

	pex->metric.pp.parse_constraint_rejected++;
	if ((0 != pex->metric.pp.parse_constraint_reject_limit) &&
	    (pex->metric.pp.parse_constraint_reject_limit <=
	     pex->metric.pp.parse_constraint_rejected))
		pex->metric.pp.parse_constraint_reject_limit_hit = true;
	metric_trace_report(pex, "reject-parse-constraint", false);
	return false;
}

static bool metric_exact_choice_state(extractor_t *pex, Parse_choice *choice,
                                      const Parse_set *set,
                                      Metric_state left_state,
                                      Metric_state right_state,
                                      Metric_state *parent_state)
{
	Metric_state state = 0;

	/* Exact state modes are conjunctive: every enabled exact checker must
	 * agree on the parent state for this child-state combination. */
	if (metric_mfc_terminal_exact_enabled(pex))
	{
		Metric_state part;

		if (!metric_mfc_terminal_choice_state(
		    pex, choice, set, metric_state_mfc_part(pex, left_state),
		    metric_state_mfc_part(pex, right_state), &part))
			return false;
		metric_state_set_mfc_part(pex, &state, part);
	}

	state |= metric_state_global_contains_one_part(pex, left_state);
	state |= metric_state_global_contains_one_part(pex, right_state);
	state |= metric_global_contains_one_exact_choice_state(pex, choice, set);
	*parent_state = state;
	return true;
}

static bool metric_root_state_allowed(const extractor_t *pex,
                                      const Metric_ranker *ranker,
                                      Metric_state state)
{
	(void)ranker;

	if (pex->metric.pp.mfc_enabled &&
	    !metric_mfc_terminal_root_state_allowed(
		    metric_state_mfc_part(pex, state)))
		return false;
	if (!pex->metric.pp.validate_enabled &&
	    !metric_global_contains_one_exact_root_state_allowed(pex, state))
		return false;

	return true;
}

static void metric_ranker_init_root_states(extractor_t *pex,
                                           Metric_ranker *ranker)
{
	ranker->root_states = malloc(ranker->state_count *
	                             sizeof(*ranker->root_states));
	assert(NULL != ranker->root_states,
	       "Out of memory allocating metric root states");

	for (Metric_state state = 0; state < ranker->state_count; state++)
	{
		if (!metric_root_state_allowed(pex, ranker, state))
			continue;
		ranker->root_states[ranker->num_root_states++] = state;
	}
}

static bool metric_possible_state_enabled(const Metric_ranker *ranker)
{
	return (NULL != ranker) && (1 < ranker->state_count);
}

static Metric_state parse_set_possible_metric_state(extractor_t *pex,
                                                    Metric_ranker *ranker,
                                                    Parse_set *set)
{
	if (!metric_possible_state_enabled(ranker)) return 0;
	if ((NULL == set) || (NULL == set->first))
		return metric_exact_state_enabled(pex) ?
		       metric_exact_state_mask(0) : 0;

	/* Possible-state masks are ranker-local pruning caches.  Keep them in
	 * the optional ranker cache rather than adding permanent fields to every
	 * Parse_set node used by ordinary extraction. */
	Metric_set_cache *cache = metric_get_set_cache(pex, ranker, set);
	if (cache->possible_state_done)
		return cache->possible_state;

	/* This over-approximation is a pruning cache, not a source of
	 * ordering.  Exact states use one bit per concrete state. */
	Metric_state possible = 0;
	if (metric_exact_state_enabled(pex))
	{
		for (Parse_choice *pc = set->first; pc != NULL; pc = pc->next)
		{
			Metric_state left_possible =
				parse_set_possible_metric_state(
					pex, ranker, pc->set[0]);
			Metric_state right_possible =
				parse_set_possible_metric_state(
					pex, ranker, pc->set[1]);
			size_t state_count = metric_exact_state_count(pex);

			for (Metric_state left_state = 0;
			     left_state < state_count; left_state++)
			{
				if (!(left_possible &
				      metric_exact_state_mask(left_state)))
					continue;

				for (Metric_state right_state = 0;
				     right_state < state_count;
				     right_state++)
				{
					Metric_state parent_state;

					if (!(right_possible &
					      metric_exact_state_mask(right_state)))
						continue;
					if (!metric_exact_choice_state(
					    pex, pc, set, left_state, right_state,
					    &parent_state))
						continue;

					possible |=
						metric_exact_state_mask(
							parent_state);
				}
			}
		}
	}

	cache->possible_state = possible;
	cache->possible_state_done = true;
	return possible;
}

static bool metric_set_state_possible(extractor_t *pex,
                                      Metric_ranker *ranker,
                                      Parse_set *set,
                                      Metric_state state)
{
	if (!metric_possible_state_enabled(ranker)) return true;

	Metric_state possible =
		parse_set_possible_metric_state(pex, ranker, set);
	if (metric_exact_state_enabled(pex))
		return 0 != (possible & metric_exact_state_mask(state));

	return 0 == (state & ~possible);
}

static bool metric_compute_candidate_metric(extractor_t *pex,
                                            Metric_ranker *ranker,
                                            Parse_choice *choice,
                                            const Parse_set *set,
                                            Metric_state local_state,
                                            const Metric_state child_state[2],
                                            const size_t child_rank[2],
                                            Parse_metric *metric,
                                            Metric_state *actual_state,
                                            Metric_candidate *child_candidate[2])
{
	/* Candidate metrics are additive over the Parse_choice tree.  The
	 * child candidates are fetched by rank, so this also validates that
	 * the requested child-rank combination exists. */
	*metric = choice_local_metric(choice, set);
	if (NULL != actual_state) *actual_state = local_state;
	if (NULL != child_candidate)
	{
		child_candidate[0] = NULL;
		child_candidate[1] = NULL;
	}

	for (int side = 0; side < 2; side++)
	{
		Parse_set *child_set = choice->set[side];
		Parse_metric child_metric;

		if ((NULL == child_set) || (NULL == child_set->first))
		{
			if ((0 != child_state[side]) ||
			    (0 != child_rank[side]))
				return false;
			child_metric = (Parse_metric){ 0 };
		}
		else
		{
			Metric_candidate *child = metric_get_state_rank(
				pex, ranker, child_set, child_state[side],
				child_rank[side]);
			if (NULL == child) return false;
			if (NULL != child_candidate)
				child_candidate[side] = child;
			if (NULL != actual_state)
				*actual_state |= child->state;
			child_metric = child->metric;
		}
		*metric = metric_add(*metric, child_metric);
	}

	return true;
}

static bool metric_push_state_candidate(extractor_t *pex,
                                        Metric_ranker *ranker,
                                        Metric_state_stream *stream,
                                        Parse_set *set, Parse_choice *choice,
                                        Metric_state left_state,
                                        size_t left_rank,
                                        Metric_state right_state,
                                        size_t right_rank)
{
	Parse_metric metric;
	Metric_state requested_state;
	Metric_candidate *child_candidate[2];
	uint8_t bounded_state[METRIC_BOUNDED_DOMAIN_MAX_MARKS] = { 0 };
	bool bounded_active = 0 < metric_bounded_domain_active_count(pex);
	bool bounded_rejected = false;

	requested_state = left_state | right_state;
	if (requested_state != stream->state) return false;

	Metric_state child_state[2] = { left_state, right_state };
	size_t child_rank[2] = { left_rank, right_rank };

	if (!metric_compute_candidate_metric(
	    pex, ranker, choice, set, 0, child_state, child_rank,
	    &metric, NULL, child_candidate))
		return false;

	if (bounded_active && !metric_candidate_bounded_summary(
	    pex, choice, set, child_candidate, bounded_state))
		bounded_rejected = true;

	Metric_candidate *candidate = pool_alloc(ranker->candidate_pool);
	candidate->ranker = ranker;
	candidate->choice = choice;
	candidate->rank[0] = left_rank;
	candidate->rank[1] = right_rank;
	candidate->state = requested_state;
	candidate->child_state[0] = left_state;
	candidate->child_state[1] = right_state;
	candidate->metric = metric;
	if (bounded_active)
		memcpy(candidate->bounded_domain_state, bounded_state,
		       sizeof(candidate->bounded_domain_state));
	candidate->serial = pex->metric.serial++;
	candidate->bounded_domain_rejected = bounded_rejected;
	candidate->parse_constraint_relevant =
		metric_choice_parse_constraint_relevant(pex, choice, set) ||
		((NULL != child_candidate[0]) &&
		 child_candidate[0]->parse_constraint_relevant) ||
		((NULL != child_candidate[1]) &&
		 child_candidate[1]->parse_constraint_relevant);

	metric_heap_push(stream->heap, candidate);
	pex->metric.trace.state_assignments_pushed++;
	return true;
}

static bool metric_push_exact_candidate(extractor_t *pex,
                                        Metric_ranker *ranker,
                                        Metric_state_stream *stream,
                                        Parse_set *set,
                                        Parse_choice *choice,
                                        Metric_state left_state,
                                        size_t left_rank,
                                        Metric_state right_state,
                                        size_t right_rank,
                                        Metric_state parent_state)
{
	Parse_metric metric;
	Metric_state computed_state;
	Metric_state child_state[2] = { left_state, right_state };
	size_t child_rank[2] = { left_rank, right_rank };
	Metric_candidate *child_candidate[2];
	uint8_t bounded_state[METRIC_BOUNDED_DOMAIN_MAX_MARKS] = { 0 };
	bool bounded_active = 0 < metric_bounded_domain_active_count(pex);
	bool bounded_rejected = false;

	if (!metric_exact_choice_state(
	    pex, choice, set, left_state, right_state, &computed_state))
		return false;
	if (computed_state != parent_state) return false;

	if (!metric_compute_candidate_metric(
	    pex, ranker, choice, set, 0, child_state, child_rank,
	    &metric, NULL, child_candidate))
		return false;
	if (bounded_active && !metric_candidate_bounded_summary(
	    pex, choice, set, child_candidate, bounded_state))
		bounded_rejected = true;

	Metric_candidate *candidate = pool_alloc(ranker->candidate_pool);
	candidate->ranker = ranker;
	candidate->choice = choice;
	candidate->rank[0] = left_rank;
	candidate->rank[1] = right_rank;
	candidate->state = parent_state;
	candidate->child_state[0] = left_state;
	candidate->child_state[1] = right_state;
	candidate->metric = metric;
	if (bounded_active)
		memcpy(candidate->bounded_domain_state, bounded_state,
		       sizeof(candidate->bounded_domain_state));
	candidate->serial = pex->metric.serial++;
	candidate->bounded_domain_rejected = bounded_rejected;
	candidate->parse_constraint_relevant =
		metric_choice_parse_constraint_relevant(pex, choice, set) ||
		((NULL != child_candidate[0]) &&
		 child_candidate[0]->parse_constraint_relevant) ||
		((NULL != child_candidate[1]) &&
		 child_candidate[1]->parse_constraint_relevant);

	metric_heap_push(stream->heap, candidate);
	pex->metric.trace.state_assignments_pushed++;
	return true;
}

static Metric_state_stream *metric_get_stream(Metric_set_cache *cache,
                                              Metric_state state)
{
	for (size_t i = 0; i < cache->num_streams; i++)
	{
		if (cache->streams[i].state == state)
			return &cache->streams[i];
	}

	if (cache->num_streams == cache->streams_size)
	{
		cache->streams_size = (0 == cache->streams_size) ?
			4 : 2 * cache->streams_size;
		cache->streams = realloc(cache->streams,
			cache->streams_size * sizeof(*cache->streams));
		assert(NULL != cache->streams,
		       "Out of memory growing metric state streams");
	}

	Metric_state_stream *stream =
		&cache->streams[cache->num_streams++];
	memset(stream, 0, sizeof(*stream));
	stream->state = state;
	return stream;
}

static void metric_start_stream(extractor_t *pex, Metric_ranker *ranker,
                                Parse_set *set,
                                Metric_state_stream *stream)
{
	if ((NULL == set) || stream->started) return;

	/* Seed the stream with the cheapest child-rank combination for every
	 * Parse_choice/state split that can produce stream->state.  Later
	 * calls to metric_stream_emit_next() advance only the neighbors of
	 * the candidate that was just emitted. */
	stream->started = true;
	if (pex->metric.trace.enabled)
	{
		pex->metric.trace.streams_started++;
		metric_trace_report(pex, "start-stream", false);
	}
	if (NULL == set->first)
	{
		stream->done = true;
		return;
	}

	stream->heap = metric_heap_new(set->num_pc);

	for (Parse_choice *pc = set->first; pc != NULL; pc = pc->next)
	{
		if (pex->metric.trace.enabled)
		{
			pex->metric.trace.choices_scanned++;
			if (0 == (pex->metric.trace.choices_scanned & 0x3fff))
				metric_trace_report(pex, "scan-choices", false);
		}
		if (metric_resources_exhausted(pex))
		{
			stream->done = true;
			return;
		}

		if (metric_exact_state_enabled(pex))
		{
			size_t state_count = metric_exact_state_count(pex);
			Metric_state left_possible =
				parse_set_possible_metric_state(
					pex, ranker, pc->set[0]);
			Metric_state right_possible =
				parse_set_possible_metric_state(
					pex, ranker, pc->set[1]);

			for (Metric_state left_state = 0;
			     left_state < state_count; left_state++)
			{
				if (!(left_possible &
				      metric_exact_state_mask(left_state)))
					continue;

				for (Metric_state right_state = 0;
				     right_state < state_count;
				     right_state++)
				{
					if (!(right_possible &
					      metric_exact_state_mask(right_state)))
						continue;

					pex->metric.trace.state_assignments_considered++;
					metric_push_exact_candidate(
						pex, ranker, stream, set, pc,
						left_state, 0, right_state, 0,
						stream->state);
				}
			}
			continue;
		}

		pex->metric.trace.state_assignments_considered++;
		metric_push_state_candidate(pex, ranker, stream, set, pc,
		                            0, 0, 0, 0);
	}
}

static void metric_stream_remember_rank(Metric_state_stream *stream,
                                        Metric_candidate *candidate)
{
	if (stream->num_ranked == stream->ranked_size)
	{
		stream->ranked_size = (0 == stream->ranked_size) ?
			16 : 2 * stream->ranked_size;
		stream->ranked = realloc(stream->ranked,
			stream->ranked_size * sizeof(*stream->ranked));
		assert(NULL != stream->ranked,
		       "Out of memory growing metric stream rank array");
	}

	stream->ranked[stream->num_ranked++] = candidate;
}

static void metric_push_candidate_successors(extractor_t *pex,
                                             Metric_ranker *ranker,
                                             Metric_state_stream *stream,
                                             Parse_set *set,
                                             Metric_candidate *candidate)
{
	if (metric_exact_state_enabled(pex))
	{
		metric_push_exact_candidate(
			pex, ranker, stream, set, candidate->choice,
			candidate->child_state[0], candidate->rank[0] + 1,
			candidate->child_state[1], candidate->rank[1],
			stream->state);
		if (0 == candidate->rank[0])
			metric_push_exact_candidate(
				pex, ranker, stream, set, candidate->choice,
				candidate->child_state[0], 0,
				candidate->child_state[1],
				candidate->rank[1] + 1, stream->state);
	}
	else
	{
		metric_push_state_candidate(pex, ranker, stream, set,
		                            candidate->choice,
		                            candidate->child_state[0],
		                            candidate->rank[0] + 1,
		                            candidate->child_state[1],
		                            candidate->rank[1]);
		if (0 == candidate->rank[0])
			metric_push_state_candidate(pex, ranker, stream, set,
			                            candidate->choice,
			                            candidate->child_state[0], 0,
			                            candidate->child_state[1],
			                            candidate->rank[1] + 1);
	}
}

static bool metric_stream_emit_next(extractor_t *pex, Metric_ranker *ranker,
                                    Parse_set *set,
                                    Metric_state_stream *stream)
{
	metric_start_stream(pex, ranker, set, stream);
	if (stream->done) return false;

	Metric_candidate *candidate;
	for (;;)
	{
		candidate = metric_heap_pop(stream->heap);
		if (NULL == candidate)
		{
			stream->done = true;
			return false;
		}

		metric_push_candidate_successors(pex, ranker, stream,
		                                 set, candidate);
		if (candidate->bounded_domain_rejected)
			continue;

		break;
	}

	metric_stream_remember_rank(stream, candidate);
	if (pex->metric.trace.enabled)
	{
		pex->metric.trace.stream_candidates_emitted++;
		metric_trace_report(pex, "stream-emit", false);
	}
	return true;
}

static Metric_candidate *metric_get_state_rank(extractor_t *pex,
                                               Metric_ranker *ranker,
                                               Parse_set *set,
                                               Metric_state state,
                                               size_t rank)
{
	if ((NULL == set) || (NULL == set->first)) return NULL;
	if (!metric_set_state_possible(pex, ranker, set, state))
		return NULL;

	Metric_set_cache *cache = metric_get_set_cache(pex, ranker, set);
	Metric_state_stream *stream = metric_get_stream(cache, state);
	while ((stream->num_ranked <= rank) && !stream->done)
	{
		if (metric_resources_exhausted(pex)) break;
		if (!metric_stream_emit_next(pex, ranker, set, stream)) break;
	}

	if (stream->num_ranked <= rank) return NULL;
	return stream->ranked[rank];
}

static bool metric_resources_exhausted(extractor_t *pex)
{
	if (NULL == pex->metric.resources) return false;

	pex->metric.resource_check_count++;
	if (0 != (pex->metric.resource_check_count & 0xfff))
		return false;

	return resources_exhausted(pex->metric.resources);
}

static void metric_remember_rank(Metric_set_cache *cache,
                                 Metric_candidate *candidate)
{
	if (cache->num_ranked == cache->ranked_size)
	{
		cache->ranked_size = (0 == cache->ranked_size) ?
			16 : 2 * cache->ranked_size;
		cache->ranked = realloc(cache->ranked,
			cache->ranked_size * sizeof(*cache->ranked));
		assert(NULL != cache->ranked,
		       "Out of memory growing metric rank array");
	}

	cache->ranked[cache->num_ranked++] = candidate;
}

static bool metric_emit_next(extractor_t *pex, Metric_ranker *ranker,
                             Parse_set *set)
{
	Metric_candidate *best = NULL;
	Metric_state best_state = 0;
	Metric_set_cache *cache;

	if (NULL == set) return false;
	cache = metric_get_set_cache(pex, ranker, set);
	if (cache->done) return false;

	for (size_t i = 0; i < ranker->num_root_states; i++)
	{
		Metric_state state = ranker->root_states[i];

		if (metric_resources_exhausted(pex)) break;

		/* The root ranker merges the next candidate from every allowed
		 * root state; child streams remain independently memoized. */
		Metric_state_stream *stream = metric_get_stream(cache, state);
		Metric_candidate *candidate = metric_get_state_rank(
			pex, ranker, set, state, stream->next_emit_rank);
		if ((NULL != candidate) &&
		    ((NULL == best) || metric_candidate_less(candidate, best)))
		{
			best = candidate;
			best_state = state;
		}
	}

	if (NULL == best)
	{
		cache->done = true;
		return false;
	}

	Metric_state_stream *best_stream = metric_get_stream(cache, best_state);
	best_stream->next_emit_rank++;
	metric_remember_rank(cache, best);
	if (pex->metric.trace.enabled)
	{
		pex->metric.trace.root_candidates_emitted++;
		metric_trace_report(pex, "root-emit", false);
	}
	return true;
}

static Metric_candidate *metric_get_rank(extractor_t *pex,
                                         Metric_ranker *ranker,
                                         Parse_set *set,
                                         size_t rank)
{
	if ((NULL == set) || (NULL == set->first)) return NULL;
	Metric_set_cache *cache = metric_get_set_cache(pex, ranker, set);

	while ((cache->num_ranked <= rank) && !cache->done)
	{
		if (!metric_emit_next(pex, ranker, set)) break;
	}

	if (cache->num_ranked <= rank) return NULL;
	return cache->ranked[rank];
}

typedef struct
{
	Parse_choice *choice;
	Parse_set *set;
	int side;
} Metric_choice_link_ref;

static bool metric_candidate_link_ref(extractor_t *pex, Metric_ranker *ranker,
                                      Parse_set *set,
                                      Metric_candidate *candidate,
                                      unsigned int target_idx,
                                      unsigned int *link_idx,
                                      Metric_choice_link_ref *ref)
{
	if ((NULL == set) || (NULL == candidate)) return false;

	/* Linkage link order follows the recursive issue_links_for_choice()
	 * order.  Walk the candidate tree in that same order so PP-reported
	 * link indexes can be mapped back to the exact Parse_choice side. */
	Parse_choice *pc = candidate->choice;
	for (int side = 0; side < 2; side++)
	{
		const char *name = choice_link_name(pex, pc, set, side);
		if (NULL == name) continue;

		if (*link_idx == target_idx)
		{
			ref->choice = pc;
			ref->set = set;
			ref->side = side;
			return true;
		}
		(*link_idx)++;
	}

	for (int side = 0; side < 2; side++)
	{
		Metric_candidate *child =
			metric_get_state_rank(pex, ranker, pc->set[side],
			                      candidate->child_state[side],
			                      candidate->rank[side]);
		if (metric_candidate_link_ref(pex, ranker, pc->set[side],
		    child, target_idx, link_idx, ref))
			return true;
	}

	return false;
}

static bool metric_find_candidate_link_ref(extractor_t *pex,
                                           Metric_candidate *root,
                                           unsigned int link_idx,
                                           Metric_choice_link_ref *ref)
{
	unsigned int cur_idx = 0;

	memset(ref, 0, sizeof(*ref));
	ref->side = -1;
	if (NULL == root) return false;
	return metric_candidate_link_ref(pex, root->ranker, pex->parse_set,
	                                 root, link_idx, &cur_idx, ref);
}

static bool metric_bounded_domain_feedback_same(
	const Metric_bounded_domain_feedback *feedback,
	const Metric_choice_link_ref *ref, Linkage lkg,
	const PP_failure *failure)
{
	const Link *link;

	if ((feedback->set != ref->set) ||
	    (feedback->l_id != ref->choice->l_id) ||
	    (feedback->r_id != ref->choice->r_id) ||
	    (feedback->side != ref->side))
		return false;

	if (!failure->has_domain_start_link ||
	    (failure->domain_start_link >= lkg->num_links))
		return false;

	link = &lkg->link_array[failure->domain_start_link];
	return (feedback->root == link->lw) &&
	       (feedback->seed == link->rw) &&
	       (NULL != link->link_name) &&
	       (0 == strcmp(feedback->link_name, link->link_name));
}

static void metric_trace_domain_feedback_ignored(
	extractor_t *, const char *, const PP_failure *, const Link *,
	const char *);

static size_t metric_bounded_domain_feedback_store(
	extractor_t *pex, const Metric_choice_link_ref *ref, Linkage lkg,
	const PP_failure *failure)
{
	const Link *link;

	if (!failure->has_domain_start_link ||
	    (failure->domain_start_link >= lkg->num_links))
		return 0;

	link = &lkg->link_array[failure->domain_start_link];
	if (NULL == link->link_name)
	{
		pex->metric.pp.bounded.ignored++;
		metric_trace_domain_feedback_ignored(
			pex, "bounded-domain", failure, link,
			"domain start link has no name");
		return 0;
	}

	if (PP_DOMAIN_REGULAR != post_process_link_domain_kind(
	    pex->postprocessor, link->link_name))
	{
		/* The compact bounded summary currently models only the
		 * ordinary-domain DFS.  Other domain kinds remain PP-only until
		 * they have an exact candidate summary. */
		pex->metric.pp.bounded.ignored++;
		metric_trace_domain_feedback_ignored(
			pex, "bounded-domain", failure, link,
			"bounded feedback supports regular domains only");
		return 0;
	}

	for (size_t i = 0; i < pex->metric.pp.bounded.num_feedbacks; i++)
	{
		if (metric_bounded_domain_feedback_same(
		    &pex->metric.pp.bounded.feedbacks[i], ref, lkg, failure))
		{
			pex->metric.pp.bounded.duplicates++;
			return 0;
		}
	}

	if (METRIC_BOUNDED_DOMAIN_MAX_MARKS <=
	    pex->metric.pp.bounded.num_feedbacks)
	{
		pex->metric.pp.bounded.ignored++;
		metric_trace_domain_feedback_ignored(
			pex, "bounded-domain", failure, link,
			"feedback mark limit reached");
		return 0;
	}

	if (pex->metric.pp.bounded.num_feedbacks ==
	    pex->metric.pp.bounded.feedbacks_size)
	{
		pex->metric.pp.bounded.feedbacks_size =
			(0 == pex->metric.pp.bounded.feedbacks_size) ?
			4 : 2 * pex->metric.pp.bounded.feedbacks_size;
		pex->metric.pp.bounded.feedbacks =
			realloc(pex->metric.pp.bounded.feedbacks,
			        pex->metric.pp.bounded.feedbacks_size *
			        sizeof(*pex->metric.pp.bounded.feedbacks));
		assert(NULL != pex->metric.pp.bounded.feedbacks,
		       "Out of memory growing metric bounded feedback");
	}

	Metric_bounded_domain_feedback *stored =
		&pex->metric.pp.bounded.feedbacks[
			pex->metric.pp.bounded.num_feedbacks++];
	stored->set = ref->set;
	stored->l_id = ref->choice->l_id;
	stored->r_id = ref->choice->r_id;
	stored->side = ref->side;
	stored->domain = failure->domain;
	stored->root = link->lw;
	stored->seed = link->rw;
	stored->link_name = safe_strdup(link->link_name);
	assert(NULL != stored->link_name,
	       "Out of memory copying metric bounded link name");
	pex->metric.pp.bounded.dirty++;

	if (verbosity_level(D_EXTRACT))
		err_msg(lg_Debug, "Info: learned metric bounded-domain "
		        "constraint for %u-%s-%u, domain %d\n",
		        (unsigned int)stored->root, stored->link_name,
		        (unsigned int)stored->seed, stored->domain);

	return 1;
}

size_t extractor_finish_metric_bounded_domain_feedback_state(
	extractor_t *pex, Metric_candidate *root, Linkage lkg,
	const PP_failure *failure)
{
	Metric_choice_link_ref ref;

	if ((NULL == pex) || !pex->metric.pp.bounded_enabled)
		return 0;
	if ((NULL == root) || (NULL == lkg) || (0 == lkg->lifo.N_violations) ||
	    (NULL == failure) || (PP_FAILURE_BOUNDED != failure->type) ||
	    !failure->has_domain_start_link || (0 == failure->num_offending_links))
		return 0;

	if (!metric_find_candidate_link_ref(pex, root,
	                                    failure->domain_start_link, &ref))
	{
		pex->metric.pp.bounded.ignored++;
		return 0;
	}

	return metric_bounded_domain_feedback_store(pex, &ref, lkg, failure);
}

void extractor_apply_metric_bounded_domain_feedback(extractor_t *pex)
{
	if ((NULL == pex) || (0 == pex->metric.pp.bounded.dirty))
		return;

	metric_ranker_free_all(pex);
	if (NULL != pex->metric.ranker_pool)
		pool_reuse(pex->metric.ranker_pool);

	pex->metric.next_rank = 0;
	pex->metric.trace_root = NULL;
	pex->metric.pp.bounded.dirty = 0;
}

static bool metric_domain_feedback_trace_enabled(const extractor_t *pex)
{
	return pex->metric.pp.feedback_trace_enabled;
}

static void metric_trace_domain_feedback_ignored(
	extractor_t *pex, const char *category, const PP_failure *failure,
	const Link *link, const char *reason)
{
	const char *link_name = (NULL == link) ? NULL : link->link_name;
	PP_domain_kind kind = post_process_link_domain_kind(
		pex->postprocessor, link_name);

	if (!metric_domain_feedback_trace_enabled(pex)) return;

	err_msg(lg_Debug, "Info: ignored metric %s feedback for "
	        "%u-%s-%u, domain %d, selector %s, domain kind %s: %s\n",
	        category,
	        (NULL == link) ? 0 : (unsigned int)link->lw,
	        (NULL == link_name) ? "(null)" : link_name,
	        (NULL == link) ? 0 : (unsigned int)link->rw,
	        (NULL == failure) ? -1 : failure->domain,
	        ((NULL == failure) || (NULL == failure->selector)) ?
	        "(null)" : failure->selector,
	        metric_domain_kind_name(kind), reason);
}

static void list_metric_links(extractor_t *pex, Metric_ranker *ranker,
                              Linkage lkg, Parse_set *set,
                              Metric_candidate *candidate)
{
	if ((NULL == set) || (NULL == candidate)) return;

	/* Materialize the lazy candidate tree into the same Linkage order as
	 * the traditional extractor, so PP failures can still be traced back
	 * to Parse_choice paths by link index. */
	Parse_choice *pc = candidate->choice;
	issue_links_for_choice(lkg, pc, set);

	for (int side = 0; side < 2; side++)
	{
		Metric_candidate *child =
			metric_get_state_rank(pex, ranker, pc->set[side],
			                      candidate->child_state[side],
			                      candidate->rank[side]);
		list_metric_links(pex, ranker, lkg, pc->set[side], child);
	}
}

static uint64_t metric_hash_mix(uint64_t h, uint64_t v)
{
	h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
	return h;
}

static uint64_t metric_candidate_signature(extractor_t *pex,
                                           Metric_ranker *ranker,
                                           Parse_set *set,
                                           Metric_candidate *candidate)
{
	if ((NULL == set) || (NULL == candidate)) return 0xcbf29ce484222325ULL;

	Parse_choice *pc = candidate->choice;
	uint64_t h = 0xcbf29ce484222325ULL;
	/* Hash the chosen Parse_choice identities and child signatures, not
	 * the emitted link strings, because duplicate suppression is only a
	 * guard against ranker-state/subproblem convergence. */
	h = metric_hash_mix(h, (uintptr_t)pc);
	h = metric_hash_mix(h, (uintptr_t)pc->md);
	h = metric_hash_mix(h, (uint64_t)pc->l_id);
	h = metric_hash_mix(h, (uint64_t)pc->r_id);

	for (int side = 0; side < 2; side++)
	{
		Metric_candidate *child =
			metric_get_state_rank(pex, ranker, pc->set[side],
			                      candidate->child_state[side],
			                      candidate->rank[side]);
		h = metric_hash_mix(h, metric_candidate_signature(
			pex, ranker, pc->set[side], child));
	}

	return (0 == h) ? 1 : h;
}

static bool metric_seen_insert(extractor_t *pex, uint64_t hash)
{
	if (!pex->metric.pp.bounded_enabled &&
	    !pex->metric.pp.global_enabled)
		return true;

	if (0 == pex->metric.seen.size)
	{
		pex->metric.seen.size = 1024;
		pex->metric.seen.hash =
			calloc(pex->metric.seen.size, sizeof(*pex->metric.seen.hash));
		assert(NULL != pex->metric.seen.hash,
		       "Out of memory allocating metric seen set");
	}

	if (pex->metric.seen.count * 2 >= pex->metric.seen.size)
	{
		uint64_t *old_hash = pex->metric.seen.hash;
		size_t old_size = pex->metric.seen.size;

		pex->metric.seen.size *= 2;
		pex->metric.seen.hash =
			calloc(pex->metric.seen.size, sizeof(*pex->metric.seen.hash));
		assert(NULL != pex->metric.seen.hash,
		       "Out of memory growing metric seen set");
		pex->metric.seen.count = 0;

		for (size_t i = 0; i < old_size; i++)
		{
			uint64_t h = old_hash[i];
			if (0 == h) continue;

			size_t idx = h & (pex->metric.seen.size - 1);
			while (0 != pex->metric.seen.hash[idx])
				idx = (idx + 1) & (pex->metric.seen.size - 1);
			pex->metric.seen.hash[idx] = h;
			pex->metric.seen.count++;
		}
		free(old_hash);
	}

	size_t idx = hash & (pex->metric.seen.size - 1);
	while (0 != pex->metric.seen.hash[idx])
	{
		if (hash == pex->metric.seen.hash[idx]) return false;
		idx = (idx + 1) & (pex->metric.seen.size - 1);
	}

	pex->metric.seen.hash[idx] = hash;
	pex->metric.seen.count++;
	return true;
}

static const char *pp_failure_type_name(PP_failure_type type)
{
	switch (type)
	{
		case PP_FAILURE_NONE:
			return "none";
		case PP_FAILURE_CONTAINS_ONE:
			return "contains_one";
		case PP_FAILURE_CONTAINS_NONE:
			return "contains_none";
		case PP_FAILURE_CONTAINS_ONE_GLOBAL:
			return "contains_one_global";
		case PP_FAILURE_MUST_FORM_CYCLE:
			return "must_form_cycle";
		case PP_FAILURE_BOUNDED:
			return "bounded";
	}
	return "unknown";
}

static bool pp_failure_list_has(const unsigned int *links, size_t num_links,
                                unsigned int link)
{
	for (size_t i = 0; i < num_links; i++)
		if (links[i] == link) return true;

	return false;
}

static void trace_add_role(char *roles, size_t roles_size, const char *role)
{
	size_t len = strlen(roles);
	int written;

	if (roles_size <= len + 1) return;

	written = snprintf(roles + len, roles_size - len,
	                   "%s%s", (0 == len) ? "" : ",", role);
	if (written < 0)
		roles[roles_size - 1] = '\0';
}

static bool pp_failure_link_roles(const PP_failure *failure,
                                  unsigned int link, char *roles,
                                  size_t roles_size)
{
	roles[0] = '\0';

	if (pp_failure_list_has(failure->domain_links,
	                        failure->num_domain_links, link))
		trace_add_role(roles, roles_size, "domain");
	if (failure->has_domain_start_link &&
	    (failure->domain_start_link == link))
		trace_add_role(roles, roles_size, "domain-start");
	if (pp_failure_list_has(failure->selector_links,
	                        failure->num_selector_links, link))
		trace_add_role(roles, roles_size, "selector");
	if (pp_failure_list_has(failure->criterion_links,
	                        failure->num_criterion_links, link))
		trace_add_role(roles, roles_size, "criterion");
	if (pp_failure_list_has(failure->offending_links,
	                        failure->num_offending_links, link))
		trace_add_role(roles, roles_size, "offending");

	return roles[0] != '\0';
}

static void trace_pp_failure_link_list(size_t trace_id, const char *name,
                                       const unsigned int *links,
                                       size_t num_links)
{
	err_msg(lg_Debug, "pp-parse-set-trace #%zu: %s:", trace_id, name);
	for (size_t i = 0; i < num_links; i++)
		err_msg(lg_Debug, " %u", links[i]);
	err_msg(lg_Debug, "\n");
}

static void trace_pp_failure_header(const PP_failure *failure, size_t trace_id)
{
	err_msg(lg_Debug,
	        "pp-parse-set-trace #%zu: type=%s violation=\"%s\" "
	        "selector=%s domain=%d truncated=%s\n",
	        trace_id, pp_failure_type_name(failure->type),
	        (NULL == failure->message) ? "" : failure->message,
	        (NULL == failure->selector) ? "" : failure->selector,
	        failure->domain, failure->truncated ? "true" : "false");

	if (NULL != failure->criteria)
	{
		err_msg(lg_Debug, "pp-parse-set-trace #%zu: criteria:",
		        trace_id);
		for (const char **crit = failure->criteria; *crit != NULL; crit++)
			err_msg(lg_Debug, " %s", *crit);
		err_msg(lg_Debug, "\n");
	}

	trace_pp_failure_link_list(trace_id, "domain-links",
	                           failure->domain_links,
	                           failure->num_domain_links);
	if (failure->has_domain_start_link)
		err_msg(lg_Debug, "pp-parse-set-trace #%zu: "
		        "domain-start-link: %u\n",
		        trace_id, failure->domain_start_link);
	trace_pp_failure_link_list(trace_id, "selector-links",
	                           failure->selector_links,
	                           failure->num_selector_links);
	trace_pp_failure_link_list(trace_id, "criterion-links",
	                           failure->criterion_links,
	                           failure->num_criterion_links);
	trace_pp_failure_link_list(trace_id, "offending-links",
	                           failure->offending_links,
	                           failure->num_offending_links);
}

static bool trace_pp_all_links_enabled(const extractor_t *pex)
{
	return pex->metric.pp.trace_all_links_enabled;
}

static void trace_pp_failure_all_links(Linkage lkg,
                                       const PP_failure *failure,
                                       size_t trace_id)
{
	err_msg(lg_Debug, "pp-parse-set-trace #%zu: all-links:\n", trace_id);

	for (unsigned int i = 0; i < lkg->num_links; i++)
	{
		const Link *link = &lkg->link_array[i];
		const char *left_word = "";
		const char *right_word = "";
		char roles[64];

		if (!pp_failure_link_roles(failure, i, roles, sizeof(roles)))
			roles[0] = '\0';

		if (NULL != lkg->word)
		{
			left_word = (NULL == lkg->word[link->lw]) ?
			            "" : lkg->word[link->lw];
			right_word = (NULL == lkg->word[link->rw]) ?
			             "" : lkg->word[link->rw];
		}
		else if ((NULL != lkg->sent) &&
		         (link->lw < lkg->sent->length) &&
		         (link->rw < lkg->sent->length))
		{
			left_word = (NULL == lkg->sent->word[link->lw].unsplit_word) ?
			            "" : lkg->sent->word[link->lw].unsplit_word;
			right_word = (NULL == lkg->sent->word[link->rw].unsplit_word) ?
			             "" : lkg->sent->word[link->rw].unsplit_word;
		}

		err_msg(lg_Debug,
		        "pp-parse-set-trace link[%u] roles=%s "
		        "link=%u-%s-%u words=%s -- %s -- %s\n",
		        i, roles, (unsigned int)link->lw,
		        (NULL == link->link_name) ? "?" : link->link_name,
		        (unsigned int)link->rw, left_word,
		        (NULL == link->link_name) ? "?" : link->link_name,
		        right_word);
	}
}

static void trace_metric_choice_link(Linkage lkg, const PP_failure *failure,
                                     const Parse_set *set,
                                     const Metric_candidate *candidate,
                                     int side, unsigned int *link_idx,
                                     unsigned int depth)
{
	Parse_choice *pc = candidate->choice;
	Connector *lc = side ? get_tracon_by_id(pc->md, pc->r_id, 1) : set->le;
	Connector *rc;
	char roles[64];
	Parse_metric local;
	Link *link;

	if (is_zero_tracon(lc)) return;

	rc = side ? set->re : get_tracon_by_id(pc->md, pc->l_id, 0);
	if (is_zero_tracon(rc)) return;

	if (*link_idx >= lkg->num_links) return;
	if (!pp_failure_link_roles(failure, *link_idx, roles, sizeof(roles)))
	{
		(*link_idx)++;
		return;
	}

	link = &lkg->link_array[*link_idx];
	local = choice_local_metric(pc, set);

	err_msg(lg_Debug,
	        "pp-parse-set-trace link[%u] roles=%s depth=%u "
	        "link=%u-%s-%u parent_set=%p[%u,%u n=%u] "
	        "pc=%p side=%d md=%s l_id=%d r_id=%d "
	        "child-rank=(%zu,%zu) local=(dis=%5.2f len=%d)\n",
	        *link_idx, roles, depth, (unsigned int)link->lw,
	        (NULL == link->link_name) ? "?" : link->link_name,
	        (unsigned int)link->rw, (void *)set,
	        (unsigned int)set->lw, (unsigned int)set->rw,
	        (unsigned int)set->null_count, (void *)pc, side,
	        (NULL == pc->md->word_string) ? "" : pc->md->word_string,
	        pc->l_id, pc->r_id, candidate->rank[0],
	        candidate->rank[1], local.disjunct_cost, local.link_cost);

	(*link_idx)++;
}

static void trace_metric_choice_links(extractor_t *pex, Linkage lkg,
                                      const PP_failure *failure,
                                      Parse_set *set,
                                      Metric_candidate *candidate,
                                      unsigned int *link_idx,
                                      unsigned int depth)
{
	if ((NULL == set) || (NULL == candidate)) return;

	Parse_choice *pc = candidate->choice;
	trace_metric_choice_link(lkg, failure, set, candidate, 0, link_idx,
	                         depth);
	trace_metric_choice_link(lkg, failure, set, candidate, 1, link_idx,
	                         depth);

	for (int side = 0; side < 2; side++)
	{
		Metric_candidate *child =
			metric_get_state_rank(pex, candidate->ranker,
			                      pc->set[side],
			                      candidate->child_state[side],
			                      candidate->rank[side]);
		trace_metric_choice_links(pex, lkg, failure, pc->set[side],
		                          child, link_idx, depth + 1);
	}
}

void extractor_trace_metric_candidate(extractor_t *pex, Linkage lkg,
                                      const PP_failure *failure,
                                      Metric_candidate *root,
                                      size_t trace_id)
{
	unsigned int link_idx = 0;

	if ((NULL == failure) || (PP_FAILURE_NONE == failure->type))
	{
		err_msg(lg_Debug,
		        "pp-parse-set-trace #%zu: no PP failure detail\n",
		        trace_id);
		return;
	}
	if (NULL == root)
	{
		err_msg(lg_Debug,
		        "pp-parse-set-trace #%zu: no metric candidate root\n",
		        trace_id);
		return;
	}

	trace_pp_failure_header(failure, trace_id);
	if (trace_pp_all_links_enabled(pex))
		trace_pp_failure_all_links(lkg, failure, trace_id);
	trace_metric_choice_links(pex, lkg, failure, pex->parse_set, root,
	                          &link_idx, 0);
}


void metric_extractor_free(extractor_t *pex)
{
	metric_ranker_free_all(pex);
	free(pex->metric.links.classes);
	free(pex->metric.links.id_table);
	for (size_t i = 0; i < pex->metric.pp.bounded.num_feedbacks; i++)
		free(pex->metric.pp.bounded.feedbacks[i].link_name);
	free(pex->metric.pp.bounded.feedbacks);
	pool_delete(pex->metric.ranker_pool);
	free(pex->metric.seen.hash);
}

bool extract_metric_links(extractor_t *pex, Linkage lkg)
{
	assert(pex->metric.enabled, "Metric extraction was not enabled");

	metric_trace_begin(pex);
	if (pex->metric.trace.enabled)
	{
		pex->metric.trace.extract_calls++;
		if (1 == pex->metric.trace.extract_calls)
			metric_trace_report(pex, "start", true);
	}

	for (;;)
	{
		/* Get the next root candidate in raw metric order, then apply
		 * exact pre-PP blockers.  Rejected candidates are simply skipped;
		 * postprocessing remains batched in parse.c. */
		metric_pp_prediction_clear(pex);
		Metric_ranker *ranker = metric_root_ranker(pex);
		Metric_candidate *candidate =
			metric_get_rank(pex, ranker,
			                pex->parse_set, pex->metric.next_rank);
		if (NULL == candidate)
		{
			metric_trace_report(pex, "rank-empty", true);
			pex->metric.trace_root = NULL;
			return false;
		}

		pex->metric.next_rank++;
		uint64_t signature = metric_candidate_signature(
			pex, ranker, pex->parse_set, candidate);
		if (!metric_seen_insert(pex, signature))
		{
			pex->metric.seen.duplicate_skipped++;
			metric_trace_report(pex, "skip-duplicate", false);
			continue;
		}
		metric_pp_predict_global_contains_one(pex, candidate->state);
		if (!metric_pp_constraints_candidate_ok(pex, ranker, candidate))
		{
			if (pex->metric.pp.parse_constraint_reject_limit_hit)
			{
				metric_trace_report(
					pex, "reject-parse-constraint-limit", true);
				pex->metric.trace_root = NULL;
				return false;
			}
			continue;
		}
		if (!metric_candidate_global_contains_one_ok(
		    pex, ranker, pex->parse_set, candidate))
		{
			metric_trace_report(pex, "reject-global", false);
			continue;
		}
		pex->metric.trace_root = candidate;
		lkg->lifo.index = (int)(pex->metric.next_rank - 1);
		list_metric_links(pex, ranker, lkg, pex->parse_set, candidate);
		metric_trace_candidate(pex, "emit", pex->metric.next_rank - 1,
		                       candidate);
		return true;
	}
}
