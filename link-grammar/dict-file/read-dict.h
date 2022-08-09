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
#define _LG_READ_DICT_H_

#include "dict-common/dict-structures.h"

Dictionary dictionary_six(const char *lang, const char *dict_name,
                          const char *pp_name, const char *cons_name,
                          const char *affix_name, const char *regex_name);
Dictionary dictionary_create_from_file(const char *lang);
bool read_dictionary(Dictionary dict);
void dict_error2(Dictionary dict, const char *s, const char *s2);

void insert_list(Dictionary dict, Dict_node * p, int l);
void free_insert_list(Dict_node *ilist);

#endif /* _LG_READ_DICT_H_ */
