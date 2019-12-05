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
#ifndef _STRING_ID_H_
#define _STRING_ID_H_

#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include "api-types.h"
#include "const-prime.h"
#include "lg_assert.h"
#ifdef _WIN32
#include "utilities.h"                  // ssize_t
#endif

#define SI_NOTFOUND 0 /* return this for strings that are not in the table */

typedef struct
{
	const char *str;
	unsigned int id;            /* a unique id for this string */
	unsigned int hash;
} ss_id;

typedef struct str_mem_pool_s str_mem_pool;

struct String_id_s
{
	size_t size;                /* the current size of the table */
	size_t count;               /* number of things currently in the table */
	size_t available_count;     /* number of available entries */
	ss_id *table;               /* the table itself */
	unsigned int prime_idx;     /* current prime number table index */
	prime_mod_func_t mod_func;  /* the function to compute a prime modulo */
	ssize_t pool_free_count;    /* string pool free space */
	char *alloc_next;           /* next string address */
	str_mem_pool *string_pool;  /* string memory pool */
};

/* If the table gets too big, we grow it. Too big is defined as being
 * more than 3/8 full. There's a huge boost from keeping this sparse. */
#define MAX_STRING_SET_TABLE_SIZE(s) ((s) * 3 / 8)

String_id *string_id_create(void);
unsigned int  string_id_add(const char *source_string, String_id *ss);
unsigned int  string_id_lookup(const char *source_string, String_id *ss);
void string_id_delete(String_id *ss);

#endif /* _STRING_ID_H_ */
