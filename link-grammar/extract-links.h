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

#include "api-structures.h"
#include "link-includes.h"

Parse_info parse_info_new(int nwords);
void free_parse_info(Parse_info);
int   build_parse_set(Sentence, match_context_t*, unsigned int cost, Parse_Options);
void  extract_links(int index, Parse_info);
