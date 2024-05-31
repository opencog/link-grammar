/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2009, 2013, 2014 Linas Vepstas                          */
/* Copyright (c) 2016, 2017, 2018, 2019 Amir Plivatsky                   */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "api-structures.h"
#include "connectors.h"
#include "disjunct-utils.h"
#include "dict-common/dict-common.h"    // contable
#include "linkage/analyze-linkage.h"    // intersect_strings
#include "post-process/post-process.h"
#include "post-process/pp-structures.h"
#include "print/print.h"                // print_disjunct_counts

#include "prune.h"
#include "resources.h"
#include "string-set.h"
#include "tokenize/word-structures.h"   // Word_struct
#include "tokenize/wordgraph.h"

#define D_PRUNE 5      /* Debug level for this file (6 for pp_prune()) */

/* To debug pp_prune(), touch this file, run "make CPPFLAGS=-DDEBUG_PP_PRUNE",
 * and then run: link-parser -v=5 -debug=prune.c . */
#ifdef DEBUG
#define DEBUG_PP_PRUNE
#endif

#ifdef DEBUG_PP_PRUNE
#define ppdebug(...) lgdebug(+D_PRUNE+1, __VA_ARGS__)
#else
#define ppdebug(...)
#endif

#define PRx(x) fprintf(stderr, ""#x)
#define PR(...) true

/* Indicator that this connector cannot be used -- that it's "obsolete".  */
#define BAD_WORD (MAX_SENTENCE+1)

typedef uint8_t WordIdx_m;     /* Storage representation of word index */

/* The dimension of the 2-element arrays below (unless stated otherwise)
 * is used as follows: [0] - left side; [1] - right side. */

/* Per-word minimum/maximum link distance descriptor.
 *
 * nw is the minimum nearest_word of the shallow connectors.
 * fw is the maximum farthest_word of the shallow connectors.
 *
 * In case at least one jet is missing in a particular direction, there
 * are no link distance constraints in that direction. For nearest_word
 * (nw) this is signified by value w for word w. For farthest word (fw)
 * this is signified by 0 for fw[0] and UNLIMITED_LEN for fw[1].
 *
 * nw_perjet is similar to nw, but its computation ignores the
 * abovementioned case in which a jet is missing in the relevant
 * direction. Moreover, if there are no jets at all at the given
 * direction, nw_perjet[0] is 0 and nw_perjet[1] is UNLIMITED_LEN (as if
 * the nearest word a missing jet can connect to is beyond the sentence
 * boundary, i.e. no connection is possible). NOTE: The use of 0 here
 * (instead of -1) is not optimal as word 0 is still inside the sentence,
 * so the checks in is_cross_mlink() when rword==0 are not optimal.
 *
 * nw_unidir is also similar to nw, but its computation ignores jets that
 * have an opposite jet in the other directions. Naturally, there are no
 * missing jets in the relevant direction.
 *
 * The connection of the shallow connector is to a greater distance than
 * the connections from the deeper ones. For word w, words in the ranges
 * (nw[0], w) or (w, nw[1]) cannot connect to words to the left or to the
 * right of w (correspondingly) without crossing a link to a shallow
 * connector of w. In addition, words in (nw[0], w) cannot connect to
 * words before fw[0] (and similarly after fw[1] for the other direction).
 *
 * The values for words which don't have disjuncts are undefined.
 *
 * These values are computed by build_mlink_table() after the first
 * power_prune() call, before invoking an additional power_prune(). */
typedef struct
{
	WordIdx_m nw[2];         /* minimum link distance */
	WordIdx_m nw_perjet[2];  /* the same, ignoring missing jets */
	WordIdx_m nw_unidir[2];  /* the same, but for unidirectional disjuncts */
	WordIdx_m fw[2];         /* maximum link distance */
} mlink_table;

typedef struct c_list_s C_list;
struct c_list_s
{
	C_list *next;
	Connector *c;
};

typedef struct power_table_s power_table;
struct power_table_s
{
	unsigned int power_table_size;
	unsigned int *table_size[2];  /* the sizes of the hash tables */
	C_list ***table[2];
	Pool_desc *memory_pool;
};

typedef struct prune_context_s prune_context;
struct prune_context_s
{
	unsigned int null_links; /* maximum number of null links */
	unsigned int null_words; /* found number of non-optional null words */
	bool *is_null_word;      /* a map of null words (indexed by word number) */
	bool islands_ok;         /* a copy of islands_ok from the parse options */
	uint8_t pass_number;     /* marks tracons for processing only once per pass*/

	/* Used for debug: Always parse after the pruning.
	 * Always parsing means always doing a full prune.
	 * It has two purposes:
	 * 1. Validation that skipping parsing is done only when really there
	 * would be no parse (with the given null_links).
	 * 2. Find the effectiveness of the parse-skipping strategy so it can
	 * be tuned and improved. */
	bool always_parse; /* value set by the test parse option "always-parse" */

	/* Per-pass counters. */
	int N_changed;     /* counts the changes of c->nearest_word fields */
	int N_deleted[2];  /* counts deletions: [0]: initial; [1]: by mem. sharing */

	power_table *pt;
	mlink_table *ml;
	Sentence sent;

	int power_cost;   /* for debug - shown in the verbose output */
	int N_xlink;      /* counts linked interval link-crossing rejections */
};

#ifdef DEBUG
GNUC_UNUSED static void print_power_table_entry(power_table *pt, int w, int dir)
{
	C_list **t = pt->table[w][dir];
	unsigned int size = pt->table_size[w][dir];

	if (size == 1) return;
	printf("w%d dir%d size=%u:\n", w, dir, size);

	for (unsigned int i = 0; i < size; i++)
	{
		if (t[i] == NULL) continue;

		printf(" [%u]: ", i);
		for (C_list *cl = t[i]; cl != NULL; cl = cl->next)
		{
			char *cstr = print_one_connector_str(cl->c, "lrs");
			printf("%s", cstr);
			free(cstr);
			if (cl->next != NULL) printf(" ");
		}
		printf("\n");
	}
}

GNUC_UNUSED static void print_power_table(Sentence sent, power_table *pt)
{
	printf("power table:\n");
	for (WordIdx w = 0; w < sent->length; w++)
	{
		for (int dir = 0; dir < 2; dir++)
			print_power_table_entry(pt, w, dir);
	}
}
#endif

/*
 * Here is what you've been waiting for: POWER-PRUNE
 *
 * The kinds of constraints it checks for are the following:
 *
 *  1) Successive connectors on the same disjunct have to go to
 *     nearer and nearer words.
 *
 *  2) Two deep connectors cannot attach to each other; i.e. a deep
 *     connector can only attach to a shallow one, and a shallow
 *     connector can attach to any connector.
 *     (A connectors is deep if it is not the first in its list; it
 *     is shallow if it is the first in its list; it is deepest if it
 *     is the last on its list.)
 *
 *  3) On two adjacent words, a pair of connectors can be used
 *     only if they're the deepest ones on their disjuncts.
 *
 *  4) On non-adjacent words (with no intervening null-linked words),
 *     a pair of connectors can be used only if at least one of them
 *     is not the deepest.
 *
 *  The data structure consists of a pair of hash tables on every word.
 *  Each bucket of a hash table has a list of pointers to connectors.
 *
 *  As with expression pruning, we make alternate left->right and
 *  right->left passes.  In the R->L pass, when we're on a word w, we make
 *  use of all the left-pointing hash tables on the words to the right of
 *  w.  After the pruning on this word, we build the left-pointing hash
 *  table this word.  This guarantees idempotence of the pass -- after
 *  doing an L->R, doing another would change nothing.
 *
 *  Each connector has an integer nearest_word field.  This refers to the
 *  closest word that it could be connected to.  These are initially
 *  determined by how deep the connector is.  For example, a deepest
 *  connector can connect to the neighboring word, so its nearest_word
 *  field is w+1 (w-1 if this is a left pointing connector).  It's
 *  neighboring shallow connector has a nearest_word value of w+2, etc.
 *
 *  The pruning process adjusts these nearest_word values as it goes along,
 *  accumulating information about any way of linking this sentence.  The
 *  pruning process stops only after no disjunct is deleted and no
 *  nearest_word values change.
 */

/*
 * From old comments:
 *
 * It worries me a little that if there are some really huge disjuncts
 * lists, then this process will probably do nothing.  (This fear turns
 * out to be unfounded.)
 *
 * New idea:  Suppose all the disjuncts of a word have a connector of type
 * c pointing to the right.  And further, suppose that there is exactly
 * one word to its right containing that type of connector pointing to the
 * left.  Then all the other disjuncts on the latter word can be deleted.
 */

/**
 * free all of the hash tables and C_lists
 */
static void power_table_delete(power_table *pt)
{
	pool_delete(pt->memory_pool);
	free(pt->table_size[0]);
	free(pt->table[0][0]);
	free(pt->table[0]);
}

static C_list **get_power_table_entry(unsigned int size, C_list **t,
                                      Connector *c)
{
	unsigned int h, s;

	h = s = connector_uc_num(c) & (size-1);
	while (NULL != t[h])
	{
		if (connector_uc_eq(t[h]->c, c)) break;

		/* Increment and try again. Every hash bucket MUST have a unique
		 * upper-case part, since later on, we only compare the lower-case
		 * parts, assuming upper-case parts are already equal. So just look
		 * for the next unused hash bucket.
		 */
		h = (h + 1) & (size-1);
		if (h == s) return NULL;
	}

	return &t[h];
}

/**
 * The disjunct d (whose left or right pointer points to c) is put
 * into the appropriate hash table
 */
static void put_into_power_table(Pool_desc *mp, unsigned int size, C_list **t,
                                 Connector *c)
{
	C_list **e = get_power_table_entry(size, t, c);

	assert(NULL != e, "Overflow");
	assert(c->refcount > 0, "refcount %d", c->refcount);

	C_list *m = pool_alloc(mp);
	m->next = *e;
	*e = m;
	m->c = c;
}

static void power_table_alloc(Sentence sent, power_table *pt)
{
	pt->power_table_size = sent->length;
	pt->table_size[0] = malloc (2 * sent->length * sizeof(unsigned int));
	pt->table_size[1] = pt->table_size[0] + sent->length;
	pt->table[0] = malloc (2 * sent->length * sizeof(C_list **));
	pt->table[1] = pt->table[0] + sent->length;
}

/**
 * Allocates and builds the initial power hash tables.
 * Each word has 2 tables - for its left and right connectors.
 * In these tables, the connectors are hashed according to their
 * uppercase part.
 * In each hash slot, the shallow connectors appear first, so when
 * matching deep connectors to the connectors in a slot, the
 * match loop can stop when there are no more shallow connectors in that
 * slot (since if both are deep, they cannot be matched).
 *
 * Each connector has a refcount filed, which indicates how many times it
 * is memory-shared in the word of its disjunct. Hence, Initially it
 * should always be > 0.
 *
 * There are two code paths for initializing the power tables:
 * 1. When a tracon sharing is not done. The words then are directly
 * scanned for their disjuncts and connectors. Each one is inserted with a
 * refcount set to 1 (because there is no connector memory sharing).
 * 2. Using the tracon list tables (left and right). Each slot is an index
 * into the connector memory block, which is the start of the
 * corresponding tracon. The word number is extracted from the deepest
 * connector (assigned to it by setup_connectors()).
 */
static void power_table_init(Sentence sent, Tracon_sharing *ts, power_table *pt)
{
	Tracon_list *tl = ts->tracon_list;

	power_table_alloc(sent, pt);

	Pool_desc *mp = pt->memory_pool = pool_new(__func__, "C_list",
	                   /*num_elements*/2048, sizeof(C_list),
	                   /*zero_out*/false, /*align*/false, /*exact*/false);

	/* The below uses variable-sized hash tables. The sizes are provided
	 * in num_cnctrs_per_word[][], which is the number of different
	 * uppercase connectors, indexed by direction and word. It is computed in
	 * pack_connectors(). */

	/* Calculate the sizes of the hash tables. */
	unsigned int num_headers = 0;
	C_list **memblock_headers;
	C_list **hash_table_header;

	unsigned int *ncu[2];
	ncu[0] = alloca(sent->length * sizeof(*ncu[0]));
	ncu[1] = alloca(sent->length * sizeof(*ncu[1]));

	for (WordIdx w = 0; w < sent->length; w++)
	{
		for (size_t dir = 0; dir < 2; dir++)
		{
			unsigned int tsize;
			unsigned int n = ts->num_cnctrs_per_word[dir][w];

			if (0 == n)
			{
				tsize = 1; /* Avoid parse-time table size checks. */
			}
			else
			{
				tsize = next_power_of_two_up(3 * n); /* At least 66% free. */
			}

			ncu[dir][w] = tsize;
			num_headers += tsize;
		}
	}

	memblock_headers = malloc(num_headers * sizeof(C_list *));
	memset(memblock_headers, 0, num_headers * sizeof(C_list *));
	hash_table_header = memblock_headers;

	for (WordIdx w = 0; w < sent->length; w++)
	{
		for (size_t dir = 0; dir < 2; dir++)
		{
			unsigned int tsize = ncu[dir][w];
			C_list **t = hash_table_header;

			hash_table_header += tsize;

			pt->table[dir][w] = t;
			pt->table_size[dir][w] = tsize;
			memset(t, 0, sizeof(C_list *) * tsize);

			if (NULL == tl)
			{
				Connector *c;

				for (Disjunct *d = sent->word[w].d; d != NULL; d = d->next)
				{
					/* Insert the deep connectors. */
					c = (dir == 0) ? d->left : d->right;
					if (c == NULL) continue;

					for (c = c->next; c != NULL; c = c->next)
						put_into_power_table(mp, tsize, t, c);
				}

				for (Disjunct *d = sent->word[w].d; d != NULL; d = d->next)
				{
					/* Insert the shallow connectors. */
					c = (dir == 0) ? d->left : d->right;
					if (c == NULL) continue;

					put_into_power_table(mp, tsize, t, c);
				}
			}
		}
	}

	assert(memblock_headers + num_headers == hash_table_header,
	   "Mismatch header sizes");

	if (NULL != tl)
	{
		/* Bulk insertion with reference count. */

		for (size_t dir = 0; dir < 2; dir++)
		{
			C_list ***tp = pt->table[dir];
			unsigned int *sizep = pt->table_size[dir];
			unsigned int sid_entries = tl->entries[dir];

			/* Insert the deep connectors, then the shallow ones. */
			for (int shallow = 0; shallow < 2; shallow++)
			{
				for (unsigned int id = 0; id < sid_entries; id++)
				{
					Connector *c = get_tracon(ts, dir, id);
					if (!!shallow != c->shallow) continue;

					int w = get_tracon_word_number(c, dir);

					put_into_power_table(mp, sizep[w], tp[w], c);
				}
			}
		}
	}
}

/**
 * This runs through all the connectors in this table, and eliminates those
 * with a zero reference count.
 * In order to support a unique uppercase part per hash entry, we need to
 * ensure that a collision chain doesn't break when the last remaining
 * connector in an entry is removed. To that end, if the entry is not last
 * in the chain, instead of removing it we replace it with a permanent
 * "tombstone" connector that cannot match.
 */
static void clean_table(unsigned int size, C_list **t)
{
	/* Table entry tombstone. */
#define UC_NUM_TOMBSTONE ((connector_uc_hash_t)-1)
	static condesc_more_t cm_no_match =
	{
		.string = "TOMBSTONE",
	};
	static condesc_t desc_no_match =
	{
		.uc_num = UC_NUM_TOMBSTONE, /* get_power_table_entry() will skip. */
		.more = &cm_no_match
	};
	static Connector con_no_match =
	{
		.desc = &desc_no_match,
		.refcount = 1,    /* Ensure it will not get removed. */
		.shallow = false, /* Faster mismatch + get_num_con_uc() skips it. */
	};

	for (unsigned int i = 0; i < size; i++)
	{
		C_list **m = &t[i];

		while (NULL != *m)
		{
			assert(0 <= (*m)->c->refcount, "refcount < 0 (%d)",
			       (*m)->c->refcount);
			if (0 == (*m)->c->refcount)
			{
				if ((*m == t[i]) && (NULL == (*m)->next) && /* Nothing remains. */
				    (NULL != t[(i+1) & (size-1)]))          /* Not end of chain. */
				{
					(*m)->c = &con_no_match;                 /* Place a tombstone. */
				}
				else
				{
					*m = (*m)->next;
				}
			}
			else
			{
				m = &(*m)->next;
			}
		}
	}
}

#if TOO_MUCH_OVERHEAD
/**
 * Validate that at least one disjunct of \p w may have no cross link.
 * FIXME maybe: Cache the checks for each word: Build an array indexed by
 * LHS nearest_word with values of the minimum RHS nearest_word of all the
 * disjuncts with this LHS nearest_word value or less than it. The size of
 * this array is ml[w].nw[1].
 **/
static bool find_no_xlink_disjunct(prune_context *pc, int w,
                                   Connector *lc, Connector *rc,
                                   int lword, int rword)
{
	Sentence sent = pc->sent;
	Disjunct *d = sent->word[w].d;

	/* One-direction disjuncts have already been validated.  */
	if ((pc->ml[w].nw[0]  == w) || (pc->ml[w].nw[1] == w)) return true;

	for (d = sent->word[w].d; d != NULL; d = d->next)
	{
		if (d->left->nearest_word < lword)
			continue;
		if (d->right->nearest_word > rword)
			continue;
		break;
	}
	if (d == NULL)
	{
		PR(N);
		return false;
	}

	/* More cross links can be detected here, but their resolution requires
	 * implementing backtracking in *_connector_list_update(). */

	return true;
}
#endif

static bool
left_table_search(prune_context *pc, int w, Connector *c,
                  bool shallow, int word_c);
static bool
right_table_search(prune_context *pc, int w, Connector *c,
                   bool shallow, int word_c);

static bool is_match(prune_context *pc,
            bool (*table_search)(prune_context *, int, Connector *, bool, int),
            int word_c, Connector *c, int w)
{
	if (c->next == NULL)
	{
	    if (!c->multi) return false;
	}
	else
	{
		c = connector_deepest(c);
	}

	/* c may only connect to a shallow connector on w. */
	return table_search(pc, w, c, false, word_c);
}

static bool is_cross_mlink(prune_context *pc,
                           Connector *lc, Connector *rc,
                           int lword, int rword)
{
	if (rword - lword == 1) return false;

	if (pc->ml == NULL) return false;

	Sentence sent = pc->sent;
	int null_allowed = pc->null_links - pc->null_words;

	if (pc->islands_ok)
	{
		if (null_allowed > 0) return false;
	}
	else
	{
		if (null_allowed > rword - lword - 1) return false;
	}

	for (int w = lword+1; w < rword; w++)
	{
		if (sent->word[w].optional) continue;
		if (pc->is_null_word[w]) continue;

		if ((w == lword+1) && (pc->ml[w].nw_perjet[1] > rword) &&
			 !is_match(pc, left_table_search, lword, lc, w))
		{
			PR(L);
			goto null_word_found;
		}
		if ((w == rword-1) && (pc->ml[w].nw_perjet[0] < lword) &&
		    !is_match(pc, right_table_search, rword, rc, w))
		{
			PR(R);
			goto null_word_found;
		}

		if ((pc->ml[w].nw_perjet[0] < lword) && (pc->ml[w].nw_perjet[1] > rword))
		{
			PR(P);
			goto null_word_found;
		}

#if 1
		/* Links of word w are not allowed to cross the edge words. */
		if ((pc->ml[w].nw[0] < lword) && PR(L)) goto null_word_found;
		if ((pc->ml[w].nw[1] > rword) && PR(R)) goto null_word_found;
#endif

		/* If w has a link to an edge word, links from the deepest
		 * connectors of this word is not allowed to cross a link to w. */
#if 1
		if (lword == pc->ml[w].nw[0])
		{
			/* w has a link to lword, or it's a null word. */
			Connector *c = connector_deepest(lc);
			if (!c->multi && (c->nearest_word > w) && PR(A)) goto null_word_found;
		}
#endif
#if 1
		if (rword == pc->ml[w].nw[1])
		{
			/* w has a link to rword, or it's a null word. */
			Connector *c = connector_deepest(rc);
			if (!c->multi && (c->nearest_word < w) && PR(B)) goto null_word_found;
		}
#endif

		if ((lc->next != NULL) && (rc->next != NULL))
		{
#if VERY_WEAK
			if ((pc->ml[w].nw[0] < lc->next->nearest_word) &&
			    (rc->next->nearest_word < w))
			{
				PR(C);
				goto null_word_found;
			}
#endif
#if 1
			if ((pc->ml[w].nw[1] > rc->next->nearest_word) &&
			    (lc->next->nearest_word > w))
			{
				PR(D);
				goto null_word_found;
			}
#endif
		}

#if TOO_MUCH_OVERHEAD
		if (!find_no_xlink_disjunct(pc, w, lc, rc, lword, rword))
			goto null_word_found;
#endif

		continue;
null_word_found:
		if (null_allowed == 0)
		{
			pc->N_xlink++;
			return true;
		}
		null_allowed--;
		continue;
	}

	return false;
}

/**
 * Find if words w1 and w2 may become adjacent due to optional words.
 * This may happen if they contain only optional words between them.
 *
 * Return true iff they may become adjacent (i.e. all the words
 * between them are optional).
 */
bool optional_gap_collapse(Sentence sent, int w1, int w2)
{
	for (int w = w1+1; w < w2; w++)
		if (!sent->word[w].optional) return false;

	return true;
}

/**
 * Find if the sentence contains more than pc->null_links nulls.
 * This function is called when we know that the gap (w1, w2) creates one
 * or more null links. Only non-optional words are counted here (optional
 * words are always allowed to be null-linked).
 *
 * Algo: First determine null_allowed - the number of nulls which is
 * allowed in the gap. If the gap includes more than null_allowed nulls
 * this implies that the sentence would include more than the desired null
 * links (pc->null_links) so true is returned to signify that the proposed
 * link cannot be done.
 * If islands_ok==true, the whole gap may be one island, so it is counted
 * only as a single null. Also, since only null words are counted here,
 * the number of the total null links may be under-estimated (efficiency
 * effect only).
 * Note: Existing null words in the gap are just skipped as they have
 * already taken into account in null_allowed.
 *
 * @return \c true only if the sentence contains more than pc->null_links
 * nulls. With \c islands_ok=true, \c false may still be returned in that
 * case because only null words are counted here.
 */
static bool more_nulls_than_allowed(prune_context *pc, int w1, int w2)
{
	int null_allowed = pc->null_links - pc->null_words;

	if (pc->islands_ok)
	{
		if (null_allowed > 0) return false;
	}
	else
	{
		if (null_allowed > w2 - w1 - 1) return false;
	}

	for (int w = w1+1; w < w2; w++)
	{
		if (pc->sent->word[w].optional) continue;
		if (pc->is_null_word[w]) continue;
		if (null_allowed == 0) return true;
		null_allowed--;
	}

	return false;
}

/**
 * This takes two connectors (and the two words that these came from) and
 * returns TRUE if it is possible for these two to match based on local
 * considerations.
 */
static bool possible_connection(prune_context *pc,
                                Connector *lc, Connector *rc,
                                int lword, int rword)
{
	int dist = rword - lword;
#ifdef DEBUG
	assert(0 < dist, "Bad word order in possible connection.");
#endif

	if (!lc_easy_match(lc->desc, rc->desc)) return false;

	/* Word range constraints. */
	if ((lc->nearest_word > rword) || (rc->nearest_word < lword)) return false;

	if (1 == dist)
	{
		if ((lc->next != NULL) || (rc->next != NULL))
			return false;
		return true;
	}

	if ((rword > lc->farthest_word) || (lword < rc->farthest_word))
		return false;

	/* We arrive here if the words are NOT next to each other. Say the
	 * gap between them contains W non-optional words. We also know
	 * that we are going to parse with N null-links (which involves
	 * sub-parsing with the range of [0, N] null-links).
	 * If there is not at least one intervening connector that can
	 * connect to an intermediate word, then:
	 * islands_ok=false:
	 * There will be W null-linked words, and W must be <= N.
	 * islands_ok=true:
	 * There will be at least one island.
	 */
	if ((lc->next == NULL) && (rc->next == NULL) &&
	    (!lc->multi || (lc->nearest_word == rword)) &&
	    (!rc->multi || (rc->nearest_word == lword)) &&
	    more_nulls_than_allowed(pc, lword, rword))
	{
		return false;
	}

	if ((lc->next != NULL) && (rc->next != NULL))
	{
		if (lc->next->nearest_word > rc->next->nearest_word)
			return false; /* Cross link. */
	}

	if (is_cross_mlink(pc, lc, rc, lword, rword))
		return false;

	return true;
}

/**
 * This returns TRUE if the right table of word w contains
 * a connector that can match to c.  shallow tells if c is shallow.
 */
static bool
right_table_search(prune_context *pc, int w, Connector *c,
                   bool shallow, int word_c)
{
	power_table *pt = pc->pt;
	unsigned int size = pt->table_size[1][w];
	C_list **e = get_power_table_entry(size, pt->table[1][w], c);

	for (C_list *cl = *e; cl != NULL; cl = cl->next)
	{
		/* Two deep connectors can't work */
		if (!shallow && !cl->c->shallow) return false;

		if (possible_connection(pc, cl->c, c, w, word_c))
			return true;
	}

	return false;
}

/**
 * This returns TRUE if the left table of word w contains
 * a connector that can match to c.  shallow tells if c is shallow
 */
static bool
left_table_search(prune_context *pc, int w, Connector *c,
                  bool shallow, int word_c)
{
	power_table *pt = pc->pt;
	unsigned int size = pt->table_size[0][w];
	C_list **e = get_power_table_entry(size, pt->table[0][w], c);

	for (C_list *cl = *e; cl != NULL; cl = cl->next)
	{
		/* Two deep connectors can't work */
		if (!shallow && !cl->c->shallow) return false;

		if (possible_connection(pc, c, cl->c, word_c, w))
			return true;
	}
	return false;
}

/**
 * Take this connector list, and try to match it with the words
 * w-1, w-2, w-3...  Returns the word to which the first connector of
 * the list could possibly be matched.  If c is NULL, returns w.  If
 * there is no way to match this list, it returns -1 (which is also
 * BAD_WORD in unsigned 8-bit representation).
 * If it does find a way to match it, it updates the c->nearest_word
 * and c->farthest_word fields correctly. When tracons are shared, this
 * update is done simultaneously on all of them. The main loop of
 * power_prune() then marks them with the pass number that is checked
 * here.
 */
static int
left_connector_list_update(prune_context *pc, Connector *c,
                           int w, bool shallow)
{
	int n, lb;
	int foundmatch = -1;

	if (c == NULL) return w;
	if (c->prune_pass == pc->pass_number) return c->nearest_word;
	n = left_connector_list_update(pc, c->next, w, false) - 1;
	if (0 > n) return -1;
	if (((int) c->nearest_word) < n) n = c->nearest_word;

	/* lb is now the leftmost word we need to check */
	lb = c->farthest_word;

	/* n is now the rightmost word we need to check */
	for (; n >= lb; n--)
	{
		pc->power_cost++;
		if (right_table_search(pc, n, c, shallow, w))
		{
			foundmatch = n;
			break;
		}
	}
	if (foundmatch < ((int) c->nearest_word))
	{
		c->nearest_word = foundmatch;
		pc->N_changed++;
	}

	if (foundmatch != -1)
	{
		int farthest_word = n;
		for (int l = lb; l < n; l++)
		{
			pc->power_cost++;
			if (right_table_search(pc, l, c, shallow, w))
			{
				farthest_word = l;
				break;
			}
		}

		if (farthest_word > (int)c->farthest_word)
		{
			c->farthest_word = farthest_word;
			pc->N_changed++;
		}
	}

	return foundmatch;
}

/**
 * Take this connector list, and try to match it with the words
 * w+1, w+2, w+3...  Returns the word to which the first connector of
 * the list could possibly be matched.  If c is NULL, returns w.
 * If there is no way to match this list, it returns BAD_WORD, which is
 * always greater than N_words - 1.
 * If it does find a way to match it, it updates the c->nearest_word and
 * c->farthest_word fields correctly.  Regarding pass_number, see the
 * comment in left_connector_list_update().
 */
static size_t
right_connector_list_update(prune_context *pc, Connector *c,
                            size_t w, bool shallow)
{
	int n, ub;
	int sent_length = (int)pc->sent->length;
	int foundmatch = BAD_WORD;

	if (c == NULL) return w;
	if (c->prune_pass == pc->pass_number) return c->nearest_word;
	n = right_connector_list_update(pc, c->next, w, false) + 1;
	if (sent_length <= n) return BAD_WORD;
	if (c->nearest_word > n) n = c->nearest_word;

	/* ub is now the rightmost word we need to check */
	ub = c->farthest_word;

	/* n is now the leftmost word we need to check */
	for (; n <= ub; n++)
	{
		pc->power_cost++;
		if (left_table_search(pc, n, c, shallow, w))
		{
			foundmatch = n;
			break;
		}
	}
	if (foundmatch > c->nearest_word) {
		c->nearest_word = foundmatch;
		pc->N_changed++;
	}

	if (n <= ub)
	{
		int farthest_word = n;
		for (int l = ub; l > n; l--)
		{
			pc->power_cost++;
			if (left_table_search(pc, l, c, shallow, w))
			{
				farthest_word = l;
				break;
			}
		}

		if (farthest_word < (int)c->farthest_word)
		{
			c->farthest_word = farthest_word;
			pc->N_changed++;
		}
	}

	return foundmatch;
}

static void mark_jet_as_good(Connector *c, int pass_number)
{
	for (; NULL != c; c = c->next)
		c->prune_pass = pass_number;
}

static void mark_jet_for_dequeue(Connector *c, bool mark_bad_word)
{
	/* The following can actually be omitted as long as (unsigned char)-1
	 * is equal to BAD_WORD. However, The question is how to define
	 * BAD_WORD cleanly so it will be immune to increasing MAX_SENTENCE. */
	if (mark_bad_word) c->nearest_word = BAD_WORD;

	for (; NULL != c; c = c->next)
	{
		c->refcount--;
	}
}

static bool is_bad(Connector *c)
{
	for (; c != NULL; c = c->next)
		if (c->nearest_word == BAD_WORD) return true;

	return false;
}

/**
 * Count null words and mark their location.
 * Optional words are not checked, as they cannot contribute to the null
 * word count.
 *
 * If word w is a new null word, mark its location in pc->is_null_word and
 * increment pc->null_word.
 * @return \c true iff a new null word increases the count to be over the
 * requested maximum pc->null_links.
 */
static bool check_null_word(prune_context *pc, int w)
{
	if (pc->always_parse) return false;
	Word *word = &pc->sent->word[w];

	if ((word->d == NULL) && !word->optional && !pc->is_null_word[w])
	{
		pc->null_words++;
		pc->is_null_word[w] = true;
		if (pc->null_words > pc->null_links) return true;
	}
	return false;
}

/**
 * Return \c true if a pass has not made any changes.
 * Also print pruning pass stats.
 */
static bool pruning_pass_end(prune_context *pc, const char *pass_dir,
                             int *prune_total)
{
	int total = pc->N_deleted[0] + pc->N_deleted[1];

	char xlink_found[32] = "";
	if (pc->N_xlink != 0)
		snprintf(xlink_found, sizeof(xlink_found), ", xlink=%d", pc->N_xlink);

	lgdebug(D_PRUNE, "Debug: %s pass changed %d and deleted %d (%d+%d)%s\n",
	        pass_dir, pc->N_changed, total, pc->N_deleted[0], pc->N_deleted[1],
	        xlink_found);

	bool pass_end = ((pc->N_changed == 0) && (total == 0));
	pc->N_changed = pc->N_deleted[0] = pc->N_deleted[1] = pc->N_xlink = 0;

	*prune_total += total;
	return pass_end;
}

/** The return value is the number of disjuncts deleted.
 *  Implementation notes:
 *  Normally tracons are memory shared (with the exception that
 *  tracons that start with a shallow connectors are not shared with
 *  ones starting with a deep connector). For further details on tracon
 *  sharing, see the comment on them in disjunct-utils.c.
 *
 *  The refcount of each connector serves as its reference count in the
 *  power table. Each time a connector that cannot match is
 *  discovered, its reference count is decreased, and the nearest_word
 *  field of its jet is assigned BAD_WORD. Due to the tracon memory
 *  sharing, each change of the reference count and the nearest_word
 *  field affects simultaneously all the identical tracons (and the
 *  corresponding connectors in the power table). The corresponding
 *  disjuncts are discarded on the fly, and additional disjuncts with jets
 *  so marked with BAD_WORD are discarded when encountered without a
 *  further check. Each tracon is handled only once in the same main loop
 *  pass by marking their connectors with the pass number in their
 *  prune_pass field.
 *
 *  Null words (words w/o disjuncts) are detected on the fly (the initial
 *  ones are detected on the first pass). When the number of the null
 *  words indicates there will be no parse with the given pc->null_links,
 *  return -1. Else return the number of discarded disjuncts.
 */
static int power_prune(Sentence sent, prune_context *pc, Parse_Options opts)
{
	int total_deleted = 0;
	bool extra_null_word = false;
	power_table *pt = pc->pt;

	pc->N_changed = 1;      /* forces it always to make at least two passes */

	do
	{
		pc->pass_number++;

		/* Left-to-right pass. */
		for (WordIdx w = 0; w < sent->length; w++)
		{
			for (Disjunct **dd = &sent->word[w].d; *dd != NULL; /* See: NEXT */)
			{
				Disjunct *d = *dd; /* just for convenience */
				if (d->left == NULL)
				{
					dd = &d->next;  /* NEXT */
					continue;
				}

				bool bad = is_bad(d->left);
				if (bad || left_connector_list_update(pc, d->left, w, true) < 0)
				{
					mark_jet_for_dequeue(d->left, true);
					mark_jet_for_dequeue(d->right, false);

					/* Discard the current disjunct. */
					*dd = d->next; /* NEXT - set current disjunct to the next one */
					if (d->is_category != 0) free(d->category);
					pc->N_deleted[(int)bad]++;
					continue;
				}

				mark_jet_as_good(d->left, pc->pass_number);
				dd = &d->next; /* NEXT */
			}

			if (check_null_word(pc, w)) extra_null_word = true;
			clean_table(pt->table_size[1][w], pt->table[1][w]);
		}

		if (pruning_pass_end(pc, "l->r", &total_deleted)) break;

		/* Right-to-left pass. */
		for (WordIdx w = sent->length-1; w != (WordIdx) -1; w--)
		{
			for (Disjunct **dd = &sent->word[w].d; *dd != NULL; /* See: NEXT */)
			{
				Disjunct *d = *dd; /* just for convenience */
				if (d->right == NULL)
				{
					dd = &d->next;  /* NEXT */
					continue;
				}

				bool bad = is_bad(d->right);
				if (bad || right_connector_list_update(pc, d->right, w, true) >= sent->length)
				{
					mark_jet_for_dequeue(d->right, true);
					mark_jet_for_dequeue(d->left, false);

					/* Discard the current disjunct. */
					*dd = d->next; /* NEXT - set current disjunct to the next one */
					if (d->is_category != 0) free(d->category);
					pc->N_deleted[(int)bad]++;
					continue;
				}

				mark_jet_as_good(d->right, pc->pass_number);
				dd = &d->next; /* NEXT */
			}

			if (check_null_word(pc, w)) extra_null_word = true;
			clean_table(pt->table_size[0][w], pt->table[0][w]);
		}

		if (pruning_pass_end(pc, "r->l", &total_deleted)) break;

		/* The above debug printouts revealed that the xlink counter doesn't
		 * get increased after the first 2 passes. So neutralize the mlink table
		 * here to save a slight overhead. */
		pc->ml = NULL;
	}
	while (!extra_null_word || pc->always_parse);

	char found_nulls[32] = "";
	if ((verbosity >= D_USER_TIMES) && !extra_null_word && (pc->null_words > 0))
		snprintf(found_nulls, sizeof(found_nulls), ", found %u", pc->null_words);
	print_time(opts, "power pruned (for %u null%s%s%s)",
					  pc->null_links, (pc->null_links != 1) ? "s" : "",
	           extra_null_word ? ", extra null" : "", found_nulls);
	if (verbosity_level(D_PRUNE))
	{
		prt_error("\n\\");
		prt_error("Debug: Power prune cost: %d\n", pc->power_cost);
		prt_error("Debug: After power_pruning (for %u null%s, sent->null_count %u):\n\\",
		          pc->null_links, (pc->null_links != 1) ? "s" : "", pc->sent->null_count);
		print_disjunct_counts(pc->sent);
	}

#ifdef DEBUG
	/* The rest of the code has a critical dependency on the correctness of
	 * the pruning. */
	for (WordIdx w = 0; w < sent->length; w++)
	{
		for (Disjunct *d = sent->word[w].d; NULL != d; d = d->next)
		{
			for (int dir = 0; dir < 2; dir++)
			{
				Connector *c = (dir) ? (d->left) : (d->right);
				for (; NULL != c; c = c->next)
				{
					assert(c->nearest_word != BAD_WORD, "dir %d w %zu", dir, w);
					assert(c->refcount > 0, "dir %d w %zu", dir, w);
				}
			}
		}
	}
#endif

	if (extra_null_word && !pc->always_parse) return -1;
	return total_deleted;
}

/* ===================================================================
   PP Pruning (original comments from version 3 - some not up to date).

   The "contains one" post-processing constraints give us a new way to
   prune.  Suppose there's a rule that says "a group that contains foo
   must contain a bar or a baz."  Here foo, bar, and baz are connector
   types.  foo is the trigger link, bar and baz are called the criterion
   links.  If, after considering the disjuncts we find that there is is
   a foo, but neither a bar, nor a baz, then we can eliminate the disjunct
   containing bar.

   Things are actually a bit more complex, because of the matching rules
   and subscripts.  The problem is that post-processing deals with link
   names, while at this point all we have to work with is connector
   names.  Consider the foo part.  Consider a connector C.  When does
   foo match C for our purposes?  It matches it if every possible link
   name L (that can result from C being at one end of that link) results
   in post_process_match(foo,L) being true.  Suppose foo contains a "*".
   Then there is no C that has this property.  This is because the *s in
   C may be replaced by some other subscripts in the construction of L.
   And the non-* chars in L will not post_process_match the * in foo.

   So let's assume that foo has no *.  Now the result we want is simply
   given by post_process_match(foo, C).  Proof: L is the same as C but
   with some *s replaced by some other letters.  Since foo contains no *
   the replacement in C of some * by some other letter could change
   post_process_match from FALSE to TRUE, but not vice versa.  Therefore
   it's conservative to use this test.

   For the criterion parts, we need to determine if there is a
   collection of connectors C1, C2,... such that by combining them you
   can get a link name L that post_process_matches bar or baz.  Here's a
   way to do this.  Say bar="Xabc".  Then we see if there are connector
   names that post_process_match "Xa##", "X#b#", and "X##c".  They must
   all be there in order for it to be possible to create a link name
   "Xabc".  A "*" in the criterion part is a little different.  In this
   case we can simply skip the * (treat it like an upper case letter)
   for this purpose.  So if bar="X*ab" then we look for "X*#b" and
   "X*a#".  (The * in this case could be treated the same as another
   subscript without breaking it.)  Note also that it's only necessary
   to find a way to match one of the many criterion links that may be in
   the rule.  If none can match, then we can delete the disjunct
   containing C.

   Here's how we're going to implement this.  We'll maintain a multiset
   of connector names.  We'll represent them in a hash table, where the
   hash function uses only the upper case letters of the connector name.
   We'll insert all the connectors into the multiset.  The multiset will
   support the operation of deletion (probably simplest to just
   decrement the count).  Here's the algorithm.

   Insert all the connectors into M.

   While the previous pass caused a count to go to 0 do:
	  For each connector C do
		 For each rule R do
			if C is a trigger for R and the criterion links
			of the rule cannot be satisfied by the connectors in
			M, Then:
			   We delete C's disjunct.  But before we do,
			   we remove all the connectors of this disjunct
			   from the multiset.  Keep tabs on whether or not
			   any of the counts went to 0.



	Efficiency hacks to be added later:
		Note for a given rule can become less and less satisfiable.
		That is, rule_satisfiable(r) for a given rule r can change from
		TRUE to FALSE, but not vice versa.  So once it's FALSE, we can just
		remember that.

		Consider the effect of a pass p on the set of rules that are
		satisfiable.  Suppose this set does not change.  Then pass p+1
		will do nothing.  This is true even if pass p caused some
		disjuncts to be deleted.  (This observation will only obviate
		the need for the last pass.)

  */

typedef struct cms_struct Cms;
struct cms_struct
{
	Cms *next;
	Connector *c;
	bool last_criterion;        /* Relevant connectors for last criterion link */
	bool left;
	bool right;
};

#define CMS_SIZE (1<<11)       /* > needed; reduce to debug memory pool */
typedef struct multiset_table_s multiset_table;
struct multiset_table_s
{
	Cms memblock[CMS_SIZE];     /* Initial Cms elements - usually enough */
	Cms *mb;                    /* Next available element from memblock */
	Pool_desc *mp;              /* In case memblock is not enough */
	String_set *sset;
	Cms *cms_table[CMS_SIZE];
};

static multiset_table *cms_table_new(Sentence sent)
{
	multiset_table *mt = malloc(sizeof(multiset_table));
	memset(mt->cms_table, 0, CMS_SIZE * sizeof(Cms *));
	mt->mb = mt->memblock;
	mt->mp = NULL;
	mt->sset = sent->string_set;

	return mt;
}

static void cms_table_delete(multiset_table *mt)
{
	if (mt->mp != NULL) pool_delete(mt->mp);
	free(mt);
}

static unsigned int cms_hash(const char *s)
{
	unsigned int i = 5381;
	if (islower((unsigned int)*s)) s++; /* skip head-dependent indicator */
	while (is_connector_name_char(*s))
	{
		i = ((i << 5) + i) + *s;
		s++;
	}
	return (i & (CMS_SIZE-1));
}

static void reset_last_criterion(multiset_table *cmt, const char *criterion)
{
	unsigned int h = cms_hash(criterion);

	for (Cms *cms = cmt->cms_table[h]; cms != NULL; cms = cms->next)
		cms->last_criterion = false;
}

/** Find if a connector t can form link x so post_process_match(s, x)==true.
 *  Only t may have a head-dependent indicator, which is skipped if found.
 *  The leading uppercase characters must exactly match.
 *  The rest (the subscript) should be able to from the subscript of x.
 *  (*e) is a character that exists in x at the same position as in s
 *  (e indicates both the character and its position in s); t is ok
 *  iff this character post-process-matches the corresponding one at s,
 *  and the rest post-process-match or are '*'.
 *  Examples:
 *     s="Xabc"; t="Xa*c"; e=s[3]; // *e == 'c'; // Returns true;
 *     s="Xabc"; t="Xa*c"; e=s[2]; // *e == 'b'; // Returns false;
 *     s="Xabc"; t="Xa*d"; e=s[1]; // *e == 'a'; // Returns false;
 *     s="X*ab"; t="Xcab"; e=s[1]; // *e == '*'; // Returns false;
 *     s="Xa*b"; t="Xacb"; e=s[1]; // *e == 'a'; // Returns false;
 *     s="Xa*b"; t="Xa";   e=s[1]; // *e == 'a'; // Returns true;
 *     s="Xa*#"; t="Xa*d"; e=s[1]; // *e == 'a'; // Returns true;
 *     s="Xa*#"; t="Xa";   e=s[2]; // *e == '*'; // Returns true;
 *     s="X";    t="Xab";  e=s[1]; // *e = '\0'; // Returns true;
 */
static bool can_form_link(const char *s, const char *t, const char *e)
{
	if (islower((unsigned char)*t)) t++; /* Skip head-dependent indicator */
	while (is_connector_name_char(*s))
	{
		if (*s != *t) return false;
		s++;
		t++;
	}
	if (is_connector_name_char(*t)) return false;
	while (*t != '\0')
	{
		if (*s == '\0') return true;
		if (*s != *t && *s != '#' && (s == e || *t != '*')) return false;
		s++;
		t++;
	}
	while (*s != '\0')
	{
		if (*s != '*' && *s != '#' && s == e) return false;
		s++;
	}
	return true;
}

#ifdef DEBUG_PP_PRUNE
static const char *connector_signs(Cms *cms)
{
	static char buf[3];
	size_t i = 0;

	if (cms->left) buf[i++] = '-';
	if (cms->right) buf[i++] = '+';
	buf[i] = '\0';

	return buf;
}
#else
#define connector_signs(x) NULL /* For YCM IDE */
#endif /* DEBUG_PP_PRUNE */

/**
 * Returns TRUE iff there is a connector name in the sentence
 * that can create a link x such that post_process_match(pp_link, x) is TRUE.
 * subscr is a pointer into pp_link that indicates the position of the
 * connector subscr that is tested.
 */
static bool match_in_cms_table(multiset_table *cmt, const char *pp_link,
                               const char *subscr)
{
	unsigned int h = cms_hash(pp_link);

	bool found = false;
	for (Cms *cms = cmt->cms_table[h]; cms != NULL; cms = cms->next)
	{
		if (cms->c->nearest_word == BAD_WORD) continue;

		if (can_form_link(pp_link, connector_string(cms->c), subscr))
		{
			ppdebug("MATCHED %s%s\n", connector_string(cms->c),connector_signs(cms));
			cms->last_criterion = true;
			found = true;
			continue;
		}
		ppdebug("NOT-MATCHED %s%s\n", connector_string(cms->c),connector_signs(cms));
	}

	return found;
}

/* FIXME? There is some code duplication here and in insert_in_cms_table()
 * but it seems cumbersome to fix it. */
static Cms *lookup_in_cms_table(multiset_table *cmt, Connector *c)
{
	unsigned int h = cms_hash(connector_string(c));

	for (Cms *cms = cmt->cms_table[h]; cms != NULL; cms = cms->next)
	{
		if (c->desc == cms->c->desc) return cms;
	}

	return NULL;
}

static Cms *cms_alloc(multiset_table *cmt)
{
	if (cmt->mb < &cmt->memblock[CMS_SIZE])
		return cmt->mb++;

	if (cmt->mp == NULL)
	{
		cmt->mp = pool_new(__func__, "Cms",
		                   /*num_elements*/CMS_SIZE, sizeof(Cms),
		                   /*zero_out*/false, /*align*/false, /*exact*/false);
	}

	return pool_alloc(cmt->mp);
}

static void insert_in_cms_table(multiset_table *cmt, Connector *c, int dir)
{
	Cms *cms, *prev = NULL;
	unsigned int h = cms_hash(connector_string(c));

	for (cms = cmt->cms_table[h]; cms != NULL; cms = cms->next)
	{
		if (c->desc == cms->c->desc) break;
		prev = cms;
	}

	if (cms == NULL)
	{
		cms = cms_alloc(cmt);
		cms->c = c;
		cms->next = cmt->cms_table[h];
		cmt->cms_table[h] = cms;
		cms->left = cms->right = false;
	}
	else
	{
		/* MRU order */
		if (prev != NULL)
		{
			prev->next = cms->next;
			cms->next = cmt->cms_table[h];
			cmt->cms_table[h] = cms;
		}
	}

	if (dir == 0)
		cms->left = true;
	else
		cms->right = true;

	cms->last_criterion = false;
}

#define AtoZ "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

/** Validate that the connectors needed in order to create a
 *  link that matches pp_link, are all found in the sentence.
 *  The sentence's connectors are in the cms table.
 */
static bool all_connectors_exist(multiset_table *cmt, const char *pp_link)
{
	ppdebug("check PP-link=%s\n", pp_link);

	const char *s;
	for (s = pp_link; is_connector_name_char(*s); s++) {}

	/* The first iteration of the next loop handles the special case
	 * which occurs if there were 0 subscripts. */
	do
	{
		ppdebug("subscript at %d\n", (int)(s-pp_link-strspn(pp_link, AtoZ)));
		if (*s == '#') continue;
		if (!match_in_cms_table(cmt, pp_link, s)) return false;
	}
	while (*s++ != '\0' && *s != '\0'); /* while characters still exist */

	return true;
}

static bool connector_has_direction(Cms *cms, int dir)
{
	return ((dir == 0) && cms->left) || ((dir == 1) && cms->right);
}

static bool any_possible_connection(multiset_table *cmt, const char *criterion)
{
	unsigned int h = cms_hash(criterion);

	for (Cms *cms1 = cmt->cms_table[h]; cms1 != NULL; cms1 = cms1->next)
	{
		if (!cms1->last_criterion) continue;

		ppdebug("TRY %s%s\n", connector_string(cms1->c), connector_signs(cms1));
		for (int dir = 0; dir < 2; dir++)
		{
			if (!connector_has_direction(cms1, dir)) continue;
			Connector *c = cms1->c;

			for (Cms *cms2 = cmt->cms_table[h]; cms2 != NULL; cms2 = cms2->next)
			{
				if (!connector_has_direction(cms2, 1-dir)) continue;
				Connector *cfl = cms2->c;

				if (easy_match_desc(cfl->desc, c->desc))
				{
					const char *link = intersect_strings(cmt->sset, cfl, c);
					if (post_process_match(criterion, link))
					{
						ppdebug("%s+ %s- PPLINK\n", connector_string(cfl), connector_string(c));
						reset_last_criterion(cmt, criterion);
						return true;
					}
					ppdebug("%s+ %s- NO PPLINK\n", connector_string(cfl), connector_string(c));
					continue;
				}
				ppdebug("%s+ %s- NOMATCH\n", connector_string(cfl), connector_string(c));
			}
		}
	}

	ppdebug(">>>No connection possible\n");
	reset_last_criterion(cmt, criterion);
	return false;
}

static bool rule_satisfiable(multiset_table *cmt, pp_linkset *ls)
{
	for (unsigned int hashval = 0; hashval < ls->hash_table_size; hashval++)
	{
		for (pp_linkset_node *p = ls->hash_table[hashval]; p != NULL; p = p->next)
		{
			/* OK, we've got our hands on one of the criterion links.
			 * Now we want to see if we can satisfy this criterion link
			 * with a collection of the links in the cms table. */
			if (all_connectors_exist(cmt, p->str))
			{
				ppdebug("TRUE\n");
				if (any_possible_connection(cmt, p->str)) return true;
			}
			reset_last_criterion(cmt, p->str);
		}
	}

	ppdebug("FALSE\n");
	return false;
}

/** Mark bad trigger connectors for later deletion by power_prune().
 */
static bool mark_bad_connectors(multiset_table *cmt, Connector *c)
{
	if (c->nearest_word == BAD_WORD)
		return true; /* Already marked (mainly by connector memory-sharing). */

	Cms *cms = lookup_in_cms_table(cmt, c);
	if (cms->c->nearest_word == BAD_WORD)
	{
		c->nearest_word = BAD_WORD;
		return true;;
	}

	return false;
}

/**
 * For selector \p s with a wildcard, validate that no connector in the
 * sentence that matches the candidate trigger connector \p t may create a
 * link x so that post_process_match(s, x)==false.
 *
 * If so, this ensures that the candidate trigger connector may form
 * only links which satisfy the selector. (This bypasses the limitation
 * on selectors with '*' that is mentioned in the original comments).
 *
 * The algo validates that the '*'s in the selector \p s and in an each
 * easy_matched()'ed sentence connector (both '*' padded as needed) are at
 * the same places.
 *
 * @param s A selector which includes a dictionary wildcard ('*').
 * @param t Candidate trigger connector.
 * @param cmt The cms table (sentence connectors)
 * @return \c true iff such a link cannot be formed.
 */
static bool selector_mismatch_wild(multiset_table *cmt, const char *s,
                                   Cms *cms_t)
{
	unsigned int h = cms_hash(s);

	ppdebug("Selector %s, trigger %s\n", s, connector_string(cms_t->c));
	for (Cms *cms = cmt->cms_table[h]; cms != NULL; cms = cms->next)
	{
		if ((cms_t->left && !cms->right) || (cms_t->right && !cms->left))
		    continue;

		size_t len_s = strlen(s);

		if (easy_match_desc(cms_t->c->desc, cms->c->desc))
		{
			const char *c = connector_string(cms->c);


			size_t len_c = strlen(c);
			for (size_t i = 0; i < len_s; i++)
			{
				if ((s[i] == '*') && ((i < len_c) && c[i] != '*'))
				{
					ppdebug("MISMATCH: %s\n", c);
					return true;
				}
			}
			ppdebug("MATCH: %s\n", c);
		}
	}

	return false;
}

static int pp_prune(Sentence sent, Tracon_sharing *ts, Parse_Options opts)
{
	if (sent->postprocessor == NULL) return 0;
	if (!opts->perform_pp_prune) return 0;

	pp_knowledge *knowledge = sent->postprocessor->knowledge;
	multiset_table *cmt = cms_table_new(sent);
	Tracon_list *tl = ts->tracon_list;

	if (NULL != tl)
	{
		for (int dir = 0; dir < 2; dir++)
		{
			for (unsigned int id = 0; id < tl->entries[dir]; id++)
			{
				Connector *c = get_tracon(ts, dir, id);

				if (0 == c->refcount) continue;
				insert_in_cms_table(cmt, c, dir);
			}
		}
	}
	else
	{
		for (WordIdx w = 0; w < sent->length; w++)
		{
			for (Disjunct *d = sent->word[w].d; d != NULL; d = d->next)
			{
				for (int dir = 0; dir < 2; dir++)
				{
					Connector *first_c = (dir) ? (d->left) : (d->right);
					for (Connector *c = first_c; c != NULL; c = c->next)
					{
						insert_in_cms_table(cmt, c, dir);
					}
				}
			}
		}
	}

	int D_deleted = 0;       /* Number of deleted disjuncts */
	int Cname_deleted = 0;   /* Number of deleted connector names */

	/* After applying a rule once we know if it will be FALSE if we need
	 * to apply it again.
	 * Values: true: Undecided yet; false: Rule unsatisfiable. */
	bool *rule_ok = alloca(knowledge->n_contains_one_rules * sizeof(bool));
	memset(rule_ok, true, knowledge->n_contains_one_rules * sizeof(bool));

	for (size_t i = 0; i < knowledge->n_contains_one_rules; i++)
	{
		pp_rule* rule = &knowledge->contains_one_rules[i]; /* The ith rule */
		const char *selector = rule->selector;  /* Selector string for this rule */
		pp_linkset *link_set = rule->link_set;  /* The set of criterion links */
		unsigned int hash = cms_hash(selector);

		for (Cms *cms = cmt->cms_table[hash]; cms != NULL; cms = cms->next)
		{
			Connector *c = cms->c;
			if (c->nearest_word == BAD_WORD) continue;

			if (!post_process_match(selector, connector_string(c))) continue;
			if (rule->selector_has_wildcard &&
			    selector_mismatch_wild(cmt, selector, cms)) continue;

			ppdebug("Rule %zu: Selector %s, Connector %s\n",
			        i, selector, connector_string(c));
			/* We know c matches the trigger link of the rule. */
			/* Now check the criterion links. */
			if (rule_ok[i] && rule_satisfiable(cmt, link_set)) break;

			rule_ok[i] = false; /* None found; this is permanent. */
			ppdebug("DELETE %s refcount %d\n", connector_string(c), c->refcount);
			c->nearest_word = BAD_WORD;
			Cname_deleted++;
			rule->use_count++;
		}
	}

	/* Iterate over all connectors and mark the bad trigger connectors. */
	if (NULL != tl)
	{
		for (int dir = 0; dir < 2; dir++)
		{
			for (unsigned int id = 0; id < tl->entries[dir]; id++)
			{
				Connector *c = get_tracon(ts, dir, id);

				if (0 == c->refcount) continue;
				if (mark_bad_connectors(cmt, c))
					D_deleted++;
			}
		}
	}
	else
	{
		for (WordIdx w = 0; w < sent->length; w++)
		{
			for (Disjunct *d = sent->word[w].d; d != NULL; d = d->next)
			{
				for (int dir = 0; dir < 2; dir++)
				{
					Connector *first_c = (dir) ? (d->left) : (d->right);
					for (Connector *c = first_c; c != NULL; c = c->next)
					{
						if (mark_bad_connectors(cmt, c))
						{
							D_deleted++;
							break;
						}
					}
				}

			}
		}
	}

	lgdebug(+D_PRUNE+1, "Deleted %d (%d connector names)\n",
	        D_deleted, Cname_deleted);

	cms_table_delete(cmt);

	print_time(opts, "pp pruning");

	return D_deleted;
}

/**
 * Return in num_con_uc the number of different connector uppercase parts
 * indexed by direction (0: left; 1: right) and word.
 * @param sent The sentence struct.
 * @param pt Power table.
 * @param ncu Returned values (indexes: word and direction). Caller allocated.
 * @return Void.
 */
static void get_num_con_uc(Sentence sent,power_table *pt,
                           unsigned int *num_con_uc[])
{
	/* For each table, count the number of non-empty slots. */
	for (WordIdx w = 0; w < sent->length; w++)
	{
		for (size_t dir = 0; dir < 2; dir++)
		{
			C_list **t = pt->table[dir][w];
			unsigned int size = pt->table_size[dir][w];

			unsigned int count = 0;
			for (unsigned int h = 0; h < size; h++)
			{
				if (NULL == t[h]) continue;
				if (!t[h]->c->shallow) continue; /* Skip deep and tombstones. */
				count++;
			}
			num_con_uc[dir][w] = count;
		}
	}
}

static void mlink_table_init(Sentence sent, mlink_table *ml)
{
	for (WordIdx w = 0; w < sent->length; w++)
	{
		ml[w] = (mlink_table)
		{
			.nw[0] = 0, .nw[1] = UNLIMITED_LEN,
			.nw_perjet[0] = 0, .nw_perjet[1] = UNLIMITED_LEN,
			.nw_unidir[0] = 0, .nw_unidir[1] = UNLIMITED_LEN,
			.fw[0] = UNLIMITED_LEN, .fw[1] = 0,
		};
	}
}

/**
 * Build the per-word minimum/maximum link distance table.
 * Optional words are ignored because they cannot constrain link crossing.
 * The table is meaningful only if at least one entry has a link constraint.
 *
 * The computation of ml[w].nw[] is done in 2 steps for efficiency.
 *
 * @param ml[out] The table (indexed by word, w/fields indexed by direction).
 * @return \c true iff the table is meaningful.
 */
static mlink_table *build_mlink_table(Sentence sent, mlink_table *ml)
{
	bool ml_exists = false;
	bool *nojet[2];

	nojet[0] = alloca(2 * sent->length * sizeof(bool));
	nojet[1] = nojet[0] + sent->length;
	memset(nojet[0], false, 2 * sent->length * sizeof(bool));

	mlink_table_init(sent, ml);

	for (WordIdx w = 0; w < sent->length; w++)
	{
		if (sent->word[w].optional) continue;

		for (Disjunct *d = sent->word[w].d; d != NULL; d = d->next)
		{
			if (NULL == d->left)
			{
				nojet[0][w] = true;
				ml[w].fw[0] = 0;
			}
			else
			{
				if (NULL == d->right)
				{
					if (d->left->nearest_word > ml[w].nw_unidir[0])
						ml[w].nw_unidir[0] = d->left->nearest_word;
				}
				else
				{
					if (d->left->nearest_word > ml[w].nw[0])
						ml[w].nw[0] = d->left->nearest_word;
				}

				if (d->left->farthest_word < ml[w].fw[0])
					ml[w].fw[0] = d->left->farthest_word;
			}

			if (NULL == d->right)
			{
				nojet[1][w] = true;;
				ml[w].fw[1] = UNLIMITED_LEN;
			}
			else
			{
				if (NULL == d->left)
				{
					if (d->right->nearest_word < ml[w].nw_unidir[1])
						ml[w].nw_unidir[1] = d->right->nearest_word;
				}
				else
				{
					if (d->right->nearest_word < ml[w].nw[1])
						ml[w].nw[1] = d->right->nearest_word;
				}

				if (d->right->farthest_word > ml[w].fw[1])
					ml[w].fw[1] = d->right->farthest_word;
			}
		}

		ml_exists |= (!nojet[0][w] || !nojet[1][w]);
	}

	if (ml_exists)
	{
		for (WordIdx w = 0; w < sent->length; w++)
		{
			if (sent->word[w].optional) continue;

			if (ml[w].nw_unidir[0] > ml[w].nw[0])
				ml[w].nw[0] = ml[w].nw_unidir[0];

			if (ml[w].nw_unidir[1] < ml[w].nw[1])
				ml[w].nw[1] = ml[w].nw_unidir[1];

			for (int dir = 0; dir < 2; dir++)
			{
				ml[w].nw_perjet[dir] = ml[w].nw[dir];
				if (nojet[dir][w])
					ml[w].nw[dir] = w;
			}
		}
	}

	if (verbosity_level(+D_PRUNE) && ml_exists)
	{
		prt_error("\n");
		for (WordIdx w = 0; w < sent->length; w++)
		{
			if (sent->word[w].optional) continue;

			if (ml[w].nw[0] != ml[w].nw[1])
			{
				/* -1 means at least one missing jet at that direction. */
				prt_error("%3zu: nearest_word (%3d %3d)", w,
				       w==ml[w].nw[0]?-1:ml[w].nw[0],
				       w==ml[w].nw[1]?-1:ml[w].nw[1]);
				prt_error("     farthest_word (%3d %3d)\n\\",
				       w==ml[w].nw[0]?-1:ml[w].fw[0],
				       w==ml[w].nw[1]?-1:ml[w].fw[1]);
			}
		}
		lg_error_flush();
	}

	return ml_exists ? ml : NULL;
}

/**
 * Old comments (from the proof-of-concept code).
 * Much is still relevant.
 *
 * Prune disjunct with a link that would need to cross links of words
 * that must have links.
 *
 * The idea:
 * Suppose that all the disjuncts of word wl have right shallow connectors
 * with nearest_word (wl_nw > 1) when (wl_nw >= wr). At this time we
 * suppose there is a linkage without null words. So at least one of these
 * disjuncts must have a link to word wr or greater (i.e. word wl has a
 * mandatory right-going link, marked m).
 *
 * We check all the words w strictly between wl and wr. Disjuncts of word
 * w with a shallow connector having nearest_word < wl would cross (link
 * x) the said link of word wl and thus can be discarded. The same can be
 * checked for mandatory left-going links.
 *
 *                    m
 *           +------------...
 *       x   |           .
 *      ...........+     .
 *           |     |     .
 *   w0 ... wl ... w ... wr ... wn
 *
 * Regarding a disjunct of wr, if its left connector connects to wl then
 * its deepest connector cannot connect to the left of wl, so disjuncts
 * whose deepest connectors having (nearest_word < wl) can be discarded.
 * On the other hand, if it doesn't connect to wl it means that wl is
 * connected to a word to its right. In that case again its deepest
 * connector (in fact any of its connectors) cannot connect to the left of
 * wl. So we can conclude that any disjunct of word wr having a deepest
 * connector with (nearest_word < wl) can be discarded.
 *
 * In addition, in all cases, in disjunct which are retained and have so
 * constrained connectors, the farthest_word of these connectors can be
 * adjusted not to get over w. Note that in the case of (wr - w == 1)
 * the deepest connector is multi, its farthest_word cannot be set to 1!
 * This is because it behaves like more that one connector, when the rest
 * may connect over w.
 *
 * In general, the mandatory links create "protected" ranges that links
 * from other words cannot get out of it or into it.
 */

/**
 * Delete disjuncts whose links would cross those implied by the mlink table.
 * Since links are not allowed to cross, such disjuncts would create null
 * links. So this optimization can only be done when parsing a sentence
 * with null_count==0 (in which null links are not allowed).
 * Possible FIXME:
 * Part of such kind of deletions are also be done in
 * possible_connection(), so there is some overlapping. However,
 * eliminating this overlap (if possible) would not cause a significant
 * speedup because these functions are lightweight.
 */
static unsigned int cross_mlink_prune(Sentence sent, mlink_table *ml)
{
	int N_deleted[2] = {0}; /* Counts deletions: 0: initial 1: by mem. sharing */
	static Connector bad_connector = { .nearest_word = BAD_WORD };

	for (unsigned int w = 0; w < sent->length; w++)
	{
		if (sent->word[w].optional) continue;
		if (sent->word[w].d == NULL) continue;

		WordIdx_m nw0 = ml[w].nw[0];
		WordIdx_m nw1 = ml[w].nw[1];
		WordIdx_m fw0 = ml[w].fw[0];
		WordIdx_m fw1 = ml[w].fw[1];

		if ((w > 0) && (nw1 != w))
		{
			for (Disjunct *d = sent->word[nw1].d; d != NULL; d = d->next)
			{
				Connector *shallow_c = d->left;

				if (shallow_c == NULL)
				{
					if ((nw1 == fw1) || ((d->right->nearest_word > fw1) && PR(1)))
					{
						/* If (nw1 == fw1) then word w must have a link to word
						 * nw1, and disjuncts of word nw1 which don't have an
						 * LHS jet can be deleted.
						 * Else, since word w cannot connect to this disjunct
						 * then if it is used then word w must connect to a word
						 * after it in the range [nw1+1, fw1]. As a result,
						 * disjuncts of nw1 with an RHS shallow connector having
						 * nearest_word > fw1 can be deleted.
						 *
						 * Naturally there are no connectors to assign BAD_WORD
						 * to. So use a dummy one. The same is done for the
						 * other direction. */
						d->left = &bad_connector;
						N_deleted[0]++;
					}
					continue;
				}

				/* Deepest connector constraint l->r. */

				if (shallow_c->nearest_word == BAD_WORD)
				{
					N_deleted[1]++;
					continue;
				}
				Connector *c = connector_deepest(shallow_c);

				if (c->nearest_word < w)
				{
					shallow_c->nearest_word = BAD_WORD;
					N_deleted[0]++;
					continue;
				}

				if (!c->multi)
					c->farthest_word = MAX(w, c->farthest_word);
			}
		}

		if ((w < sent->length-1) && (nw0 != w))
		{
			for (Disjunct *d = sent->word[nw0].d; d != NULL; d = d->next)
			{
				Connector *shallow_c = d->right;

				if (shallow_c == NULL)
				{
					/* See the comments in the handling of the other direction. */
					if ((nw0 == fw0) || ((d->left->nearest_word < fw0) && PR(0)))
					{
						d->right = &bad_connector;
						N_deleted[0]++;
						PR(0);
					}
					continue;
				}

				/* Deepest connector constraint r->l. */

				if (shallow_c->nearest_word == BAD_WORD)
				{
					N_deleted[1]++;
					continue;
				}
				Connector *c = connector_deepest(shallow_c);

				if (c->nearest_word > w)
				{
					shallow_c->nearest_word = BAD_WORD;
					N_deleted[0]++;
					continue;
				}

				if (!c->multi)
					c->farthest_word = MIN(w, c->farthest_word);
			}
		}

		/* Remove disjuncts that are blocked by mandatory l->r links. */
		for (unsigned int rw = w+1; rw < nw1; rw++)
		{
			for (Disjunct *d = sent->word[rw].d; d != NULL; d = d->next)
			{
				Connector *shallow_c = d->left;
				if (shallow_c == NULL) continue;
				if (shallow_c->nearest_word == BAD_WORD)
				{
					N_deleted[1]++;
					continue;
				}

				if (shallow_c->nearest_word < w)
				{
					shallow_c->nearest_word = BAD_WORD;
					N_deleted[0]++;
					continue;
				}

				shallow_c->farthest_word = MAX(w, shallow_c->farthest_word);
				if (d->right != NULL)
					d->right->farthest_word = MIN(fw1, d->right->farthest_word);
			}
		}

		/* Remove disjuncts that are blocked by mandatory r->l links. */
		for (unsigned int lw = nw0+1; lw < w; lw++)
		{
			for (Disjunct *d = sent->word[lw].d; d != NULL; d = d->next)
			{
				Connector *shallow_c = d->right;
				if (shallow_c == NULL) continue;
				if (shallow_c->nearest_word == BAD_WORD)
				{
					N_deleted[1]++;
					continue;
				}

				if (shallow_c->nearest_word > w)
				{
					shallow_c->nearest_word = BAD_WORD;
					N_deleted[0]++;
					continue;
				}

				shallow_c->farthest_word = MIN(w, shallow_c->farthest_word);
				if (d->left != NULL)
					d->left->farthest_word = MAX(fw0, d->left->farthest_word);
			}
		}
	}

	lgdebug(+D_PRUNE, "Debug: [nw] detected %d (%d+%d)\n",
	        N_deleted[0] + N_deleted[1], N_deleted[0], N_deleted[1]);
	return N_deleted[0] + N_deleted[1];
}

/**
 * Prune useless disjuncts.
 * @param null_count Optimize for parsing with this null count.
 * MAX_SENTENCE means null count optimizations is essentially ignored.
 * @param[out] \p ncu[sent->length][2] Number of different connector
 * uppercase parts per word per direction (for the fast matcher tables).
 * @return The minimal null_count expected in parsing.
 */
unsigned int pp_and_power_prune(Sentence sent, Tracon_sharing *ts,
                                unsigned int null_count, Parse_Options opts,
                                unsigned int *ncu[2])
{
	prune_context pc = {0};
	power_table pt;

	power_table_init(sent, ts, &pt);

	bool no_mlink = !!test_enabled("no-mlink");
	mlink_table *ml = alloca(sent->length * sizeof(*pc.ml));

	pc.always_parse = test_enabled("always-parse");
	pc.sent = sent;
	pc.pt = &pt;
	pc.null_links = null_count;
	pc.islands_ok = opts->islands_ok;
	pc.is_null_word = alloca(sent->length * sizeof(*pc.is_null_word));
	memset(pc.is_null_word, 0, sent->length * sizeof(*pc.is_null_word));

	int num_deleted = power_prune(sent, &pc, opts); /* pc->ml is NULL here */

	if ((num_deleted > 0) && !no_mlink)
	{
		pc.ml = build_mlink_table(sent, ml);
		print_time(opts, "Built mlink_table%s", pc.ml ? "" : " (empty)");
		if (pc.ml != NULL)
		{
			if (null_count == 0)
				cross_mlink_prune(sent, pc.ml);
			num_deleted = power_prune(sent, &pc, opts);
		}
	}

	if (num_deleted != -1)
	{
		if (pp_prune(sent, ts, opts) > 0)
			num_deleted = power_prune(sent, &pc, opts);

		if ((num_deleted > 0) && !no_mlink)
		{
			pc.ml = build_mlink_table(sent, ml);
			print_time(opts, "Built mlink_table%s", pc.ml ? "" : " (empty)");
			if (pc.ml != NULL)
			{
				if (null_count == 0)
					cross_mlink_prune(sent, pc.ml);
				power_prune(sent, &pc, opts);
			}
		}
	}

	/* It is not cost-effective to make additional pp_prune() & power_prune()
	 * as most of the times too few disjunct are deleted. */

	/* Initialize tentative values. */
	unsigned int min_nulls = sent->null_count;
	bool parsing_to_be_done = true;

	if (null_count == MAX_SENTENCE)
	{
		/* No prune optimization - this is the last pruning for this sentence. */
		min_nulls = pc.null_words;
	}
	else if ((pc.null_words > sent->null_count) && !pc.always_parse)
	{
		min_nulls = sent->null_count + 1; /* The most that can be inferred */
		parsing_to_be_done = false;
	}

	if (parsing_to_be_done)
		get_num_con_uc(sent, &pt, ncu);

	power_table_delete(&pt);

	return min_nulls;
}
