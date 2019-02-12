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
#include "dict-common/dict-common.h"     // For contable
#include "post-process/post-process.h"
#include "post-process/pp-structures.h"
#include "print/print.h"  // For print_disjunct_counts()

#include "prune.h"
#include "resources.h"
#include "string-set.h"
#include "tokenize/word-structures.h" // for Word_struct
#include "tokenize/wordgraph.h"
#include "utilities.h"                // strdupa()

#define D_PRUNE 5

typedef Connector * connector_table;

/* Indicator that this connector cannot be used -- that its "obsolete".  */
#define BAD_WORD (MAX_SENTENCE+1)

typedef struct c_list_s C_list;
struct c_list_s
{
	C_list * next;
	Connector * c;
	bool shallow;
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
	Cms * next;
	const char * name;
	int count; /* the number of times this is in the multiset */
};

#define CMS_SIZE (2<<10)
typedef struct multiset_table_s multiset_table;
struct multiset_table_s
{
	Cms * cms_table[CMS_SIZE];
};

typedef struct prune_context_s prune_context;
struct prune_context_s
{
	bool null_links;
	int power_cost;
	int N_changed;   /* counts the number of changes
						   of c->nearest_word fields in a pass */
	power_table *pt;
	Sentence sent;
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

/**
 * The disjunct d (whose left or right pointer points to c) is put
 * into the appropriate hash table
 */
static void put_into_power_table(Pool_desc *mp, unsigned int size, C_list ** t,
                                 Connector * c, bool shal)
{
	unsigned int h = connector_uc_num(c) & (size-1);
	assert(c->suffix_id>0, "suffix_id %d", c->suffix_id);

	C_list *m = pool_alloc(mp);
	m->next = t[h];
	t[h] = m;
	m->c = c;
	m->shallow = shal;
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
 * The suffix_id of each connector serves as its reference count.
 * Hence, it should always be > 0.
 *
 * There are two code paths for initializing the power tables:
 * 1. When disjunct-jets sharing is not done. The words then are
 * directly scanned for their disjuncts and connectors. Each ones
 * is inserted with a reference count (as suffix_id) set to 1.
 * 2. Using the disjunct-jet tables (left and right). Each slot
 * contains only a pointer to a disjunct-jet. The word number is
 * extracted from the deepest connector (that has been assigned to it by
 * setup_connectors()).
 *
 * FIXME: Find a way to not use a reference count (to increase
 * efficiency).
 */
static void power_table_init(Sentence sent, power_table *pt)
{
	unsigned int i;
#define TOPSZ 32768
	size_t lr_table_max_usage = MIN(sent->dict->contable.num_con, TOPSZ);

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
		if (sent->jet_sharing.num_cnctrs_per_word[0])
			len = sent->jet_sharing.num_cnctrs_per_word[0][w];
		else
			len = left_connector_count(sent->word[w].d);
		l_size = next_power_of_two_up(MIN(len, lr_table_max_usage));
		pt->l_table_size[w] = l_size;
		l_t = pt->l_table[w] = (C_list **) xalloc(l_size * sizeof(C_list *));
		for (i=0; i<l_size; i++) l_t[i] = NULL;

		if (sent->jet_sharing.num_cnctrs_per_word[1])
			len = sent->jet_sharing.num_cnctrs_per_word[1][w];
		else
			len = right_connector_count(sent->word[w].d);
		r_size = next_power_of_two_up(MIN(len, lr_table_max_usage));
		pt->r_table_size[w] = r_size;
		r_t = pt->r_table[w] = (C_list **) xalloc(r_size * sizeof(C_list *));
		for (i=0; i<r_size; i++) r_t[i] = NULL;

		if (!sent->jet_sharing.num_cnctrs_per_word[0])
		{
			/* Insert the deep connectors. */
			for (Disjunct *d = sent->word[w].d; d != NULL; d = d->next)
			{
				Connector *c;

				c = d->right;
				if (c != NULL)
				{
					c->suffix_id = 1;
					for (c = c->next; c != NULL; c = c->next)
					{
						c->suffix_id = 1;
						put_into_power_table(mp, r_size, r_t, c, false);
					}
				}

				c = d->left;
				if (c != NULL)
				{
					c->suffix_id = 1;
					for (c = c->next; c != NULL; c = c->next)
					{
						c->suffix_id = 1;
						put_into_power_table(mp, l_size, l_t, c, false);
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
					put_into_power_table(mp, r_size, r_t, c, true);
				}
				c = d->left;
				if (c != NULL)
				{
					put_into_power_table(mp, l_size, l_t, c, true);
				}
			}
		}
	}

	if (sent->jet_sharing.num_cnctrs_per_word[0])
	{
		/* Bulk insertion with reference count. Note: IDs start from 1. */

		/* Insert the deep connectors. */
		for (int dir = 0; dir < 2; dir++)
		{
			C_list ***tp;
			unsigned int *sizep;

			if (dir== 0)
			{
				tp = pt->l_table;
				sizep = pt->l_table_size;
			}
			else
			{
				tp = pt->r_table;
				sizep = pt->r_table_size;
			}

			for (unsigned int id = 1; id < sent->jet_sharing.entries[dir] + 1; id++)
			{
				Connector *htc = sent->jet_sharing.table[dir][id];
				Connector *deepest;

				for (deepest = htc; NULL != deepest->next; deepest = deepest->next)
					;
				int w = deepest->nearest_word + ((dir== 0) ? 1 : -1);

				unsigned int size = sizep[w];
				C_list **t = tp[w];
				int suffix_id = htc->suffix_id;

				for (Connector *c = htc->next; NULL != c; c = c->next)
				{
					c->suffix_id = suffix_id;
					put_into_power_table(mp, size, t, c, false);
				}
			}

			/* Insert the shallow connectors. */
			for (unsigned int id = 1; id < sent->jet_sharing.entries[dir] + 1; id++)
			{
				Connector *htc = sent->jet_sharing.table[dir][id];
				Connector *deepest;

				for (deepest = htc; NULL != deepest->next; deepest = deepest->next)
					;
				int w = deepest->nearest_word + ((dir == 0) ? 1 : -1);

				unsigned int size = sizep[w];
				C_list **t = tp[w];

				put_into_power_table(mp, size, t, htc, true);
			}
		}
	}
}

/**
 * This runs through all the connectors in this table, and eliminates those
 * with a zero reference count.
 */
static void clean_table(unsigned int size, C_list **t)
{
	for (unsigned int i = 0; i < size; i++)
	{
		C_list **m = &t[i];

		while (NULL != *m)
		{
			if (0 == (*m)->c->suffix_id)
				*m = (*m)->next;
			else
				m = &(*m)->next;
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

/**
 * This takes two connectors (and whether these are shallow or not)
 * (and the two words that these came from) and returns TRUE if it is
 * possible for these two to match based on local considerations.
 */
static bool possible_connection(prune_context *pc,
                                Connector *lc, Connector *rc,
                                int lword, int rword)
{
	int dist;
	if (!easy_match_desc(lc->desc, rc->desc)) return false;

	if ((lc->nearest_word > rword) || (rc->nearest_word < lword)) return false;

	dist = rword - lword;
	// assert(0 < dist, "Bad word order in possible connection.");

	/* Word range constraints */
	if (1 == dist)
	{
		if ((lc->next != NULL) || (rc->next != NULL)) return false;
	}
	else
	if (dist > lc->length_limit || dist > rc->length_limit)
	{
		return false;
	}
	/* If the words are NOT next to each other, then there must be
	 * at least one intervening connector (i.e. cannot have both
	 * lc->next and rc->next being null).  But we only enforce this
	 * when we think its still possible to have a complete parse,
	 * i.e. before we allow null-linked words.
	 */
	else
	if (!pc->null_links &&
	    (lc->next == NULL) &&
	    (rc->next == NULL) &&
	    (!lc->multi) && (!rc->multi) &&
	    !optional_gap_collapse(pc->sent, lword, rword))
	{
		return false;
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
	unsigned int size, h;
	C_list *cl;
	power_table *pt = pc->pt;

	size = pt->r_table_size[w];
	h = connector_uc_num(c) & (size-1);

	for (cl = pt->r_table[w][h]; cl != NULL; cl = cl->next)
	{
		/* Two deep connectors can't work */
		if (!shallow && !cl->shallow) return false;

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
	unsigned int size, h;
	C_list *cl;
	power_table *pt = pc->pt;
	if (c->nearest_word == BAD_WORD) printf("left_table_search BAD_WORD\n");

	size = pt->l_table_size[w];
	h = connector_uc_num(c) & (size-1);
	for (cl = pt->l_table[w][h]; cl != NULL; cl = cl->next)
	{
		/* Two deep connectors can't work */
		if (!shallow && !cl->shallow) return false;

		if (possible_connection(pc, c, cl->c, word_c, w))
			return true;
	}
	return false;
}

/**
 * Take this connector list, and try to match it with the words
 * w-1, w-2, w-3...  Returns the word to which the first connector of
 * the list could possibly be matched.  If c is NULL, returns w.  If
 * there is no way to match this list, it returns a negative number.
 * If it does find a way to match it, it updates the c->nearest_word fields
 * correctly. When disjunct-jets are shared, this update is done
 * simultaneously on all of them, and the next time the same disjunct-jet
 * is encountered (in the same word disjunct loop at least), the work here is
 * trivial. FIXME: Mark such jets for the duration of this loop so
 * they will be skipped.
 */
static int
left_connector_list_update(prune_context *pc, Connector *c,
                           int w, bool shallow)
{
	int n, lb;
	int foundmatch = -1;

	if (c == NULL) return w;
	n = left_connector_list_update(pc, c->next, w, false) - 1;
	if (0 > n) return -1;
	if (((int) c->nearest_word) < n) n = c->nearest_word;

	/* lb is now the leftmost word we need to check */
	lb = w - c->length_limit;
	if (0 > lb) lb = 0;

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
 * the list could possibly be matched.  If c is NULL, returns w.  If
 * there is no way to match this list, it returns a number greater than
 * N_words - 1.   If it does find a way to match it, it updates the
 * c->nearest_word fields correctly. See the comment on that in
 * left_connector_list_update().
 */
static size_t
right_connector_list_update(prune_context *pc, Connector *c,
                            size_t w, bool shallow)
{
	int n, ub;
	int sent_length = (int)pc->sent->length;
	int foundmatch = sent_length;

	if (c == NULL) return w;
	n = right_connector_list_update(pc, c->next, w, false) + 1;
	if (sent_length <= n) return sent_length;
	if (c->nearest_word > n) n = c->nearest_word;

	/* ub is now the rightmost word we need to check */
	ub = w + c->length_limit;
	if (ub > sent_length) ub = sent_length - 1;

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

static void mark_connector_sequence_for_dequeue(Connector *c, bool mark_bad_word)
{
	for (; NULL != c; c = c->next)
	{
		if (mark_bad_word) c->nearest_word = BAD_WORD;
		c->suffix_id--; /* Reference count. */
	}
}

/** The return value is the number of disjuncts deleted.
 *  Implementation notes:
 *  Normally all the identical disjunct-jets are memory shared.
 *  The suffix_id of each connector serves as its reference count
 *  in the power table. Each time when a connector that cannot match
 *  is discovered, its reference count is decreased, and its
 *  nearest_word field is assigned BAD_WORD. Due to the memory sharing,
 *  each such an assignment affects immediately all the identical
 *  disjunct-jets.
 *  */
static int power_prune(Sentence sent, Parse_Options opts)
{
	power_table pt;
	prune_context pc;
	int N_deleted[2] = {0}; /* [0] counts first deletions, [1] counts dups. */
	int total_deleted = 0;

	power_table_alloc(sent, &pt);
	power_table_init(sent, &pt);

	pc.pt = &pt;
	pc.power_cost = 0;
	pc.null_links = (opts->min_null_count > 0);
	pc.N_changed = 1;  /* forces it always to make at least two passes */
	pc.sent = sent;

	while (1)
	{
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

				bool is_bad = d->left->nearest_word == BAD_WORD;

				if (is_bad || left_connector_list_update(&pc, d->left, w, true) < 0)
				{
					mark_connector_sequence_for_dequeue(d->left, true);
					mark_connector_sequence_for_dequeue(d->right, false);

					/* discard the current disjunct */
					*dd = d->next; /* NEXT - set current disjunct to the next one */
					N_deleted[(int)is_bad]++;
					continue;
				}

				dd = &d->next; /* NEXT */
			}

			clean_table(pt.r_table_size[w], pt.r_table[w]);
		}

		total_deleted += N_deleted[0] + N_deleted[1];
		lgdebug(D_PRUNE, "Debug: l->r pass changed %d and deleted %d (%d+%d)\n",
		        pc.N_changed, N_deleted[0]+N_deleted[1], N_deleted[0], N_deleted[1]);

		if (pc.N_changed == 0 && N_deleted[0] == 0 && N_deleted[1] == 0) break;
		pc.N_changed = N_deleted[0] = N_deleted[1] = 0;

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

				bool is_bad = d->right->nearest_word == BAD_WORD;

				if (is_bad || right_connector_list_update(&pc, d->right, w, true) >= sent->length)
				{
					mark_connector_sequence_for_dequeue(d->right, true);
					mark_connector_sequence_for_dequeue(d->left, false);

					/* Discard the current disjunct. */
					*dd = d->next; /* NEXT - set current disjunct to the next one */
					N_deleted[(int)is_bad]++;
					continue;
				}

				dd = &d->next; /* NEXT */
			}

			clean_table(pt.l_table_size[w], pt.l_table[w]);
		}

		total_deleted += N_deleted[0] + N_deleted[1];
		lgdebug(D_PRUNE, "Debug: r->l pass changed %d and deleted %d (%d+%d)\n",
		        pc.N_changed, N_deleted[0]+N_deleted[1], N_deleted[0], N_deleted[1]);

		if (pc.N_changed == 0 && N_deleted[0] == 0 && N_deleted[1] == 0) break;
		pc.N_changed = N_deleted[0] = N_deleted[1] = 0;
	}
	power_table_delete(&pt);

	lgdebug(D_PRUNE, "Debug: power prune cost: %d\n", pc.power_cost);

	print_time(opts, "power pruned");
	if (verbosity_level(D_PRUNE))
	{
		prt_error("\n\\");
		prt_error("Debug: After power_pruning:\n\\");
		print_disjunct_counts(sent);
	}

#ifdef DEBUG
	for (WordIdx w = 0; w < sent->length; w++)
	{
		for (Disjunct *d = sent->word[w].d; NULL != d; d = d->next)
		{
			for (Connector *c = d->left; NULL != c; c = c->next)
				assert(c->nearest_word != BAD_WORD);
			for (Connector *c = d->right; NULL != c; c = c->next)
				assert(c->nearest_word != BAD_WORD);
		}
	}
#endif

	return total_deleted;
}

/* ===================================================================
   PP Pruning

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

static multiset_table * cms_table_new(void)
{
	multiset_table *mt;
	int i;

	mt = (multiset_table *) xalloc(sizeof(multiset_table));

	for (i=0; i<CMS_SIZE; i++) {
		mt->cms_table[i] = NULL;
	}
	return mt;
}

static void cms_table_delete(multiset_table *mt)
{
	Cms * cms, *xcms;
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

static unsigned int cms_hash(const char * s)
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

/**
 * This returns TRUE if there is a connector name C in the table
 * such that post_process_match(pp_match_name, C) is TRUE
 */
static bool match_in_cms_table(multiset_table *cmt, const char * pp_match_name)
{
	Cms * cms;
	for (cms = cmt->cms_table[cms_hash(pp_match_name)]; cms != NULL; cms = cms->next)
	{
		if (post_process_match(pp_match_name, cms->name)) return true;
	}
	return false;
}

static Cms * lookup_in_cms_table(multiset_table *cmt, const char * str)
{
	Cms * cms;
	for (cms = cmt->cms_table[cms_hash(str)]; cms != NULL; cms = cms->next)
	{
		if (string_set_cmp(str, cms->name)) return cms;
	}
	return NULL;
}

static void insert_in_cms_table(multiset_table *cmt, const char * str)
{
	Cms * cms;
	unsigned int h;
	cms = lookup_in_cms_table(cmt, str);
	if (cms != NULL) {
		cms->count++;
	} else {
		cms = (Cms *) xalloc(sizeof(Cms));
		cms->name = str;  /* don't copy the string...just keep a pointer to it.
							 we won't free these later */
		cms->count = 1;
		h = cms_hash(str);
		cms->next = cmt->cms_table[h];
		cmt->cms_table[h] = cms;
	}
}

/**
 * Delete the given string from the table.  Return TRUE if
 * this caused a count to go to 0, return FALSE otherwise.
 */
static bool delete_from_cms_table(multiset_table *cmt, const char * str)
{
	Cms * cms = lookup_in_cms_table(cmt, str);
	if (cms != NULL && cms->count > 0)
	{
		cms->count--;
		return (cms->count == 0);
	}
	return false;
}

static bool rule_satisfiable(multiset_table *cmt, pp_linkset *ls)
{
	unsigned int hashval;
	const char * t;
	char *name, *s;
	pp_linkset_node *p;
	int bad, n_subscripts;

	for (hashval = 0; hashval < ls->hash_table_size; hashval++)
	{
		for (p = ls->hash_table[hashval]; p!=NULL; p=p->next)
		{
			/* ok, we've got our hands on one of the criterion links */
			name = strdupa(p->str);
			/* could actually use the string in place because we change it back */

			/* now we want to see if we can satisfy this criterion link */
			/* with a collection of the links in the cms table */

			s = name;
			if (islower((int)*s)) s++; /* skip head-dependent indicator */
			for (; isupper((int)*s); s++) {}
			for (;*s != '\0'; s++) if (*s != '*') *s = '#';

			s = name;
			t = p->str;
			if (islower((int)*s)) s++; /* skip head-dependent indicator */
			if (islower((int)*t)) t++; /* skip head-dependent indicator */
			for (; isupper((int) *s); s++, t++) {}

			/* s and t remain in lockstep */
			bad = 0;
			n_subscripts = 0;
			for (;*s != '\0' && bad==0; s++, t++) {
				if (*s == '*') continue;
				n_subscripts++;
				/* after the upper case part, and is not a * so must be a regular subscript */
				*s = *t;
				if (!match_in_cms_table(cmt, name)) bad++;
				*s = '#';
			}

			if (n_subscripts == 0) {
				/* now we handle the special case which occurs if there
				   were 0 subscripts */
				if (!match_in_cms_table(cmt, name)) bad++;
			}

			/* now if bad==0 this criterion link does the job
			   to satisfy the needs of the trigger link */

			if (bad == 0) return true;
		}
	}
	return false;
}

static void delete_unmarked_disjuncts(Sentence sent)
{
	size_t w;
	Disjunct *d_head, *d, *dx;

	for (w=0; w<sent->length; w++) {
		d_head = NULL;
		for (d=sent->word[w].d; d != NULL; d=dx) {
			dx = d->next;
			if (d->marked) {
				d->next = d_head;
				d_head = d;
			}
		}
		sent->word[w].d = d_head;
	}
}

static int pp_prune(Sentence sent, Parse_Options opts)
{
	pp_knowledge * knowledge;
	size_t i, w;
	int total_deleted, N_deleted;
	bool change, deleteme;
	multiset_table *cmt;

	if (sent->postprocessor == NULL) return 0;
	if (!opts->perform_pp_prune) return 0;

	knowledge = sent->postprocessor->knowledge;

	cmt = cms_table_new();

	for (w = 0; w < sent->length; w++)
	{
		Disjunct *d;
		for (d = sent->word[w].d; d != NULL; d = d->next)
		{
			char dir;
			d->marked = true;
			for (dir=0; dir < 2; dir++)
			{
				Connector *c;
				for (c = ((dir) ? (d->left) : (d->right)); c != NULL; c = c->next)
				{
					insert_in_cms_table(cmt, connector_string(c));
				}
			}
		}
	}

	total_deleted = 0;
	change = true;
	while (change)
	{
		char dir;

		change = false;
		N_deleted = 0;
		for (w = 0; w < sent->length; w++)
		{
			Disjunct *d;
			for (d = sent->word[w].d; d != NULL; d = d->next)
			{
				if (!d->marked) continue;
				deleteme = false;
				for (i = 0; i < knowledge->n_contains_one_rules; i++)
				{
					pp_rule* rule = &knowledge->contains_one_rules[i]; /* the ith rule */
					const char * selector = rule->selector;  /* selector string for this rule */
					pp_linkset * link_set = rule->link_set;  /* the set of criterion links */

					if (rule->selector_has_wildcard) continue;  /* If it has a * forget it */

					for (dir = 0; dir < 2; dir++)
					{
						Connector *c;
						for (c = ((dir) ? (d->left) : (d->right)); c != NULL; c = c->next)
						{

							if (!post_process_match(selector, connector_string(c))) continue;

							/*
							printf("pp_prune: trigger ok.  selector = %s  c->string = %s\n", selector, c->string);
							*/

							/* We know c matches the trigger link of the rule. */
							/* Now check the criterion links */

							if (!rule_satisfiable(cmt, link_set))
							{
								deleteme = true;
								rule->use_count++;
							}
							if (deleteme) break;
						}
						if (deleteme) break;
					}
					if (deleteme) break;
				}

				if (deleteme)         /* now we delete this disjunct */
				{
					N_deleted++;
					total_deleted++;
					d->marked = false; /* mark for deletion later */
					for (dir=0; dir < 2; dir++)
					{
						Connector *c;
						for (c = ((dir) ? (d->left) : (d->right)); c != NULL; c = c->next)
						{
							change |= delete_from_cms_table(cmt, connector_string(c));
						}
					}
				}
			}
		}

		lgdebug(D_PRUNE, "Debug: pp_prune pass deleted %d\n", N_deleted);
	}
	cms_table_delete(cmt);

	if (total_deleted > 0)
	{
		delete_unmarked_disjuncts(sent);
		if (verbosity_level(D_PRUNE))
		{
			prt_error("\n\\");
			prt_error("Debug: After pp_prune:\n\\");
			print_disjunct_counts(sent);
		}
	}

	print_time(opts, "pp pruning");

	return total_deleted;
}


/**
 * Do the following pruning steps until nothing happens:
 * power pp power pp power pp....
 * Make sure you do them both at least once.
 */
void pp_and_power_prune(Sentence sent, Parse_Options opts)
{
	power_prune(sent, opts);
	pp_prune(sent, opts);

	return;

	// Not reached. We can actually gain a few percent of
	// performance be skipping the loop below. Mostly, it just
	// does a lot of work, and pretty much finds nothing.
	// And so we skip it.
#ifdef ONLY_IF_YOU_THINK_THIS_IS_WORTH_IT
	for (;;) {
		if (pp_prune(sent, opts) == 0) break;
		if (power_prune(sent, opts) == 0) break;
	}
#endif
}
