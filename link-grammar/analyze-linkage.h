/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */ 
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

void init_analyze(Sentence);
void free_analyze(Sentence);

Linkage_info  analyze_thin_linkage(Sentence, Parse_Options, int pass);
void          extract_thin_linkage(Sentence, Parse_Options, Linkage);
void          free_DIS_tree(DIS_node *);

void zero_sublinkage(Sublinkage *s);

#ifdef USE_FAT_LINKAGES
Linkage_info  analyze_fat_linkage (Sentence, Parse_Options, int pass);
void          extract_fat_linkage (Sentence, Parse_Options, Linkage);
#endif /* USE_FAT_LINKAGES */

