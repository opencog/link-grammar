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
#ifndef _TRACON_SET_H_
#define _TRACON_SET_H_

#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include "api-types.h"
#include "const-prime.h"
#include "error.h"

#ifdef DEBUG
#ifndef TRACON_SET_DEBUG
#define TRACON_SET_DEBUG
#endif
#endif

typedef size_t tid_hash_t;
typedef struct
{
	Connector *clist;
	tid_hash_t hash;
#ifdef TRACON_SET_DEBUG
	unsigned int pri_collN;
	unsigned int sec_collN;
#endif
} clist_slot;

typedef struct
{
	size_t size;       /* the current size of the table */
	size_t available_count;     /* number of available entries */
	clist_slot *table; /* the table itself */
	prime_mod_func_t mod_func;  /* the function to compute a prime modulo */
	unsigned int prime_idx;     /* current prime number table index */
	bool shallow;      /* consider shallow connector */
#ifdef TRACON_SET_DEBUG
	/* size_t is used here instead of uint64_t to prevent the need for PRIu64. */
	size_t addN;       /* Number of tries to add */
	size_t pri_collN;  /* Number of primary collisions */
	size_t sec_collN;  /* Number of secondary collisions */
	unsigned int resetN;       /* Number of table resets */
#endif
} Tracon_set;

/* If the table gets too big, we grow it. Too big is defined as being
 * more than 3/8 full. There's a huge boost from keeping this sparse. */
#define MAX_TRACON_SET_TABLE_SIZE(s) ((s) * 3 / 8)

Tracon_set *tracon_set_create(void);
Connector **tracon_set_add(Connector *, Tracon_set *);
Connector *tracon_set_lookup(const Connector *, Tracon_set *);
void tracon_set_delete(Tracon_set *);
void tracon_set_shallow(bool, Tracon_set *);
void tracon_set_reset(Tracon_set *);
#endif /* _TRACON_SET_H_ */
