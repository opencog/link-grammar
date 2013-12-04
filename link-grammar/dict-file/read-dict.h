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

#ifndef _LG_READ_DICT_H_
#define  _LG_READ_DICT_H_

#include <link-grammar/dict-structures.h>

LINK_BEGIN_DECLS

void print_dictionary_data(Dictionary dict);
void print_dictionary_words(Dictionary dict);
void print_expression(Exp *);

Boolean boolean_dictionary_lookup(Dictionary dict, const char *);
Boolean find_word_in_dict(Dictionary dict, const char *);

int  delete_dictionary_words(Dictionary dict, const char *);

Dict_node * dictionary_lookup_list(Dictionary dict, const char *);
Dict_node * abridged_lookup_list(Dictionary dict, const char *);
void free_lookup_list(Dict_node *);

Dict_node * insert_dict(Dictionary dict, Dict_node * n, Dict_node * newnode);

Dictionary dictionary_create_from_utf8(const char * input);

/* Below are not really for public consumption.
 * These need to be moved to a private header file. TODO XXX */
Exp *       Exp_create(Dictionary dict);
void        free_dictionary(Dictionary dict);
Boolean read_dictionary(Dictionary dict);
void add_empty_word(Dictionary dict, Dict_node *);


LINK_END_DECLS

#endif /* _LG_READ_DICT_H_ */
