/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2013 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "link-includes.h"

#ifdef USE_FAT_LINKAGES
void free_deletable(Sentence sent);
void free_effective_dist(Sentence sent);
void build_effective_dist(Sentence sent, int has_conjunction);
void build_deletable(Sentence sent, int has_conjunction);
#endif /* USE_FAT_LINKAGES */

void prepare_to_parse(Sentence, match_context_t*, Parse_Options);

