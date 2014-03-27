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

#include "api-types.h"
#include "link-includes.h"

const char * intersect_strings(Sentence, const char *, const char *);

Linkage_info  analyze_thin_linkage(Sentence, Parse_Options, int pass);
void          extract_thin_linkage(Sentence, Parse_Options, Linkage);

void zero_sublinkage(Sublinkage *s);

#ifdef USE_FAT_LINKAGES
void init_analyze(Sentence);
void free_analyze(Sentence);

void          free_DIS_tree(DIS_node *);
Linkage_info  analyze_fat_linkage (Sentence, Parse_Options, int pass);
void          extract_fat_linkage (Sentence, Parse_Options, Linkage);
#endif /* USE_FAT_LINKAGES */

