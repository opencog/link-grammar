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

#include "link-includes.h"  // For the version number

LINK_BEGIN_DECLS

#include "api-types.h"

int  read_dictionary(Dictionary dict);
void dict_display_word_info(Dictionary dict, const char * s);
void dict_display_word_expr(Dictionary dict, const char * s);
void print_dictionary_data(Dictionary dict);
void print_dictionary_words(Dictionary dict);
void print_expression(Exp *);
int  boolean_dictionary_lookup(Dictionary dict, const char *);
int  delete_dictionary_words(Dictionary dict, const char *);

Dict_node * dictionary_lookup_list(Dictionary dict, const char *);
Dict_node * abridged_lookup_list(Dictionary dict, const char *);
void free_lookup_list(Dict_node *);

Dict_node * insert_dict(Dictionary dict, Dict_node * n, Dict_node * newnode);
void        free_dictionary(Dictionary dict);
Exp *       Exp_create(Dictionary dict);

Dictionary dictionary_create_from_utf8(const char * input);

LINK_END_DECLS
