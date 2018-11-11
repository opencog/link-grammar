/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright 2018 Amir Plivatsky                                         */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/
#ifndef _STRING_SET_ID_H_
#define _STRING_SET_ID_H_

#include <string.h>
#include <stddef.h>

#include "api-types.h"
#include "lg_assert.h"

typedef struct
{
	const char *str;
	int id;            /* a unique id for this string */
} ss_id;

typedef struct
{
	size_t size;       /* the current size of the table */
	size_t count;      /* number of things currently in the table */
	ss_id *table;      /* the table itself */
} String_id;

String_id *string_id_create(void);
int string_id_add(const char *, String_id *);
int string_id_lookup(const char *, String_id *);
void string_id_delete(String_id *);

#endif /* _STRING_SET_ID_H_ */
