/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2013,2014,2015 Linas Vepstas                            */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <limits.h>

#include "link-includes.h"
#include "api-structures.h"
#include "connectors.h"
#include "count.h"
#include "disjunct-utils.h"
#include "fast-match.h"
#include "resources.h"
#include "tokenize/word-structures.h" // for Word_struct
#include "utilities.h"

/* This file contains the exhaustive search algorithm. */

#define D_COUNT 5 /* General debug level for this file. */

typedef struct Table_connector_s Table_connector;
struct Table_connector_s
{
	Table_connector  *next;
	int              l_id, r_id;
	Count_bin        count;
	unsigned int     null_count;
	unsigned int     hash;
};

typedef uint8_t null_count_m;  /* Storage representation of null_count */
typedef uint8_t WordIdx_m;     /* Storage representation of word index */

/* An element in a table indexed by tracon_id and w.
 * The status field indicates if parsing the range [tracon_id, w) would
 * yield a zero/none-zero leftcount/rightcount, when the zero prediction
 * is valid for null counts up to null_count (or for any null count if
 * null_count is ANY_NULL_COUNT). */
typedef struct
{
	null_count_m null_count; /* status==0 valid up to this null count */
	int8_t status;         /* -1: Needs update; 0: No count; 1: Count possible */
	uint8_t check_next;      /* Next word to check */
} Table_lrcnt;

/* Using the word-vectors for very short sentences has too much overhead. */
static const size_t min_len_word_vector = 10; /* This is just an estimation. */

/* Special values for Table_lrcnt fields null_count and check_next. */
#define ANY_NULL_COUNT (MAX_SENTENCE + 1)
#define INCREMENT_WORD ((uint8_t)(MAX_SENTENCE+1)) /* last checked word + 1 */

typedef Table_lrcnt *wordvecp;
static Table_lrcnt lrcnt_cache_zero; /* A sentinel for status==0 */

#if defined DEBUG || DEBUG_COUNT_COST
#define COUNT_COST(...) __VA_ARGS__
#else
#define COUNT_COST(...)
#endif

struct count_context_s
{
	fast_matcher_t *mchxt;
	Sentence sent;
	/* int     null_block; */ /* not used, always 1 */
	bool    islands_ok;
	bool    exhausted;
	uint8_t num_growth;       /* Number of table growths, for debug */
	bool    keep_table;
	bool    is_short;
	unsigned int checktimer;  /* Avoid excess system calls */
	unsigned int table_size;
	unsigned int table_mask;
	unsigned int log2_table_size;
	unsigned int table_available_count;
	unsigned int table_lrcnt_size[2];
	Table_connector ** table;
	wordvecp *table_lrcnt[2];  /* Per dir wordvec, indexed by tracon_id */
	Pool_desc *wordvec_pool;
	Resources current_resources;
	COUNT_COST(unsigned long long count_cost[3];)
};
#define MAX_TABLE_SIZE(s) (s / 10) /* Low load factor, for speed */
#define MAX_LOG2_TABLE_SIZE 24     /* 128 on 64-bit systems */

static void free_table(count_context_t *ctxt)
{
	ctxt->table = NULL;
	ctxt->table_size = 0;
}

static void free_kept_table(void);

/**
 * Allocate memory for the connector-pair table and initialize table-size
 * related fields (table_size and table_available_count). Reuse the
 * previous table memory if the request is for a table that fits. If this is
 * not a call to grow the table, free it if not reused.
 *
 * @ctxt[in, out] Table info.
 * @param shift log2 table size, or \c 0 for table growth. On table growth,
 * increase the new table size by 2.
 */
static void table_alloc(count_context_t *ctxt, unsigned int shift)
{
	static TLS Table_connector **kept_table = NULL;
	static TLS unsigned int log2_kept_table_size = 0;

	if (ctxt == NULL)
	{
		free(kept_table);
		return;
	}

	if (shift == 0)
		shift = ctxt->log2_table_size + 1; /* Double the table size */

	/* Keep the table indefinitely (or until exiting), so that it can
	 * be reused. This avoids a large overhead in malloc/free when
	 * large memory blocks are allocated. Large block in Linux trigger
	 * system calls to mmap/munmap that eat up a lot of time.
	 * (Up to 20%, depending on the sentence and CPU.) */
	ctxt->table_size = (1U << shift);
	if ((shift > log2_kept_table_size) || (kept_table == NULL))
	{
		if (log2_kept_table_size == 0) atexit(free_kept_table);
		log2_kept_table_size = shift;

		if (kept_table) free(kept_table);
		kept_table = malloc(sizeof(Table_connector *) * ctxt->table_size);
	}

	memset(kept_table, 0, sizeof(Table_connector *) * ctxt->table_size);

	/* This is here and not in init_table() because it must be set
	 * also when the table growths. */
	ctxt->log2_table_size = shift;
	ctxt->table_mask = ctxt->table_size - 1;
	ctxt->table = kept_table;

	if (shift >= MAX_LOG2_TABLE_SIZE)
		ctxt->table_available_count = UINT_MAX; /* Prevent growth */
	else
		ctxt->table_available_count = MAX_TABLE_SIZE(ctxt->table_size);
}

/**
 * This function is called on program exit so no memory remains allocated.
 * This is not really necessary because even for debug the message on
 * memory that left allocated can be put in an ignore-list.
 *
 * FIXME: Fix thread memory leak resulted by not freeing table memory
 * on thread exit.
 */
static void free_kept_table(void)
{
	table_alloc(NULL, 0);
}

/**
 * Allocate the table with a sentence-depended table size.  Use
 * sent->length as a hint for the initial table size. Usually, this
 * saves on dynamic table growth, which is costly.
 * */
static void init_table(count_context_t *ctxt, Sentence sent)
{
	if (ctxt->table) free_table(ctxt);

	/* A piecewise exponential function determines the size of the
	 * hash table. Probably should make use of the actual number of
	 * disjuncts, rather than just the number of words.
	 */
	unsigned int shift;
	if (sent->length >= 16)
	{
		shift = 14 + sent->length / 16;
	}
	else
	{
		shift = 14;
	}
#if 0 /* Not yet */
	shift += (unsigned int)sent->num_disjuncts / (sent->length * 200);
#endif

	if (MAX_LOG2_TABLE_SIZE < shift) shift = MAX_LOG2_TABLE_SIZE;
	lgdebug(+D_COUNT, "Initial connector table log2 size %u\n", shift);

	table_alloc(ctxt, shift);
}

static void free_table_lrcnt(count_context_t *ctxt)
{
	if (verbosity_level(D_COUNT))
	{
		unsigned int nonzero = 0;
		unsigned int any_null = 0;
		unsigned int zero = 0;
		unsigned int non_max_null = 0;
#if POOL_NEXT

		Pool_location loc = { 0 };
		while ((Table_lrcnt *t = pool_next(ctxt->wordvec_pool, &loc)) != NULL)
		{
			if (t->status == -1) continue;
			if (t->status == 1)
				nonzero++;
			else if (t->null_count == ANY_NULL_COUNT)
				any_null++;
			else if (ctxt->sent->null_count > t->null_count)
				non_max_null++;
			else if (ctxt->sent->null_count == t->null_count)
				zero++;
		}
#endif

		const unsigned int num_values = ctxt->wordvec_pool->curr_elements;
		lgdebug(+0, "Values %u (usage = non_max_null %u + other %u, "
		        "other = any_null_zero %u + zero %u + nonzero %u)\n",
		        num_values, non_max_null, num_values-non_max_null,
		        any_null, zero, nonzero);

		for (unsigned int dir = 0; dir < 2; dir++)
		{
			unsigned int table_usage = 0;

			for (size_t i = 0; i < ctxt->table_lrcnt_size[dir]; i++)
			{
				if (ctxt->table_lrcnt[dir][i] != NULL) continue;
				table_usage++;
			}

			lgdebug(+0, "Direction %u: Using %u/%u tracons %.2f%%\n\\",
			        dir, table_usage, ctxt->table_lrcnt_size[dir],
			        100.0f*table_usage / ctxt->table_lrcnt_size[dir]);
		}
	}

	for (unsigned int dir = 0; dir < 2; dir++)
	{
		free(ctxt->table_lrcnt[dir]);
		ctxt->table_lrcnt[dir] = NULL;
	}
	pool_delete(ctxt->wordvec_pool);
}

static void init_table_lrcnt(count_context_t *ctxt, Sentence sent)
{
	for (unsigned int dir = 0; dir < 2; dir++)
	{
		const size_t sz = sizeof(wordvecp) * ctxt->table_lrcnt_size[dir];
		ctxt->table_lrcnt[dir] = malloc(sz);
		memset(ctxt->table_lrcnt[dir], 0, sz);
	}

	const size_t initial_size = MIN(sent->length/2, 16) *
		                   (ctxt->table_lrcnt_size[0] + ctxt->table_lrcnt_size[1]);

	ctxt->wordvec_pool =
		pool_new(__func__, "Table_lrcnt", /*num_elements*/initial_size,
		         sizeof(Table_lrcnt), /*zero_out*/false,
		         /*align*/false, /*exact*/false);
}

//#define DEBUG_TABLE_STAT
#if defined(DEBUG) || defined(DEBUG_TABLE_STAT)
static size_t hit, miss;  /* Table value found/not found */
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

	int z = 0, nz = 0;  /* Number of entries with zero and non-zero counts */
	int c, total_c = 0; /* Chain length */
	int N = 0;          /* NULL table slots */
	int null_count[256] = { 0 };   /* null_count histogram */
	int chain_length[64] = { 0 };  /* Chain length histogram */
	bool table_stat_entries = test_enabled("count-table-entries");
	int n;              /* For printf() pretty printing */

	for (unsigned int i = 0; i < ctxt->table_size; i++)
	{
		c = 0;
		Table_connector *t = ctxt->table[i];
		if (t == NULL) N++;
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

	int used_slots = ctxt->table_size-N;
	/* The used= value is TotalValues/TableSize (not UsedSlots/TableSize). */
	printf("Connector table: num_growth=%u msb=%u slots=%6d/%6u (%5.2f%%) "
	       "avg-chain=%4.2f values=%6d (z=%5d nz=%5d N=%5d) used=%5.2f%% "
	       "acc=%zu (hit=%zu miss=%zu) (sent_len=%zu dis=%u)\n",
	       ctxt->num_growth, ctxt->log2_table_size, used_slots, ctxt->table_size,
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
				printf("%u: %d\n", nc, null_count[nc]);
		}
	}

	if (table_stat_entries)
	{
		for (unsigned int nc = 0; nc < ARRAY_SIZE(null_count); nc++)
		{
			if (0 == null_count[nc]) continue;

			printf("Null count %u:\n", nc);
			for (unsigned int i = 0; i < ctxt->table_size; i++)
			{
				for (Table_connector *t = ctxt->table[i]; t != NULL; t = t->next)
				{
					if (t->null_count != nc) continue;

					printf("[%u]%n", i, &n);
					printf("%*d %5d c=%lld\n",  15-n, t->l_id, t->r_id, t->count);
				}
			}
		}
	}

	hit = miss = 0;
#endif /* DEBUG_TABLE_STAT */
}

static void table_grow(count_context_t *ctxt)
{
	table_alloc(ctxt, 0);

	/* Rehash. */
	Table_connector *oe;
	Pool_location loc = { 0 };
	while ((oe = pool_next(ctxt->sent->Table_connector_pool, &loc)) != NULL)
	{
		unsigned int ni = oe->hash & ctxt->table_mask;

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
                                     unsigned int hash, Count_bin c)
{
	if (ctxt->table_available_count == 0) table_grow(ctxt);

	int l_id = (NULL != le) ? le->tracon_id : lw;
	int r_id = (NULL != re) ? re->tracon_id : rw;
	unsigned int i = hash & ctxt->table_mask;
	Table_connector *n = pool_alloc(ctxt->sent->Table_connector_pool);

	if (ctxt->table[i] == NULL)
		ctxt->table_available_count--;

	n->l_id = l_id;
	n->r_id = r_id;
	n->null_count = null_count;
	n->next = ctxt->table[i];
	n->count = c;
	n->hash = hash;
	ctxt->table[i] = n;

	return c;
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
                   unsigned int null_count, unsigned int *hash)
{
	int l_id = (NULL != le) ? le->tracon_id : lw;
	int r_id = (NULL != re) ? re->tracon_id : rw;

	unsigned int h = pair_hash(lw, rw, l_id, r_id, null_count);
	Table_connector *t = ctxt->table[h & ctxt->table_mask];

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

/**
 * Generate a word skip vector.
 * This implements an idea similar to the Boyer-Moore algo for searches.
 */
static void generate_word_skip_vector(count_context_t *ctxt, wordvecp wv,
                                      Connector *le, Connector *re,
                                      int start_word, int end_word,
                                      int lw, int rw)
{
	if (le != NULL)
	{
		int check_word = start_word;
		int i;

		if (wv == NULL) wv = ctxt->table_lrcnt[0][le->tracon_id];
		unsigned int sent_nc = ctxt->sent->null_count;
		for (i = start_word + 1; i < end_word; i++)
		{
			Table_lrcnt *e = &wv[i - le->nearest_word];
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

		if (wv == NULL) wv = ctxt->table_lrcnt[1][re->tracon_id];
		unsigned int sent_nc = ctxt->sent->null_count;
		for (i = start_word + 1; i < end_word; i++)
		{
			Table_lrcnt *e = &wv[i - re->farthest_word];
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
 * Cache lookup:
 * Is the range [c, w) going to yield a nonzero leftcount / rightcount?
 *
 * @param ctxt Count context.
 * @param dir Direction - 0: leftcount, 1: rightcount.
 * @param c The connector that starts the range.
 * @param w The word that ends the range.
 * @param null_count The current null_count to check.
 * @param cw The word of this connector.
 *
 * Return these values:
 * @param lnull_start First null count to check (the previous ones can be
 * skipped because the cache indicates they yield a zero count.)
 * @return Cache entry for the given range. Possible values:
 *    NULL - A nonzero count may be encountered for null_count>=lnull_start.
 *    Table_lrcnt_zero - A zero count would result.
 *    Cache pointer - An update for null_count>=lnull_start is needed.
 */
static Table_lrcnt *is_lrcnt(count_context_t *ctxt, int dir, Connector *c,
                             unsigned int wordvec_index,
                             unsigned int null_count, unsigned int *null_start)
{
	if (ctxt->is_short) return NULL;

	wordvecp *wv = &ctxt->table_lrcnt[dir][c->tracon_id];
	if (*wv == NULL)
	{
		if (null_start != NULL)
		{
			/* Create a new cache entry, to be updated by lrcnt_cache_update(). */
			const size_t wordvec_size = abs(c->farthest_word - c->nearest_word) + 1;
			*wv = pool_alloc_vec(ctxt->wordvec_pool, wordvec_size);
			memset(*wv, -1, sizeof(Table_lrcnt) * wordvec_size);

			*null_start = 0;
			assert(wordvec_index < ctxt->sent->length, "Bad wordvec index");
			return &(*wv)[wordvec_index]; /* Needs update */
		}
		return NULL;
	}
	Table_lrcnt *lp = &(*wv)[wordvec_index];

	/* Below, several checks are done on the cache. The function returns
	 * on the first one that succeeds. */

	if (lp->status == -1)
		return lp; /* Needs update */

	if  (lp->status == 1)
	{
		/* The range yields a nonzero leftcount/rightcount for some
		 * null_count. But we can still skip the initial null counts. */
		if (null_start != NULL)
			*null_start = (null_count_m)(lp->null_count + 1);
		return NULL; /* No update needed - this is a permanent status */
	}

	if (null_count <= lp->null_count)
	{
		/* Here (lp->status == 0) which means our count would
		 * be zero - so nothing to do for this word. */
		return &lrcnt_cache_zero;
	}

	/* The null counts greater than lp->null_count have not
	 * been handled yet for the given range (the cache will get
	 * updated for them by lrcnt_cache_update()). */
	if (null_start == NULL) return NULL;
	*null_start = lp->null_count + 1;
	return lp;
}

bool no_count(count_context_t *ctxt, int dir, Connector *c,
              unsigned int wordvec_index, unsigned int null_count)
{
	return
		&lrcnt_cache_zero ==
		is_lrcnt(ctxt, dir, c, wordvec_index, null_count, NULL);
}

static bool lrcnt_cache_update(Table_lrcnt *lrcnt_cache, bool lrcnt_found,
                              bool match_list, unsigned int null_count)
{
	bool lrcnt_status_changed = (lrcnt_cache->status != (int)lrcnt_found);
	unsigned int prev_null_count = lrcnt_cache->null_count;

	if (!lrcnt_found)
		lrcnt_cache->null_count = match_list ? null_count : ANY_NULL_COUNT;
	lrcnt_cache->status = (int)lrcnt_found;

	return lrcnt_status_changed || (prev_null_count != lrcnt_cache->null_count);
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
#ifdef PERFORM_COUNT_HISTOGRAMMING
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
	Count_bin *count = table_lookup(ctxt, lw, rw, le, re, null_count, NULL);
	if (NULL == count) return count_unknown;
	return *count;
}

/**
 *  Clamp the given count to INT_MAX.
 *  The returned value is normally unused by the callers
 *  (to be used when debugging overflows).
 */
static bool parse_count_clamp(Count_bin *total)
{
	if (INT_MAX < hist_total(total))
	{
		/*  Sigh. Overflows can and do occur, esp for the ANY language. */
#ifdef PERFORM_COUNT_HISTOGRAMMING
		total->total = INT_MAX;
#else
		*total = INT_MAX;
#endif /* PERFORM_COUNT_HISTOGRAMMING */

		return true;
	}

	return false;
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
		snprintf(m_result, sizeof(m_result), "(M=%lld)", hist_total(c));

	level++;
	prt_error("%*sdo_count%s:%d lw=%d rw=%d le=%s(%d) re=%s(%d) null_count=%u\n\\",
		level*2, "", m_result, lineno, lw, rw, V(le),ID(le,lw), V(re),ID(re,rw), null_count);
	Count_bin r = do_count1(lineno, ctxt, lw, rw, le, re, null_count);
	prt_error("%*sreturn%.*s:%d=%lld\n",
	          LBLSZ+level*2, "", (!!c)*3, "(M)", lineno, hist_total(&r));
	level--;

	return r;
}

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
	Count_bin total = hist_zero();
	int start_word, end_word, w;

	if (is_panic(ctxt)) return hist_zero();

	/* TODO: static_assert() that null_count is an unsigned int. */
	assert (null_count < INT_MAX, "Bad null count %d", (int)null_count);

	unsigned int h = 0; /* Initialized value only needed to prevent a warning. */
	{
		Count_bin* const c = table_lookup(ctxt, lw, rw, le, re, null_count, &h);
		if (c != NULL) return *c;
		/* The table entry is to be updated with the found linkage count
		 * before we return. The hash h will be used to locate it. */
	}

	unsigned int unparseable_len = rw-lw-1;

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

		/* Here null_count != 0 and we allow islands (a set of words
		 * linked together but separate from the rest of the sentence).
		 * Because we don't know here if an optional word is just
		 * skipped or is a real null-word (see the comment above) we
		 * try both possibilities: If a real null is encountered, the
		 * rest of the sentence must contain one less null-word. Else
		 * the rest of the sentence still contains the required number
		 * of null words. */
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
				if (parse_count_clamp(&total))
				{
#if 0
					printf("OVERFLOW 1\n");
#endif
				}
			}

			hist_accumv(&total, 0.0,
				do_count(ctxt, w, rw, NULL, NULL, try_null_count-1));
			if (parse_count_clamp(&total))
			{
#if 0
				printf("OVERFLOW 2\n");
#endif
			}
		}
		return table_store(ctxt, lw, rw, le, re, null_count, h, total);
	}

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
	wordvecp wv = NULL;
	int woffset = 0;
	if (!ctxt->is_short)
	{
		if (le != NULL)
		{
			wv = ctxt->table_lrcnt[0][le->tracon_id];
			woffset = le->nearest_word;
		}
		else
		{
			wv = ctxt->table_lrcnt[1][re->tracon_id];
			woffset = re->farthest_word;
		}
	}

	for (w = start_word; w < end_word; w = next_word)
	{
		COUNT_COST(ctxt->count_cost[0]++;)

		/* Use the word-skip vector to skip over words that are
		 * known to yield a zero count. */
		if ((wv != NULL) && (w != end_word -1))
		{
			next_word = wv[w - woffset].check_next;
			if (next_word == INCREMENT_WORD) next_word = w + 1;
		}
		else
		{
			next_word = w + 1;
		}

		/* Start of nonzero leftcount/rightcount range cache check. It is
		 * extremely effective for long sentences, but doesn't speed up
		 * short ones.
		 *
		 * FIXME: lrcnt_optimize==false doubles the Table_connector cache!
		 * Try to fix it by not caching zero counts in Table_connector when
		 * lrcnt_optimize==false. If this can be fixed, a significant
		 * speedup is expected. */

		Table_lrcnt *lrcnt_cache = NULL;
		bool lrcnt_found = false;     /* TRUE iff a range yielded l/r count */
		bool lrcnt_optimize = true;   /* Perform l/r count optimization */
		unsigned int lnull_start = 0; /* First null_count to check */
		unsigned int lnull_end = null_count; /* Last null_count to check */
		Connector *fml_re = re;       /* For form_match_list() only */
#define S(c) (!c?"(nil)":connector_string(c))

		if (le != NULL)
		{
			lrcnt_cache =
				is_lrcnt(ctxt, 0, le, w - le->nearest_word, null_count, &lnull_start);
			if (lrcnt_cache == &lrcnt_cache_zero) continue;

			if (lrcnt_cache != NULL)
			{
				lrcnt_optimize = false;
			}
			else if ((re != NULL) && (re->farthest_word <= w))
			{
				/* If it is already known that "re" would yield a zero
				 * rightcount, there is no need to fetch the right match list.
				 * The code below will still check for possible l_bnr counts. */
				Table_lrcnt *rgc =
					is_lrcnt(ctxt, 1, re, w - re->farthest_word, null_count, NULL);
				if (rgc == &lrcnt_cache_zero) fml_re = NULL;
			}
		}
		else
		{
			/* Here re != NULL. */
			unsigned int rnull_start;
			lrcnt_cache =
				is_lrcnt(ctxt, 1, re, w - re->farthest_word, null_count, &rnull_start);
			if (lrcnt_cache == &lrcnt_cache_zero) continue;

			if (lrcnt_cache != NULL)
			{
				lrcnt_optimize = false;
				if (rnull_start <= null_count)
					lnull_end -= rnull_start;
			}
		}
		/* End of nonzero leftcount/rightcount range cache check. */

		size_t mlb = form_match_list(mchxt, w, le, lw, fml_re, rw);

#ifdef VERIFY_MATCH_LIST
		unsigned int id = get_match_list_element(mchxt, mlb) ?
		                  get_match_list_element(mchxt, mlb)->match_id : 0;
#endif

		for (size_t mle = mlb; get_match_list_element(mchxt, mle) != NULL; mle++)
		{
			COUNT_COST(ctxt->count_cost[1]++;)

			Disjunct *d = get_match_list_element(mchxt, mle);
			bool Lmatch = d->match_left;
			bool Rmatch = d->match_right;

#ifdef VERIFY_MATCH_LIST
			assert(id == d->match_id, "Modified id (%u!=%u)", id, d->match_id);
#endif

			for (unsigned int lnull_cnt = lnull_start; lnull_cnt <= lnull_end; lnull_cnt++)
			{
				COUNT_COST(ctxt->count_cost[2]++;)

				int rnull_cnt = null_count - lnull_cnt;
				/* Now lnull_cnt and rnull_cnt are the null-counts we're
				 * requiring in those parts respectively. */
				bool leftpcount = false;
				bool rightpcount = false;

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
				if (Lmatch)
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

				if (Rmatch && (leftpcount || (le == NULL)))
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

#define COUNT(c, do_count) \
	{ c = TRACE_LABEL(c, do_count); }

				if (!leftpcount && !rightpcount) continue;

				if (!(leftpcount && rightpcount))
				{
					if (leftpcount)
					{
						/* Evaluate using the left match, but not the right. */
						COUNT(l_bnr, do_count(ctxt, w, rw, d->right, re, rnull_cnt));
					}
					else if (le == NULL)
					{
						/* Evaluate using the right match, but not the left. */
						COUNT(r_bnl, do_count(ctxt, lw, w, le, d->left, lnull_cnt));
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
				Count_bin leftcount = hist_zero();
				Count_bin rightcount = hist_zero();
				if (leftpcount &&
				    (!lrcnt_optimize || rightpcount || (0 != hist_total(&l_bnr))))
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

					if (0 < hist_total(&leftcount))
					{
						lrcnt_found = true;
						lrcnt_optimize = true;

						/* Evaluate using the left match, but not the right */
						CACHE_COUNT(l_bnr, hist_muladdv(&total, &leftcount, d->cost, count),
							do_count(ctxt, w, rw, d->right, re, rnull_cnt));
					}
				}

				if (rightpcount &&
				    (!lrcnt_optimize || (0 < hist_total(&leftcount)) || (0 != hist_total(&r_bnl))))
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

					if (0 < hist_total(&rightcount))
					{
						if (le == NULL)
						{
							lrcnt_found = true;
							lrcnt_optimize = true;

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
		}

		if (lrcnt_cache != NULL)
		{
			bool match_list = (get_match_list_element(mchxt, mlb) != NULL);
			if (lrcnt_cache_update(lrcnt_cache, lrcnt_found, match_list, null_count))
				lrcnt_cache_changed = true;
		}

		pop_match_list(mchxt, mlb);
	}

	if (lrcnt_cache_changed)
		generate_word_skip_vector(ctxt, wv, le, re, start_word, end_word, lw, rw);

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
	ctxt->is_short = sent->length <= min_len_word_vector;

	/* Cannot reuse since its content is invalid on an increased null_count. */
	init_table_lrcnt(ctxt, sent);

	hist = do_count(ctxt, -1, sent->length, NULL, NULL, sent->null_count+1);

	table_stat(ctxt);
	return (int)hist_total(&hist);
}

/* sent_length is used only as a hint for the hash table size ... */
count_context_t * alloc_count_context(Sentence sent, Tracon_sharing *ts)
{
	count_context_t *ctxt = (count_context_t *) xalloc (sizeof(count_context_t));
	memset(ctxt, 0, sizeof(count_context_t));

	/* 1. next_id keeps the last tracon_id used, so we need +1 for array size.
	 * 2. In do_count(), le and re are right and left connectors respectively.
	 */
	for (unsigned int dir = 0; dir < 2; dir++)
		ctxt->table_lrcnt_size[dir] = ts->next_id[!dir] + 1;

	ctxt->sent = sent;
	/* consecutive blocks of this many words are considered as
	 * one null link. */
	/* ctxt->null_block = 1; */

	if (NULL != sent->Table_connector_pool)
	{
		pool_reuse(sent->Table_connector_pool);
	}
	else
	{
		sent->Table_connector_pool =
			pool_new(__func__, "Table_connector",
			         /*num_elements*/16384, sizeof(Table_connector),
			         /*zero_out*/false, /*align*/false, /*exact*/false);
	}

	init_table(ctxt, sent);
	return ctxt;
}

void free_count_context(count_context_t *ctxt, Sentence sent)
{
	if (NULL == ctxt) return;
	COUNT_COST(lgdebug(+D_COUNT,
	           "Count cost per: word %llu, disjunct %llu, null_count %llu\n",
	            ctxt->count_cost[0], ctxt->count_cost[1], ctxt->count_cost[2]);)

	free_table(ctxt);
	free_table_lrcnt(ctxt);
	xfree(ctxt, sizeof(count_context_t));
}
