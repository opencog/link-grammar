/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2009, 2013, 2014 Linas Vepstas                          */
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
#include "post-process/post-process.h"
#include "post-process/pp-structures.h"
#include "print/print.h"                // print_disjunct_counts

#include "prune.h"
#include "resources.h"
#include "string-set.h"
#include "tokenize/word-structures.h"   // Word_struct
#include "tokenize/wordgraph.h"

#define D_PRUNE 5 /* Debug level for this file. */

/* To debug pp_prune(), touch this file, run "make CPPFLAGS=-DDEBUG_PP_PRUNE",
 * and the run: link-parser -v=5 -debug=prune.c . */
#if defined DEBUG || defined DEBUG_PP_PRUNE
#define ppdebug(...) lgdebug(+D_PRUNE, __VA_ARGS__)
#else
#define ppdebug(...)
#endif

typedef Connector *connector_table;

/* Indicator that this connector cannot be used -- that its "obsolete".  */
#define BAD_WORD (MAX_SENTENCE+1)

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
	unsigned int *l_table_size;  /* the sizes of the hash tables */
	unsigned int *r_table_size;
	C_list *** l_table;
	C_list *** r_table;
	Pool_desc *memory_pool;
};

typedef struct cms_struct Cms;
struct cms_struct
{
	Cms *next;
	Connector *c;
};

#define CMS_SIZE (2<<10)
typedef struct multiset_table_s multiset_table;
struct multiset_table_s
{
	Cms *cms_table[CMS_SIZE];
};

typedef struct prune_context_s prune_context;
struct prune_context_s
{
	unsigned int null_links; /* maximum number of null links */
	unsigned int null_words; /* found number of non-optional null words */
	bool *is_null_word;      /* a map of null words (indexed by word number) */
	uint8_t pass_number;     /* marks tracons for processing only once per pass*/
	int N_changed;   /* counts the changes of c->nearest_word fields in a pass */

	power_table *pt;
	Sentence sent;

	int power_cost;  /* for debug - shown in the verbose output */
};

/*

  The algorithms in this file prune disjuncts from the disjunct list
  of the sentence that can be eliminated by a simple checks.  The first
  check works as follows:

  A series of passes are made through the sentence, alternating
  left-to-right and right-to-left.  Consider the left-to-right pass (the
  other is symmetric).  A set S of connectors is maintained (initialized
  to be empty).  Now the disjuncts of the current word are processed.
  If a given disjunct's left pointing connectors have the property that
  at least one of them has no connector in S to which it can be matched,
  then that disjunct is deleted. Now the set S is augmented by the right
  connectors of the remaining disjuncts of that word.  This completes
  one word.  The process continues through the words from left to right.
  Alternate passes are made until no disjunct is deleted.

  It worries me a little that if there are some really huge disjuncts lists,
  then this process will probably do nothing.  (This fear turns out to be
  unfounded.)

  Notes:  Power pruning will not work if applied before generating the
  "and" disjuncts.  This is because certain of it's tricks don't work.
  Think about this, and finish this note later....
  Also, currently I use the standard connector match procedure instead
  of the pruning one, since I know power pruning will not be used before
  and generation.  Replace this to allow power pruning to work before
  generating and disjuncts.

  Currently it seems that normal pruning, power pruning, and generation,
  pruning, and power pruning (after "and" generation) and parsing take
  about the same amount of time.  This is why doing power pruning before
  "and" generation might be a very good idea.

  New idea:  Suppose all the disjuncts of a word have a connector of type
  c pointing to the right.  And further, suppose that there is exactly one
  word to its right containing that type of connector pointing to the left.
  Then all the other disjuncts on the latter word can be deleted.
  (This situation is created by the processing of "either...or", and by
  the extra disjuncts added to a "," neighboring a conjunction.)

*/

/*
  Here is what you've been waiting for: POWER-PRUNE

  The kinds of constraints it checks for are the following:

	1) successive connectors on the same disjunct have to go to
	   nearer and nearer words.

	2) two deep connectors cannot attach to each other
	   (A connectors is deep if it is not the first in its list; it
	   is shallow if it is the first in its list; it is deepest if it
	   is the last on its list.)

	3) on two adjacent words, a pair of connectors can be used
	   only if they're the deepest ones on their disjuncts

	4) on two non-adjacent words, a pair of connectors can be used only
	   if not [both of them are the deepest].

   The data structure consists of a pair of hash tables on every word.
   Each bucket of a hash table has a list of pointers to connectors.
   These nodes also store if the chosen connector is shallow.
*/
/*
   As with normal pruning, we make alternate left->right and right->left
   passes.  In the R->L pass, when we're on a word w, we make use of
   all the left-pointing hash tables on the words to the right of w.
   After the pruning on this word, we build the left-pointing hash table
   this word.  This guarantees idempotence of the pass -- after doing an
   L->R, doing another would change nothing.

   Each connector has an integer c_word field.  This refers to the closest
   word that it could be connected to.  These are initially determined by
   how deep the connector is.  For example, a deepest connector can connect
   to the neighboring word, so its c_word field is w+1 (w-1 if this is a left
   pointing connector).  It's neighboring shallow connector has a c_word
   value of w+2, etc.

   The pruning process adjusts these c_word values as it goes along,
   accumulating information about any way of linking this sentence.
   The pruning process stops only after no disjunct is deleted and no
   c_word values change.

   The difference between RUTHLESS and GENTLE power pruning is simply
   that GENTLE uses the deletable region array, and RUTHLESS does not.
   So we can get the effect of these two different methods simply by
   always ensuring that deletable[][] has been defined.  With nothing
   deletable, this is equivalent to RUTHLESS.   --DS, 7/97
*/

/**
 * free all of the hash tables and C_lists
 */
static void power_table_delete(power_table *pt)
{
	pool_delete(pt->memory_pool);
	for (WordIdx w = 0; w < pt->power_table_size; w++)
	{
		xfree((char *)pt->l_table[w], pt->l_table_size[w] * sizeof (C_list *));
		xfree((char *)pt->r_table[w], pt->r_table_size[w] * sizeof (C_list *));
	}
	xfree(pt->l_table_size, 2 * pt->power_table_size * sizeof(unsigned int));
	xfree(pt->l_table, 2 * pt->power_table_size * sizeof(C_list **));
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

	assert(NULL != e, "put_into_power_table: Overflow");
	assert(c->refcount > 0, "refcount %d", c->refcount);

	C_list *m = pool_alloc(mp);
	m->next = *e;
	*e = m;
	m->c = c;
}

static void power_table_alloc(Sentence sent, power_table *pt)
{
	pt->power_table_size = sent->length;
	pt->l_table_size = xalloc (2 * sent->length * sizeof(unsigned int));
	pt->r_table_size = pt->l_table_size + sent->length;
	pt->l_table = xalloc (2 * sent->length * sizeof(C_list **));
	pt->r_table = pt->l_table + sent->length;
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
	unsigned int i;
#define TOPSZ 32768
	size_t lr_table_max_usage = MIN(sent->dict->contable.num_con, TOPSZ);

	power_table_alloc(sent, pt);

	Pool_desc *mp = pt->memory_pool = pool_new(__func__, "C_list",
	                   /*num_elements*/2048, sizeof(C_list),
	                   /*zero_out*/false, /*align*/false, /*exact*/false);

	for (WordIdx w = 0; w < sent->length; w++)
	{
		size_t l_size, r_size;
		C_list **l_t, **r_t;
		size_t len;

		/* The below uses variable-sized hash tables. This seems to
		 * provide performance that is equal or better than the best
		 * fixed-size performance.
		 * The best fixed-size performance seems to come at about
		 * a 1K table size, for both English and Russian. (Both have
		 * about 100 fixed link-types, and many thousands of auto-genned
		 * link types (IDxxx idioms for both, LLxxx suffix links for
		 * Russian).  Pluses and minuses:
		 * + small fixed tables are faster to initialize.
		 * - small fixed tables have more collisions
		 * - variable-size tables require counting connectors.
		 *   (and the more complex code to go with)
		 * CPU cache-size effects ...
		 */
		if (NULL != tl)
			len = tl->num_cnctrs_per_word[0][w];
		else
			len = left_connector_count(sent->word[w].d);
		len++; /* Ensure at least one empty entry for get_power_table_entry(). */
		l_size = next_power_of_two_up(MIN(len, lr_table_max_usage));
		pt->l_table_size[w] = l_size;
		l_t = pt->l_table[w] = (C_list **) xalloc(l_size * sizeof(C_list *));
		for (i=0; i<l_size; i++) l_t[i] = NULL;

		if (NULL != tl)
			len = tl->num_cnctrs_per_word[1][w];
		else
			len = right_connector_count(sent->word[w].d);
		len++;
		r_size = next_power_of_two_up(MIN(len, lr_table_max_usage));
		pt->r_table_size[w] = r_size;
		r_t = pt->r_table[w] = (C_list **) xalloc(r_size * sizeof(C_list *));
		for (i=0; i<r_size; i++) r_t[i] = NULL;

		if (NULL == tl)
		{
			/* Insert the deep connectors. */
			for (Disjunct *d = sent->word[w].d; d != NULL; d = d->next)
			{
				Connector *c;

				c = d->right;
				if (c != NULL)
				{
					for (c = c->next; c != NULL; c = c->next)
					{
						put_into_power_table(mp, r_size, r_t, c);
					}
				}

				c = d->left;
				if (c != NULL)
				{
					for (c = c->next; c != NULL; c = c->next)
					{
						put_into_power_table(mp, l_size, l_t, c);
					}
				}
			}

			/* Insert the shallow connectors. */
			for (Disjunct *d = sent->word[w].d; d != NULL; d = d->next)
			{
				Connector *c;

				c = d->right;
				if (c != NULL)
				{
					put_into_power_table(mp, r_size, r_t, c);
				}
				c = d->left;
				if (c != NULL)
				{
					put_into_power_table(mp, l_size, l_t, c);
				}
			}
		}
	}

	if (NULL != tl)
	{
		/* Bulk insertion with reference count. */

		for (int dir = 0; dir < 2; dir++)
		{
			C_list ***tp;
			unsigned int *sizep;
			unsigned int sid_entries = tl->entries[dir];

			if (dir == 0)
			{
				tp = pt->l_table;
				sizep = pt->l_table_size;
			}
			else
			{
				tp = pt->r_table;
				sizep = pt->r_table_size;
			}

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
	static condesc_t desc_no_match =
	{
		.uc_num = (connector_hash_t)-1, /* get_power_table_entry() will skip. */
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
			assert(0 <= (*m)->c->refcount, "clean_table: refcount < 0 (%d)",
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

static bool non_opt_words_gt(unsigned int nl, Sentence sent, int w1, int w2)
{
	unsigned int non_optional_word = 0;

	for (int w = w1+1; w < w2; w++)
	{
		if (sent->word[w].optional) continue;
		non_optional_word++;
		if (non_optional_word > nl) return true;
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
	int dist;
	if (!lc_easy_match(lc->desc, rc->desc)) return false;

	if ((lc->nearest_word > rword) || (rc->nearest_word < lword)) return false;

	dist = rword - lword;
	// assert(0 < dist, "Bad word order in possible connection.");

	/* Word range constraints */
	if (1 == dist)
	{
		if ((lc->next != NULL) || (rc->next != NULL)) return false;
	}
	else
	if (rword > lc->farthest_word || lword < rc->farthest_word)
	{
		return false;
	}
	else
	{
		/* We arrive here if the words are NOT next to each other. Say the
		 * gape between them contains W non-optional words. We also know
		 * that we are going to parse with N null-links (which involves
		 * sub-parsing with the range of [0, N] null-links).
		 * If there is not at least one intervening connector (i.e. both
		 * of lc->next and rc->next are NULL) then there will be W
		 * null-linked words, and W must be <= N.
		 * Notes:
		 * 1. Optional words are disregarded because they are allowed to be
		 * null-linked independently of the requested parsing null links.
		 * 2. When island_ok=true this optimization is valid only for N = 0.
		 */
		if ((lc->next == NULL) && (rc->next == NULL) &&
			 (!lc->multi) && (!rc->multi) &&
			 non_opt_words_gt(pc->null_links, pc->sent, lword, rword))
		{
			return false;
		}

		if ((lc->next != NULL) && (rc->next != NULL))
		{
			if (lc->next->nearest_word > rc->next->nearest_word)
				return false; /* Cross link. */
		}
	}

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
	unsigned int size = pt->r_table_size[w];
	C_list **e = get_power_table_entry(size, pt->r_table[w], c);

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
 * This returns TRUE if the right table of word w contains
 * a connector that can match to c.  shallow tells if c is shallow
 */
static bool
left_table_search(prune_context *pc, int w, Connector *c,
                  bool shallow, int word_c)
{
	power_table *pt = pc->pt;
	unsigned int size = pt->l_table_size[w];
	C_list **e = get_power_table_entry(size, pt->l_table[w], c);

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
 * fields correctly. When tracons are shared, this update is done
 * simultaneously on all of them. The main loop of power_prune() then
 * marks them with the pass number that is checked here.
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
	for (; n >= lb ; n--)
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
	return foundmatch;
}

/**
 * Take this connector list, and try to match it with the words
 * w+1, w+2, w+3...  Returns the word to which the first connector of
 * the list could possibly be matched.  If c is NULL, returns w.
 * If there is no way to match this list, it returns BAD_WORD, which is
 * always greater than N_words - 1.   If it does find a way to match it,
 * it updates the c->nearest_word fields correctly.
 * Regarding pass_number, see the comment in left_connector_list_update().
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
	for (; n <= ub ; n++)
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
	Word *word = &pc->sent->word[w];

	if ((word->d == NULL) && !word->optional && !pc->is_null_word[w])
	{
		pc->null_words++;
		pc->is_null_word[w] = true;
		if (pc->null_words > pc->null_links) return true;
	}
	return false;
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
	int N_deleted[2] = {0}; /* [0] counts first deletions, [1] counts dups. */
	int total_deleted = 0;
	bool extra_null_word = false;
	power_table *pt = pc->pt;

	pc->N_changed = 1;      /* forces it always to make at least two passes */

	do
	{
		pc->pass_number++;

		/* left-to-right pass */
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
					N_deleted[(int)bad]++;
					continue;
				}

				mark_jet_as_good(d->left, pc->pass_number);
				dd = &d->next; /* NEXT */
			}

			if (check_null_word(pc, w)) extra_null_word = true;
			clean_table(pt->r_table_size[w], pt->r_table[w]);
		}

		total_deleted += N_deleted[0] + N_deleted[1];
		lgdebug(D_PRUNE, "Debug: l->r pass changed %d and deleted %d (%d+%d)\n",
		        pc->N_changed, N_deleted[0]+N_deleted[1], N_deleted[0], N_deleted[1]);

		if (pc->N_changed == 0 && N_deleted[0] == 0 && N_deleted[1] == 0) break;
		pc->N_changed = N_deleted[0] = N_deleted[1] = 0;

		/* right-to-left pass */
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
					N_deleted[(int)bad]++;
					continue;
				}

				mark_jet_as_good(d->right, pc->pass_number);
				dd = &d->next; /* NEXT */
			}

			if (check_null_word(pc, w)) extra_null_word = true;
			clean_table(pt->l_table_size[w], pt->l_table[w]);
		}

		total_deleted += N_deleted[0] + N_deleted[1];
		lgdebug(D_PRUNE, "Debug: r->l pass changed %d and deleted %d (%d+%d)\n",
		        pc->N_changed, N_deleted[0]+N_deleted[1], N_deleted[0], N_deleted[1]);

		if (pc->N_changed == 0 && N_deleted[0] == 0 && N_deleted[1] == 0) break;
		pc->N_changed = N_deleted[0] = N_deleted[1] = 0;
	}
	while (!extra_null_word);

	print_time(opts, "power pruned (for %u null%s%s)",
	           pc->null_links, (pc->null_links != 1) ? "s" : "",
	           extra_null_word ? ", extra null" : "");
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

	if (extra_null_word) return -1;
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

static multiset_table *cms_table_new(void)
{
	multiset_table *mt = (multiset_table *) xalloc(sizeof(multiset_table));
	memset(mt, 0, sizeof(multiset_table));

	return mt;
}

static void cms_table_delete(multiset_table *mt)
{
	Cms *cms, *xcms;
	int i;
	for (i=0; i<CMS_SIZE; i++)
	{
		for (cms = mt->cms_table[i]; cms != NULL; cms = xcms)
		{
			xcms = cms->next;
			xfree(cms, sizeof(Cms));
		}
	}
	xfree(mt, sizeof(multiset_table));
}

static unsigned int cms_hash(const char *s)
{
	unsigned int i = 5381;
	if (islower((int) *s)) s++; /* skip head-dependent indicator */
	while (isupper((int) *s))
	{
		i = ((i << 5) + i) + *s;
		s++;
	}
	return (i & (CMS_SIZE-1));
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
 *     s="Xabc"; t="Xa*c"; e=s[2]; // *e == '*'; // Returns true;
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
	if (islower(*t)) t++; /* Skip head-dependent indicator */
	while (isupper(*s))
	{
		if (*s != *t) return false;
		s++;
		t++;
	}
	if (isupper(*t)) return false;
	while (*t != '\0')
	{
		if (*s == '\0') return true;
		if (*s != *t && *s != '#' && (s == e || *t != '*')) return false;
		s++;
		t++;
	}
	while (*s != '\0')
	{
		if (*s != '*' && *s != '#') return false;
		s++;
	}
	return true;
}

/**
 * Returns TRUE iff there is a connector name c in the table
 * that can create a link x such that post_process_match(pp_link, x) is TRUE.
 */
static bool match_in_cms_table(multiset_table *cmt, const char *pp_link,
                               const char *c)
{
	unsigned int h = cms_hash(pp_link);

	for (Cms *cms = cmt->cms_table[h]; cms != NULL; cms = cms->next)
	{
			if (can_form_link(pp_link, connector_string(cms->c), c))
			{
				ppdebug("MATCHED %s\n", connector_string(cms->c));
				return true;
			}
			ppdebug("NOT-MATCHED %s \n", connector_string(cms->c));
	}

	return false;
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

static void insert_in_cms_table(multiset_table *cmt, Connector *c)
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
		cms = (Cms *) xalloc(sizeof(Cms));
		cms->c = c;
		cms->next = cmt->cms_table[h];
		cmt->cms_table[h] = cms;
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
}

#ifdef ppdebug
const char *AtoZ = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
#endif
/** Validate that the connectors needed in order to create a
 *  link that matches pp_link, are all found in the sentence.
 *  The sentence's connectors are in the cms table.
 */

static bool all_connectors_exist(multiset_table *cmt, const char *pp_link)
{
	ppdebug("check PP-link=%s\n", pp_link);

	const char *s;
	for (s = pp_link; isupper(*s); s++) {}

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
				return true;
			}
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


static int pp_prune(Sentence sent, Tracon_sharing *ts, Parse_Options opts)
{
	pp_knowledge *knowledge;
	multiset_table *cmt;

	if (sent->postprocessor == NULL) return 0;
	if (!opts->perform_pp_prune) return 0;

	knowledge = sent->postprocessor->knowledge;
	cmt = cms_table_new();

	Tracon_list *tl = ts->tracon_list;
	if (NULL != tl)
	{
		for (int dir = 0; dir < 2; dir++)
		{
			for (unsigned int id = 0; id < tl->entries[dir]; id++)
			{
				Connector *c = get_tracon(ts, dir, id);

				if (0 == c->refcount) continue;
				insert_in_cms_table(cmt, c);
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
						insert_in_cms_table(cmt, c);
					}
				}
			}
		}
	}

	int D_deleted = 0;       /* Number of deleted disjuncts */
	int Cname_deleted = 0;   /* Number of deleted connector names */

	/* Since the cms table is unchanged, after applying a rule once we
	 * know if it will be TRUE or FALSE if we need to apply it again.
	 * Values: -1: Undecided yet; 0: Rule unsatisfiable; 1 Rule satisfiable. */
	uint8_t *rule_ok = alloca(knowledge->n_contains_one_rules * sizeof(bool));
	memset(rule_ok, -1, knowledge->n_contains_one_rules * sizeof(bool));

	for (size_t i = 0; i < knowledge->n_contains_one_rules; i++)
	{
		if (rule_ok[i] == 1) continue;

		pp_rule* rule = &knowledge->contains_one_rules[i]; /* The ith rule */
		const char *selector = rule->selector;  /* Selector string for this rule */
		pp_linkset *link_set = rule->link_set;  /* The set of criterion links */
		unsigned int hash = cms_hash(selector);

		if (rule->selector_has_wildcard)
		{
			rule_ok[i] = 1;
			continue;  /* If it has a * forget it */
		}

		for (Cms *cms = cmt->cms_table[hash]; cms != NULL; cms = cms->next)
		{
			Connector *c = cms->c;
			if (!post_process_match(selector, connector_string(c))) continue;

			ppdebug("Rule %zu: Selector %s, Connector %s\n",
			        i, selector, connector_string(c));
			/* We know c matches the trigger link of the rule. */
			/* Now check the criterion links */
			if ((rule_ok[i] == 0) || !rule_satisfiable(cmt, link_set))
			{
				rule_ok[i] = 0;
				ppdebug("DELETE %s refcount %d\n", connector_string(c), c->refcount);
				c->nearest_word = BAD_WORD;
				Cname_deleted++;
				rule->use_count++;
			}
			else
			{
				rule_ok[i] = 1;
				break;
			}
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

	lgdebug(+D_PRUNE, "Deleted %d (%d connector names)\n",
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
		for (int dir = 0; dir < 2; dir++)
		{
			C_list **t;
			unsigned int size;

			if (dir == 0)
			{
				t = pt->l_table[w];
				size = pt->l_table_size[w];
			}
			else
			{
				t = pt->r_table[w];
				size = pt->r_table_size[w];
			}

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

/**
 * Prune useless disjuncts.
 */
void pp_and_power_prune(Sentence sent, Tracon_sharing *ts,
                        unsigned int null_count, Parse_Options opts,
                        unsigned int *ncu[2])
{
	prune_context pc = {0};
	power_table pt;

	power_table_init(sent, ts, &pt);

	pc.sent = sent;
	pc.pt = &pt;
	pc.null_links = null_count;
	pc.is_null_word = alloca(sent->length * sizeof(*pc.is_null_word));
	memset(pc.is_null_word, 0, sent->length * sizeof(*pc.is_null_word));

	power_prune(sent, &pc, opts);
	if (pp_prune(sent, ts, opts) > 0)
		power_prune(sent, &pc, opts);

	/* No benefit for now to make additional pp_prune() & power_prune() -
	 * additional deletions are very rare and even then most of the
	 * times only one disjunct is deleted. */

	get_num_con_uc(sent, &pt, ncu);
	power_table_delete(&pt);

	return;
}
