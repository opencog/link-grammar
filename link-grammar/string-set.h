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
#ifndef _STRING_SET_H_
#define _STRING_SET_H_

#include <stddef.h>
#include "api-types.h"

struct String_set_s
{
   size_t size;       /* the current size of the table */
   size_t count;      /* number of things currently in the table */
   char ** table;     /* the table itself */
};

String_set * string_set_create(void);
const char * string_set_add(const char * source_string, String_set * ss);
const char * string_set_lookup(const char * source_string, String_set * ss);
void         string_set_delete(String_set *ss);

#endif /* _STRING_SET_H_ */
