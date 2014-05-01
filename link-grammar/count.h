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
int  do_match(Sentence, Connector *a, Connector *b, int wa, int wb);
s64  do_parse(Sentence sent, int mincost, Parse_Options opts);
#ifdef USE_FAT_LINKAGES
void conjunction_prune(Sentence sent, Parse_Options opts);
void count_set_effective_distance(Sentence sent);
void count_unset_effective_distance(Sentence sent);
#endif /* USE_FAT_LINKAGES */
void delete_unmarked_disjuncts(Sentence sent);

void init_count(Sentence sent);
void free_count(Sentence sent);

