/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (C) 2018, 2019 Amir Plivatsky                               */
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
#include "string-id.h"
#include "utilities.h"

/**
 * This is an adaptation of the string_id module for generating
 * sequential unique identifiers (starting from 1) for strings.
 * Identifier 0 is reserved to denote "no identifier".
 */

#define STR_POOL
#define MEM_POOL_INIT (2*1024)
#define MEM_POOL_INCR (4*1024)
struct str_mem_pool_s
{
	str_mem_pool *prev;
	size_t size;
	char block[0];
};

static unsigned int hash_string(const char *str, const String_id *ss)
{
	unsigned int accum = 0;
	for (; *str != '\0'; str++)
		accum = (139 * accum) + (unsigned char)*str;
	return accum;
}

#ifdef STR_POOL
static void ss_pool_alloc(size_t pool_size_add, String_id *ss)
{
	str_mem_pool *new_mem_pool = malloc(pool_size_add);
	new_mem_pool->size = pool_size_add;

	new_mem_pool->prev = ss->string_pool;
	ss->string_pool = new_mem_pool;
	ss->alloc_next = ss->string_pool->block;

	ss->pool_free_count = pool_size_add - sizeof(str_mem_pool);
	ASAN_POISON_MEMORY_REGION(ss->string_pool+sizeof(str_mem_pool), ss->pool_free_count);
}

static void ss_pool_delete(String_id *ss)
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

static char *ss_stralloc(size_t str_size, String_id *ss)
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

String_id *string_id_create(void)
{
	String_id *ss;
	ss = (String_id *) malloc(sizeof(String_id));
	ss->prime_idx = 0;
	ss->size = s_prime[ss->prime_idx];
	ss->mod_func = prime_mod_func[ss->prime_idx];
	ss->table = malloc(ss->size * sizeof(ss_id));
	memset(ss->table, 0, ss->size*sizeof(ss_id));
	ss->count = 0;
	ss->string_pool = NULL;
	ss_pool_alloc(MEM_POOL_INIT, ss);
	ss->available_count = MAX_STRING_SET_TABLE_SIZE(ss->size);

	return ss;
}

static bool place_found(const char *str, const ss_id *slot, unsigned int hash,
                         String_id *ss)
{
	if (slot->str == NULL) return true;
	if (hash != slot->hash) return false;
	return (strcmp(slot->str, str) == 0);
}

/**
 * lookup the given string in the table.  Return an index
 * to the place it is, or the place where it should be.
 */
static unsigned int find_place(const char *str, unsigned int h, String_id *ss)
{
	unsigned int coll_num = 0;
	unsigned int key = ss->mod_func(h);

	/* Quadratic probing. */
	while (!place_found(str, &ss->table[key], h, ss))
	{
		key += 2 * ++coll_num - 1;
		if (key >= ss->size) key = ss->mod_func(key);
	}

	return key;
}

static void grow_table(String_id *ss)
{
	String_id old;
	size_t i;
	unsigned int p;

	old = *ss;
	ss->prime_idx++;
	ss->size = s_prime[ss->prime_idx];
	ss->mod_func = prime_mod_func[ss->prime_idx];
	ss->table = malloc(ss->size * sizeof(ss_id));
	memset(ss->table, 0, ss->size*sizeof(ss_id));
	for (i=0; i<old.size; i++)
	{
		if (old.table[i].str != NULL)
		{
			p = find_place(old.table[i].str, old.table[i].hash, ss);
			ss->table[p] = old.table[i];
		}
	}
	ss->available_count = MAX_STRING_SET_TABLE_SIZE(ss->size);

	/* printf("growing from %zu to %zu\n", old.size, ss->size); */
	free(old.table);
}

unsigned int string_id_add(const char *source_string, String_id *ss)
{
	assert(source_string != NULL, "STRING_SET: Can't insert a null string");

	unsigned int h = hash_string(source_string, ss);
	unsigned int p = find_place(source_string, h, ss);

	if (ss->table[p].str != NULL) return ss->table[p].id;

	size_t len = strlen(source_string) + 1;
	char *str;

	str = ss_stralloc(len, ss);
	memcpy(str, source_string, len);
	ss->table[p].str = str;
	ss->table[p].hash = h;
	ss->table[p].id = ss->count + 1;
	ss->count++;
	ss->available_count--;

	int keep_id = ss->table[p].id;

	/* We just added it to the table. */
	if (ss->available_count == 0) grow_table(ss); /* Too full */

	return keep_id;
}

unsigned int string_id_lookup(const char * source_string, String_id * ss)
{
	unsigned int h = hash_string(source_string, ss);
	unsigned int p = find_place(source_string, h, ss);

	return (ss->table[p].str == NULL) ? SID_NOTFOUND : ss->table[p].id;
}

void string_id_delete(String_id *ss)
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
