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

typedef struct Disjunct_struct Disjunct;

/**********************************************************************
 *
 * Lookup-list.
 *
 ***********************************************************************/

link_public_api(Dict_node *)
	dictionary_lookup_list(const Dictionary dict, const char *word);
link_public_api(Dict_node *)
	dictionary_lookup_wild(const Dictionary dict, const char *word);
link_public_api(void)
	free_lookup_list(const Dictionary dict, Dict_node * n);

/**********************************************************************
 *
 * Generation mode. Experimental and subject to changes.
 *
 ***********************************************************************/

link_public_api(const Category *)
	dictionary_get_categories(const Dictionary dict);

link_public_api(const Category_cost *)
	linkage_get_categories(const Linkage linkage, WordIdx w);

link_public_api(Disjunct **)              /* To be freed by the caller */
	sentence_unused_disjuncts(Sentence);

link_public_api(char *)
	disjunct_expression(Disjunct *);       /* To be freed by the caller */

link_public_api(const Category_cost *)
	disjunct_categories(Disjunct *);

/***********************************************************************/

/* Return true if word can be found. */
link_public_api(bool)
	dictionary_word_is_known(const Dictionary dict, const char *word);

/* This was exported and used by mistake! */
bool boolean_dictionary_lookup(const Dictionary dict, const char *word);

/* XXX the below probably does not belong ...  ?? */
Dict_node * insert_dict(Dictionary dict, Dict_node * n, Dict_node * newnode);

LINK_END_DECLS

#endif /* _LG_DICT_API_H_ */
