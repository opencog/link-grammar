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
#include "connectors.h"                 // Connector

typedef struct count_context_s count_context_t;

Count_bin *table_lookup(count_context_t *, int, int,
                        const Connector *, const Connector *,
                        unsigned int, size_t *);
int do_parse(Sentence, fast_matcher_t*, count_context_t*, Parse_Options);
bool no_count(count_context_t *, int, Connector *, unsigned int, unsigned int);
match_list_cache *get_cached_match_list(count_context_t *, int, int, Connector *);

count_context_t *alloc_count_context(Sentence, Tracon_sharing*);
void free_count_context(count_context_t*, Sentence);

/**
 * Are the nearest_word of the end connectors in the range [lw, rw] and
 * also don't need a link cross?
 */
static inline bool valid_nearest_words(const Connector *le, const Connector *re,
                                       int lw, int rw)
{
	int r_limit;

	if (likely(re != NULL))
	{
		if (unlikely(re->nearest_word < lw)) return false;
		r_limit = re->nearest_word;
	}
	else
	{
		r_limit = rw;
	}
	if (likely(le != NULL) && unlikely(le->nearest_word > r_limit)) return false;

	return true;
}

#endif /* _COUNT_H */
