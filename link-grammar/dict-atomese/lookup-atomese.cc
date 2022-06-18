/*
 * lookup-atomese.cc
 *
 * Implement the word-lookup callbacks
 *
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 */

#include <cstdlib>

extern "C" {
#include "lookup-atomese.h"
};

bool as_lookup(Dictionary dict, const char *s)
{
printf("duuude called as_lookup for %s\n", s);
	return false;
}

Dict_node * as_lookup_list(Dictionary dict, const char *s)
{
printf("duuude called as_lookup_list for %s\n", s);
	return NULL;
}

Dict_node * as_lookup_wild(Dictionary dict, const char *s)
{
printf("duuude called as_lookup_wild for %s\n", s);
	return NULL;
}

void as_free_llist(Dictionary dict, Dict_node *llist)
{
	Dict_node * dn;
	while (llist != NULL)
	{
		dn = llist->right;
		free(llist);
		llist = dn;
	}
}

void as_close(Dictionary dict)
{
printf("duuude called as_close\n");
}
