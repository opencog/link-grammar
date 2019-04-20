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

/** Returns the number of disjuncts and connectors in the sentence. */
static void count_disjuncts_and_connectors(Sentence sent, int *dca, int *cca)
{
	int ccnt = 0, dcnt = 0;

	for (WordIdx w = 0; w < sent->length; w++)
	{
		for (Disjunct *d = sent->word[w].d; d != NULL; d = d->next)
		{
			dcnt++;
			for (Connector *c = d->right; c !=NULL; c = c->next) ccnt++;
			for (Connector *c = d->left; c != NULL; c = c->next) ccnt++;
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
 * mCnameD<suffix_id>(nearest_word, length_limit)x
 * optional m: "@" for multi (else nothing)
 * Cname: Connector name
 * Optional D: "-" / "+" (if dir != -1)
 * Optional <suffix>: suffix_id (if not 0)
 * Optional (nearest_word, length_limit): if both are not 0
 * x: Shallow/deep indication as "s" / "d" (if shallow != -1)
 */
void print_one_connector(Connector * e, int dir, int shallow)
{
	printf("%s%s", e->multi ? "@" : "", connector_string(e));
	if (-1 != dir) printf("%c", "-+"[dir]);
	if (e->suffix_id)
		printf("<%d>", e->suffix_id);
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

typedef struct
{
	Connector *cblock_base;
	Connector *cblock;
	Disjunct *dblock;
	String_id *csid;             /* Connector suffix encoding. */
	/* Table of seen sequences. A 32bit index (instead of Connector *) is
	 * used for better use of the CPU cache. */
	uint32_t *id_table;
	size_t id_table_size;
	size_t num_id;
} pack_context;

static void id_table_check(pack_context *pc, unsigned int id_index)
{

	if (id_index >= pc->id_table_size)
	{
		size_t new_id_table_size = (0 == pc->id_table_size) ?
			id_index : pc->id_table_size * 2;
		size_t old_bytes = pc->id_table_size * sizeof(uint32_t *);
		size_t new_bytes = new_id_table_size * sizeof(uint32_t *);

		pc->id_table = realloc(pc->id_table, new_bytes);
		memset((char *)pc->id_table + old_bytes, 0, new_bytes - old_bytes);
		pc->id_table_size = new_id_table_size;
	}
}

/**
 * Pack the connectors in consecutive memory locations.
 * Use trailing sequence sharing if trailing connector encoding is used
 * (indicated by the existence of its String_id). Same sequences
 * with different directions are not shared, to enable latter modifications
 * in sequences of one direction without affecting the other direction
 * (not implemented yet in published stuff).
 */
static Connector *pack_connectors(pack_context *pc, Connector *origc, int dir)
{
	Connector head;
	Connector *prevc = &head;
	Connector *newc = &head;
	Connector *lcblock = pc->cblock; /* For convenience. */

	for (Connector *t = origc; NULL != t;  t = t->next)
	{
		newc = NULL;

		if (NULL != pc->csid)
		{
			/* Encoding is used - share trailing connector sequences. */
			unsigned int id_index = (2 * t->suffix_id) + dir;
			id_table_check(pc, id_index);
			uint32_t cblock_index = pc->id_table[id_index];

			if (0 == cblock_index)
			{
				/* The first time we encounter this connector sequence.
				 * It will be copied to this cached location (below). */
				cblock_index = (int)(lcblock - pc->cblock_base);
				pc->id_table[id_index] = cblock_index;
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
					cblock_index = (int)(lcblock - pc->cblock_base);
					pc->id_table[id_index] = cblock_index;
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
					for (Connector *c = t->next; NULL != c; c = c->next)
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
 * Convert integer to ASCII into the given buffer.
 * Use base 64 for compactness.
 * The following characters shouldn't appear in the result:
 * CONSEP, ",", and ".", because they are reserved for separators.
 * Return the number of characters in the buffer (not including the '\0').
 */
#define PTOA_BASE 64
static unsigned int ptoa_compact(char* buffer, uintptr_t ptr)
{
	char *p = buffer;
	do
	{
	  *p++ = '0' + (ptr % PTOA_BASE);
	  ptr /= PTOA_BASE;
	}
	 while (ptr > 0);

	*p = '\0';

	return (unsigned int)(p - buffer);
}

#define WORD_OFFSET 256 /* Reserved for null connectors. */

/**
 *  Set dummy suffix_id's.
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
				c->suffix_id = id++;
			}
			for (Connector *c = d->right; NULL != c; c = c->next)
			{
				c->suffix_id = id++;
			}
		}
	}

	return id + 1;
}

#define CONSEP '&'      /* Connector string separator in the suffix sequence. */
#define MAX_LINK_NAME_LENGTH 10 // XXX Use a global definition
#define MAX_LINKS 20            // XXX Use a global definition

/**
 * Set a hash identifier per connector according to the trailing sequence
 * it starts. Ensure it is unique per word and alternative.
 *
 * Originally, the classic parser memoized the number of possible linkages
 * per a given connector-pair using connector addresses. Since an
 * exhaustive search is done, such an approach has two main problems for
 * long sentences:
 * 1. A very big hash table (Table_connector in count.c) is used due to
 * the huge number of connectors (100Ks) in long sentences, a thing that
 * causes a severe memory cache trash (to the degree that absolutely most
 * of the memory accesses are L3 misses).
 * 2. Repeated linkage detailed calculation for what seemed identical
 * connectors. A hint for this solution was the use of 0 hash values for
 * NULL connectors.
 *
 * The idea that is implemented here is based on the fact that the number
 * of linkages between the same words using any of their connector-pair
 * endpoints is governed only by these connectors and the connectors after
 * them (since cross links are not permitted). This allows us to share
 * the memoizing table hash value between connectors that have the same
 * "connector sequence suffix" (trailing connectors) on their disjuncts.
 * As a result, long sentences have significantly less different connector
 * hash values than their total number of connectors.
 *
 * Algorithm:
 * The string-set code has been adapted (see string-id.c) to generate
 * a unique identifier per string.
 * All the connector sequence suffixes of each disjunct are generated here
 * as strings, which are used for getting a unique identifier for the
 * starting connector of each such sequence.
 * In order to save the need to cache and check the endpoint word numbers
 * the connector identifiers should not be shared between words. We also
 * should consider the case of alternatives - trailing connector sequences
 * that belong to disjuncts of different alternatives may have different
 * linkage counts because some alternatives-connectivity checks (to the
 * middle disjunct) are done in the fast-matcher.
 * Prepending the gword_set encoding solves both of these requirements.
 */
static void enumerate_connector_suffixes(pack_context *pc, Disjunct *d)
{
	if (pc->csid == NULL) return;

#define MAX_GWORD_ENCODING 32   /* Actually up to 12 characters. */
	char cstr[((MAX_LINK_NAME_LENGTH + 3) * MAX_LINKS) + MAX_GWORD_ENCODING];

	/* Generate a string with a disjunct Gword encoding. It makes
	 * unique trailing connector sequences of different words and
	 * alternatives of a word, so they will get their own suffix_id.
	 */
	size_t lg = ptoa_compact(cstr, (uintptr_t)d->originating_gword);
	cstr[lg++] = ',';

	for (int dir = 0; dir < 2; dir ++)
	{
		//printf("Word %zu d=%p dir %s\n", w, d, dir?"RIGHT":"LEFT");

		Connector *first_c = (0 == dir) ? d->left : d->right;
		if (first_c == NULL) continue;

		Connector *cstack[MAX_LINKS];
		size_t ci = 0;
		size_t l = lg;

		//print_connector_list(d->word_string, dir?"RIGHT":"LEFT", first_c);
		for (Connector *c = first_c; NULL != c; c = c->next)
			cstack[ci++] = c;

		for (Connector **cp = &cstack[--ci]; cp >= &cstack[0]; cp--)
		{
			if ((*cp)->multi)
				cstr[l++] = '@'; /* May have different linkages. */
			l += lg_strlcpy(cstr+l, connector_string(*cp), sizeof(cstr)-l);

			if (l > sizeof(cstr)-2) /* Leave room for CONSEP */
			{
				/* This is improbable, given the big cstr buffer. */
				prt_error("Warning: enumerate_connector_suffixes(): "
				          "Buffer overflow.\nParsing may be wrong.\n");
			}

			int id = string_id_add(cstr, pc->csid) + WORD_OFFSET;
			(*cp)->suffix_id = id;
			//printf("ID %d trail=%s\n", id, cstr);

			if (cp != &cstack[0]) /* string_id_add() efficiency. */
				cstr[l++] = CONSEP;
		}
	}
}

/**
 * Pack the given disjunct chain in a continuous memory block.
 * If the disjunct is NULL, return NULL.
 */
static Disjunct *pack_disjuncts(pack_context *pc, Disjunct *origd)
{
	Disjunct head;
	Disjunct *prevd = &head;
	Disjunct *newd = &head;
	Disjunct *ldblock = pc->dblock; /* For convenience. */

	for (Disjunct *t = origd; NULL != t; t = t->next)
	{
		newd = ldblock++;
		newd->word_string = t->word_string;
		newd->cost = t->cost;
		newd->originating_gword = t->originating_gword;

		enumerate_connector_suffixes(pc, t);
		newd->left = pack_connectors(pc, t->left, 0);
		newd->right = pack_connectors(pc, t->right, 1);

		prevd->next = newd;
		prevd = newd;
	}
	newd->next = NULL;

	pc->dblock = ldblock;
	return head.next;
}

#define ID_TABLE_SZ 8192 /* Initial size of the suffix_id table */
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
bool pack_sentence(Sentence sent)
{
	int dcnt = 0;
	int ccnt = 0;
	bool do_share = sent->length >= sent->min_len_sharing;

	if (!do_share)
	{
		lgdebug(D_DISJ, "enumerate_connectors_sequentially\n");
		enumerate_connectors_sequentially(sent);
		return false;
	}

	count_disjuncts_and_connectors(sent, &dcnt, &ccnt);

#define CONN_ALIGNMENT sizeof(Connector)
	size_t dsize = dcnt * sizeof(Disjunct);
	dsize = ALIGN(dsize, CONN_ALIGNMENT); /* Align connector block. */
	size_t csize = ccnt * sizeof(Connector);
	void *memblock = malloc(dsize + csize);
	Disjunct *dblock = memblock;
	Connector *cblock = (Connector *)((char *)memblock + dsize);
	sent->disjuncts_connectors_memblock = memblock;
	pack_context pc =
	{
		.cblock_base = cblock,
		.cblock = cblock,
		.dblock = dblock,
		.csid = NULL,
	};

	if (do_share)
	{
		if (sent->connector_suffix_id == NULL)
			sent->connector_suffix_id = string_id_create();
		pc.csid = sent->connector_suffix_id;
		id_table_check(&pc, ID_TABLE_SZ); /* Allocate initial table. */
		pc.num_id = 0;
	}

	for (WordIdx i = 0; i < sent->length; i++)
		sent->word[i].d = pack_disjuncts(&pc, sent->word[i].d);

	pool_delete(sent->Disjunct_pool);
	pool_delete(sent->Connector_pool);
	sent->Disjunct_pool = NULL;

	if (do_share)
	{
		free(pc.id_table);
		lgdebug(+D_DISJ, "Info: %zu connectors shared\n", &cblock[ccnt] - pc.cblock);
		/* On long sentences, many MB of connector-space are saved, but we
		 * cannot use a realloc() here without the overhead of relocating
		 * the pointers in the used part of memblock (if realloc() returns a
		 * different address). */

		/* Support incremental suffix_id generation (only one time is needed). */
		const char *snumid[] = { "NUMID", "NUMID1" };

		int t = string_id_lookup(snumid[0], pc.csid);
		int numid = string_id_add(snumid[(int)(t > 0)], pc.csid) + WORD_OFFSET;
		lgdebug(D_DISJ, "Debug: Using trailing hash (length %zu): suffix_id %d\n",
				  sent->length, numid);
	}

	return do_share;
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

/* =================== Disjunct jet sharing ================= */
static void jet_sharing_init(Sentence sent)
{
	unsigned int **ccnt = sent->jet_sharing.num_cnctrs_per_word;

	for (int dir = 0; dir < 2; dir++)
		ccnt[dir] = malloc(sent->length * sizeof(**ccnt));
}

void free_jet_sharing(Sentence sent)
{
	jet_sharing_t *js = &sent->jet_sharing;
	if (NULL == js->table[0]) return;

	for (int dir = 0; dir < 2; dir++)
	{
		free(js->table[dir]);
		js->table[dir] = NULL;
		free(js->num_cnctrs_per_word[dir]);
		string_id_delete(js->csid[dir]);
		js->csid[dir] = NULL;
	}
}

/** Create a copy of the given connector chain and return it;
 *  Put the number of copied connectors in numc.
 */
static Connector *connectors_copy(Pool_desc *connector_pool, Connector *c,
                                  unsigned int *numc)
{
	Connector head;
	Connector *prev = &head;

	for (; NULL != c; c = c->next)
	{
		Connector *n = pool_alloc(connector_pool);
		*n = *c;
		prev->next = n;
		prev = n;
		(*numc)++;
	}

	prev->next = NULL;
	return head.next;
}

#define JET_TABLE_SIZE 256 /* Auto-extended. */

/** Memory-share identical disjunct jets.
 * Two jets are considered identical if they are on the same side of the
 * disjunct and their entire connector sequence is identical. A basic
 * assumption, which is not asserted here, is that the nearest_word and
 * length limit fields in the identical jets are also the same.
 *
 * For each disjunct side separately, a unique ID per jet (which starts
 * from 1 so 0 is an invalid ID) is used to identify it. The IDs are
 * shared by all the words for efficiency. The suffix_id field of the
 * shallow connector is used as a counter of the number of shared jets
 * in a particular table slot.
 *
 * Possible FIXME: The shared jets uses a new memory pool that is
 * allocated here.  The intention was to increase the memory locality for
 * power_prune() (which come next) by avoiding memory "holes" that would
 * otherwise be created by the sharing. A later test proves this doesn't
 * matter much for sentences as big as 170 tokens. So this can be
 * simplified, or be used to prevent the need to save/restore actual
 * memory for one-step-parse (with the cost of a more complex memory
 * management logic).
 *
 * The 'rebuild' flag is for use in the optional second stage of a
 * one-step-parse, to save CPU on creating the jet tables. This is done by
 * reusing the String_id descriptors and the jet tables memory allocation.
 * (Creating the jet tables again is currently needed because the jet
 * addresses are currently getting changed when they are restored in this
 * second stage. See the above FIXME.)
 *
 * Note: The number of different jets per word is kept in
 * num_cnctrs_per_word[dir][w], for the use of power_prune() (sizing its
 * per-word tables aka power_table). It is used as an approximation to the
 * number of different connectors. Because this may not be accurate, it is
 * multiplied by 2 to be more on the safe side.
 */
void share_disjunct_jets(Sentence sent, bool rebuild)
{
	jet_sharing_t *js = &sent->jet_sharing;

	lgdebug(+D_DISJ, "skip=%d rebuild=%d table=%d\n",
	        sent->length < sent->min_len_sharing, rebuild, NULL != js->table[0]);

	if (sent->length < sent->min_len_sharing) return;

	size_t jet_table_size[2];
	size_t jet_table_entries[2] = {0};
	JT_entry **jet_table = js->table;

	assert(!rebuild || (js->table[0] && js->csid[0]), "jet rebuild with no info");
	assert(rebuild || (!js->table[0] && !js->csid[0]), "jet !rebuild with info");

	if (!rebuild)
		jet_sharing_init(sent);

	/* FIXME? Add to jet_sharing_init. */
	for (int dir = 0; dir < 2; dir++)
	{
		if (NULL == js->csid[dir])
		{
			js->csid[dir] = string_id_create();
			(void)string_id_add("dummy", js->csid[dir]); /* IDs start from 1. */
		}
		if (rebuild)
		{
			jet_table_size[dir] = js->entries[dir] + 1;
			memset(jet_table[dir], 0, jet_table_size[dir] * sizeof(*jet_table[0]));
		}
		else
		{
			jet_table_size[dir] = JET_TABLE_SIZE;
			jet_table[dir] = calloc(jet_table_size[dir], sizeof(*jet_table[0]));
		}
	}

	int ccnt, dcnt;
	count_disjuncts_and_connectors(sent, &ccnt, &dcnt);

	/* +1: Avoid allocating 0 elements. */
	ccnt = MAX(ccnt, 1);
	dcnt = MAX(dcnt, 1);

	Pool_desc *old_Disjunct_pool = sent->Disjunct_pool;
	sent->Disjunct_pool = pool_new(__func__, "Disjunct",
	                   /*num_elements*/dcnt, sizeof(Disjunct),
	                   /*zero_out*/false, /*align*/false, /*exact*/true);
	Pool_desc *old_Connector_pool = sent->Connector_pool;
	sent->Connector_pool = pool_new(__func__, "Connector",
	                   /*num_elements*/ccnt, sizeof(Connector),
	                   /*zero_out*/false, /*align*/false, /*exact*/true);

	for (WordIdx w = 0; w < sent->length; w++)
	{
		Disjunct head;
		Disjunct *prev = &head;
		unsigned int numc[2] = {0}; /* Current word different connectors. */

		for (Disjunct *d = sent->word[w].d; d != NULL; d = d->next)
		{
			Disjunct *n = pool_alloc(sent->Disjunct_pool);
			*n = *d; /* Also initial left/right to NULL ("continue" below). */
			prev->next = n;
			prev = n;

			char cstr[((MAX_LINK_NAME_LENGTH + 3) * MAX_LINKS)];
			cstr[0] = (char)(w + 1); /* Avoid '\0'. */

			for (int dir = 0; dir < 2; dir ++)
			{
				size_t l = 1;

				Connector *first_c = (0 == dir) ? d->left : d->right;
				if (NULL == first_c) continue;

				for (Connector *c = first_c; NULL != c; c = c->next)
				{
					if (c->multi)
						cstr[l++] = '@'; /* Why does this matter for power pruning? */
					l += lg_strlcpy(cstr+l, connector_string(c), sizeof(cstr)-l);
					if (l > sizeof(cstr)-3) break;  /* Leave room for CONSEP + '@' */
					if (NULL != c->next) /* string_id_add() efficiency. */
						cstr[l++] = CONSEP;
				}
				if (l > sizeof(cstr)-2)
				{
					/* This is improbable, given the big cstr buffer. */
					prt_error("Warning: share_disjunct_jets(): "
					          "Buffer overflow.\nParsing may be wrong.\n");
				}

				if (jet_table_entries[dir] + 1 >= jet_table_size[dir])
				{
					size_t old_bytes = jet_table_size[dir] * sizeof(*jet_table[0]);
					jet_table[dir] = realloc(jet_table[dir], old_bytes * 2);
					memset(jet_table[dir]+jet_table_size[dir], 0, old_bytes);
					jet_table_size[dir] *= 2;
				}

				int id = string_id_add(cstr, js->csid[dir]);
#if 0
				printf("%zu%c %d: %s\n", w, dir?'+':'-', id, cstr);
#endif

				if (NULL == jet_table[dir][id].c)
				{
					jet_table_entries[dir]++;
					jet_table[dir][id].c =
						connectors_copy(sent->Connector_pool, first_c, &numc[dir]);
					/* Very subtle - for potential disjunct save
					 * (one-step-parse) that is done after the previous
					 * jet-sharing since it has set non-0 suffix_id. */
					jet_table[dir][id].c->suffix_id = 0;
				}
				*((0 == dir) ? &n->left : &n->right) = jet_table[dir][id].c;
				jet_table[dir][id].c->suffix_id++;
#if 0
				printf("w%zu%c: ", w, dir?'+':'-');
				print_connector_list(first_c);
				printf("\n");
#endif
			}
		}

		prev->next = NULL;
		sent->word[w].d = head.next;

		/* Keep power-prune hash table sizes. */
		js->num_cnctrs_per_word[0][w] = numc[0] * 2;
		js->num_cnctrs_per_word[1][w] = numc[1] * 2;
	}

	pool_delete(old_Disjunct_pool);
	pool_delete(old_Connector_pool);

	for (int dir = 0; dir < 2; dir++)
	{
		js->table[dir] = jet_table[dir];
		js->entries[dir] = (unsigned int)jet_table_entries[dir];
	}
	lgdebug(+D_DISJ, "Total number of jets %d (%d+,%d-)\n",
	        js->entries[0]+js->entries[1], js->entries[0], js->entries[1]);
}
/* ========================= END OF FILE ========================*/
