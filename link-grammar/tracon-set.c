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
		((c->desc->uc_num)<<18) +
		(((unsigned int)c->multi)<<31) +
		(unsigned int)c->desc->lc_letters;
	}

	return accum;
}

static unsigned int find_prime_for(size_t count)
{
	size_t i;
	for (i = 0; i < MAX_S_PRIMES; i ++)
	   if (count < MAX_TRACON_SET_TABLE_SIZE(s_prime[i])) return i;

	assert(0, "find_prime_for(%zu): Absurdly big count", count);
	return 0;
}

void tracon_set_reset(Tracon_set *ss)
{
	size_t ncount = MAX(ss->count, ss->ocount);

	/* Table sizing heuristic: The number of tracons as a function of
	 * word number is usually first increasing and then decreasing.
	 * Continue the trend of the last 2 words. */
	if (ss->count > ss->ocount)
		ncount = ncount * 3 / 4;
	else
		ncount = ncount * 4 / 3;
	unsigned int prime_idx = find_prime_for(ncount);
	if (prime_idx < ss->prime_idx) ss->prime_idx = prime_idx;

	ss->size = s_prime[ss->prime_idx];
	ss->mod_func = prime_mod_func[ss->prime_idx];
	memset(ss->table, 0, ss->size*sizeof(clist_slot));
	ss->ocount = ss->count;
	ss->count = 0;
	ss->available_count = MAX_TRACON_SET_TABLE_SIZE(ss->size);
}

Tracon_set *tracon_set_create(void)
{
	Tracon_set *ss = (Tracon_set *) malloc(sizeof(Tracon_set));

	ss->prime_idx = 0;
	ss->size = s_prime[ss->prime_idx];
	ss->mod_func = prime_mod_func[ss->prime_idx];
	ss->table = (clist_slot *) malloc(ss->size * sizeof(clist_slot));
	memset(ss->table, 0, ss->size*sizeof(clist_slot));
	ss->count = ss->ocount = 0;
	ss->shallow = false;
	ss->available_count = MAX_TRACON_SET_TABLE_SIZE(ss->size);

	return ss;
}

/**
 * The connectors must be exactly equal.
 */
static bool connector_equal(const Connector *c1, const Connector *c2)
{
	return c1->desc == c2->desc && (c1->multi == c2->multi);
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
	lgdebug(+5, "%ld accesses, chain %.4f\n",
	        fp_count, 1.*(fp_count+coll_count)/fp_count);
}
#define PRT_STAT(...) __VA_ARGS__
#else
#define PRT_STAT(...)
#endif

static bool place_found(const Connector *c, const clist_slot *slot, unsigned int hash,
                         Tracon_set *ss)
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
static unsigned int find_place(const Connector *c, unsigned int h, Tracon_set *ss)
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
	ss->available_count = MAX_STRING_SET_TABLE_SIZE(ss->size);

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
	assert(clist != NULL, "Connector-ID: Can't insert a null list");

	/* We may need to add it to the table. If the table got too big,
	 * first we grow it. */
	if (ss->available_count == 0) grow_table(ss);

	unsigned int h = hash_connectors(clist, ss->shallow);
	unsigned int p = find_place(clist, h, ss);

	if (ss->table[p].clist != NULL)
		return &ss->table[p].clist;

	ss->table[p].hash = h;
	ss->count++;
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
