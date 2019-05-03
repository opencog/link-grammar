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

#include <stdint.h>                     // uintptr_t

#include "const-prime.h"
#include "memory-pool.h"
#include "string-set.h"
#include "utilities.h"

/**
 * Suppose you have a program that generates strings and keeps pointers to them.
   The program never needs to change these strings once they're generated.
   If it generates the same string again, then it can reuse the one it
   generated before.  This is what this package supports.

   String_set is the object.  The functions are:

   char * string_set_add(char * source_string, String_set * ss);
     This function returns a pointer to a string with the same
     contents as the source_string.  If that string is already
     in the table, then it uses that copy, otherwise it generates
     and inserts a new one.

   char * string_set_lookup(char * source_string, String_set * ss);
     This function returns a pointer to a string with the same
     contents as the source_string.  If that string is not already
     in the table, returns NULL;

   String_set * string_set_create(void);
     Create a new empty String_set.

   string_set_delete(String_set *ss);
     Free all the space associated with this string set.

   The implementation uses probed hashing (i.e. not bucket).
 */

static unsigned int hash_string(const char *str, const String_set *ss)
{
	unsigned int accum = 0;
	for (;*str != '\0'; str++)
		accum = (7 * accum) + (unsigned char)*str;
	return accum;
}

#ifdef STR_POOL
static void ss_pool_alloc(size_t pool_size_add, String_set *ss)
{
	str_mem_pool *new_mem_pool = malloc(pool_size_add);
	new_mem_pool->size = pool_size_add;

	new_mem_pool->prev = ss->string_pool;
	ss->string_pool = new_mem_pool;
	ss->alloc_next = ss->string_pool->block;

	ss->pool_free_count = pool_size_add - sizeof(str_mem_pool);
	ASAN_POISON_MEMORY_REGION(ss->string_pool+sizeof(str_mem_pool), ss->pool_free_count);
}

static void ss_pool_delete(String_set *ss)
{
	str_mem_pool *m, *m_prev;

	for (m = ss->string_pool; m != NULL; m = m_prev)
	{
		ASAN_UNPOISON_MEMORY_REGION(m, m->size);
		m_prev = m->prev;
		free(m);
	}
}

#define STR_ALIGNMENT 16

static char *ss_stralloc(size_t str_size, String_set *ss)
{
	ss->pool_free_count -= str_size;
	if (ss->pool_free_count < 0)
		ss_pool_alloc((str_size&MEM_POOL_INCR) + MEM_POOL_INCR, ss);

	char *str_address = ss->alloc_next;
	ASAN_UNPOISON_MEMORY_REGION(str_address, str_size);
	ss->alloc_next += str_size;
	ss->alloc_next = (char *)ALIGN((uintptr_t)ss->alloc_next, STR_ALIGNMENT);
	size_t total_size = str_size + (ss->alloc_next - str_address);

	ss->pool_free_count -= total_size;
	return str_address;
}
#else
#define ss_stralloc(x, ss) malloc(x)
#define ss_pool_alloc(a, b)
#endif

String_set * string_set_create(void)
{
	String_set *ss;
	ss = (String_set *) malloc(sizeof(String_set));
	ss->prime_idx = 0;
	ss->size = s_prime[ss->prime_idx];
	ss->mod_func = prime_mod_func[ss->prime_idx];
	ss->table = malloc(ss->size * sizeof(ss_slot));
	memset(ss->table, 0, ss->size*sizeof(ss_slot));
	ss->count = 0;
	ss->string_pool = NULL;
	ss_pool_alloc(MEM_POOL_INIT, ss);

	return ss;
}

static bool place_found(const char *str, const ss_slot *slot, unsigned int hash,
                         String_set *ss)
{
	if (slot->str == NULL) return true;
	if (hash != slot->hash) return false;
	return (strcmp(slot->str, str) == 0);
}

/**
 * lookup the given string in the table.  Return an index
 * to the place it is, or the place where it should be.
 */
static unsigned int find_place(const char *str, unsigned int h, String_set *ss)
{
	unsigned int coll_num = 0;
	unsigned int key = ss->mod_func(h);

	/* Quadratic probing. */
	while (!place_found(str, &ss->table[key], h, ss))
	{
		key += 2 * ++coll_num - 1;
		if (key >= ss->size) key -= ss->size;
	}

	return key;
}

static void grow_table(String_set *ss)
{
	String_set old;
	size_t i;
	unsigned int p;

	old = *ss;
	ss->prime_idx++;
	ss->size = s_prime[ss->prime_idx];
	ss->mod_func = prime_mod_func[ss->prime_idx];
	ss->table = malloc(ss->size * sizeof(ss_slot));
	memset(ss->table, 0, ss->size*sizeof(ss_slot));
	for (i=0; i<old.size; i++)
	{
		if (old.table[i].str != NULL)
		{
			p = find_place(old.table[i].str, old.table[i].hash, ss);
			ss->table[p] = old.table[i];
		}
	}
	/* printf("growing from %zu to %zu\n", old.size, ss->size); */
	free(old.table);
}

const char * string_set_add(const char * source_string, String_set * ss)
{
	assert(source_string != NULL, "STRING_SET: Can't insert a null string");

	unsigned int h = hash_string(source_string, ss);
	unsigned int p = find_place(source_string, h, ss);

	if (ss->table[p].str != NULL) return ss->table[p].str;

	size_t len = strlen(source_string) + 1;
	char *str;

#ifdef DEBUG
	/* Store the String_set structure address for debug verifications */
	size_t mlen = (len&~(sizeof(ss)-1)) + 2*sizeof(ss);
	str = ss_stralloc(mlen, ss);
	*(String_set **)&str[mlen-sizeof(ss)] = ss;
#else /* !DEBUG */
	str = ss_stralloc(len, ss);
#endif /* DEBUG */
	memcpy(str, source_string, len);
	ss->table[p].str = str;
	ss->table[p].hash = h;
	ss->count++;

	/* We just added it to the table.  If the table got too big,
	 * we grow it.  Too big is defined as being more than 3/8 full.
	 * There's a huge boost from keeping this sparse. */
	if ((8 * ss->count) > (3 * ss->size)) grow_table(ss);

	return str;
}

const char * string_set_lookup(const char * source_string, String_set * ss)
{
	unsigned int h = hash_string(source_string, ss);
	unsigned int p = find_place(source_string, h, ss);

	return ss->table[p].str;
}

void string_set_delete(String_set *ss)
{
	if (ss == NULL) return;

#ifdef STR_POOL
	ss_pool_delete(ss);
#else
	size_t i;

	for (i=0; i<ss->size; i++)
	{
		if (ss->table[i].str != NULL) free((void *)ss->table[i].str);
	}
#endif /* STR_POOL */

	free(ss->table);
	free(ss);
}
