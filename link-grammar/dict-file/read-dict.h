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

#ifndef _LG_READ_DICT_H_
#define  _LG_READ_DICT_H_

#include <link-grammar/dict-structures.h>

void print_dictionary_data(Dictionary dict);
void print_dictionary_words(Dictionary dict);

Dict_node * dictionary_file_lookup_list(Dictionary dict, const char *);

int  delete_dictionary_words(Dictionary dict, const char *);

Dictionary dictionary_create_from_file(const char * lang);
Boolean read_dictionary(Dictionary dict);


#endif /* _LG_READ_DICT_H_ */
