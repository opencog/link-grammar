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

#include "const-prime.h"
#include "connectors.h"
#include "tracon-set.h"
#include "utilities.h"

#ifdef TRACON_SET_DEBUG
#include "disjunct-utils.h"            // print_connector_list_str
#endif

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

static tid_hash_t hash_connectors(const Connector *c, unsigned int shallow)
{
	tid_hash_t accum = (shallow && c->shallow) ? 1000003 : 0;

	return (accum + connector_list_hash(c)) * FIBONACCI_MULT;
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
 * possible reimplementation of a similar idea.
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

#ifdef TRACON_SET_DEBUG
static void tracon_set_print(Tracon_set *ss)
{
	if (test_enabled("tracon-set-print"))
	{
		clist_slot *t;

		printf("tracon_set_print %p:\n", ss);
		for (size_t i = 0; i < ss->size; i++)
		{
			t = &ss->table[i];
			if (0 == t->hash) continue;
			tid_hash_t x = ss->mod_func(t->hash);
			char *cstr = print_connector_list_str(t->clist, 0);
			printf("[%zu]: h %zu pri %u sec %u %c %s\n", i, (size_t)t->hash, t->pri_collN,
			       t->sec_collN, "yn"[i == x], cstr);
			free(cstr);
		}
	}
}

static void tracon_set_stats(Tracon_set *a, Tracon_set *ss, const char *where)
{
	lgdebug(+D_TRACON_SET,
	        "%p: %s: reset %u prime_idx %u acc %zu used %2.2f%% "
	        "coll/acc %.4f chain %.4f\n",
	        a, where, ss->resetN, ss->prime_idx, ss->addN,
	        100.f * ((int)MAX_TRACON_SET_TABLE_SIZE(ss->size) - ss->available_count) / ss->size,
	        1.* ss->pri_collN/ss->addN,
	        1. * (ss->pri_collN + ss->sec_collN) / ss->addN);
}
#else
static void tracon_set_print(Tracon_set *ss){};
static void tracon_set_stats(Tracon_set *a, Tracon_set *ss, const char *where){};
#endif

void tracon_set_reset(Tracon_set *ss)
{
#ifdef TRACON_SET_DEBUG
	ss->resetN++;
#endif
	tracon_set_stats(ss, ss, "reset");
	tracon_set_print(ss);
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

#ifdef TRACON_SET_DEBUG
	lgdebug(+D_TRACON_SET, "%p: prime_idx %u available_count %zu\n",
	        ss, ss->prime_idx, ss->available_count);
#endif

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

static bool place_found(const Connector *c, const clist_slot *slot,
                        tid_hash_t hash, Tracon_set *ss)
{
	if (slot->clist == NULL) return true;
	if (hash != slot->hash) return false;
	if (!connector_list_equal(slot->clist, c)) return false;
	if (ss->shallow && (slot->clist->shallow != c->shallow)) return false;
	return true;
}

/**
 * lookup the given string in the table.  Return an index
 * to the place it is, or the place where it should be.
 */
static tid_hash_t find_place(const Connector *c, tid_hash_t h,
                             Tracon_set *ss)
{
	unsigned int coll_num = 0;
	tid_hash_t key = ss->mod_func(h);

	/* Quadratic probing. */
	while (!place_found(c, &ss->table[key], h, ss))
	{
#ifdef TRACON_SET_DEBUG
		if (0 == coll_num)
		{
			ss->pri_collN++;
			ss->table[key].pri_collN++;
		}
		else
		{
			ss->sec_collN++;
			ss->table[key].sec_collN++;
		}
#endif
		key += 2 * ++coll_num - 1;
		if (key >= ss->size) key = ss->mod_func(key);
	}

	return key;
}

static void grow_table(Tracon_set *ss)
{
	Tracon_set old = *ss;

	tracon_set_stats(ss, &old, "before grow");
	ss->prime_idx++;
	ss->size = s_prime[ss->prime_idx];
	ss->mod_func = prime_mod_func[ss->prime_idx];
	ss->table = (clist_slot *)malloc(ss->size * sizeof(clist_slot));
	memset(ss->table, 0, ss->size*sizeof(clist_slot));
	for (size_t i = 0; i < old.size; i++)
	{
		if (old.table[i].clist != NULL)
		{
			tid_hash_t p = find_place(old.table[i].clist, old.table[i].hash, ss);
			ss->table[p] = old.table[i];
		}
	}
	ss->available_count = MAX_TRACON_SET_TABLE_SIZE(ss->size) -
		MAX_TRACON_SET_TABLE_SIZE(old.size);

	tracon_set_stats(ss, ss, "after grow");
	free(old.table);
}

void tracon_set_shallow(bool shallow, Tracon_set *ss)
{
	ss->shallow = shallow;
}

Connector **tracon_set_add(Connector *clist, Tracon_set *ss)
{
	assert(clist != NULL, "Can't insert a null list");
#ifdef TRACON_SET_DEBUG
	ss->addN++;
#endif

	/* We may need to add it to the table. If the table got too big,
	 * first we grow it. */
	if (ss->available_count == 0) grow_table(ss);

	tid_hash_t h = hash_connectors(clist, ss->shallow);
	tid_hash_t p = find_place(clist, h, ss);

	if (ss->table[p].clist != NULL)
		return &ss->table[p].clist;

	ss->table[p].hash = h;
	ss->available_count--;

	return &ss->table[p].clist;
}

Connector *tracon_set_lookup(const Connector *clist, Tracon_set *ss)
{
	tid_hash_t h = hash_connectors(clist, ss->shallow);
	tid_hash_t p = find_place(clist, h, ss);
	return ss->table[p].clist;
}

void tracon_set_delete(Tracon_set *ss)
{
	if (ss == NULL) return;
	tracon_set_stats(ss, ss, "delete");
	tracon_set_print(ss);
	free(ss->table);
	free(ss);
}
