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
#include "lg_assert.h"

typedef struct
{
	Connector *clist;
	unsigned int hash;
} clist_slot;

typedef struct
{
	size_t size;       /* the current size of the table */
	size_t count;      /* number of things currently in the table */
	size_t ocount;     /* the count before reset */
	clist_slot *table;    /* the table itself */
	unsigned int prime_idx;     /* current prime number table index */
	prime_mod_func_t mod_func;  /* the function to compute a prime modulo */
	bool shallow;      /* consider shallow connector */
} Tracon_set;

Tracon_set *tracon_set_create(void);
Connector **tracon_set_add(Connector *, Tracon_set *);
Connector *tracon_set_lookup(const Connector *, Tracon_set *);
void tracon_set_delete(Tracon_set *);
void tracon_set_shallow(bool, Tracon_set *);
void tracon_set_reset(Tracon_set *);
#endif /* _TRACON_SET_H_ */
