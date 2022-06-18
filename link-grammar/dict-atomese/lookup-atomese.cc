/*
 * lookup-atomese.cc
 *
 * Implement the word-lookup callbacks
 *
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 */
#ifdef HAVE_ATOMESE

#include <cstdlib>
#include <opencog/atomspace/AtomSpace.h>
#include <opencog/persist/api/StorageNode.h>
#include <opencog/persist/cog-storage/CogStorage.h>
#include <opencog/nlp/types/atom_types.h>
#undef STRINGIFY

extern "C" {
#include "../link-includes.h"            // For Dictionary
#include "../dict-common/dict-common.h"  // for Dictionary_s
#include "lookup-atomese.h"
};

using namespace opencog;

class Local
{
public:
	AtomSpacePtr asp;
	StorageNodePtr stnp;
};

void as_open(Dictionary dict, const char* url)
{
	Local* local = new Local;
	local->asp = createAtomSpace();

	/* The cast below forces the shared lib constructor to run. */
	/* That's needed to force the factory to get installed. */
	Handle hsn = local->asp->add_node(COG_STORAGE_NODE, url);
	local->stnp = CogStorageNodeCast(hsn);

	local->stnp->open();

	// XXX FIXME -- if we cannot connect, then should hard-fail.
	if (local->stnp->connected())
		printf("Connected to %s\n", url);
	else
		printf("Failed to connect to %s\n", url);

	dict->as_server = (void*) local;
}

void as_close(Dictionary dict)
{
	if (nullptr == dict->as_server) return;
	Local* local = (Local*) (dict->as_server);
	local->stnp->close();
	delete local;
	dict->as_server = nullptr;
}

bool as_lookup(Dictionary dict, const char *s)
{
	Local* local = (Local*) (dict->as_server);
printf("duuude called as_lookup for >>%s<<\n", s);
	Handle wrd = local->asp->get_node(WORD_NODE, s);
	if (nullptr == wrd)
		wrd = local->asp->add_node(WORD_NODE, s);

	// Are there any Sections in the local atomspace?
	size_t nsects = wrd->getIncomingSetSizeByType(SECTION);
	if (0 == nsects)
		local->stnp->fetch_incoming_by_type(wrd, SECTION);

	nsects = wrd->getIncomingSetSizeByType(SECTION);
printf("duuude as_lookup for >>%s<< sects=%lu\n", s, nsects);
	return 0 != nsects;
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
#endif /* HAVE_ATOMSPACE */
