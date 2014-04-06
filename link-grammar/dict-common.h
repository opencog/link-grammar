/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2013, 2014 Linas Vepstas                                */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _LG_DICT_COMMON_H_
#define  _LG_DICT_COMMON_H_

#include "api-types.h"
#include "dict-structures.h"

/* The functions here are for link-grammar internal use only.
 * They are not part of the public API. */

bool find_word_in_dict(Dictionary dict, const char *);

Exp * Exp_create(Dictionary);
void add_empty_word(Dictionary, Dict_node *);

void patch_subscript(char *);

Affix_table_con affix_list_find(Dictionary affix_table, const char *);

/* Connector names for the lists in the affix file */
#define AFDICT_RPUNC "RPUNC"
#define AFDICT_LPUNC "LPUNC"
#define AFDICT_QUOTES "QUOTES"
#define AFDICT_UNITS "UNITS"

/* SUF is used in the Russian dict; PRE is not used anywhere, yet ... */
#define AFDICT_SUF "SUF"
#define AFDICT_PRE "PRE"
#define AFDICT_MPRE "MPRE" /* Multi-prefix, currently for Hebrew */
#define AFDICT_SANEMORPHISM "SANEMORPHISM" /* Regexp for sane_morphism() */

#define AFFIX_COUNT_MEM_INCREMENT 64

#endif /* _LG_DICT_COMMON_H_ */
