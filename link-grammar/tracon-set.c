/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright 2019 Amir Plivatsky                                         */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#define D_TRACON_SET 8                 // Debug level for this file

#ifdef DEBUG
#include <inttypes.h>                   // format macros
#endif

#include "const-prime.h"
#include "connectors.h"
#include "tracon-set.h"
#include "utilities.h"

/**
 * This is an adaptation of the string_set module for detecting unique
 * connector trailing sequences (aka tracons).
 *
 * It is used to generate tracon encoding for the parsing and for the
 * pruning steps. Not like string_set, the actual hash values here
 * are external.
 *
 * A tracon is identified by (the address of) its first connector.
 *
 * The API here is similar to that of string_set, with these differences:
 *
 * 1. tracon_set_add() returns a pointer to the hash slot of the tracon if
 * it finds it. Else it returns a pointer to a NULL hash slot, and
 * the caller is expected to assign to it the tracon (its address
 * after it is copied to the destination buffer).
 *
 * 2. A new API tracon_set_shallow() is used to require that tracons
 * which start with a shallow connector will not be considered the same
 * as ones that start with a deep connector. The power pruning algo
 * depends on that.
 *
 * 3. A new API tracon_set_reset() is used to clear the hash table slots
 * (only, their value remains intact).
 */

static unsigned int hash_connectors(const Connector *c, unsigned int shallow)
{
	unsigned int accum = shallow && c->shallow;

	for (; c != NULL; c = c->next)
	{
		accum = (19 * accum) +
		c->desc->uc_num +
		(((unsigned int)c->multi)<<20) +
		(((unsigned int)c->desc->lc_letters)<<22);
	}

	return accum;
}

#if 0
/**
 * @count Expected number of table elements.
 * @return Prime number to use
 *
 * This function was used for shrinking the table in a try to cause less
 * cache/swap trashing if the table temporary grows very big. However, it
 * had a bug, and it is not clear when to shrink the table - shrinking it
 * unnecessarily can cause an overhead of a table growth. Keep for
 * possible reimlementation of a similar idea.
 */
static unsigned int find_prime_for(size_t count)
{
	size_t i;
	for (i = 0; i < MAX_S_PRIMES; i ++)
	   if (count < MAX_TRACON_SET_TABLE_SIZE(s_prime[i])) return i;

	lgdebug(+0, "Warning: %zu: Absurdly big count", count);
	return -1;
}
#endif

void tracon_set_reset(Tracon_set *ss)
{
	memset(ss->table, 0, ss->size * sizeof(clist_slot));
	ss->available_count = MAX_TRACON_SET_TABLE_SIZE(ss->size);
}

Tracon_set *tracon_set_create(void)
{
	Tracon_set *ss = (Tracon_set *) malloc(sizeof(Tracon_set));

	memset(ss, 0, sizeof(Tracon_set));
	// ss->prime_idx = 0;
	ss->size = s_prime[ss->prime_idx];
	ss->mod_func = prime_mod_func[ss->prime_idx];
	ss->table = (clist_slot *) malloc(ss->size * sizeof(clist_slot));
	memset(ss->table, 0, ss->size * sizeof(clist_slot));
	ss->available_count = MAX_TRACON_SET_TABLE_SIZE(ss->size);

	return ss;
}

/**
 * The connectors must be exactly equal.
 */
static bool connector_equal(const Connector *c1, const Connector *c2)
{
	return (c1->desc == c2->desc) && (c1->multi == c2->multi);
}

/** Return TRUE iff the tracon is exactly the same. */
static bool connector_list_equal(const Connector *c1, const Connector *c2)
{
	while ((c1 != NULL) && (c2 != NULL)) {
		if (!connector_equal(c1, c2)) return false;
		c1 = c1->next;
		c2 = c2->next;
	}
	return (c1 == NULL) && (c2 == NULL);
}

#if defined DEBUG || defined TRACON_SET_DEBUG
uint64_t fp_count;
uint64_t coll_count;
static void prt_stat(void)
{
	lgdebug(D_TRACON_SET, "tracon_set: %"PRIu64" accesses, chain %.4f\n",
	        fp_count, 1.*(fp_count+coll_count)/fp_count);
}
#define PRT_STAT(...) __VA_ARGS__
#else
#define PRT_STAT(...)
#endif

static bool place_found(const Connector *c, const clist_slot *slot,
                        unsigned int hash, Tracon_set *ss)
{
	if (slot->clist == NULL) return true;
	if (hash != slot->hash) return false;
	if (!connector_list_equal(slot->clist, c)) return false;
	if (ss->shallow && (slot->clist->shallow != c->shallow)) return false;
	return connector_list_equal(slot->clist, c);
}

/**
 * lookup the given string in the table.  Return an index
 * to the place it is, or the place where it should be.
 */
static unsigned int find_place(const Connector *c, unsigned int h,
                               Tracon_set *ss)
{
	PRT_STAT(if (fp_count == 0) atexit(prt_stat); fp_count++;)
	unsigned int coll_num = 0;
	unsigned int key = ss->mod_func(h);

	/* Quadratic probing. */
	while (!place_found(c, &ss->table[key], h, ss))
	{
		PRT_STAT(coll_count++;)
		key += 2 * ++coll_num - 1;
		if (key >= ss->size) key = ss->mod_func(key);
	}

	return key;
}

static void grow_table(Tracon_set *ss)
{
	Tracon_set old = *ss;

	PRT_STAT(uint64_t fp_count_save = fp_count;)
	ss->prime_idx++;
	ss->size = s_prime[ss->prime_idx];
	ss->mod_func = prime_mod_func[ss->prime_idx];
	ss->table = (clist_slot *)malloc(ss->size * sizeof(clist_slot));
	memset(ss->table, 0, ss->size*sizeof(clist_slot));
	for (size_t i = 0; i < old.size; i++)
	{
		if (old.table[i].clist != NULL)
		{
			unsigned int p = find_place(old.table[i].clist, old.table[i].hash, ss);
			ss->table[p] = old.table[i];
		}
	}
	ss->available_count = MAX_STRING_SET_TABLE_SIZE(ss->size) -
		MAX_STRING_SET_TABLE_SIZE(old.size);

	/* printf("growing from %zu to %zu\n", old.size, ss->size); */
	PRT_STAT(fp_count = fp_count_save);
	free(old.table);
}

void tracon_set_shallow(bool shallow, Tracon_set *ss)
{
	ss->shallow = shallow;
}

Connector **tracon_set_add(Connector *clist, Tracon_set *ss)
{
	assert(clist != NULL, "Can't insert a null list");

	/* We may need to add it to the table. If the table got too big,
	 * first we grow it. */
	if (ss->available_count == 0) grow_table(ss);

	unsigned int h = hash_connectors(clist, ss->shallow);
	unsigned int p = find_place(clist, h, ss);

	if (ss->table[p].clist != NULL)
		return &ss->table[p].clist;

	ss->table[p].hash = h;
	ss->available_count--;

	return &ss->table[p].clist;
}

Connector *tracon_set_lookup(const Connector *clist, Tracon_set *ss)
{
	unsigned int h = hash_connectors(clist, ss->shallow);
	unsigned int p = find_place(clist, h, ss);
	return ss->table[p].clist;
}

void tracon_set_delete(Tracon_set *ss)
{
	if (ss == NULL) return;
	free(ss->table);
	free(ss);
}
