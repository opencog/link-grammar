/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this softwares.   */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _EXTRACT_LINKS_H
#define _EXTRACT_LINKS_H

#include "api-structures.h"
#include "link-includes.h"

Parse_info parse_info_new(int nwords);
void free_parse_info(Parse_info);

void parse_info_set_rand_state(Parse_info, unsigned int);

bool build_parse_set(Sentence, Parse_info,
                     fast_matcher_t*, count_context_t*,
                      unsigned int null_count, Parse_Options);

void extract_links(Linkage, Parse_info);

#endif /* _EXTRACT_LINKS_H */
