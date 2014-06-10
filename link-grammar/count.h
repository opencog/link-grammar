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

#include <link-grammar/api-structures.h>
#include "structures.h" /* for s64 */

s64  table_lookup(count_context_t *, int, int, Connector *, Connector *, unsigned int);
/* XXX FIXME do_match does not need a context if its not fat ... */
bool  do_match(count_context_t *, Connector *a, Connector *b, int wa, int wb);
s64  do_parse(Sentence, match_context_t*, count_context_t*, int mincost, Parse_Options);
#ifdef USE_FAT_LINKAGES
void conjunction_prune(Sentence, count_context_t*, Parse_Options);
void count_set_effective_distance(count_context_t*, Sentence);
void count_unset_effective_distance(count_context_t*);
#endif /* USE_FAT_LINKAGES */
void delete_unmarked_disjuncts(Sentence sent);

count_context_t* alloc_count_context(size_t);
void free_count_context(count_context_t*);

