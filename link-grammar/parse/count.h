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

#ifndef _COUNT_H
#define _COUNT_H

#include "fast-match.h"                 // fast_matcher_t
#include "histogram.h"                  // Count_bin

typedef struct count_context_s count_context_t;

Count_bin* table_lookup(count_context_t *, int, int, Connector *, Connector *, unsigned int);
int do_parse(Sentence, fast_matcher_t*, count_context_t*, Parse_Options);

count_context_t* alloc_count_context(Sentence);
void free_count_context(count_context_t*, Sentence);
#endif /* _COUNT_H */
