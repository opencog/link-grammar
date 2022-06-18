/*
 * lookup-atomese.cc
 *
 * Implement the word-lookup callbacks
 *
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 */

#include <cstdlib>
#include <opencog/atomspace/AtomSpace.h>
#undef STRINGIFY

extern "C" {
#include "../link-includes.h"            // For Dictionary
#include "../dict-common/dict-common.h"  // for Dictionary_s
#include "lookup-atomese.h"
};

using namespace opencog;

void as_open(Dictionary dict, const char* url)
{
printf("duuude called as_open\n");
	AtomSpacePtr asp = createAtomSpace();
	dict->as_server = (void*) new AtomSpacePtr(asp);
}

void as_close(Dictionary dict)
{
printf("duuude called as_close\n");
	if (nullptr == dict->as_server) return;
	delete dict->as_server;
	dict->as_server = nullptr;
}

bool as_lookup(Dictionary dict, const char *s)
{
	AtomSpacePtr* aspp = (AtomSpacePtr*) (dict->as_server);
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
