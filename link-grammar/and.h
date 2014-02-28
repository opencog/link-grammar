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

#include "link-includes.h"

#ifdef USE_FAT_LINKAGES

#include "api-types.h"

#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)) && defined(__ELF__)
#define link_private    __attribute__((__visibility__("hidden")))
#else
#define link_private
#endif


link_private void       free_AND_tables(Sentence sent);
link_private void       print_AND_statistics(Sentence sent);
link_private void       init_andable_hash_table(Dictionary dict);
link_private void       free_andable_hash_table(Dictionary dict);
link_private void       initialize_conjunction_tables(Sentence sent);
link_private int        is_canonical_linkage(Sentence sent);
link_private Disjunct * build_AND_disjunct_list(Sentence sent, const char *);
link_private Disjunct * build_COMMA_disjunct_list(Sentence sent);
link_private Disjunct * explode_disjunct_list(Sentence sent, Disjunct *);
link_private void       build_conjunction_tables(Sentence);
link_private void       compute_pp_link_array_connectors(Sentence sent, Sublinkage *sublinkage);

/* Following need to be visible to sat solver, can't be private */
int        set_has_fat_down(Sentence sent);
#endif /* USE_FAT_LINKAGES */
const char * intersect_strings(Sentence sent, const char * s, const char * t);
