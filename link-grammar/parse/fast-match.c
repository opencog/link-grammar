/**************************************************************************/
/* Copyright (c) 2004                                                     */
/* Daniel Sleator, David Temperley, and John Lafferty                     */
/* Copyright (c) 2014 Linas Vepstas                                       */
/* Copyright (c) 2015-2022 Amir Plivatsky                                 */
/* All rights reserved                                                    */
/*                                                                        */
/* Use of the link grammar parsing system is subject to the terms of the  */
/* license set forth in the LICENSE file included with this software.     */
/* This license allows free redistribution and use in source and binary   */
/* forms, with or without modification, subject to certain conditions.    */
/*                                                                        */
/**************************************************************************/

#include "api-structures.h"             // Sentence_s
#include "connectors.h"
#include "disjunct-utils.h"
#include "fast-match.h"
#include "string-set.h"
#include "dict-common/dict-common.h"    // contable
#include "tokenize/word-structures.h"   // Word_struct
#include "tokenize/wordgraph.h"
#include "tokenize/tok-structures.h"    // TODO provide gword access methods!
#include "utilities.h"                  // UNREACHABLE

/**
 * The entire goal of this file is provide a fast lookup of all of the
 * disjuncts on a given word that might be able to connect to a given
 * connector on the left or the right. The main entry point is
 * form_match_list(), which performs this lookup.
 *
 * The lookup is fast, because it uses a precomputed lookup table to
 * find the match candidates.  The lookup table is stocked by looking
 * at all disjuncts on all words, and sorting them into bins organized
 * by connectors they could potentially connect to.  The lookup table
 * is created by calling the alloc_fast_matcher() function.
 *
 * free_fast_matcher() is used to free the matcher.
 * form_match_list() manages its memory as a "stack" - match-lists are
 * pushed on this stack. The said stack size gets over 2048 entries only
 * for long and/or complex sentences.
 * pop_match_list() releases the memory that form_match_list() returned
 * by unwinding this stack.
 */

#define D_FAST_MATCHER 9 /* General debug level for this file. */

#define MATCH_LIST_SIZE_INIT 4096 /* the initial size of the match-list stack */
#define MATCH_LIST_SIZE_INC 2     /* match-list stack increase size factor */

#ifndef ML_COMPAT
#define ML_COMPAT 0 /* 1: Disjunct order compatible to V5.6.2 (slower). */
#endif /* ML_COMPAT */

/*
 * Each lookup table entry points to list of disjuncts whose shallow
 * connector has the same uppercase part. These lists are sorted according
 * to the nearest_word of these connectors. The sorting is done using
 * "bucket sort" in which the number of bins is equal to the different
 * number of values of nearest word so only one sorting round is needed.
 */
typedef struct sortbin_s sortbin;
struct sortbin_s
{
	Match_node *head;
#if ML_COMPAT
	Match_node *tail;
#endif
};

/**
 * Push a match-list element into the match-list array.
 */
static void push_match_list_element(fast_matcher_t *ctxt, int id, Disjunct *d)
{
	if (ctxt->match_list_end >= ctxt->match_list_size)
	{
		ctxt->match_list_size *= MATCH_LIST_SIZE_INC;
		ctxt->match_list = realloc(ctxt->match_list,
		                      ctxt->match_list_size * sizeof(*ctxt->match_list));
	}

#ifdef VERIFY_MATCH_LIST
	if (id != 0) d->match_id = id;
#endif
	ctxt->match_list[ctxt->match_list_end++] = d;
}

/**
 * Free all of the hash tables and Match_nodes
 */
void free_fast_matcher(Sentence sent, fast_matcher_t *mchxt)
{
	if (NULL == mchxt) return;

	free(mchxt->l_table[0]);
	free(mchxt->match_list);
	lgdebug(+6, "Sentence length %zu, match_list_size %zu\n",
	        mchxt->size, mchxt->match_list_size);

	pool_delete(mchxt->mld_pool);
	pool_delete(mchxt->mlc_pool);
	xfree(mchxt->l_table_size, mchxt->size * sizeof(unsigned int));
	xfree(mchxt->l_table, mchxt->size * sizeof(Match_node **));
	xfree(mchxt, sizeof(fast_matcher_t));
}

static Match_node *match_list_not_found = NULL;

static Match_node **get_match_table_entry(unsigned int size, Match_node **t,
                                          Connector * c, int dir)
{
	unsigned int h, s;
	s = h = connector_uc_hash(c) & (size-1);

	if (dir == 1) {
		while (NULL != t[h])
		{
			if (connector_uc_eq(t[h]->d->right, c)) break;

			/* Increment and try again. Every hash bucket MUST have
			 * a unique upper-case part, since later on, we only
			 * compare the lower-case parts, assuming upper-case
			 * parts are already equal. So just look for the next
			 * unused hash bucket.
			 */
			h = (h + 1) & (size-1);
			if (h == s) return &match_list_not_found;
		}
	}
	else
	{
		while (NULL != t[h])
		{
			if (connector_uc_eq(t[h]->d->left, c)) break;
			h = (h + 1) & (size-1);
			if (h == s) return &match_list_not_found;
		}
	}

	return &t[h];
}

/**
 * Add the match nodes at sbin to the table entry of their shallow connector.
 */
static void add_to_table_entry(unsigned int tsize, Match_node **table,
                               int dir, sortbin *sbin)
{
	Match_node *m_next;
	for (Match_node *m = sbin->head; NULL != m; m = m_next)
	{
		Connector *c = (0 == dir) ? m->d->left : m->d->right;
		assert(NULL != c, "NULL connector");

		Match_node **xl = get_match_table_entry(tsize, table, c, dir);
		assert(&match_list_not_found != xl, "get_match_table_entry: Overflow");

		m_next = m->next; /* Remember before overwriting. */

		/* Insert m at head of list */
		m->next = *xl;
		*xl = m;
	}
}

/**
 * Put the nearest_word-sorted disjuncts at sbin into the given hash table.
 * Since add_to_table_entry() inserts at entry's head, insert in the
 * reverse order than the needed match-list order of nearest_word.
 *
 * @param tsize The hash table size.
 * @param table The hash table.
 * @param w The word to which these disjuncts belong.
 * @param dir 0: Put it into a right table; 1: Put it  into a left table.
 * @param sbin Indexed by the disjuncts' shallow-connector nearest_word.
 * @param sent_length Sentence length, for sbin index upper bound.
 */
static void put_into_match_table(unsigned int tsize, Match_node **table,
                                 int w, int dir, sortbin *sbin,
                                 size_t sent_length)
{
	if (0 == dir)
	{
		/* Left match-list needed order: Increasing nearest_word. */
		for (WordIdx sbw = 0; sbw < (WordIdx)w; sbw++)
		{
			add_to_table_entry(tsize, table, dir, &sbin[sbw]);
		}
	}
	else
	{
		/* Right match-list needed order: Decreasing nearest_word. */
		for (WordIdx sbw = sent_length-1; sbw > (WordIdx)w; sbw--)
		{
			add_to_table_entry(tsize, table, dir, &sbin[sbw]);
		}
	}
}

static void clean_sortbin(sortbin *sbin, size_t sent_length)
{
	for (unsigned int i = 0; i < sent_length; i++)
				sbin[i].head = NULL;
}

/**
 * Put the Match_node m into a list according to nearest_word.
 * This is a bucket sort.
 */
static void sort_by_nearest_word(Match_node *m, sortbin *sbin, int nearest_word)
{

	sbin = &sbin[nearest_word]; /* Bucket selection. */

#if ML_COMPAT
	if (NULL == sbin->head)
	{
		sbin->head = m;
	}
	else
	{
		sbin->tail->next = m;
	}
	sbin->tail = m;
	m->next = NULL;
#else
	m->next = sbin->head;
	sbin->head = m;
#endif
}

fast_matcher_t* alloc_fast_matcher(const Sentence sent, unsigned int *ncu[])
{
	assert(sent->length > 0, "Sentence length is 0");

	fast_matcher_t *ctxt;

	ctxt = (fast_matcher_t *) xalloc(sizeof(fast_matcher_t));
	ctxt->size = sent->length;
	ctxt->l_table_size = xalloc(2 * sent->length * sizeof(unsigned int));
	ctxt->r_table_size = ctxt->l_table_size + sent->length;
	ctxt->l_table = xalloc(2 * sent->length * sizeof(Match_node **));
	ctxt->r_table = ctxt->l_table + sent->length;
	memset(ctxt->l_table, 0, 2 * sent->length * sizeof(Match_node **));

	ctxt->match_list_size = MATCH_LIST_SIZE_INIT;
	ctxt->match_list = xalloc(ctxt->match_list_size * sizeof(*ctxt->match_list));
	ctxt->match_list_end = 0;

	if (NULL != sent->Match_node_pool)
	{
		pool_reuse(sent->Match_node_pool);
	}
	else
	{
		sent->Match_node_pool =
			pool_new(__func__, "Match_node",
			         /*num_elements*/2048, sizeof(Match_node),
			         /*zero_out*/false, /*align*/true, /*exact*/false);
	}
	ctxt->mld_pool =
		pool_new(__func__, "Match list cache",
		         /*num_elements*/512*1024, sizeof(Disjunct *),
		         /*zero_out*/false, /*align*/false, /*exact*/false);
	ctxt->mlc_pool =
		pool_new(__func__, "Match list counts",
		         /*num_elements*/512*1024, sizeof(count_t),
		         /*zero_out*/false, /*align*/false, /*exact*/false);

	sortbin *sbin = alloca(sent->length * sizeof(sortbin));

	/* Calculate the sizes of the hash tables. */
	unsigned int num_headers = 0;
	Match_node **memblock_headers;
	Match_node **hash_table_header;

	for (WordIdx w = 0; w < sent->length; w++)
	{
		for (int dir = 0; dir < 2; dir++)
		{
			unsigned int tsize;
			unsigned int n = ncu[dir][w];

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

	memblock_headers = malloc(num_headers * sizeof(Match_node *));
	memset(memblock_headers, 0, num_headers * sizeof(Match_node *));
	hash_table_header = memblock_headers;

	for (WordIdx w = 0; w < sent->length; w++)
	{
		clean_sortbin(sbin, sent->length);

		/* Sort the disjuncts of each word according to the nearest word
		 * that their left and right shallow connectors can connect to.
		 * For performance of the parsing stage, the loop over
		 * the disjuncts is done separately for the left and right
		 * connectors, so the Match_node memory of consequent disjuncts in
		 * the match lists will be adjacent in memory (a noticeable
		 * speedup due to a better use of the CPU cache). */
		for (Disjunct *d = sent->word[w].d; NULL != d; d = d->next)
		{
			if (d->left != NULL)
			{
				Match_node *m = pool_alloc(sent->Match_node_pool);
				m->d = d;
				sort_by_nearest_word(m, sbin, d->left->nearest_word);
			}
		}
		for (Disjunct *d = sent->word[w].d; NULL != d; d = d->next)
		{
			if (d->right != NULL)
			{
				Match_node *m = pool_alloc(sent->Match_node_pool);
				m->d = d;
				sort_by_nearest_word(m, sbin, d->right->nearest_word);
			}
		}

		/* Build the hash tables. */
		for (int dir = 0; dir < 2; dir++)
		{
			unsigned int tsize = ncu[dir][w];
			Match_node **t = hash_table_header;

			hash_table_header += tsize;

			if (0 == dir)
			{
				ctxt->l_table[w] = t;
				ctxt->l_table_size[w] = tsize;
			}
			else
			{
				ctxt->r_table[w] = t;
				ctxt->r_table_size[w] = tsize;
			}

			put_into_match_table(tsize, t, w, dir, sbin, sent->length);
		}
	}

	assert(memblock_headers + num_headers == hash_table_header,
	   "Mismatch header sizes");
	return ctxt;
}

#if 0
/**
 * Print statistics on various connector matching aspects.
 * A summary can be found by the shell commands:
 * link-parser < file.batch | grep match_stats: | sort | uniq -c
 */
static void match_stats(Connector *c1, Connector *c2)
{
	if (NULL == c1) printf("match_stats: cache\n");
	if (NULL == c2) return;
	if ((1 == c1->uc_start) && (1 == c2->uc_start) &&
	    (c1->string[0] == c2->string[0]))
	{
		printf("match_stats: h/d mismatch\n");
	}

	if (0 == c1->lc_start) printf("match_stats: no lc (c1)\n");
	if (0 == c2->lc_start) printf("match_stats: no lc (c2)\n");

	if (string_set_cmp(c1->string, c2->string)) printf("match_stats: same\n");

	const char *a = &c1->string[c1->lc_start];
	const char *b = &c2->string[c2->lc_start];
	do
	{
		if (*a != *b && (*a != '*') && (*b != '*')) printf("match_stats: lc false\n");
		a++;
		b++;
	} while (*a != '\0' && *b != '\0');
	printf("match_stats: lc true\n");
}
#else
#define match_stats(a, b)
#endif

#ifdef DEBUG
#undef N
#define N(c) (c?connector_string(c):"")

/**
 * Print the match list, including connector match indications.
 * Usage: link-parser -verbosity=9 -debug=print_match_list
 * Output format:
 * MATCH_NODE list_id:  lw>lc   [=]   left<w>right   [=]    rc<rw
 *
 * Regretfully this version doesn't indicate which match shortcut has been
 * used, and which nodes are from mr or ml or both (the full print version
 * clutters the source code very much, as it needs to be inserted in plenty
 * of places.)
 */
static void print_match_list(fast_matcher_t *ctxt, int id, size_t mlb, int w,
                             Connector *lc, int lw,
                             Connector *rc, int rw,
                             Disjunct ***mlcl, Disjunct ***mlcr)
{
	if (!verbosity_level(D_FAST_MATCHER)) return;
	Disjunct **m = &ctxt->match_list[mlb];

	for (; NULL != *m; m++)
	{
		Disjunct *d = *m;

		prt_error("MATCH_NODE %c%c %5d: %02d>%-9s %c %9s<%02d>%-9s %c %9s<%02d\n",
		       (mlcl == NULL) ? ' ' : 'L', (mlcr == NULL) ? ' ' : 'R',
		       id, lw , N(lc), d->match_left ? '=': ' ',
		       N(d->left), w, N(d->right),
		       d->match_right? '=' : ' ', N(rc), rw);
	}
}
#else
#define print_match_list(...)
#endif

typedef struct
{
	const condesc_t *desc;
	bool match;
} match_cache;

/**
 * Match the lower-case parts of connectors, and the head-dependent,
 * using a cache of the most recent compare.  Due to the way disjuncts
 * are written, we are often asked to compare to the same connector
 * 3 or 4 times in a row. So if we already did that compare, just use
 * the cached result. (i.e. the caching here is almost trivial, but it
 * works well).
 */
static bool do_match_with_cache(Connector *a, Connector *b, match_cache *c_con)
{
	/* The following uses a string-set compare - string_set_cmp() cannot
	 * be used here because c_con->string may be NULL. */
	match_stats(c_con->string == a->string ? NULL : a, NULL);
	UNREACHABLE(connector_desc(a) == NULL); // clang static analyzer suppression.
	if (c_con->desc == connector_desc(a))
	{
		/* The match_cache desc field is initialized to NULL, and this is
		 * enough because the connector desc field cannot be NULL, as it
		 * actually fetched a non-empty match list. */
		PRAGMA_MAYBE_UNINITIALIZED
		return c_con->match;
		PRAGMA_END
	}

	/* No cache match. Check if the connectors match and cache the result.
	 * We know that the uc parts of the connectors are the same, because
	 * we fetch the matching lists according to the uc part or the
	 * connectors to be matched. So the uc parts are not checked here. */
	c_con->match = lc_easy_match(connector_desc(a), connector_desc(b));
	c_con->desc = connector_desc(a);

	return c_con->match;
}

typedef struct
{
	const Gword *gword;
	bool same_alternative;
} gword_cache;

/**
 * Return true iff c1 and c2 are from the same alternative.
 * An optimization for English checks if one of the connectors belongs
 * to an original sentence word (c2 is checked first for an inline
 * optimization opportunity).
 * If a wordgraph word of the checked connector is the same
 * as of the previously checked one, use the cached result.
 * (The first wordgraph word is used for cache validity indication,
 * but there is only one most of the times anyway.)
 */
#define OPTIMIZE_EN
static bool alt_connection_possible(Connector *c1, Connector *c2,
                                    gword_cache *c_con)
{
	bool same_alternative = false;

#ifdef OPTIMIZE_EN
	/* Try a shortcut first. */
	if ((c2->originating_gword->o_gword->hier_depth == 0) ||
	    (c1->originating_gword->o_gword->hier_depth == 0))
	{
			return true;
	}
#endif /* OPTIMIZE_EN */

	if (c1->originating_gword->o_gword == c_con->gword)
		return c_con->same_alternative;

	/* Each of the loops is of one iteration most of the times. */
	for (const gword_set *ga = c1->originating_gword; NULL != ga; ga = ga->next)
	{
		for (const gword_set *gb = c2->originating_gword; NULL != gb; gb = gb->next)
		{
			if (in_same_alternative(ga->o_gword, gb->o_gword))
			{
				 same_alternative = true;
				 break;
			}
		}
		if (same_alternative) break;
	}

	c_con->same_alternative = same_alternative;
	c_con->gword = c1->originating_gword->o_gword;

	return same_alternative;
}

/**
 * Add match-list termination element (NULL).
 * Optionally print it (for debug).
 * Return the match list start.
 */
static size_t terminate_match_list(fast_matcher_t *ctxt, int id,
                             size_t ml_start, int w,
                             Connector *lc, int lw,
                             Connector *rc, int rw,
                             Disjunct ***mlcl, Disjunct ***mlcr)
{
	push_match_list_element(ctxt, 0, NULL);
	print_match_list(ctxt, id, ml_start, w, lc, lw, rc, rw, mlcl, mlcr);
	return ml_start;
}

/**
 * Forms and returns a list of disjuncts coming from word w, that
 * actually matches lc or rc or both. The lw and rw are the words from
 * which lc and rc came respectively.
 *
 * The list is returned in an array of Match_nodes.  This list
 * contains no duplicates, because when processing the ml list, only
 * elements whose match_left is true are included, and such elements are
 * not included again when processing the mr list.
 *
 * Note that if both lc and rc match the corresponding connectors of w,
 * match_left is set to true when the ml list is processed and the
 * disjunct is then added to the result list, and match_right of the
 * same disjunct is set to true when the mr list is processed, and this
 * disjunct is not added again.
 *
 * "lc optimization" (see below) marks here a special optimization for the
 * left connector (lc): If it is non-NULL and doesn't match a disjunct,
 * there is no need to try to match the right connector (rc), since the
 * linkage count would be 0 in any case. See do_count() for details. In
 * that case we can skip the corresponding match list element, or even
 * immediately return an empty match list if there are no matches for lc.
 */
size_t
form_match_list(fast_matcher_t *ctxt, int w,
                Connector *lc, int lw,
                Connector *rc, int rw,
                Disjunct ***mlcl, Disjunct ***mlcr)
{
	Match_node *mx, *mr_end;
	size_t front = get_match_list_position(ctxt);
	Match_node *ml = NULL, *mr = NULL; /* Initialize in case of NULL lc or rc. */
	Disjunct **cmx;
	match_cache mc;
	gword_cache gc = { .same_alternative = false };

	if ((mlcl != NULL) && (*mlcl == NULL)) mlcl = NULL;
	if ((mlcr != NULL) && (*mlcr == NULL)) mlcr = NULL;

	if (mlcl == NULL)
	{
		/* Get the lists of candidate matching disjuncts of word w for lc and
		 * rc. Consider each of these lists only if the farthest_word of lc/rc
		 * reaches at least the word w.
		 * Note: The commented out (w <= lc->farthest_word) is checked at the
		 * callers and is left here for documentation. */
		if ((lc != NULL) /* && (w <= lc->farthest_word) */)
		{
			ml = *get_match_table_entry(ctxt->l_table_size[w], ctxt->l_table[w], lc, 0);
		}
		if ((lc != NULL) && (ml == NULL)) /* lc optimization */
			return terminate_match_list(ctxt, -1, front, w, lc, lw, rc, rw, mlcl, mlcr);
	}

	if (mlcr == NULL)
	{
		if ((rc != NULL) && (w >= rc->farthest_word))
		{
			mr = *get_match_table_entry(ctxt->r_table_size[w], ctxt->r_table[w], rc, 1);
		}
		if ((ml == NULL) && (mlcl == NULL) && (mr == NULL))
			return terminate_match_list(ctxt, -2, front, w, lc, lw, rc, rw, mlcl, mlcr);
	}

#ifdef VERIFY_MATCH_LIST
	static int id = 0;
	int lid = ++id; /* A local copy, for multi-threading support. */
#else
	const int lid = 0;
#endif

	lgdebug(+D_FAST_MATCHER, "MATCH_LIST %c%c %5d mlb %zu\n",
		       (mlcl == NULL) ? ' ' : 'L', (mlcr == NULL) ? ' ' : 'R', id, front);

	if (mlcr == NULL)
	{
		for (mx = mr; mx != NULL; mx = mx->next)
		{
			if (mx->d->right->nearest_word > rw) break;
			mx->d->match_left = false;
		}
		mr_end = mx;
	}
	else
	{
		for (cmx = *mlcr; *cmx != NULL; cmx++)
		{
			(*cmx)->match_left = false;
		}
		mr_end = NULL; /* Prevent a gcc "mnay be uninitialkized" warning */
	}

	/* Construct the list of things that could match the left. */
	if (mlcl == NULL)
	{
		mc.desc = NULL;
		gc.gword = NULL;

		for (mx = ml; mx != NULL; mx = mx->next)
		{
			if (mx->d->left->nearest_word < lw) break;
			if (lw < mx->d->left->farthest_word) continue;

			mx->d->match_left = do_match_with_cache(mx->d->left, lc, &mc) &&
			                    alt_connection_possible(mx->d->left, lc, &gc);
			if (!mx->d->match_left) continue;
			mx->d->match_right = false;

			push_match_list_element(ctxt, lid, mx->d);
		}

		if ((lc != NULL) && is_no_match_list(ctxt, front)) /* lc optimization */
			return terminate_match_list(ctxt, -3, front, w, lc, lw, rc, rw, mlcl, mlcr);
	}
	else
	{
		for (cmx = *mlcl; *cmx != NULL; cmx++)
		{
			(*cmx)->match_left = true;
			(*cmx)->match_right = false;
			push_match_list_element(ctxt, lid, *cmx);
		}
	}

	/* Append the list of things that could match the right.
	 * Note that it is important to set here match_right correctly even
	 * if we are going to skip this element here because its match_left
	 * is true, since then it means it is already included in the match
	 * list. */
	if (mlcr == NULL)
	{
		mc.desc = NULL;
		gc.gword = NULL;
		for (mx = mr; mx != mr_end; mx = mx->next)
		{
			if (rw > mx->d->right->farthest_word) continue;

			if ((lc != NULL) && !mx->d->match_left) continue; /* lc optimization */
			mx->d->match_right = do_match_with_cache(mx->d->right, rc, &mc) &&
			                     alt_connection_possible(mx->d->right, rc, &gc);
			if (!mx->d->match_right || mx->d->match_left) continue;

			push_match_list_element(ctxt, lid, mx->d);
		}
	}
	else
	{
		for (cmx = *mlcr; *cmx != NULL; cmx++)
		{
			if ((lc != NULL) && !(*cmx)->match_left) continue; /* lc optimization*/
			(*cmx)->match_right = true;
			(*cmx)->rcount_index = (uint32_t)(cmx - *mlcr);
			if ((*cmx)->match_left) continue;
			push_match_list_element(ctxt, lid, *cmx);
		}
	}

	return terminate_match_list(ctxt, lid, front, w, lc, lw, rc, rw, mlcl, mlcr);
}
