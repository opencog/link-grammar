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
			if (d->cost < dx->cost) dx->cost = d->cost;

			dx->originating_gword =
				gword_set_union(dx->originating_gword, d->originating_gword);

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
void print_connector_list(Connector * e)
{
	for (;e != NULL; e=e->next)
	{
		printf("%s%s", e->multi ? "@" : "", connector_string(e));
		if (e->suffix_id)
			printf("<%d>", e->suffix_id);
		if ((0 != e->nearest_word) || (0 != e->length_limit))
			printf("(%d,%d)", e->nearest_word, e->length_limit);

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

typedef struct
{
	Connector *cblock_base;
	Connector *cblock;
	Disjunct *dblock;
	String_id *csid;             /* Connector suffix encoding. */
	/* Table of seen sequences. A 32bit index (instead of Connector *) is
	 * used for better use of the CPU cache. */
	uint32_t *suffix_id_table;
} pack_context;

/**
 * Pack the connectors in consecutive memory locations.
 * Use trailing sequence sharing if trailing connector encoding is used
 * (indicated by the existence of suffix_id_table). Same sequences
 * with different directions are not shared, to enable latter modifications
 * in sequences of one direction without affecting the other direction.
 */
static Connector *pack_connectors(pack_context *pc, Connector *origc, int dir)
{
	Connector head;
	Connector *prevc = &head;
	Connector *newc = &head;
	Connector *lcblock = pc->cblock; /* For convenience. */

	for (Connector *t = origc; t != NULL;  t = t->next)
	{
		newc = NULL;

		if (pc->suffix_id_table != NULL)
		{
			uint32_t cblock_index = pc->suffix_id_table[(2 * t->suffix_id) + dir];
			if (cblock_index == 0)
			{
				/* The first time we encounter this connector sequence.
				 * It will be copied to this cached location (below). */
				cblock_index = lcblock - pc->cblock_base;
				pc->suffix_id_table[(2 * t->suffix_id) + dir] = cblock_index;
			}
			else
			{
				newc = &pc->cblock_base[cblock_index];
				if (t->nearest_word != newc->nearest_word)
				{
					/* This is a rare case in which it is not the same, and
					 * hence we cannot use it. The simple "caching" that is used
					 * cannot memoize more than one sequence per suffix_id.
					 * Notes:
					 * 1. Maybe such different connectors should be just assigned
					 * a different suffix_id.
					 * 2. In case the parsing will ever depend on other
					 * Connector fields, their check should be added here. */
					newc = NULL; /* Don't share it. */

					/* Slightly increase cache hit (MRU). */
					cblock_index = lcblock - pc->cblock_base;
					pc->suffix_id_table[(2 * t->suffix_id) + dir] = cblock_index;
				}
#if 1 /* Extra validation - validate the next connectors in the sequence. */
				else
				{
					/* Here we know that the first connector in the current
					 * sequence have the same nearest_word as the cached one.
					 * In that case it doesn't seem possible that the rest will
					 * not have consistent values too.
					 * However, if they don't, then don't share this sequence.
					 * Else the linkages will not be the same. */
					Connector *n = newc;
					for (Connector *c = t->next; c != NULL; c = c->next)
					{
						n = n->next;
						if (t->nearest_word != newc->nearest_word)
						{
							lgdebug(+0, "Warning: Different nearest_word.\n");
							newc = NULL; /* Don't share it. */
							break;
						}
					}
				}
#endif
			}
		}

		if (newc == NULL)
		{
			/* No sharing is done. */
			newc = lcblock++;
			*newc = *t;
		}
		else
		{
#ifdef DEBUG
			/* Connectors of disjuncts with the same suffix_id must
			 * originate from the same Gwords. */
			assert(t->originating_gword == newc->originating_gword);
#endif
			prevc->next = newc;

			/* Just shared a trailing connector sequence, nothing more to do. */
			pc->cblock = lcblock;
			return head.next;
		}

		prevc->next = newc;
		prevc = newc;
	}
	newc->next = NULL;

	pc->cblock = lcblock;
	return head.next;
}
/**
 * Duplicate the given disjunct chain.
 * If the disjunct is NULL, return NULL.
 */
static Disjunct *pack_disjuncts(pack_context *pc, Disjunct *origd)
{
	Disjunct head;
	Disjunct *prevd = &head;
	Disjunct *newd = &head;
	Disjunct *ldblock = pc->dblock; /* For convenience. */

	for (Disjunct *t = origd; t != NULL; t = t->next)
	{
		newd = ldblock++;
		newd->word_string = t->word_string;
		newd->cost = t->cost;
		newd->originating_gword = t->originating_gword;

		newd->left = pack_connectors(pc, t->left, 0);
		newd->right = pack_connectors(pc, t->right, 1);

		prevd->next = newd;
		prevd = newd;
	}
	newd->next = NULL;

	pc->dblock = ldblock;
	return head.next;
}

#define SHORTEST_SENTENCE_TO_PACK 9
#define SHORTEST_SENTENCE_TO_SHARE 25

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
void pack_sentence(Sentence sent, bool real_suffix_ids)
{
	int dcnt = 0;
	int ccnt = 0;
	bool do_share;

	if (!real_suffix_ids && (sent->length < SHORTEST_SENTENCE_TO_PACK)) return;
	do_share = (real_suffix_ids && (sent->length >= SHORTEST_SENTENCE_TO_SHARE));

	for (WordIdx w = 0; w < sent->length; w++)
	{
		for (Disjunct *d = sent->word[w].d; NULL != d; d = d->next)
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
	pack_context pc = {
		.cblock_base = cblock,
		.cblock = cblock,
		.dblock = dblock
	};

	if (do_share)
	{
		pc.suffix_id_table = malloc(2 * sent->num_suffix_id * sizeof(uint32_t *));
		memset(pc.suffix_id_table,0, 2 * sent->num_suffix_id * sizeof(uint32_t *));
	}

	for (WordIdx i = 0; i < sent->length; i++)
		sent->word[i].d = pack_disjuncts(&pc, sent->word[i].d);

	pool_delete(sent->Disjunct_pool);
	pool_delete(sent->Connector_pool);
	sent->Disjunct_pool = NULL;

	if (do_share)
	{
		free(pc.suffix_id_table);
		lgdebug(+5, "Info: %zu connectors shared\n", &cblock[ccnt] - pc.cblock);
		/* On long sentences, many MB of connector-space are saved, but we
		 * cannot use a realloc() here without the overhead of relocating
		 * the pointers in the used part of memblock (if realloc() returns a
		 * different address). */
	}

}

/* ============ Save and restore sentence disjuncts ============ */
void save_disjuncts(Sentence sent, Disjuncts_desc_t *ddesc)
{
	ddesc->disjuncts = malloc(sent->length * sizeof(Disjunct *));

	ddesc->Disjunct_pool = pool_new(__func__, "Disjunct",
	                   /*num_elements*/2048, sizeof(Disjunct),
	                   /*zero_out*/false, /*align*/false, /*exact*/false);
	ddesc->Connector_pool = pool_new(__func__, "Connector",
	                   /*num_elements*/8192, sizeof(Connector),
	                   /*zero_out*/false, /*align*/false, /*exact*/false);

	for (size_t i = 0; i < sent->length; i++)
	{
		ddesc->disjuncts[i] =
			disjuncts_dup(ddesc->Disjunct_pool, ddesc->Connector_pool,
			              sent->word[i].d);
	}
}

void restore_disjuncts(Sentence sent, Disjuncts_desc_t *ddesc)
{
	sent->Disjunct_pool = ddesc->Disjunct_pool;
	ddesc->Disjunct_pool = NULL;

	sent->Connector_pool = ddesc->Connector_pool;

	for (WordIdx w = 0; w < sent->length; w++)
		sent->word[w].d = ddesc->disjuncts[w];
}

void free_saved_disjuncts(Disjuncts_desc_t *ddesc)
{
	if (NULL != ddesc->Disjunct_pool)
	{
		pool_delete(ddesc->Disjunct_pool);
		pool_delete(ddesc->Connector_pool);
	}
	ddesc->Disjunct_pool = NULL;
	free(ddesc->disjuncts);
}
/* ========================= END OF FILE ========================*/
