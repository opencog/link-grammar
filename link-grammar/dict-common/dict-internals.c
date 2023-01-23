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

void dict_free_lookup(Dictionary dict, Dict_node* llist)
{
	Dict_node * dn;
	while (llist != NULL)
	{
		dn = llist->right;
		free(llist);
		llist = dn;
	}
}
