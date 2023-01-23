/*************************************************************************/
/* Copyright (c) 2023  Linas Vepstas                                     */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "dict-structures.h"
#include "link-includes.h"

Dict_node * dict_node_new(void);
void dict_node_noop(Dictionary dict);
void dict_node_free_list(Dict_node *llist);
void dict_node_free_lookup(Dictionary, Dict_node*);
