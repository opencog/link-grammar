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
#include "histogram.h"                  // linkage count definitions

typedef struct count_context_s count_context_t;

count_t *table_lookup(count_context_t *, int, int,
                        const Connector *, const Connector *,
                        unsigned int, size_t *);
int do_parse(Sentence, fast_matcher_t*, count_context_t*, Parse_Options);
bool no_count(count_context_t *, int, Connector *, unsigned int, unsigned int);

count_context_t *alloc_count_context(Sentence, Tracon_sharing*);
void free_count_context(count_context_t*, Sentence);
#endif /* _COUNT_H */
