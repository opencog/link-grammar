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

#ifndef _LG_DICT_API_H_
#define  _LG_DICT_API_H_

#include "dict-structures.h"
#include "link-includes.h"

LINK_BEGIN_DECLS

/**
 * Declaration of dictionary-related functions that link-grammar users
 * are free to use in their applications.  That is, these are a part of
 * the public API to the link-parser system.
 */


Dict_node * dictionary_lookup_list(const Dictionary, const char *);
Dict_node * dictionary_lookup_wild(const Dictionary, const char *);

/* Return true if word can be found. */
bool dictionary_word_is_known(const Dictionary, const char *);

void free_lookup_list(const Dictionary, Dict_node *);


/* XXX the below probably does not belong ...  ?? */
Dict_node * insert_dict(Dictionary dict, Dict_node * n, Dict_node * newnode);

LINK_END_DECLS

#endif /* _LG_DICT_API_H_ */
