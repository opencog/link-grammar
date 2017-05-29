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

#include "api-structures.h"
#include "histogram.h" /* for s64 */

Count_bin* table_lookup(count_context_t *, int, int, Connector *, Connector *, unsigned int);
Count_bin do_parse(Sentence, fast_matcher_t*, count_context_t*, int null_count, Parse_Options);

count_context_t* alloc_count_context(size_t);
void free_count_context(count_context_t*);
#endif /* _COUNT_H */
