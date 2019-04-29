/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright 2018, 2019, Amir Plivatsky                                  */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/
#include <string.h>
#include <limits.h>                     // UINT_MAX

#include "api-structures.h"             // Sentence
#include "connectors.h"
#include "tracon-set.h"
#include "disjunct-utils.h"
#include "print/print-util.h"
#include "memory-pool.h"
#include "utilities.h"
#include "tokenize/tok-structures.h"    // XXX TODO provide gword access methods!
#include "tokenize/word-structures.h"

/* Disjunct utilities ... */

#define D_DISJ 5                        /* Verbosity level for this file. */

/**
 * free_disjuncts() -- free the list of disjuncts pointed to by c
 * (does not free any strings)
 */
void free_disjuncts(Disjunct *c)
{
	Disjunct *c1;
	for (;c != NULL; c = c1) {
		c1 = c->next;
		free_connectors(c->left);
		free_connectors(c->right);
		xfree((char *)c, sizeof(Disjunct));
	}
}

void free_sentence_disjuncts(Sentence sent)
{
	if (NULL != sent->dc_memblock)
	{
		free(sent->dc_memblock);
		sent->dc_memblock = NULL;
	}
	else if (NULL != sent->Disjunct_pool)
	{
		pool_delete(sent->Disjunct_pool);
		pool_delete(sent->Connector_pool);
		sent->Disjunct_pool = NULL;
	}
}

/**
 * Destructively catenates the two disjunct lists d1 followed by d2.
 * Doesn't change the contents of the disjuncts.
 * Traverses the first list, but not the second.
 */
Disjunct * catenate_disjuncts(Disjunct *d1, Disjunct *d2)
{
	Disjunct * dis = d1;

	if (d1 == NULL) return d2;
	if (d2 == NULL) return d1;
	while (dis->next != NULL) dis = dis->next;
	dis->next = d2;
	return d1;
}

/** Returns the number of disjuncts in the list pointed to by d */
unsigned int count_disjuncts(Disjunct * d)
{
	unsigned int count = 0;
	for (; d != NULL; d = d->next)
	{
		count++;
	}
	return count;
}

/** Returns the number of disjuncts and connectors in the sentence. */
static void count_disjuncts_and_connectors(Sentence sent,
                                           unsigned int *dca, unsigned int *cca)
{
	unsigned int ccnt = 0, dcnt = 0;

	for (WordIdx w = 0; w < sent->length; w++)
	{
		for (Disjunct *d = sent->word[w].d; d != NULL; d = d->next)
		{
			dcnt++;
			for (Connector *c = d->left; c != NULL; c = c->next) ccnt++;
			for (Connector *c = d->right; c !=NULL; c = c->next) ccnt++;
		}
	}

	*cca = ccnt;
	*dca = dcnt;
}
/* ============================================================= */

typedef struct disjunct_dup_table_s disjunct_dup_table;
struct disjunct_dup_table_s
{
	size_t dup_table_size;
	Disjunct *dup_table[];
};

/**
 * This is a hash function for disjuncts
 *
 * This is the old version that doesn't check for domination, just
 * equality.
 */
static inline unsigned int old_hash_disjunct(disjunct_dup_table *dt, Disjunct * d)
{
	unsigned int i;
	i = 0;
	for (Connector *e = d->left ; e != NULL; e = e->next) {
		i = (5 * (i + e->desc->uc_num)) + (unsigned int)e->desc->lc_letters + 7;
	}
	for (Connector *e = d->right ; e != NULL; e = e->next) {
		i = (5 * (i + e->desc->uc_num)) + (unsigned int)e->desc->lc_letters + 7;
	}
#if 0 /* Redundant - the connector hashing has enough entropy. */
	i += string_hash(d->word_string);
#endif
	i += (i>>10);

	d->dup_hash = i;
	return (i & (dt->dup_table_size-1));
}

/**
 * The connectors must be exactly equal.
 */
static bool connectors_equal_prune(Connector *c1, Connector *c2)
{
	return c1->desc == c2->desc && (c1->multi == c2->multi);
}

/** returns TRUE if the disjuncts are exactly the same */
static bool disjuncts_equal(Disjunct * d1, Disjunct * d2)
{
	Connector *e1, *e2;

	e1 = d1->left;
	e2 = d2->left;
	while ((e1 != NULL) && (e2 != NULL)) {
		if (!connectors_equal_prune(e1, e2)) return false;
		e1 = e1->next;
		e2 = e2->next;
	}
	if ((e1 != NULL) || (e2 != NULL)) return false;

	e1 = d1->right;
	e2 = d2->right;
	while ((e1 != NULL) && (e2 != NULL)) {
		if (!connectors_equal_prune(e1, e2)) return false;
		e1 = e1->next;
		e2 = e2->next;
	}
	if ((e1 != NULL) || (e2 != NULL)) return false;

	/* Save CPU time by comparing this last, since this will
	 * almost always be true. Rarely, the strings are not from
	 * the same string_set and hence the 2-step comparison. */
	if (d1->word_string == d2->word_string) return true;
	return (strcmp(d1->word_string, d2->word_string) == 0);
}

#if 0
int de_fp = 0;
int de_total = 0;
static void disjuncts_equal_stat(void)
{
		fprintf(stderr, "disjuncts_equal FP %d/%d\n", de_fp, de_total);
}

static bool disjuncts_equal(Disjunct * d1, Disjunct * d2)
{
	if (de_total == 0) atexit(disjuncts_equal_stat);
	de_total++;

	bool rc = disjuncts_equal1(d1, d2);
	if (!rc) de_fp++;

	return rc;
}
#endif

#if 0
/**
 * Duplicate the given connector chain.
 * If the argument is NULL, return NULL.
 */
static Connector *connectors_dup(Pool_desc *connector_pool, Connector *origc)
{
	Connector head;
	Connector *prevc = &head;
	Connector *newc = &head;

	for (Connector *t = origc; t != NULL;  t = t->next)
	{
		newc = connector_new(connector_pool, NULL, NULL);
		*newc = *t;

		prevc->next = newc;
		prevc = newc;
	}
	newc->next = NULL;

	return head.next;
}

/**
 * Duplicate the given disjunct chain.
 * If the argument is NULL, return NULL.
 */
static Disjunct *disjuncts_dup(Pool_desc *Disjunct_pool, Pool_desc *Connector_pool,
                        Disjunct *origd)
{
	Disjunct head;
	Disjunct *prevd = &head;
	Disjunct *newd = &head;

	for (Disjunct *t = origd; t != NULL; t = t->next)
	{
		newd = pool_alloc(Disjunct_pool);
		newd->word_string = t->word_string;
		newd->cost = t->cost;
		newd->left = connectors_dup(Connector_pool, t->left);
		newd->right = connectors_dup(Connector_pool, t->right);
		newd->originating_gword = t->originating_gword;
		prevd->next = newd;
		prevd = newd;
	}
	newd->next = NULL;

	return head.next;
}
#endif

static disjunct_dup_table * disjunct_dup_table_new(size_t sz)
{
	disjunct_dup_table *dt;

	dt = malloc(sz * sizeof(Disjunct *) + sizeof(disjunct_dup_table));
	dt->dup_table_size = sz;

	memset(dt->dup_table, 0, sz * sizeof(Disjunct *));

	return dt;
}

static void disjunct_dup_table_delete(disjunct_dup_table *dt)
{
	free(dt);
}

#ifdef DEBUG
GNUC_UNUSED static int gword_set_len(const gword_set *gl)
{
	int len = 0;
	for (; NULL != gl; gl = gl->next) len++;
	return len;
}
#endif

/**
 * Return a new gword_set element, initialized from the given element.
 * @param old_e Existing element.
 */
static gword_set *gword_set_element_new(gword_set *old_e)
{
	gword_set *new_e = malloc(sizeof(gword_set));
	*new_e = (gword_set){0};

	new_e->o_gword = old_e->o_gword;
	gword_set *chain_next = old_e->chain_next;
	old_e->chain_next = new_e;
	new_e->chain_next = chain_next;

	return new_e;
}

/**
 * Add an element to existing gword_set. Uniqueness is assumed.
 * @return A new set with the element.
 */
static gword_set *gword_set_add(gword_set *gset, gword_set *ge)
{
	gword_set *n = gword_set_element_new(ge);
	n->next = gset;
	gset = n;

	return gset;
}

/**
 * Combine the given gword sets.
 * The gword sets are not modified.
 * This function is used for adding the gword pointers of an eliminated
 * disjunct to the ones of the kept disjuncts, with no duplicates.
 *
 * @param kept gword_set of the kept disjunct.
 * @param eliminated gword_set of the eliminated disjunct.
 * @return Use copy-on-write semantics - the gword_set of the kept disjunct
 * just gets returned if there is nothing to add to it. Else - a new gword
 * set is returned.
 */
static gword_set *gword_set_union(gword_set *kept, gword_set *eliminated)
{
	/* Preserve the gword pointers of the eliminated disjunct if different. */
	gword_set *preserved_set = NULL;
	for (gword_set *e = eliminated; NULL != e; e = e->next)
	{
		gword_set *k;

		/* Ensure uniqueness. */
		for (k = kept; NULL != k; k = k->next)
			if (e->o_gword == k->o_gword) break;
		if (NULL != k) continue;

		preserved_set = gword_set_add(preserved_set, e);
	}

	if (preserved_set)
	{
		/* Preserve the originating gword pointers of the remaining disjunct. */
		for (gword_set *k = kept; NULL != k; k = k->next)
			preserved_set = gword_set_add(preserved_set, k);
		kept = preserved_set;
	}

	return kept;
}

/**
 * Takes the list of disjuncts pointed to by d, eliminates all
 * duplicates, and returns a pointer to a new list.
 */
Disjunct *eliminate_duplicate_disjuncts(Disjunct *dw)
{
	unsigned int count = 0;
	disjunct_dup_table *dt;

	dt = disjunct_dup_table_new(next_power_of_two_up(2 * count_disjuncts(dw)));

	for (Disjunct **dd = &dw; *dd != NULL; /* See: NEXT */)
	{
		Disjunct *dx;
		Disjunct *d = *dd;
		unsigned int h = old_hash_disjunct(dt, d);

		for (dx = dt->dup_table[h]; dx != NULL; dx = dx->dup_table_next)
		{
			if (d->dup_hash != dx->dup_hash) continue;
			if (disjuncts_equal(dx, d)) break;
		}
		if (dx == NULL)
		{
			d->dup_table_next = dt->dup_table[h];
			dt->dup_table[h] = d;
		}
		else
		{
			/* Discard the current disjunct. */
			if (d->cost < dx->cost) dx->cost = d->cost;

			dx->originating_gword =
				gword_set_union(dx->originating_gword, d->originating_gword);

			count++;
			*dd = d->next; /* NEXT - set current disjunct to the next one. */
			continue;
		}
		dd = &d->next; /* NEXT */
	}

	lgdebug(+D_DISJ+(0==count)*1000, "Killed %u duplicates\n", count);

	disjunct_dup_table_delete(dt);
	return dw;
}

/* ============================================================= */

/* Return the stringified disjunct.
 * Be sure to free the string upon return.
 */

static void prt_con(Connector *c, dyn_str * p, char dir)
{
	if (NULL == c) return;
	prt_con (c->next, p, dir);

	if (c->multi)
	{
		append_string(p, "@%s%c ", connector_string(c), dir);
	}
	else
	{
		append_string(p, "%s%c ", connector_string(c), dir);
	}
}

char * print_one_disjunct(Disjunct *dj)
{
	dyn_str *p = dyn_str_new();

	prt_con(dj->left, p, '-');
	prt_con(dj->right, p, '+');

	return dyn_str_take(p);
}

/* ============================================================= */

/**
 * returns the number of connectors in the left lists of the disjuncts.
 */
int left_connector_count(Disjunct * d)
{
	int i=0;
	for (;d!=NULL; d=d->next) {
		for (Connector *c = d->left; c!=NULL; c = c->next) i++;
	}
	return i;
}

int right_connector_count(Disjunct * d)
{
	int i=0;
	for (;d!=NULL; d=d->next) {
	  for (Connector *c = d->right; c!=NULL; c = c->next) i++;
	}
	return i;
}

/* ============================================================= */

/**
 * Record the wordgraph word to which the X-node belongs, in each of its
 * disjuncts.
 */
void word_record_in_disjunct(const Gword * gw, Disjunct * d)
{
	for (;d != NULL; d=d->next) {
		d->originating_gword = (gword_set *)&gw->gword_set_head;
	}
}


/* ================ Pack disjuncts and connectors ============== */
/* Print one connector with all the details.
 * mCnameD<tracon_id>(nearest_word, length_limit)x
 * optional m: "@" for multi (else nothing)
 * Cname: Connector name
 * Optional D: "-" / "+" (if dir != -1)
 * Optional <tracon_id>: tracon_id (if not 0)
 * Optional (nearest_word, length_limit): if both are not 0
 * x: Shallow/deep indication as "s" / "d" (if shallow != -1)
 */
void print_one_connector(Connector * e, int dir, int shallow)
{
	printf("%s%s", e->multi ? "@" : "", connector_string(e));
	if (-1 != dir) printf("%c", "-+"[dir]);
	if (e->tracon_id)
	{
		if (e->refcount)
			printf("<%d,%d>", e->tracon_id, e->refcount);
		else
			printf("<%d>", e->tracon_id);
	}
	if ((0 != e->nearest_word) || (0 != e->length_limit))
		printf("(%d,%d)", e->nearest_word, e->length_limit);
	if (-1 != shallow)
		printf("%c", (0 == shallow) ? 'd' : 's');
}

void print_connector_list(Connector * e)
{
	for (;e != NULL; e=e->next)
	{
		print_one_connector(e, /*dir*/-1, /*shallow*/-1);
		if (e->next != NULL) printf(" ");
	}
}

void print_disjunct_list(Disjunct * dj)
{
	int i = 0;
	char word[MAX_WORD + 32];

	for (;dj != NULL; dj=dj->next)
	{
		lg_strlcpy(word, dj->word_string, sizeof(word));
		patch_subscript_mark(word);
		printf("%16s: ", word);
		printf("[%d](%4.2f) ", i++, dj->cost);
		print_connector_list(dj->left);
		printf(" <--> ");
		print_connector_list(dj->right);
		printf("\n");
	}
}

void print_all_disjuncts(Sentence sent)
{
		for (WordIdx w = 0; w < sent->length; w++)
		{
			printf("Word %zu:\n", w);
			print_disjunct_list(sent->word[w].d);
		}
}


/* ============= Connector encoding, sharing and packing ============= */

/*
 * sentence_pack() copies the disjuncts and connectors to a continuous
 * memory block. This facilitate a better memory caching for long
 * sentences.
 *
 * In addition, it shares the memory of identical trailing connector
 * sequences, aka "tracons". Tracons are considered identical if they
 * belong to the same Gword (or same word for the pruning step) and
 * contain identical connectors in the same order (with one exception:
 * shallow connectors must have the same nearest_word as tracon leading
 * deep connectors). Connectors are considered identical if they have
 * the same string representation (including "multi" and the direction
 * mark) with an additional requirement if the packing is done for the
 * pruning step - shallow and deep connectors are then not considered
 * identical. In both cases the exception regarding shallow connectors
 * is because shallow connectors can match any connector, while deep
 * connectors can match only shallow connectors. Note: For efficiency,
 * the actual connector string representation is not used for connector
 * comparison.
 *
 * For the parsing step, identical tracons are assigned a unique tracon
 * ID, which is kept in their first connector. The rest of their
 * connectors also have tracon IDs, which belong to tracons starting
 * with that connectors.
 *
 * For the pruning step, all the tracon IDs are set to 0 (see below for
 * how tracon_id is used during the power pruning).
 *
 * For the pruning step, more things are done:
 * Additional data structure - a tracon list - is constructed, which
 * includes a tracon table and per-word prune table sizes. These data
 * structure consists of 2 identical parts - one for each tracon
 * direction (left/right). The tracon table is indexed by (tracon_id -
 * 1), and it indexes the connectors memory block (it doesn't use
 * pointers in order to save memory on 64-bit CPUs because it may
 * contain in the order of 100K entries for very long sentences).
 * Also, a refcount field is set for each tracon to tell how many
 * tracons are memory-shared at that connector address.
 *
 * Tracons are used differently in the pruning and parsing steps.
 *
 * Power Pruning:
 * The first connector of each tracon is inserted into the power table,
 * along with its reference count. When a connector cannot be matched,
 * this means that all the disjuncts that contain its tracon also cannot
 * be matched. When it is marked as bad (cannot match) or good (match
 * found), due to the tracon memory sharing all the connectors that
 * share the same memory are marked simultaneously, and thus are
 * detected when the next disjuncts are examined without a need to
 * further process them. This saves much processing and also drastically
 * reduces the "power_cost". Setting the nearest_word field is hence
 * done only once per tracon on each pass. The tracon IDs are used to
 * detect already-processed tracons - they are assigned the pass number
 * so each tracon is processed only once per pass. The connector
 * refcount field is used to discard connectors from the power table
 * when all the disjuncts that contain them are discarded.
 *
 * PP pruning:
 * Here too only the first connector in each tracon needs to be
 * examined. Marking a connector with BAD_WORD simultaneously leaves
 * a mark in the corresponding connector in the cms table and in all
 * the disjuncts that share it.
 *
 * Parsing:
 * Originally, the classic parser memoized the number of possible
 * linkages per a given connector-pair using connector addresses. Since
 * an exhaustive search is done, such an approach has two main problems
 * for long sentences:
 * 1. A very big count hash table (Table_connector in count.c) is used
 * due to the huge number of connectors (100Ks) in long sentences, a
 * thing that causes a severe CPU cache trash (to the degree that
 * absolutely most of the memory accesses are L3 misses).
 * 2. Repeated linkage detailed calculation for what seems identical
 * connectors. A hint for the tracon idea was the use of 0 hash values
 * for NULL connectors, which is the same for all the disjuncts of the
 * same word (they can be considered a private case of a tracon - a
 * "null tracon").
 *
 * The idea that is implemented here is based on the fact that the
 * number of linkages between the same words using any of their
 * connector-pair endpoints is governed only by these connectors and the
 * connectors after them (since cross links are not permitted). Using
 * tracon IDs as the hash keys allows to share the memoizing table
 * counts between connectors that start the same tracons. As a
 * result, long sentences have significantly less different connector
 * hash values than their total number of connectors.
 *
 * In order to save the need to cache and check the endpoint word
 * numbers the tracon IDs should not be shared between words. They also
 * should not be shared between alternatives since connectors that belong
 * to disjuncts of different alternatives may have different linkage
 * counts because some alternatives-connectivity checks (to the middle
 * disjunct) are done in the fast-matcher. These restrictions are
 * implemented by using different tracon IDs per Gword.
 *
 * The tracon memory sharing is currently not directly used in the
 * parsing algo besides reducing the needed CPU cache by a large factor.
 *
 * Algo of generating tracon Ids, shared tracons and the tracon list:
 * The string-set code has been adapted (see tracon-set.c) to hash
 * tracons. The tracon-set hash table slots are Connector pointers which
 * point to the memory block of the sentence connectors. When a tracon
 * is not found in the hash table, a new tracon ID is assigned to it,
 * and the tracon is copied to the said connector memory block. However,
 * if it is found, its address is used instead of copying the
 * connectors, thus sharing its memory with identical tracons. The
 * tracon-set hash table is cleared after each word (for pruning tracons)
 * or Gword (for parsing tracons), thus ensuring that the tracons IDs are
 * not shared between words (or Gwords).
 *
 * Some tracon features:
 * - Each connector starts some tracon.
 * - Connectors of identical tracons share their memory.
 *
 * Jets:
 * A jet is a (whole) ordered set of connectors all pointing in the same
 * direction (left, or right). Every disjunct can be split into two jets;
 * that is, a disjunct is a pair of jets, and so each word consists of a
 * collection of pairs of jets. The first connector in a jet called
 * a "shallow" connector. Connectors that are not shallow are deep.
 * See the comments in prune.c for their connection properties.
 * A jet is also a tracon.
 *
 * Note: This comment is referred-to in disjunct-utils.h, so changes
 * here may need to be reflected in the comments there too.
 */

static void id_table_check(Tracon_list *tl, unsigned int index, int dir)
{

	if (index >= tl->table_size[dir])
	{
		size_t new_id_table_size = (0 == tl->table_size[dir]) ?
			index : tl->table_size[dir] * 2;
		size_t old_bytes = tl->table_size[dir] * sizeof(uint32_t *);
		size_t new_bytes = new_id_table_size * sizeof(uint32_t *);

		tl->table[dir] = realloc(tl->table[dir], new_bytes);
		memset((char *)tl->table[dir] + old_bytes, 0, new_bytes - old_bytes);
		tl->table_size[dir] = new_id_table_size;
	}
}

/**
 * Pack the connectors in an array; memory-share and enumerate tracons.
 */
static Connector *pack_connectors(Tracon_sharing *ts, Connector *origc, int dir,
                                  int w)
{
	if (NULL == origc) return NULL;

	Connector head;
	Connector *prevc = &head;
	Connector *newc = &head;
	Connector *lcblock = ts->cblock; /* For convenience. */
	Tracon_list *tl = ts->tracon_list;

	for (Connector *o = origc; NULL != o;  o = o->next)
	{
		newc = NULL;

		if (NULL != ts)
		{
			/* Encoding is used - share trailing connector sequences. */
			Connector **tracon = tracon_set_add(o, ts->csid[dir]);

			if (NULL == *tracon)
			{
				/* The first time we encounter this connector sequence. */
				*tracon = lcblock; /* Save its future location in the tracon_set. */

				if (NULL != tl)
				{
					id_table_check(tl, tl->entries[dir], dir);
					uint32_t cblock_index = (uint32_t)(lcblock - ts->cblock_base);
					tl->table[dir][tl->entries[dir]] = cblock_index;
					tl->entries[dir]++;
				}
			}
			else
			{
				newc = *tracon;
				if (NULL == tl)
				{
					if (o->nearest_word != newc->nearest_word)
					{
						/* This is a rare case in which a shallow and deep
						 * connectors don't have the same nearest_word, because
						 * a shallow connector may mach a deep connector
						 * earlier. Because the nearest word is different, we we
						 * cannot share it. (Such shallow and deep Tracons could
						 * be shared separately, but because this is a rare
						 * event there is no need to do that.)
						 * Note:
						 * In case the parsing ever depends on other Connector
						 * fields, their will be a need to add a check for them
						 * here. */
						newc = NULL; /* Don't share it. */
					}
				}
			}
		}

		if (newc == NULL)
		{
			/* No sharing is done. */
			newc = lcblock++;
			*newc = *o;

			if (NULL == ts->tracon_list)
			{
				/* For the parsing sharing we need a unique ID. */
				newc->tracon_id = ts->next_id[dir]++;
			}
			else
			{
				newc->refcount = 1; /* No sharing yet. */
				newc->tracon_id = 0;
				tl->num_cnctrs_per_word[dir][w]++;
			}
		}
		else
		{
			if (NULL != ts->tracon_list)
			{
				for (Connector *n = newc; NULL != n; n = n->next)
					n->refcount++;
			}
			prevc->next = newc;

			/* Just shared a trailing connector sequence, nothing more to do. */
			ts->cblock = lcblock;
			return head.next;
		}

		prevc->next = newc;
		prevc = newc;
	}
	newc->next = NULL;

	ts->cblock = lcblock;
	return head.next;
}

#define WORD_OFFSET 256 /* Reserved for null connectors. */

/**
 *  Set dummy tracon_id's.
 *  To be used for short sentences.
 */
static int enumerate_connectors_sequentially(Sentence sent)
{
	int id = WORD_OFFSET;

	for (WordIdx w = 0; w < sent->length; w++)
	{
		for (Disjunct *d = sent->word[w].d; NULL != d; d = d->next)
		{
			for (Connector *c = d->left; NULL != c; c = c->next)
			{
				c->tracon_id = id++;
			}
			for (Connector *c = d->right; NULL != c; c = c->next)
			{
				c->tracon_id = id++;
			}
		}
	}

	return id + 1;
}

/**
 * Pack the given disjunct chain in a continuous memory block.
 * If the disjunct is NULL, return NULL.
 */
static Disjunct *pack_disjuncts(Tracon_sharing *ts, Disjunct *origd, int w)
{
	Disjunct head;
	Disjunct *prevd = &head;
	Disjunct *newd = &head;
	Disjunct *ldblock = ts->dblock; /* For convenience. */
	uintptr_t token = (uintptr_t)w;

	for (Disjunct *t = origd; NULL != t; t = t->next)
	{
		newd = ldblock++;
		newd->word_string = t->word_string;
		newd->cost = t->cost;
		newd->originating_gword = t->originating_gword;

		if (NULL == ts->tracon_list)
		    token = (uintptr_t)t->originating_gword;

		if (token != ts->last_token)
		{
			ts->last_token = token;
			tracon_set_reset(ts->csid[0]);
			tracon_set_reset(ts->csid[1]);
			//printf("TOKEN token %ld\n", token);
		}
		newd->left = pack_connectors(ts, t->left, 0, w);
		newd->right = pack_connectors(ts, t->right, 1,  w);

		prevd->next = newd;
		prevd = newd;
	}
	newd->next = NULL;

	ts->dblock = ldblock;
	return head.next;
}

#define ID_TABLE_SZ 8192 /* Initial size of the tracon_id table */

/** Create a context descriptor for disjuncts & connector memory "packing".
 * - Allocate a memory block for all the disjuncts & connectors.
 *   The current Connector struct size is 32 bit, and the intention is
 *   to keep it with a power-of-2 size. The idea is to put an integral
 *   number of connectors in each cache line (assumed to be >= Connector
 *   struct size, e.g. 64 bytes), so one connector will not need 2 cache
 *   lines.
 *
 *    The allocated memory block includes 3 sections , in that order:
 *    1. A block for disjuncts, when it start is not aligned (the
 *    disjunct size is currently 56 bytes and cannot be reduced much).
 *    2. A small alignment gap, that ends in a 64-byte boundary.
 *    3. A block of connectors, which is so aligned to 64-byte boundary.
 *
 * - If the packing is done for the pruning step, allocate Tracon list
 *   stuff too. In that case also call tracon_set_shallow() so Tracons
 *   starting with a shallow connector will be considered different than
 *   similar ones starting with a deep connector.
 *
 * @return The said context descriptor.
 */
static Tracon_sharing *pack_sentence_init(Sentence sent, bool is_pruning)
{
	unsigned int dcnt = 0;
	unsigned int ccnt = 0;
	Tracon_sharing *ts;

	count_disjuncts_and_connectors(sent, &dcnt, &ccnt);

#define CONN_ALIGNMENT sizeof(Connector)
	size_t dsize = dcnt * sizeof(Disjunct);
	dsize = ALIGN(dsize, CONN_ALIGNMENT); /* Align connector block. */
	size_t csize = ccnt * sizeof(Connector);
	void *memblock = malloc(dsize + csize);
	Disjunct *dblock = memblock;
	Connector *cblock = (Connector *)((char *)memblock + dsize);

	ts = malloc(sizeof(Tracon_sharing));
	memset(ts, 0, sizeof(Tracon_sharing));

	ts->memblock = memblock;
	ts->cblock_base = cblock;
	ts->cblock = cblock;
	ts->dblock = dblock;
	ts->num_connectors = ccnt;
	ts->num_disjuncts = dcnt;
	ts->word_offset = is_pruning ? 1 : WORD_OFFSET;
	ts->next_id[0] = ts->next_id[1] = ts->word_offset;
	ts->last_token = (uintptr_t)-1;

	ts->csid[0] = tracon_set_create();
	ts->csid[1] = tracon_set_create();

	if (is_pruning)
	{
		ts->tracon_list = malloc(sizeof(Tracon_list));
		memset(ts->tracon_list, 0, sizeof(Tracon_list));
		ts->tracon_list->memblock_sz = dsize + csize;
		unsigned int **ncpw = ts->tracon_list->num_cnctrs_per_word;
		for (int dir = 0; dir < 2; dir++)
		{
			ncpw[dir] = malloc(sent->length * sizeof(**ncpw));
			memset(ncpw[dir], 0, sent->length * sizeof(**ncpw));

			tracon_set_shallow(true, ts->csid[dir]);
			id_table_check(ts->tracon_list, ID_TABLE_SZ, dir); /* Allocate table. */
		}
	}

	return ts;
}

void free_tracon_sharing(Tracon_sharing *ts)
{
	if (NULL == ts) return;

	for (int dir = 0; dir < 2; dir++)
	{
		if (NULL != ts->tracon_list)
		{
			free(ts->tracon_list->num_cnctrs_per_word[dir]);
			free(ts->tracon_list->table[dir]);
		}
		tracon_set_delete(ts->csid[dir]);
		ts->csid[dir] = NULL;
	}
	free(ts->tracon_list);
	ts->tracon_list = NULL;

	free(ts);
}

/**
 * Pack all disjunct and connectors into a one big memory block.
 * This facilitate a better memory caching for long sentences
 * (a performance gain of a few percents).
 *
 * The current Connector struct size is 32 bit, and future ones may be
 * smaller, but still with a power-of-2 size.
 * The idea is to put an integral number of connectors in each cache line
 * (assumed to be >= Connector struct size, e.g. 64 bytes),
 * so one connector will not need 2 cache lines.
 *
 * The allocated memory includes 3 sections , in that order:
 * 1. A block for disjuncts, when it start is not aligned (the disjunct size
 * is currently 56 bytes and cannot be reduced much).
 * 2. A small alignment gap, that ends in a 64-byte boundary.
 * 3. A block of connectors, which is so aligned to 64-byte boundary.
 *
 * A trailing sequence encoding and sharing is done too.
 * Note: Connector sharing, trailing hash and packing always go together.
 */
static Tracon_sharing *pack_sentence(Sentence sent, bool is_pruning)
{
	bool do_share = sent->length >= sent->min_len_encoding;
	Tracon_sharing *ts;

	if (!do_share)
	{
		if (!is_pruning)
		{
			lgdebug(D_DISJ, "enumerate_connectors_sequentially\n");
			enumerate_connectors_sequentially(sent);
		}
		return NULL;
	}

	ts = pack_sentence_init(sent, is_pruning);

	for (WordIdx w = 0; w < sent->length; w++)
	{
		sent->word[w].d = pack_disjuncts(ts, sent->word[w].d, w);
	}

	/* On long sentences, many MB of connector-space are saved, but we
	 * cannot use a realloc() here without the overhead of relocating
	 * the pointers in the used part of memblock (if realloc() returns a
	 * different address). */

	return ts;
}

Tracon_sharing *pack_sentence_for_pruning(Sentence sent)
{
	Tracon_sharing *ts = pack_sentence(sent, true);

	lgdebug(D_DISJ, "Debug: Trailing hash for pruning (len %zu): "
	        "tracon_id %zu (%zu+,%zu-), shared connectors %ld\n", sent->length,
	        ts->tracon_list->entries[0]+ts->tracon_list->entries[1],
	        ts->tracon_list->entries[0], ts->tracon_list->entries[1],
	        &ts->cblock_base[ts->num_connectors] - ts->cblock);

	return ts;
}

Tracon_sharing *pack_sentence_for_parsing(Sentence sent)
{
	Tracon_sharing *ts = pack_sentence(sent, false);

	lgdebug(D_DISJ, "Debug: Trailing hash for parsing (len %zu): "
	        "tracon_id %d (%d+,%d-), shared connectors %ld\n", sent->length,
	        (ts->next_id[0]-ts->word_offset)+(ts->next_id[1]-ts->word_offset),
	        ts->next_id[0]-ts->word_offset, ts->next_id[1]-ts->word_offset,
	        &ts->cblock_base[ts->num_connectors] - ts->cblock);

	return ts;
}

/* ============ Save and restore sentence disjuncts ============ */
void *save_disjuncts(Sentence sent, Tracon_sharing *ts, Disjunct **disjuncts)
{
	Tracon_list *tl = ts->tracon_list;

	for (WordIdx w = 0; w < sent->length; w++)
		disjuncts[w] = sent->word[w].d;

	void *saved_memblock = malloc(tl->memblock_sz);
	memcpy(saved_memblock, ts->memblock, ts->tracon_list->memblock_sz);

	return saved_memblock;
}

void restore_disjuncts(Sentence sent, Disjunct **disjuncts,
                       void *saved_memblock, Tracon_sharing *ts)
{
	if (NULL == saved_memblock) return;

	for (WordIdx w = 0; w < sent->length; w++)
		sent->word[w].d = disjuncts[w];

	memcpy(ts->memblock, saved_memblock, ts->tracon_list->memblock_sz);
}
