/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <string.h>

#include "api-structures.h"             // Sentence
#include "connectors.h"
#include "disjunct-utils.h"
#include "print/print-util.h"
#include "utilities.h"
#include "tokenize/tok-structures.h"    // XXX TODO provide gword access methods!
#include "tokenize/word-structures.h"

/* Disjunct utilities ... */

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
	if (NULL != sent->disjuncts_connectors_memblock)
	{
		free(sent->disjuncts_connectors_memblock);
		sent->disjuncts_connectors_memblock = NULL;
	}
	else
	{
		for (WordIdx i = 0; i < sent->length; i++)
		{
			free_disjuncts(sent->word[i].d);
		}
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

/* ============================================================= */

typedef struct disjunct_dup_table_s disjunct_dup_table;
struct disjunct_dup_table_s
{
	size_t dup_table_size;
	Disjunct ** dup_table;
};

/**
 * This is a hash function for disjuncts
 *
 * This is the old version that doesn't check for domination, just
 * equality.
 */
static inline unsigned int old_hash_disjunct(disjunct_dup_table *dt, Disjunct * d)
{
	Connector *e;
	unsigned int i;
	i = 0;
	for (e = d->left ; e != NULL; e = e->next) {
		i = (5 * (i + e->desc->uc_num)) + (unsigned int)e->desc->lc_letters + 7;
	}
	for (e = d->right ; e != NULL; e = e->next) {
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

/**
 * Duplicate the given connector chain.
 * If the argument is NULL, return NULL.
 */
static Connector *connectors_dup(Connector *origc)
{
	Connector head;
	Connector *prevc = &head;
	Connector *newc = &head;
	Connector *t;

	for (t = origc; t != NULL;  t = t->next)
	{
		newc = connector_new(NULL, NULL);
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
Disjunct *disjuncts_dup(Disjunct *origd)
{
	Disjunct head;
	Disjunct *prevd = &head;
	Disjunct *newd = &head;
	Disjunct *t;

	for (t = origd; t != NULL; t = t->next)
	{
		newd = (Disjunct *)xalloc(sizeof(Disjunct));
		newd->word_string = t->word_string;
		newd->cost = t->cost;
		newd->left = connectors_dup(t->left);
		newd->right = connectors_dup(t->right);
		newd->originating_gword = t->originating_gword;
		prevd->next = newd;
		prevd = newd;
	}
	newd->next = NULL;

	return head.next;
}

static disjunct_dup_table * disjunct_dup_table_new(size_t sz)
{
	size_t i;
	disjunct_dup_table *dt;
	dt = (disjunct_dup_table *) xalloc(sizeof(disjunct_dup_table));

	dt->dup_table_size = sz;
	dt->dup_table = (Disjunct **) xalloc(sz * sizeof(Disjunct *));

	for (i=0; i<sz; i++) dt->dup_table[i] = NULL;

	return dt;
}

static void disjunct_dup_table_delete(disjunct_dup_table *dt)
{
	xfree(dt->dup_table, dt->dup_table_size * sizeof(Disjunct *));
	xfree(dt, sizeof(disjunct_dup_table));
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
 * It frees the disjuncts that are eliminated.
 */
Disjunct * eliminate_duplicate_disjuncts(Disjunct * d)
{
	unsigned int i, h, count;
	Disjunct *dn, *dx;
	disjunct_dup_table *dt;

	count = 0;
	dt = disjunct_dup_table_new(next_power_of_two_up(2 * count_disjuncts(d)));

	while (d != NULL)
	{
		dn = d->next;
		h = old_hash_disjunct(dt, d);

		for (dx = dt->dup_table[h]; dx != NULL; dx = dx->next)
		{
			if (d->dup_hash != dx->dup_hash) continue;
			if (disjuncts_equal(dx, d)) break;
		}
		if (dx == NULL)
		{
			d->next = dt->dup_table[h];
			dt->dup_table[h] = d;
		}
		else
		{
			d->next = NULL;  /* to prevent it from freeing the whole list */
			if (d->cost < dx->cost) dx->cost = d->cost;

			dx->originating_gword =
				gword_set_union(dx->originating_gword, d->originating_gword);

			free_disjuncts(d);
			count++;
		}
		d = dn;
	}

	/* d is already null */
	for (i=0; i < dt->dup_table_size; i++)
	{
		for (dn = dt->dup_table[i]; dn != NULL; dn = dx) {
			dx = dn->next;
			dn->next = d;
			d = dn;
		}
	}

	lgdebug(+5+(0==count)*1000, "Killed %u duplicates\n", count);

	disjunct_dup_table_delete(dt);
	return d;
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
	Connector *c;
	int i=0;
	for (;d!=NULL; d=d->next) {
		for (c = d->left; c!=NULL; c = c->next) i++;
	}
	return i;
}

int right_connector_count(Disjunct * d)
{
	Connector *c;
	int i=0;
	for (;d!=NULL; d=d->next) {
	  for (c = d->right; c!=NULL; c = c->next) i++;
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
static Connector *pack_connectors_dup(Connector *origc, Connector **cblock)
{
	Connector head;
	Connector *prevc = &head;
	Connector *newc = &head;
	Connector *t;
	Connector *lcblock = *cblock; /* Optimization. */

	for (t = origc; t != NULL;  t = t->next)
	{
		newc = lcblock++;
		*newc = *t;

		prevc->next = newc;
		prevc = newc;
	}
	newc->next = NULL;

	*cblock = lcblock;
	return head.next;
}

/**
 * Duplicate the given disjunct chain.
 * If the argument is NULL, return NULL.
 */
static Disjunct *pack_disjuncts_dup(Disjunct *origd, Disjunct **dblock, Connector **cblock)
{
	Disjunct head;
	Disjunct *prevd = &head;
	Disjunct *newd = &head;
	Disjunct *t;
	Disjunct *ldblock = *dblock; /* Optimization. */

	for (t = origd; t != NULL; t = t->next)
	{
		newd = ldblock++;
		newd->word_string = t->word_string;
		newd->cost = t->cost;

		newd->left = pack_connectors_dup(t->left, cblock);
		newd->right = pack_connectors_dup(t->right, cblock);
		newd->originating_gword = t->originating_gword;
		prevd->next = newd;
		prevd = newd;
	}
	newd->next = NULL;

	*dblock = ldblock;
	return head.next;
}

#define SHORTEST_SENTENCE_TO_PACK 9

/**
 * Pack all disjunct and connectors into one big memory block.
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
 * FIXME: 1. Find the "best" value for SHORTEST_SENTENCE_TO_PACK.
 * 2. Maybe this check should be done in too stages, the second one
 * will use number of disjunct and connector thresholds.
 */
void pack_sentence(Sentence sent)
{
	int dcnt = 0;
	int ccnt = 0;

	if (sent->length < SHORTEST_SENTENCE_TO_PACK) return;
	for (size_t w = 0; w < sent->length; w++)
	{
		Disjunct *d;

		for (d = sent->word[w].d; NULL != d; d = d->next)
		{
			dcnt++;
			for (Connector *c = d->right; c!=NULL; c = c->next) ccnt++;
			for (Connector *c = d->left; c != NULL; c = c->next) ccnt++;
		}
	}

#define CONN_ALIGNMENT sizeof(Connector)
	size_t dsize = dcnt * sizeof(Disjunct);
	dsize = ALIGN(dsize, CONN_ALIGNMENT); /* Align connector block. */
	size_t csize = ccnt * sizeof(Connector);
	void *memblock = malloc(dsize + csize);
	Disjunct *dblock = memblock;
	Connector *cblock = (Connector *)((char *)memblock + dsize);
	sent->disjuncts_connectors_memblock = memblock;

	for (size_t i = 0; i < sent->length; i++)
	{
		Disjunct *word_disjuncts = sent->word[i].d;

		sent->word[i].d = pack_disjuncts_dup(sent->word[i].d, &dblock, &cblock);
		free_disjuncts(word_disjuncts);
	}
}
/* ========================= END OF FILE ========================*/
