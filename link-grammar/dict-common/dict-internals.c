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

#include <stdlib.h>

#include "dict-internals.h"

void dict_node_noop(Dictionary dict)
{
}

Dict_node * dict_node_new(void)
{
   return (Dict_node*) malloc(sizeof(Dict_node));
}

void dict_node_free_list(Dict_node *llist)
{
   while (llist != NULL)
   {
      Dict_node * n = llist->right;
      free(llist);
      llist = n;
   }
}

void dict_node_free_lookup(Dictionary dict, Dict_node* llist)
{
	dict_node_free_list(llist);
}
