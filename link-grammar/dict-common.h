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

#include "dict-structures.h"

/* The functions here are for link-grammar internal use only.
 * They are not part of the public API. */

void    free_dictionary(Dictionary dict);

/* XXX this belongs in the dict-file directory, only. FIXME ... */
Boolean read_dictionary(Dictionary dict);

Exp * Exp_create(Dictionary);
void add_empty_word(Dictionary, Dict_node *);

void patch_subscript(char*);

#endif /* _LG_DICT_COMMON_H_ */
