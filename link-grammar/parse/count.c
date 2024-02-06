/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2013,2014,2015 Linas Vepstas                            */
/* Copyright (c) 2015-2022 Amir Plivatsky                                */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <limits.h>
#include <inttypes.h>                   // PRIu64
#if HAVE_THREADS_H && !__EMSCRIPTEN__
#include <threads.h>
#endif /* HAVE_THREADS_H && !__EMSCRIPTEN__ */

#include "link-includes.h"
#include "api-structures.h"
#include "connectors.h"
#include "count.h"
#include "dict-common/dict-common.h"    // IS_GENERATION
#include "disjunct-utils.h"
#include "fast-match.h"
#include "resources.h"
#include "tokenize/word-structures.h"   // for Word_struct
#include "utilities.h"

/* This file contains the exhaustive search algorithm. */

#define D_COUNT 5 /* General debug level for this file. */

typedef uint8_t null_count_m;  /* Storage representation of null_count */
typedef uint8_t WordIdx_m;     /* Storage representation of word index */

/* Allow to disable the use of the various caches (for debug). */
const bool ENABLE_WORD_SKIP_VECTOR = true;
const bool ENABLE_MATCH_LIST_CACHE = true;
const bool ENABLE_TABLE_LRCNT = true;  // Also controls the above two caches.
const bool USE_TABLE_TRACON = true;    // The table is always maintained.

typedef struct Table_tracon_s Table_tracon;
struct Table_tracon_s
{
	Table_tracon     *next;
	int              l_id, r_id;
	Count_bin        count;      // Normally int32_t.
	null_count_m     null_count;
	size_t           hash;       // Generation needs more than 32 bits.
};

/* Most of the time, do_count() yields a zero leftcount/rightcount when it
 * parses a word range in which one end is a certain tracon and the other
 * end is a word that is between the nearest_word and farthest_word of the
 * first connector of that tracon (words out of this range are not checked).
 * Memoizing this results in a huge speedup for sentences that are not
 * short.
 *
 * To that end, two cache arrays, one of leftcount and the other for
 * rightcount, indexed by the tracon number (tracon_id), are maintained.
 *
 * Each element of these arrays points to a vector, called here word-vector,
 * with a size equal to abs(nearest_word - farthest_word) + 1.
 *
 * Each element of this vector predicts, in it's status field, whether the
 * expected count is zero for a given null-count (when the word it
 * represents is the end word of the range).
 * This prediction is valid for null-counts up to null_count (or for any
 * null-count if null_count is ANY_NULL_COUNT).
 *
 * The check_next filed indicates the next word to be checked. Each
 * word-vector is updated each time one of its elements gets updated, so
 * check_next skips all the words for which the count is known to be zero. */

typedef struct
{
	/* Arrays of match list information for sentences with null_count==0.
	 * The count at index x is due to the disjuncts at the same index x. */
	match_list_cache *mlc0;

	null_count_m null_count; /* status==0 valid up to this null count */
	int8_t status;           /* -1: Needs update; 0: No count; 1: Count possible */
	WordIdx_m check_next;    /* Next word to check */
} count_expectation;

/* Using the word-vectors for very short sentences has too much overhead. */
static const size_t min_len_word_vector = 10; /* This is just an estimation. */

/* Special values for Table_lrcnt fields null_count and check_next. */
#define ANY_NULL_COUNT (MAX_SENTENCE + 1)
#define INCREMENT_WORD ((uint8_t)(MAX_SENTENCE+1)) /* last checked word + 1 */

typedef count_expectation *wordvecp;       /* Indexed by middle-word offset */
static count_expectation lrcnt_cache_zero; /* A sentinel indicating status==0 */

#if defined DEBUG || DEBUG_COUNT_COST
#define COUNT_COST(...) __VA_ARGS__
#else
#define COUNT_COST(...)
#endif

typedef struct
{
	wordvecp *tracon_wvp;      /* Indexed by tracon_id */
	uint32_t num_tracon_id;    /* Number of tracon IDs */
} Table_lrcnt;

struct count_context_s
{
	fast_matcher_t *mchxt;
	Sentence sent;
	/* int     null_block; */ /* not used, always 1 */
	bool    islands_ok;
	bool    exhausted;
	uint8_t num_growth;       /* Number of table growths, for debug */
	bool    is_short;
	uint32_t checktimer;      /* Avoid excess system calls */
	size_t table_size;        /* Can exceed 2**32 during generation. */
	size_t table_mask;        /* 2**table_size -1 */
	size_t table_available_count; /* derated table_size by hash load factor */
	Table_tracon ** table;
	Table_lrcnt table_lrcnt[2];  /* Left/right wordvec */
	Pool_desc *mlc_pool;         /* Match list cache */
	Resources current_resources;
	COUNT_COST(uint64_t count_cost[3];)
};

/* Wikipedia claims that load factors of 0.6 to 0.75 are OK, and that
 * a load factor of less than 0.25 is pointless. We set it to 0.333.
 */
#define INV_LOAD_FACTOR 3 /* One divided by load factor. */

/* Avoid pathological cases leading to failure */
#define MAX_LOG2_TABLE_SIZE ((sizeof(size_t)==4) ? 25 : 34)

/**
 * Provide an estimate for the number of Table_tracon entries that will
 * be needed.
 *
 * The number of entries actually used was measured in discussion
 *     https://github.com/opencog/link-grammar/discussions/1402
 * Based on this, an upper bound on the entries needed is
 *    3 * num_disjuncts * log_2(num_words)
 * i.e. more than this is almost never needed. A lower bound is
 *    0.5 * num_disjuncts * log_2(num_words)
 * i.e. more than this is *always* needed.
 *
 * In both conventional and MST dictionaries, more than 500K entries is
 * almost never needed. In a handful of extreme cases, 2M was observed.
 */
static size_t estimate_tracon_entries(Sentence sent)
{
	unsigned int nwords = sent->length;
	unsigned int log2_nwords = 0;
	while (nwords) { log2_nwords++; nwords >>= 1; }

	size_t tblsize = 3 * log2_nwords * sent->num_disjuncts;
	if (tblsize < 512) tblsize = 512; // Happens rarely on short sentences.
	return tblsize;
}

#if HAVE_THREADS_H && !__EMSCRIPTEN__
/* Each thread will get it's own version of the `kept_table`.
 * If the program creates zillions of threads, then there will
 * be a mem-leak if this table is not released when each thread
 * exits. This code arranges so that `free_tls_table` is called
 * when the thread exists.
 */
static void free_tls_table(void* ptr_to_table)
{
	if (NULL == ptr_to_table) return;

	Table_tracon** kept_table = *((Table_tracon***)ptr_to_table);
	if (NULL == kept_table) return;

	free(kept_table);
	*((Table_tracon***) ptr_to_table) = NULL;
}

static tss_t key;
static void make_key(void)
{
	tss_create(&key, free_tls_table);
}
#endif /* HAVE_THREADS_H && !__EMSCRIPTEN__ */

/**
 * Allocate memory for the connector-pair hash table and initialize
 * table-size related fields (table_size and table_available_count).
 * Reuse the previous hash table memory if the request is for a table
 * that fits.
 *
 * @ctxt[in, out] Table info.
 * @param logsz Log2 requested table size, or \c 0 if the table needs
 *              to be expanded. Tables are expanded by a factor of 2.
 */
static void table_alloc(count_context_t *ctxt, unsigned int logsz)
{
	static TLS Table_tracon **kept_table = NULL;
	static TLS size_t kept_table_size = 0;

	size_t reqsz = 1ULL << logsz;
	if (0 < logsz && reqsz <= ctxt->table_size) return; // It's big enough, already.

#if HAVE_THREADS_H && !__EMSCRIPTEN__
	// Install a thread-exit handler, to free kept_table on thread-exit.
	static once_flag flag = ONCE_FLAG_INIT;
	call_once(&flag, make_key);

	if (NULL == kept_table)
		tss_set(key, &kept_table);
#endif /* HAVE_THREADS_H && !__EMSCRIPTEN__ */

	if (logsz == 0)
		ctxt->table_size *= 2; /* Double the table size */
	else
		ctxt->table_size = reqsz;

	if ((1ULL << MAX_LOG2_TABLE_SIZE) <= ctxt->table_size)
		ctxt->table_size  = (1ULL << MAX_LOG2_TABLE_SIZE);

	lgdebug(+D_COUNT, "Tracon table size %lu\n", ctxt->table_size);

	/* Keep the table indefinitely (until thread-exit), so that it can
	 * be reused. This avoids a large overhead in malloc/free when
	 * large memory blocks are allocated. Large blocks in Linux trigger
	 * system calls to mmap/munmap that eat up a lot of time.
	 * (Up to 20%, depending on the sentence and CPU.)
	 *
	 * FYI: the new tracon tables are (much?) smaller than the older
	 * connector tables, so maybe this reuse is no longer needed?
	 */
	if (kept_table_size < ctxt->table_size)
	{
		kept_table_size = ctxt->table_size;

		if (kept_table) free(kept_table);
		kept_table = malloc(sizeof(Table_tracon *) * ctxt->table_size);
	}
	ctxt->table = kept_table;

	memset(ctxt->table, 0, sizeof(Table_tracon *) * ctxt->table_size);

	ctxt->table_mask = ctxt->table_size - 1;

	// This assures that the table load will stay under the load factor.
	ctxt->table_available_count = ctxt->table_size / INV_LOAD_FACTOR;
}

/**
 * Allocate the tracon hash table with a sentence-dependent table size.
 * This saves on dynamic table growth, which is costly.
 *
 * We estimate the number of required hash table slots by estimating
 * the number of entries that will be required, and then multiplying by
 * INV_LOAD_FACTOR so that the hash table usage remains sparse.
 */
static void init_table(count_context_t *ctxt)
{
	// Number of tracon entries we expect to store.
	size_t tblsz = estimate_tracon_entries(ctxt->sent);

	// Adjust by the table load factor.
	tblsz *= INV_LOAD_FACTOR;

	unsigned int logsz = 0;
	while (tblsz) { logsz++; tblsz >>= 1; }

	table_alloc(ctxt, logsz);
}

static void free_table_lrcnt(count_context_t *ctxt)
{
	if (ctxt->is_short) return;

	if (verbosity_level(D_COUNT))
	{
		unsigned int nonzero = 0, any_null = 0, zero = 0, non_max_null = 0;
		unsigned int cml = 0, cml_disjunct = 0; /* Cached match lists */
		Pool_location loc = { 0 };
		wordvecp t;

		/* Note: Due to a current pool_next() problem for vector elements,
		 * the stats results here may be slightly skewed.  See the comment
		 * on that in memory-pool.h. */
		while ((t = pool_next(ctxt->sent->wordvec_pool, &loc)) != NULL)
		{
			if (t->status == -1) continue;
			if (t->status == 1)
			{
				nonzero++;
				if (t->mlc0 != NULL)
				{
					cml++;
					for (Disjunct *d = t->mlc0->d; d != NULL; d++)
						cml_disjunct++;
				}
			}
			else if (t->null_count == ANY_NULL_COUNT)
				any_null++;
			else if (ctxt->sent->null_count > t->null_count)
				non_max_null++;
			else if (ctxt->sent->null_count == t->null_count)
				zero++;
		}

		const unsigned int num_values =
			pool_num_elements_issued(ctxt->sent->wordvec_pool);
		lgdebug(+0, "Values %u (usage = non_max_null %u + other %u, "
		        "other = any_null_zero %u + zero %u + nonzero %u); "
		        "%u disjuncts in %u cache entries\n",
		        num_values, non_max_null, num_values-non_max_null,
		        any_null, zero, nonzero, cml_disjunct, cml);

		for (unsigned int dir = 0; dir < 2; dir++)
		{
			unsigned int table_usage = 0;

			for (size_t i = 0; i < ctxt->table_lrcnt[dir].num_tracon_id; i++)
			{
				if (ctxt->table_lrcnt[dir].tracon_wvp[i] != NULL) continue;
				table_usage++;
			}

			lgdebug(+0, "Direction %u: Using %u/%u tracons %.2f%%\n\\",
			        dir, table_usage, ctxt->table_lrcnt[dir].num_tracon_id,
			        100.0f*table_usage / ctxt->table_lrcnt[dir].num_tracon_id);
		}
	}

	pool_delete(ctxt->mlc_pool);

	for (unsigned int dir = 0; dir < 2; dir++)
	{
		free(ctxt->table_lrcnt[dir].tracon_wvp);
		ctxt->table_lrcnt[dir].tracon_wvp = NULL;
	}
}

/// Estimate the proper size of the match pool based on experimental
/// data. An upper bound of twice the number of expressions seems to
/// handle almost all cases. This is based on the graph in
/// https://github.com/opencog/link-grammar/discussions/1402#discussioncomment-4826342
/// The estimate is meant to be an *upper bound* for how many will be
/// needed; the goal is to avoid allocation to get more, because it is
/// expensive.
///
/// FYI, Expression pool sizes in excess of 10M entries have been observed.
static size_t match_list_pool_size_estimate(Sentence sent)
{
	size_t expsz = pool_num_elements_issued(sent->Exp_pool);

	size_t mlpse = 2 * expsz;
	if (mlpse < 4090) mlpse = 4090;

	// Code below does a pool_alloc_vec(match_list_size) and we want to
	// ensure that this estimate is greater than the match list size.
	// The match list size can never be larger than the number of
	// disjuncts, so the pool_alloc_vec() will never fail if we do this:
	size_t maxndj = 0;
	for (WordIdx w = 0; w < sent->length; w++)
		if (maxndj < sent->word[w].num_disjuncts)
			maxndj = sent->word[w].num_disjuncts;

	// Generation can have millions of disjuncts on the wild-cards.
	// But the match list will never get that big.
	if (512*1024 < maxndj) maxndj = 512*1024;

	if (mlpse < maxndj) mlpse = maxndj;

	return mlpse;
}

static void init_table_lrcnt(count_context_t *ctxt)
{
	Sentence sent = ctxt->sent;

	for (unsigned int dir = 0; dir < 2; dir++)
	{
		const size_t sz = sizeof(wordvecp) * ctxt->table_lrcnt[dir].num_tracon_id;
		ctxt->table_lrcnt[dir].tracon_wvp = malloc(sz);
		memset(ctxt->table_lrcnt[dir].tracon_wvp, 0, sz);
	}

	const size_t initial_size = MIN(sent->length/2, 16) *
		(ctxt->table_lrcnt[0].num_tracon_id + ctxt->table_lrcnt[1].num_tracon_id);

	if (NULL != sent->wordvec_pool)
	{
		pool_reuse(sent->wordvec_pool);
	}
	else
	{
		ctxt->sent->wordvec_pool =
			pool_new(__func__, "count_expectation", /*num_elements*/initial_size,
			         sizeof(count_expectation), /*zero_out*/true,
			         /*align*/false, /*exact*/false);
	}

	const size_t match_list_pool_size = match_list_pool_size_estimate(sent);

	/* FIXME: Modify pool_alloc_vec() to use dynamic block sizes. */
	ctxt->mlc_pool =
		pool_new(__func__, "Match list cache",
		         /*num_elements*/match_list_pool_size, sizeof(match_list_cache),
		         /*zero_out*/false, /*align*/false, /*exact*/false);
}

#ifdef DEBUG
#define DEBUG_TABLE_STAT
#endif /* DEBUG */
//#define DEBUG_TABLE_STAT
#ifdef DEBUG_TABLE_STAT
static uint64_t hit, miss;  /* Table entry stats */
#define TABLE_STAT(...) __VA_ARGS__
#else
#define TABLE_STAT(...)
#endif

/**
 * Provide data for insights on the effectively of the connector pair table.
 * Hits, misses, chain length, number of elements with zero/nonzero counts.
 */
static void table_stat(count_context_t *ctxt)
{
#ifdef DEBUG_TABLE_STAT
	if (!verbosity_level(+D_COUNT)) return;

	size_t z = 0, nz = 0;  /* Number of entries with zero and non-zero counts */
	size_t c, total_c = 0; /* Chain length */
	size_t N = 0;          /* NULL table slots */
	size_t null_count[256] = { 0 };   /* null_count histogram */
	int chain_length[64] = { 0 };     /* Chain length histogram */
	bool table_stat_entries = test_enabled("count-table-entries");

	for (size_t i = 0; i < ctxt->table_size; i++)
	{
		Table_tracon *t = ctxt->table[i];

		c = 0;
		if (t == NULL)
		{
			N++;
		}
		else
		{
			assert(t->hash != 0, "Invalid hash value: 0");
			assert((hist_total(&t->count)>=0)&&(hist_total(&t->count) <= INT_MAX),
			       "Invalid count %"COUNT_FMT, hist_total(&t->count));
			assert((ctxt->table_lrcnt[0].num_tracon_id == 0) ||
			       t->l_id < (int)ctxt->sent->length ||
			       ((t->l_id >= 255)&&(t->l_id < (int)ctxt->table_lrcnt[0].num_tracon_id)),
			       "invalid l_id %d", t->l_id);
			assert((ctxt->table_lrcnt[1].num_tracon_id == 0) ||
			       t->r_id <= (int)ctxt->sent->length ||
			       ((t->r_id > 255)&&(t->r_id < (int)ctxt->table_lrcnt[1].num_tracon_id)),
			       "invalid r_id %d", t->r_id);
		}
		for (; t != NULL; t = t->next)
		{
			c++;
			if (hist_total(&t->count) == 0)
				z++;
			else
				nz++;
			null_count[t->null_count]++;
		}
		if (c > 0) /* Slot 0 used for length overflow */
		{
			chain_length[c >= (int)ARRAY_SIZE(chain_length) ? 0 : c]++;
			total_c += c;
		}
		//if (c != 0) printf("Connector table [%d] c=%d\n", i, c);
	}

	size_t used_slots = ctxt->table_size-N;
	unsigned int logsz = 0;
	size_t tblsz = ctxt->table_size;
	while (tblsz) { logsz++; tblsz >>= 1; }
	/* The used= value is TotalValues/TableSize (not UsedSlots/TableSize). */
	printf("Connector table: num_growth=%u msb=%u slots=%6zu/%6zu (%5.2f%%) "
	       "avg-chain=%4.2f values=%6zu (z=%5zu nz=%5zu N=%5zu) used=%5.2f%% "
	       "acc=%"PRIu64" (hit=%"PRIu64" miss=%"PRIu64") (sent_len=%zu dis=%u)\n",
	       ctxt->num_growth, logsz, used_slots, ctxt->table_size,
			 100.0f*used_slots/ctxt->table_size, 1.0f*total_c/used_slots,
	       z+nz, z, nz, N, 100.0f*(z+nz)/ctxt->table_size,
	       hit+miss, hit, miss, ctxt->sent->length, ctxt->sent->num_disjuncts);

	printf("Chain length:\n");
	for (size_t i = 1; i < ARRAY_SIZE(chain_length); i++)
		if (chain_length[i] > 0) printf("%zu: %d\n", i, chain_length[i]);
	if (chain_length[0] > 0) printf(">63: %d\n", chain_length[0]);

	if (!((null_count[1] == 1) && (null_count[2] == 0)))
	{
		printf("Null count:\n");
		for (unsigned int nc = 0; nc < ARRAY_SIZE(null_count); nc++)
		{
			if (0 != null_count[nc])
				printf("%u: %zu\n", nc, null_count[nc]);
		}
	}

	if (table_stat_entries)
	{
		for (unsigned int nc = 0; nc < ARRAY_SIZE(null_count); nc++)
		{
			if (0 == null_count[nc]) continue;

			printf("Null count %u:\n", nc);
			for (size_t i = 0; i < ctxt->table_size; i++)
			{
				for (Table_tracon *t = ctxt->table[i]; t != NULL; t = t->next)
				{
					if (t->null_count != nc) continue;

					int n = printf("[%zu]", i);
					printf("%*d %5d c=%"COUNT_FMT"\n",  15-n, t->l_id, t->r_id, t->count);
				}
			}
		}
	}

	hit = miss = 0;
#endif /* DEBUG_TABLE_STAT */
}

static void table_grow(count_context_t *ctxt)
{
	// If we somehow hit the max size, disallow further growth.
	// Reset the `table_available_count` so we don't come here again.
	if ((1ULL << MAX_LOG2_TABLE_SIZE) <= ctxt->table_size)
	{
		ctxt->table_available_count = UINT_MAX;
		return;
	}

	table_alloc(ctxt, 0);

	/* Rehash. */
	Table_tracon *oe;
	Pool_location loc = { 0 };
	while ((oe = pool_next(ctxt->sent->Table_tracon_pool, &loc)) != NULL)
	{
		size_t ni = oe->hash & ctxt->table_mask;

		if (ctxt->table[ni] == NULL) ctxt->table_available_count--;
		oe->next = ctxt->table[ni];
		ctxt->table[ni] = oe;
	}

	ctxt->num_growth++;
}

/**
 * Stores the value in the table.  Assumes it's not already there.
 */
static Count_bin table_store(count_context_t *ctxt,
                           int lw, int rw,
                           const Connector *le, const Connector *re,
                           unsigned int null_count,
                           size_t hash, w_Count_bin c)
{
	if (ctxt->table_available_count == 0) table_grow(ctxt);

	int l_id = (NULL != le) ? le->tracon_id : lw;
	int r_id = (NULL != re) ? re->tracon_id : rw;

	if (!USE_TABLE_TRACON)
	{
		// In case a table count already exist, check its consistency.
		Count_bin *e = table_lookup(ctxt, lw, rw, le, re, null_count, NULL);
		if (e != NULL)
		{
			assert((e == NULL) || (hist_total(&c) == hist_total(e)),
			       "Inconsistent count for w(%d,%d) tracon_id(%d,%d)",
			       lw, rw, l_id, r_id);
			return c;
		}

		// The count is still stored, for the above consistency check
		// and for mk_parse_set().
	}

	size_t i = hash & ctxt->table_mask;
	Table_tracon *n = pool_alloc(ctxt->sent->Table_tracon_pool);

	if (ctxt->table[i] == NULL)
		ctxt->table_available_count--;

	n->l_id = l_id;
	n->r_id = r_id;
	n->null_count = null_count;
	n->next = ctxt->table[i];
	n->count = (Count_bin)c; /* c is already clamped (by parse_count_clamp()) */
	n->hash = hash;
	ctxt->table[i] = n;

	return n->count;
}

/**
 * Return the count for this quintuple if there, NULL otherwise.
 *
 * @param hash[out] If non-null, return the entry hash (undefined if
 * the entry is not found).
 * @return The count for this quintuple if there, NULL otherwise.
 */
inline Count_bin *
table_lookup(count_context_t *ctxt, int lw, int rw,
             const Connector *le, const Connector *re,
             unsigned int null_count, size_t *hash)
{

	int l_id = (NULL != le) ? le->tracon_id : lw;
	int r_id = (NULL != re) ? re->tracon_id : rw;
	size_t h = pair_hash(lw, rw, l_id, r_id, null_count);
	Table_tracon *t = ctxt->table[h & ctxt->table_mask];

	if (!USE_TABLE_TRACON && (hash != NULL))
	{
		*hash = h;
		return NULL;
	}

	for (; t != NULL; t = t->next)
	{
		if ((t->l_id == l_id) && (t->r_id == r_id) &&
		    (t->null_count == null_count))
		{
			TABLE_STAT(hit++);
			return &t->count;
		}
	}
	TABLE_STAT(miss++);

	if (hash != NULL) *hash = h;
	TABLE_STAT(miss++);
	return NULL;
}

extern  Count_bin *
table_lookup(count_context_t *, int, int,
             const Connector *, const Connector *,
             unsigned int, size_t *);

/**
 * Generate a word skip vector.
 * This implements an idea similar to the Boyer-Moore algo for searches.
 */
static void generate_word_skip_vector(count_context_t *ctxt, wordvecp wv,
                                      Connector *le, Connector *re,
                                      int start_word, int end_word,
                                      int lw, int rw)
{
	if (!ENABLE_WORD_SKIP_VECTOR) return;

	if (le != NULL)
	{
		int check_word = start_word;
		int i;

		if (wv == NULL) wv = ctxt->table_lrcnt[0].tracon_wvp[le->tracon_id];
		unsigned int sent_nc = ctxt->sent->null_count;
		for (i = start_word + 1; i < end_word; i++)
		{
			wordvecp e = &wv[i - le->nearest_word];
			e->check_next = INCREMENT_WORD;
			if((e->status != 0) || (sent_nc > e->null_count))
			{
				wv[check_word - le->nearest_word].check_next = i;
				check_word = i;
			}
		}
		if (check_word <= end_word - 1)
			wv[check_word - le->nearest_word].check_next = end_word;

#if 0
		printf("id %d w(%3d, %3d), se(%3d, %3d) sent_nc %u size %d\n",
		       le->tracon_id, lw, rw, start_word, end_word,
		       ctxt->sent->null_count, le->farthest_word-le->nearest_word+1);
		for (i = start_word; i < end_word; i++)
		{
			Table_lrcnt *e = &wv[i - le->nearest_word];
			printf("\tw%-3d idx %-3d status %d nc %-3u next %d\n",
			       i,  i - le->nearest_word, e->status, e->null_count,
			       e->check_next);
		}
#endif
	}
	else
	{
		int check_word = start_word;
		int i;

		if (wv == NULL) wv = ctxt->table_lrcnt[1].tracon_wvp[re->tracon_id];
		unsigned int sent_nc = ctxt->sent->null_count;
		for (i = start_word + 1; i < end_word; i++)
		{
			wordvecp e = &wv[i - re->farthest_word];
			e->check_next = INCREMENT_WORD;
			if((e->status != 0) || (sent_nc > e->null_count))
			{
				wv[check_word - re->farthest_word].check_next = i;
				check_word = i;
			}
		}
		if (check_word <= end_word - 1)
			wv[check_word - re->farthest_word].check_next = end_word;

#if 0
		printf("id %d w(%3d, %3d), se(%3d, %3d) sent_nc %u size %d\n",
		       le->tracon_id, lw, rw, start_word, end_word,
		       ctxt->sent->null_count, re->nearest_word-re->farthest_word+1);
		for (i = start_word; i < end_word; i++)
		{
			Table_lrcnt *e = &wv[i - re->farthest_word];
			printf("\tw%-3d idx %-3d status %d nc %-3u next %d\n",
			       i,  i - re->farthest_word, e->status, e->null_count,
			       e->check_next);
		}
#endif
	}
}

/**
 *  Clamp the given count to INT_MAX.
 *  The returned value is normally unused by the callers
 *  (to be used when debugging overflows).
 */
static bool parse_count_clamp(w_Count_bin *total)
{
	if (INT_MAX < hist_total(total))
	{
		/*  Sigh. Overflows can and do occur, esp for the ANY language. */
#if PERFORM_COUNT_HISTOGRAMMING
		total->total = INT_MAX;
#else
		*total = INT_MAX;
#endif /* PERFORM_COUNT_HISTOGRAMMING */

		return true;
	}

	return false;
}

static void lrcnt_keep_count(wordvecp lrcnt_cache, bool dir, Disjunct *d,
                             w_Count_bin leftcount, w_Count_bin rightcount)
{
#if PERFORM_COUNT_HISTOGRAMMING
	return; /* Not supported. */
#endif
	w_Count_bin count = dir ? rightcount : leftcount;
	parse_count_clamp(&count);
	d->lrcount = (Count_bin)count;
}

/**
 * Cache the match list elements that yield a non-zero count.
 */
static void lrcnt_cache_match_list(wordvecp lrcnt_cache, count_context_t *ctxt,
                                   size_t mlb, bool dir)
{
	size_t dcnt = 0;
	size_t i  = 0;
	fast_matcher_t *mchxt = ctxt->mchxt;

	if (!ENABLE_MATCH_LIST_CACHE) return;

	for (i = mlb; get_match_list_element(mchxt, i) != NULL; i++)
	{
		Disjunct *d = get_match_list_element(mchxt, i);
		dcnt += (int)(dir ? d->match_right : d->match_left);
	}
	dassert(dcnt > 0, "No disjuncts to cache");
	//if (dcnt == i-mlb) return NULL;
	lgdebug(+9, "MATCH_LIST %9d dir=%d mlb %zu cached %zu/%zu\n",
	        get_match_list_element(mchxt, mlb)->match_id, dir, mlb, dcnt, i-mlb);

	match_list_cache *ml = pool_alloc_vec(ctxt->mlc_pool, dcnt + 1);
	if (ml == NULL) return; /* Cannot allocate cache array */

	dcnt = 0;
	for (i = mlb; get_match_list_element(mchxt, i) != NULL; i++)
	{
		Disjunct *d = get_match_list_element(mchxt, i);
		if ((dir == 0) ? d->match_left : d->match_right)
		{
			ml[dcnt].d = d;
			assert(d->lrcount > 0, "Invalid linkage count %d", d->lrcount);
			ml[dcnt].count = d->lrcount;
			dcnt++;
		}
	}
	ml[dcnt].d = NULL;

	lrcnt_cache->mlc0 = ml;
}

/**
 * lrcnt cache entry inspection:
 * Does this entry indicate a nonzero leftcount / rightcount?
 *
 * @param wv Word vector element
 * @param null_count The current null-count to check.
 *
 * Return these values:
 * @param null_start[out] First null count to check (the previous ones can be
 * skipped because the cache indicates they yield a zero count.)
 * @return Cache entry for the given range. Possible values:
 *    NULL - A nonzero count may be encountered for \c null_count>=null_start.
 *    lrcnt_cache_zero - A zero count would result.
 *    Cache pointer - An update for \c null_count>=null_start is needed.
 */
static wordvecp lrcnt_check(wordvecp wvp, unsigned int null_count,
                                unsigned int *null_start)
{
	/* Below, several checks are done on the cache. The function returns
	 * on the first one that succeeds. */

	if (wvp->status == -1)
	{
		if (null_start != NULL) *null_start = 0;
		return wvp; /* Needs update */
	}

	if  (wvp->status == 1)
	{
		/* The range yields a nonzero leftcount/rightcount for some
		 * null_count. But we can still skip the initial null counts. */
		if (null_start != NULL)
			*null_start = (null_count_m)(wvp->null_count + 1);
		return NULL; /* No update needed - this is a permanent status */
	}

	if (null_count <= wvp->null_count)
	{
		/* Here (wvp->status == 0) which means our count would
		 * be zero - so nothing to do for this word. */
		return &lrcnt_cache_zero;
	}

	/* The null counts greater than wvp->null_count have not
	 * been handled yet for the given range (the cache will get
	 * updated for them by lrcnt_expectation_update()). */
	if (null_start == NULL) return NULL;
	*null_start = wvp->null_count + 1;
	return wvp;
}

static wordvecp *get_lrcnt_wvpa(count_context_t *ctxt, Connector *le,
                                Connector *re)
{
	if (ctxt->is_short) return NULL;

	int dir = (int)(le == NULL);
	int tracon_id = (le == NULL) ? re->tracon_id : le->tracon_id;

	return &ctxt->table_lrcnt[dir].tracon_wvp[tracon_id];
}

static wordvecp alloc_lrcnt_wv(count_context_t *ctxt, wordvecp *wvp,
                               Connector *le, Connector *re)
{
	if (*wvp == NULL)
	{
		Connector *c = (le == NULL) ? re : le;
		/* Create a cache entry, to be updated by lrcnt_expectation_update(). */
		const size_t wordvec_size = abs(c->farthest_word - c->nearest_word) + 1;
		*wvp = pool_alloc_vec(ctxt->sent->wordvec_pool, wordvec_size);
		/* FIXME: Eliminate the need to initialize these fields with -1. */
		for (size_t i = 0; i < wordvec_size; i++)
		{
			(*wvp)[i].status = -1;
			(*wvp)[i].null_count = -1;
			(*wvp)[i].check_next = -1;
		}
	}

	return *wvp;
}

bool no_count(count_context_t *ctxt, int dir, Connector *c,
              unsigned int wordvec_index, unsigned int null_count)
{
	if (ctxt->is_short) return false;

	wordvecp wvp = ctxt->table_lrcnt[dir].tracon_wvp[c->tracon_id];
	if (wvp == NULL) return false;
	wordvecp lrcnt_cache = &wvp[wordvec_index];

	return (lrcnt_check(lrcnt_cache, null_count, NULL) == &lrcnt_cache_zero);
}

match_list_cache *get_cached_match_list(count_context_t *ctxt, int dir, int w,
                                        Connector *c)
{
	if (ctxt->sent->null_count != 0) return NULL;
	if (ctxt->is_short) return NULL;

	wordvecp wv = ctxt->table_lrcnt[dir].tracon_wvp[c->tracon_id];
	if (wv == NULL) return NULL;

	return wv[w - ((dir == 0) ? c->nearest_word : c->farthest_word)].mlc0;
}

static bool lrcnt_expectation_update(wordvecp wv, bool lrcnt_found,
                                     bool match_list, unsigned int null_count)
{
	bool lrcnt_status_changed = (wv->status != (int)lrcnt_found);
	unsigned int prev_null_count = wv->null_count;

	if (!lrcnt_found)
		wv->null_count = match_list ? null_count : ANY_NULL_COUNT;
	wv->status = (int)lrcnt_found;

	return lrcnt_status_changed || (prev_null_count != wv->null_count);
}

static bool is_panic(count_context_t *ctxt)
{
	/* Panic mode: Return a parse bypass indication if resources are
	 * exhausted.  checktimer is a device to avoid a gazillion system calls
	 * to get the timer value. On circa-2018 machines, it results in
	 * several timer calls per second. */
	if (ctxt->exhausted) return true;
	ctxt->checktimer++;
	if (((0 == ctxt->checktimer%(1<<18)) && (ctxt->current_resources != NULL) &&
	     //fprintf(stderr, "T") &&
	     resources_exhausted(ctxt->current_resources)))
	{
		ctxt->exhausted = true;
		return true;
	}

	return false;
}

#define NO_COUNT -1
#if PERFORM_COUNT_HISTOGRAMMING
#define INIT_NO_COUNT (Count_bin){.total = NO_COUNT}
#else
#define INIT_NO_COUNT NO_COUNT
#endif
Count_bin count_unknown = INIT_NO_COUNT;

/**
 * psuedocount is used to check to see if a parse is even possible,
 * so that we don't waste CPU time performing an actual count, only
 * to discover that it is zero.
 *
 * A table entry with a count 0 indicates that the parse is not possible.
 * In that case we can skip parsing. If it is not 0, it is the parse
 * result and we can use it and skip parsing too.
 * However, if an entry is not in the hash table, we have to assume the
 * worst case: that the count might be non-zero.  To indicate that case,
 * return the special sentinel value \c count_unknown.
 */
static Count_bin pseudocount(count_context_t * ctxt,
                           int lw, int rw, Connector *le, Connector *re,
                           unsigned int null_count)
{
	/* This check is not necessary for correctness, but it saves CPU time.
	 * If a cross link would result, immediately return 0. Note that there is
	 * no need to check here if the nearest_word fields are in the range
	 * [lw, rw] due to the way start_word/end_word are computed, and due to
	 * nearest_word checks in form_match_list(). */
	if ((le != NULL) && (re != NULL) && (le->nearest_word > re->nearest_word))
		return hist_zero();

	if (!USE_TABLE_TRACON) return count_unknown;

	Count_bin *count = table_lookup(ctxt, lw, rw, le, re, null_count, NULL);
	if (NULL == count) return count_unknown;
	return *count;
}

/**
 * Return the number of optional words strictly between w1 and w2.
 */
static int num_optional_words(count_context_t *ctxt, int w1, int w2)
{
	int n = 0;

	for (int w = w1+1; w < w2; w++)
		if (ctxt->sent->word[w].optional) n++;

	return n;
}

#ifdef DEBUG
#define DO_COUNT_TRACE
#endif

#ifdef DO_COUNT_TRACE
#define D_COUNT_TRACE 8
#define LBLSZ 11
#define TRACE_LABEL(l, do_count) \
	(verbosity_level(D_COUNT_TRACE, "do_count") ? \
	 prt_error("%-*s", LBLSZ, STRINGIFY(l)) : 0, do_count)
#define V(c) (!c?"(nil)":connector_string(c))
#define ID(c,w) (!c?w:c->tracon_id)
static Count_bin do_count1(int lineno, count_context_t *ctxt,
                         int lw, int rw,
                         Connector *le, Connector *re,
                         unsigned int null_count);

static Count_bin do_count(int lineno, count_context_t *ctxt,
                        int lw, int rw,
                        Connector *le, Connector *re,
                        unsigned int null_count)
{
	static int level;

	if (!verbosity_level(D_COUNT_TRACE))
		return do_count1(lineno, ctxt, lw, rw, le, re, null_count);

	Count_bin *c = table_lookup(ctxt, lw, rw, le, re, null_count, NULL);
	char m_result[64] = "";
	if (c != NULL)
		snprintf(m_result, sizeof(m_result), "(M=%"COUNT_FMT")", hist_total(c));

	level++;
	prt_error("%*sdo_count%s:%d lw=%d rw=%d le=%s(%d) re=%s(%d) null_count=%u\n\\",
		level*2, "", m_result, lineno, lw, rw, V(le),ID(le,lw), V(re),ID(re,rw), null_count);
	Count_bin r = do_count1(lineno, ctxt, lw, rw, le, re, null_count);
	prt_error("%*sreturn%.*s:%d=%"COUNT_FMT"\n",
	          LBLSZ+level*2, "", (!!c)*3, "(M)", lineno, hist_total(&r));
	level--;

	return r;
}

/*
 * See do_parse() for the purpose of this function.
 *
 * The returned number of parses (called "count" here) is a 32-bit
 * integer. However, this count may sometimes be very big - much larger than
 * can be represented in 32-bits. In such a case, it is just enough to know
 * that such an "overflow" occurred. Internally, large counts are clamped to
 * INT_MAX (2^31-1) - see parse_count_clamp() (we refer below to such
 * values as "clamped"). If the top-level do_count() (the one that is
 * called from do_parse()) returns this value, it means that an overflow
 * has occurred.
 *
 * The function uses a 64-bit signed integer as a count accumulator - named
 * "total". The maximum value it can hold is 2^63-1. If it becomes greater
 * than INT_MAX, it is considered as a count overflow. Care must be
 * taken that this total itself does not overflow, else this detection
 * mechanism would malfunction. To that end, each value from which
 * this total is computed must be small enough so it does not overflow.
 *
 * The function has 4 code sections to calculate the count. Each of them,
 * when entered, returns a value which is clamped (or doesn't need to be
 * clamped). The are marked in the code with "Path 1a", "Path 1b",
 * "Path 2", and "Path 3".
 *
 * Path 1a, Path 1b: If there is a possible linkage between the given
 * words, return 1, else return 0. Here, a count overflow cannot occur.
 *
 * Path 2: The total accumulates the result of the do_count() invocations
 * that are done in a loop. The upper bound on the number of iterations is
 * twice (outer loop) the maximum number of word disjuncts (inner loop).
 * Assuming no more than 2^31 disjuncts per word, and considering that
 * each value is a result of do_count() which is clamped, the total is
 * less than (2*2^31)*(2^31`-1), which is less than 2^63-1, and hence just
 * needs to be clamped before returning.
 *
 * Path 3: The total is calculated as a sum of series of multiplications.
 * To prevent its overflow, we ensure that each term (including the total
 * itself) would not be greater than INT_MAX (2^31-1). Then the result will
 * not be more than (2^31-1)+((2^31-1)*(2^31-1)), which is less than
 * 2^63-1. In this path, each multiplication term that may be greater then
 * INT_MAX (leftcount and rightcount) is clamped before the
 * multiplication, and the total is clamped after the multiplication.
 * Multiplication terms that result from caching (or directly from
 * do_count()) are already clamped.
 */

#define do_count do_count1
#else
#define TRACE_LABEL(l, do_count) (do_count)
#endif /* DO_COUNT TRACE */
static Count_bin do_count(
#ifdef DO_COUNT_TRACE
#undef do_count
#define do_count(...) do_count(__LINE__, __VA_ARGS__)
                          int lineno,
#endif
                          count_context_t *ctxt,
                          int lw, int rw,
                          Connector *le, Connector *re,
                          unsigned int null_count)
{
	w_Count_bin total = hist_zero();
	int start_word, end_word, w;

	/* This check is not necessary for correctness. It typically saves
	 * significant CPU and Table_tracon memory because in many cases a
	 * linkage is not possible due to nearest_word restrictions. */
	if (!valid_nearest_words(le, re, lw, rw)) return hist_zero();

	if (is_panic(ctxt)) return hist_zero();

	/* TODO: static_assert() that null_count is an unsigned int. */
	assert (null_count < INT_MAX, "Bad null count %d", (int)null_count);

	size_t h = 0; /* Initialized value only needed to prevent a warning. */
	{
		Count_bin* const c = table_lookup(ctxt, lw, rw, le, re, null_count, &h);
		if (c != NULL) return *c;
		/* The table entry is to be updated with the found linkage count
		 * before we return. The hash h will be used to locate it. */
	}

	unsigned int unparseable_len = rw-lw-1;

	/* Path 1a. */

#if 1
	/* This check is not necessary for correctness, as it is handled in
	 * the general case below. It looks like it should be slightly faster. */
	if (unparseable_len == 0)
	{
		/* lw and rw are neighboring words */
		/* You can't have a linkage here with null_count > 0 */
		if ((le == NULL) && (re == NULL) && (null_count == 0))
			return table_store(ctxt, lw, rw, le, re, null_count, h, hist_one());

		return table_store(ctxt, lw, rw, le, re, null_count, h, hist_zero());
	}
#endif


	/* The left and right connectors are null, but the two words are
	 * NOT next to each-other. */
	if ((le == NULL) && (re == NULL))
	{
		int nopt_words = num_optional_words(ctxt, lw, rw);

		/* Path 1b. */

		if ((null_count == 0) ||
		    (!ctxt->islands_ok && (lw != -1) && (ctxt->sent->word[lw].d != NULL)))
		{
			/* The null_count of skipping n words is just n.
			 * In case the unparsable range contains optional words, we
			 * don't know here how many of them are actually skipped, because
			 * they may belong to different alternatives and essentially just
			 * be ignored.  Hence the inequality - sane_linkage_morphism()
			 * will discard the linkages with extra null words. */
			if ((null_count <= unparseable_len) &&
			    (null_count >= unparseable_len - nopt_words))
				return table_store(ctxt, lw, rw, le, re, null_count, h, hist_one());

			return table_store(ctxt, lw, rw, le, re, null_count, h, hist_zero());
		}

		/* Path 2. */

		/* Here null_count != 0 and we allow islands (a set of words
		 * linked together but separate from the rest of the sentence).
		 * Because we don't know here if an optional word is just
		 * skipped or is a real null-word (see the comment above) we
		 * try both possibilities: If a real null is encountered, the
		 * rest of the sentence must contain one less null-word. Else
		 * the rest of the sentence still contains the required number
		 * of null words. */

		/* total (w_Count_bin which is int64_t) cannot overflow in this
		 * loop since the number of disjuncts in the inner loop is
		 * surely < 2^31, the outer loop can be iterated at most twice,
		 * and do_count() may return at most 2^31-1. However, it may
		 * become > 2^31-1 and hence needs to be clamped after the loop. */
		w = lw + 1;
		for (int opt = 0; opt <= (int)ctxt->sent->word[w].optional; opt++)
		{
			unsigned int try_null_count = null_count + opt;

			for (Disjunct *d = ctxt->sent->word[w].d; d != NULL; d = d->next)
			{
				if (d->left == NULL)
				{
					hist_accumv(&total, d->cost,
						do_count(ctxt, w, rw, d->right, NULL, try_null_count-1));
				}
			}

			hist_accumv(&total, 0.0,
				do_count(ctxt, w, rw, NULL, NULL, try_null_count-1));
		}

		if (parse_count_clamp(&total))
		{
#if 0
			printf("OVERFLOW 1\n");
#endif
		}
		return table_store(ctxt, lw, rw, le, re, null_count, h, total);
	}

	/* Path 3. */

	/* The word range (lw, rw) gets split in all tentatively possible ways
	 * to LHS term and RHS term.
	 * There can be a total count > 0 only if one of the following
	 * multiplications results in a value > 0. This in turn is possible
	 * only if both multiplication terms > 0:
	 *    LHS term    RHS term
	 * 1. leftcount x rightcount
	 * 2. leftcount x l_bnr
	 * 3. r_bnl     x rightcount
	 *
	 * In addition:
	 * - leftcount > 0   is possible only if there is match_left.
	 * - rightcount > 0  is possible only if there is match_right.
	 * - r_bnl > 0       is possible only if le==NULL.
	 *
	 * Since the result count is a sum of multiplications of the LHS and
	 * RHS counts, if one of them is zero, we can skip calculating the
	 * other one.
	 */

	if (le == NULL)
	{
		/* No leftcount, and no rightcount for (w < re->farthest_word). */
		start_word = MAX(lw+1, re->farthest_word);
	}
	else
	{
		/* The following cannot be optimized like end_word due to l_bnr. */
		start_word = le->nearest_word;
	}

	if (re == NULL)
	{
		/* No rightcount, and no leftcount for (w > le->farthest_word). */
		end_word = MIN(rw, le->farthest_word+1);
	}
	else
	{
		/* If the LHS count for a word would be zero for a left-end connector
		 * due to the distance of this word, we can skip its handling
		 * entirely. So the checked word interval can be shortened. */
		if ((le != NULL) && (re->nearest_word > le->farthest_word))
			end_word = le->farthest_word + 1;
		else
			end_word = re->nearest_word + 1;
	}

	fast_matcher_t *mchxt = ctxt->mchxt;
	bool lrcnt_cache_changed = false;
	int next_word = MAX_SENTENCE;

	/* Select the table and word-vector offset for word skipping. */
	wordvecp wvp = NULL;
	int woffset = 0;
	if (!ctxt->is_short)
	{
		wordvecp *wvpa = get_lrcnt_wvpa(ctxt, le, re);
		wvp = alloc_lrcnt_wv(ctxt, wvpa, le, re);
		woffset = (le == NULL) ? re->farthest_word : le->nearest_word;
	}

	for (w = start_word; w < end_word; w = next_word)
	{
		COUNT_COST(ctxt->count_cost[0]++;)

		/* Start of nonzero leftcount/rightcount range cache check. It is
		 * extremely effective for long sentences, but doesn't speed up
		 * short ones.
		 *
		 * FIXME: l/rcnt_optimize==false doubles the Table_tracon cache!
		 * Try to fix it by not caching zero counts in Table_tracon when
		 * l/rcnt_optimize==false. If this can be fixed, a significant
		 * speedup is expected. */

		wordvecp lrcnt_cache = NULL;
		bool lrcnt_found = false;     /* TRUE iff a range yielded l/r count */
		bool lcnt_optimize = true;   /* Perform left count optimization */
		bool rcnt_optimize = true;   /* Perform right count optimization */
		unsigned int lnull_start = 0; /* First null_count to check */
		unsigned int lnull_end = null_count; /* Last null_count to check */
		Connector *fml_re = re;       /* For form_match_list() only */
		match_list_cache *mlcl = NULL, *mlcr = NULL;
		bool using_cached_match_list = false;
		unsigned int lcount_index = 0; /* Cached left count index */

		if (ctxt->is_short)
		{
			next_word = w + 1;
		}
		else
		{
			lrcnt_cache = &wvp[w - woffset];

			/* Use the word-skip vector to skip over words that are
			 * known to yield a zero count. */
			next_word = lrcnt_cache->check_next;
			if (next_word == INCREMENT_WORD) next_word = w + 1;

			if (le != NULL)
			{
				lrcnt_cache = lrcnt_check(lrcnt_cache, null_count, &lnull_start);
				if (lrcnt_cache == &lrcnt_cache_zero) continue;

				if (lrcnt_cache != NULL)
				{
					lcnt_optimize = false;
				}

				if ((re != NULL) && (re->farthest_word <= w))
				{
					/* If it is already known that "re" would yield a zero
					 * rightcount, there is no need to fetch the right match list.
					 * The code below will still check for possible l_bnr counts. */
					if (no_count(ctxt, 1, re, w - re->farthest_word, null_count))
						fml_re = NULL;
				}
			}
			else
			{
				/* Here re != NULL. */
				unsigned int rnull_start;
				lrcnt_cache = lrcnt_check(lrcnt_cache, null_count, &rnull_start);
				if (lrcnt_cache == &lrcnt_cache_zero) continue;

				if (lrcnt_cache != NULL)
				{
					rcnt_optimize = false;
					if (rnull_start <= null_count)
						lnull_end -= rnull_start;
				}
			}
			/* End of nonzero leftcount/rightcount range cache check. */

			if ((lrcnt_cache == NULL) && (ctxt->sent->null_count == 0))
			{
				using_cached_match_list = true;

				if (le != NULL)
				{
					mlcl = get_cached_match_list(ctxt, 0, w, le);
				}
				if (fml_re != NULL && ((le == NULL) || (re->farthest_word <= w)))
				{
					mlcr = get_cached_match_list(ctxt, 1, w, re);
				}
			}
		}

		size_t mlb = form_match_list(mchxt, w, le, lw, fml_re, rw, mlcl, mlcr);

#ifdef VERIFY_MATCH_LIST
		Disjunct *od = get_match_list_element(mchxt, mlb);
		uint16_t id = od ? od->match_id : 0;
#endif

		for (size_t mle = mlb; get_match_list_element(mchxt, mle) != NULL; mle++)
		{
			COUNT_COST(ctxt->count_cost[1]++;)

			Disjunct *d = get_match_list_element(mchxt, mle);
#ifdef VERIFY_MATCH_LIST
			assert(id == d->match_id, "Modified id (%u!=%u)", id, d->match_id);
#endif
			bool Lmatch = d->match_left;
			bool Rmatch = d->match_right;
			w_Count_bin leftcount = NO_COUNT;
			w_Count_bin rightcount = NO_COUNT;
			bool leftpcount = false;
			bool rightpcount = false;

			d->match_left = d->match_right = false;

			if (using_cached_match_list)
			{
				/* If a cached left match list is used, form_match_list() always
				 * uses all of its disjuncts. But if a cached right match list
				 * is used and le!=0, form_match_list() omits its disjuncts that
				 * don't have a left match (see "lc optimization" there)).
				 * Since the left/right cached count elements correspond to the
				 * full cached match lists (see count_expectation), an
				 * incremental counter can be used for the cached left count,
				 * but for the cached right count we depend on form_match_list()
				 * to set d->rcount_index as the index into the corresponding
				 * cached right count array. */
				if (Lmatch && (mlcl != NULL))
				{
					leftpcount = true;
					leftcount = mlcl[lcount_index++].count;
				}
				if (Rmatch && (mlcr != NULL))
				{
					rightpcount = true;
					rightcount = mlcr[d->rcount_index].count;
				}
			}

			for (unsigned int lnull_cnt = lnull_start; lnull_cnt <= lnull_end; lnull_cnt++)
			{
				COUNT_COST(ctxt->count_cost[2]++;)

				int rnull_cnt = null_count - lnull_cnt;
				/* Now lnull_cnt and rnull_cnt are the null-counts we're
				 * requiring in those parts respectively. */

				if (!using_cached_match_list)
				{
					leftcount = NO_COUNT;
					rightcount = NO_COUNT;
					leftpcount = false;
					rightpcount = false;
				}

				Count_bin l_any = INIT_NO_COUNT;
				Count_bin r_any = INIT_NO_COUNT;
				Count_bin l_cmulti = INIT_NO_COUNT;
				Count_bin l_dmulti = INIT_NO_COUNT;
				Count_bin l_dcmulti = INIT_NO_COUNT;
				Count_bin l_bnr = INIT_NO_COUNT;
				Count_bin r_cmulti = INIT_NO_COUNT;
				Count_bin r_dmulti = INIT_NO_COUNT;
				Count_bin r_dcmulti = INIT_NO_COUNT;
				Count_bin r_bnl = (le == NULL) ? INIT_NO_COUNT : hist_zero();

				/* Now, we determine if (based on table only) we can see that
				   the current range is not parsable. */

				/* The result count is a sum of multiplications of
				 * LHS and RHS counts. If one of them is zero, we can skip
				 * calculating the other one.
				 *
				 * So, first perform pseudocounting as an optimization. If
				 * the pseudocount is zero, then we know that the true
				 * count will be zero.
				 *
				 * Cache the result in the l_* and r_* variables, so a table
				 * lookup can be skipped in cases we cannot skip the actual
				 * calculation and a table entry exists. */
				if (Lmatch && !leftpcount)
				{
					l_any = pseudocount(ctxt, lw, w, le->next, d->left->next, lnull_cnt);
					leftpcount = (hist_total(&l_any) != 0);
					if (!leftpcount && le->multi)
					{
						l_cmulti =
							pseudocount(ctxt, lw, w, le, d->left->next, lnull_cnt);
						leftpcount |= (hist_total(&l_cmulti) != 0);
					}
					if (!leftpcount && d->left->multi)
					{
						l_dmulti =
							pseudocount(ctxt, lw, w, le->next, d->left, lnull_cnt);
						leftpcount |= (hist_total(&l_dmulti) != 0);
					}
					if (!leftpcount && le->multi && d->left->multi)
					{
						l_dcmulti =
							pseudocount(ctxt, lw, w, le, d->left, lnull_cnt);
						leftpcount |= (hist_total(&l_dcmulti) != 0);
					}
				}

				if (Rmatch && !rightpcount && (leftpcount || (le == NULL)))
				{
					r_any = pseudocount(ctxt, w, rw, d->right->next, re->next, rnull_cnt);
					rightpcount = (hist_total(&r_any) != 0);
					if (!rightpcount && re->multi)
					{
						r_cmulti =
							pseudocount(ctxt, w, rw, d->right->next, re, rnull_cnt);
						rightpcount |= (hist_total(&r_cmulti) != 0);
					}
					if (!rightpcount && d->right->multi)
					{
						r_dmulti =
							pseudocount(ctxt, w,rw, d->right, re->next, rnull_cnt);
						rightpcount |= (hist_total(&r_dmulti) != 0);
					}
					if (!rightpcount && d->right->multi && re->multi)
					{
						r_dcmulti =
							pseudocount(ctxt, w, rw, d->right, re, rnull_cnt);
						rightpcount |= (hist_total(&r_dcmulti) != 0);
					}
				}

				/* Perform a table lookup for a possible cyclic solution. */
				if (leftpcount)
				{
					l_bnr = pseudocount(ctxt, w, rw, d->right, re, rnull_cnt);
				}
				else
				{
					if (!rightpcount) continue; /* Left and right counts are 0. */
					if (le == NULL)
					{
						r_bnl = pseudocount(ctxt, lw, w, le, d->left, lnull_cnt);
					}
				}

#define CACHE_COUNT(c, how_to_count, do_count) \
	{ \
		Count_bin count = (hist_total(&c) == NO_COUNT) ? \
			TRACE_LABEL(c, do_count) : c; \
		how_to_count; \
	}
				/* If the pseudocounting above indicates one of the terms
				 * in the count multiplication is zero,
				 * we know that the true total is zero. So we don't
				 * bother counting the other term at all, in that case. */

				/* To enable 31-bit overflow detection, `total`, `leftcount`
				 * and `rightcount` are signed 64-bit, and are clamped cached
				 * values, or are clamped below before they are used.  `total`
				 * is initially 0 and is clamped at the end of each iteration.
				 * So the result will never be more than
				 * (2^31-1)+((2^31-1)*(2^31-1)),
				 * which is less than 2^63-1. */
				if (leftpcount &&
				    (!lcnt_optimize || rightpcount || (0 != hist_total(&l_bnr))))
				{
					if (hist_total(&leftcount) == NO_COUNT)
					{
						CACHE_COUNT(l_any, leftcount = count,
							do_count(ctxt, lw, w, le->next, d->left->next, lnull_cnt));
						if (le->multi)
							CACHE_COUNT(l_cmulti, hist_accumv(&leftcount, d->cost, count),
								do_count(ctxt, lw, w, le, d->left->next, lnull_cnt));
						if (d->left->multi)
							CACHE_COUNT(l_dmulti, hist_accumv(&leftcount, d->cost, count),
								do_count(ctxt, lw, w, le->next, d->left, lnull_cnt));
						if (d->left->multi && le->multi)
							CACHE_COUNT(l_dcmulti, hist_accumv(&leftcount, d->cost, count),
								do_count(ctxt, lw, w, le, d->left, lnull_cnt));
					}

					if (0 < hist_total(&leftcount))
					{
						parse_count_clamp(&leftcount); /* May be up to 4*2^31. */
						lrcnt_found = true;
						d->match_left = true;

						/* Evaluate using the left match, but not the right */
						CACHE_COUNT(l_bnr, hist_muladdv(&total, &leftcount, d->cost, count),
							do_count(ctxt, w, rw, d->right, re, rnull_cnt));
					}
				}

				if (rightpcount &&
				    (!rcnt_optimize || (0 < hist_total(&leftcount)) || (0 != hist_total(&r_bnl))))
				{
					if (hist_total(&rightcount) == NO_COUNT)
					{
						CACHE_COUNT(r_any, rightcount = count,
							do_count(ctxt, w, rw, d->right->next, re->next, rnull_cnt));
						if (re->multi)
							CACHE_COUNT(r_cmulti, hist_accumv(&rightcount, d->cost, count),
								do_count(ctxt, w, rw, d->right->next, re, rnull_cnt));
						if (d->right->multi)
							CACHE_COUNT(r_dmulti, hist_accumv(&rightcount, d->cost, count),
								do_count(ctxt, w, rw, d->right, re->next, rnull_cnt));
						if (d->right->multi && re->multi)
							CACHE_COUNT(r_dcmulti, hist_accumv(&rightcount, d->cost, count),
								do_count(ctxt, w, rw, d->right, re, rnull_cnt));
					}

					if (0 < hist_total(&rightcount))
					{
						parse_count_clamp(&rightcount); /* May be up to 4*2^31. */
						if (le == NULL)
						{
							lrcnt_found = true;
							d->match_right = true;

							/* Evaluate using the right match, but not the left */
							CACHE_COUNT(r_bnl, hist_muladdv(&total, &rightcount, d->cost, count),
								do_count(ctxt, lw, w, le, d->left, lnull_cnt));
						}
						else
						{
							/* Total number where links are used on both side.
							 * Note that we don't have leftcount if le == NULL. */
							hist_muladd(&total, &leftcount, 0.0, &rightcount);
						}
					}
				}

				parse_count_clamp(&total);
			}

			if ((lrcnt_cache != NULL) && (d->match_left || d->match_right) &&
			    (ctxt->sent->null_count == 0))
			{
				lrcnt_keep_count(lrcnt_cache, /*dir*/le == NULL, d, leftcount,
				                 rightcount);
			}
		}

		if (lrcnt_cache != NULL)
		{
			bool match_list = (get_match_list_element(mchxt, mlb) != NULL);
			if (lrcnt_expectation_update(lrcnt_cache, lrcnt_found, match_list,
			                             null_count))
			{
				lrcnt_cache_changed = true;
			}
			if (lrcnt_found && (ctxt->sent->null_count == 0))
				lrcnt_cache_match_list(lrcnt_cache, ctxt, mlb, /*dir*/le == NULL);
		}

		pop_match_list(mchxt, mlb);
	}

	if (lrcnt_cache_changed)
		generate_word_skip_vector(ctxt, wvp, le, re, start_word, end_word, lw, rw);

	return table_store(ctxt, lw, rw, le, re, null_count, h, total);
}

/**
 * Returns the number of ways the sentence can be parsed with the
 * specified null count. Assumes that the fast-matcher and the count
 * context have already been initialized, and will be freed later. The
 * "null_count" argument is the number of words that are allowed to
 * have no links to them.
 *
 * This the full-fledged parser, but it only 'counts', in order to
 * avoid an explosion of allocated memory structures to hold each
 * possible parse.  Thus, to see an 'actual' parse, a second pass
 * must be made, with build_parse_set(), to get actual parse structures.
 *
 * The work is split up this way for two reasons:
 * 1) A given sentence may have thousands of parses, and the user is
 *    interested in only a few.
 * 2) A given sentence may have billions of parses, in which case,
 *    allocating for each would blow out RAM.
 * So, basically, its good to know how many parses to expect, before
 * starting to allocate parse structures.
 *
 * The count returned here is meant to be completely accurate; it is
 * not an approximation!
 *
 * Currently, the code has been designed to maintain a histogram of
 * the cost of each of the parses. The number and width of the bins
 * is adjustable in histogram.c. At this time, the histogram is not
 * used anywhere, and a 3-5% speedup is available if it is avoided.
 * We plan to use this histogram, later ....
 */
int do_parse(Sentence sent, fast_matcher_t *mchxt, count_context_t *ctxt,
             Parse_Options opts)
{
	Count_bin hist;

	ctxt->current_resources = opts->resources;
	ctxt->exhausted = false;
	ctxt->checktimer = 0;
	ctxt->islands_ok = opts->islands_ok;
	ctxt->mchxt = mchxt;

	hist = do_count(ctxt, -1, sent->length, NULL, NULL, sent->null_count+1);

	table_stat(ctxt);
	return (int)hist_total(&hist);
}

/* sent_length is used only as a hint for the hash table size ... */
count_context_t * alloc_count_context(Sentence sent, Tracon_sharing *ts)
{
	count_context_t *ctxt = malloc (sizeof(count_context_t));
	memset(ctxt, 0, sizeof(count_context_t));

	ctxt->sent = sent;
	ctxt->is_short = !ENABLE_TABLE_LRCNT ||
		((sent->length <= min_len_word_vector) && !IS_GENERATION(ctxt->sent->dict));

	if (!ctxt->is_short)
	{
		/* next_id keeps the last tracon_id used, so we need +1 for array size.  */
		for (unsigned int dir = 0; dir < 2; dir++)
			ctxt->table_lrcnt[dir].num_tracon_id = ts->next_id[!dir] + 1;

		init_table_lrcnt(ctxt);
	}

	/* consecutive blocks of this many words are considered as
	 * one null link. */
	/* ctxt->null_block = 1; */

	if (NULL != sent->Table_tracon_pool)
	{
		pool_reuse(sent->Table_tracon_pool);
	}
	else
	{
		/* The estimate_tracon_entries() provides an upper limit to
		 * how many might get allocated in a "typical" sentence. The
		 * lower bound is 12 times smaller. So, start with that. If
		 * this isn't enough, each new alloc will grab another chunk
		 * of this size. Thus, a typical sentence will expand the
		 * tracon pool six times. But this logic holds only if a new
		 * pool is allocated each time. When re-using, with sentences
		 * of highly variable sizes, if seems best to fix this value.
		 *
		 * size_t num_elts = estimate_tracon_entries(sent);
		 * num_elts /= 12;
		 */
		sent->Table_tracon_pool =
			pool_new(__func__, "Table_tracon",
			         16382 /* num_elts */, sizeof(Table_tracon),
			         /*zero_out*/false, /*align*/false, /*exact*/false);
	}

	init_table(ctxt);
	return ctxt;
}

void free_count_context(count_context_t *ctxt, Sentence sent)
{
	if (NULL == ctxt) return;
	COUNT_COST(lgdebug(+D_COUNT,
	           "Count cost per: word %"PRIu64", "
	           "disjunct %"PRIu64", null_count %"PRIu64"\n",
	            ctxt->count_cost[0], ctxt->count_cost[1], ctxt->count_cost[2]);)

	free_table_lrcnt(ctxt);
	free(ctxt);
}
