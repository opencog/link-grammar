/*
 * lookup-atomese.cc
 *
 * Implement the word-lookup callbacks
 *
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 */
#ifdef HAVE_ATOMESE

#include <cstdlib>
#include <opencog/atoms/value/StringValue.h>
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
	Handle linkp;
};

/// Open a connection to a CogServer located at url.
void as_open(Dictionary dict, const char* url)
{
	Local* local = new Local;
	local->asp = createAtomSpace();

	// Create the connector predicate.
	local->linkp = local->asp->add_node(PREDICATE_NODE,
		"*-LG connector string-*");

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

/// Close the connection to the cogserver.
void as_close(Dictionary dict)
{
	if (nullptr == dict->as_server) return;
	Local* local = (Local*) (dict->as_server);
	local->stnp->close();
	delete local;
	dict->as_server = nullptr;
}

/// Return true if the given word can be found in the dictionary,
/// else return false.
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
	{
		local->stnp->fetch_incoming_by_type(wrd, SECTION);
		local->stnp->barrier();
	}

	nsects = wrd->getIncomingSetSizeByType(SECTION);
printf("duuude as_lookup for >>%s<< sects=%lu\n", s, nsects);
	return 0 != nsects;
}

// Return a cached LG-compatible link string.
// Assisgn a new name, if one does not exist.
static std::string get_linkname(Local* local, const Handle& lnk)
{
	// If we've already cached a connector string for this,
	// just return it.  Else build and cache a string.
	StringValuePtr vname = StringValueCast(lnk->getValue(local->linkp));
	if (vname)
		return vname->value()[0];

	std::string slnk = "XXX";
	vname = createStringValue(slnk);
	lnk->setValue(local->linkp, ValueCast(vname));
	return slnk;
}

Dict_node * as_lookup_list(Dictionary dict, const char *s)
{
	Local* local = (Local*) (dict->as_server);
	Dict_node * dn = nullptr;

printf("duuude called as_lookup_list for >>%s<<\n", s);
	Handle wrd = local->asp->get_node(WORD_NODE, s);

	// Loop over all Sections on the word.
	HandleSeq sects = wrd->getIncomingSetByType(SECTION);
	for (const Handle& sect: sects)
	{
		Exp* exp = nullptr;

		// The connector sequence the secnd atom.
		// Loop over the connectors in the connector sequence.
		const Handle& conseq = sect->getOutgoingAtom(1);
		for (const Handle& ctcr : conseq->getOutgoingSet())
		{
			const Handle& lnk = ctcr->getOutgoingAtom(0);
			const Handle& dir = ctcr->getOutgoingAtom(1);
			std::string slnk = get_linkname(local, lnk);
			const std::string& sdir = dir->get_name();

printf("duuude got connector %s %s\n", slnk.c_str(), sdir.c_str());
			Exp* e = Exp_create(dict->Exp_pool);
			e->type = CONNECTOR_type;
			e->operand_next = NULL;
			e->cost = 0.0;
			e->multi = false;
			e->condesc = condesc_add(&dict->contable,
				string_set_add(slnk.c_str(), dict->string_set));
			e->dir =
				string_set_add(sdir.c_str(), dict->string_set);

			exp = e;
#if 0
			if (xx)
			{
				Exp* join = Exp_create(dict->Exp_pool);
				join->type = AND_type;
				join->cost = 0.0;
				join->operand_first = e;
				join->operand_next = NULL;
				e->operand_next = ??
				exp = join;
rest->operand_next = NULL;
			}
#endif
		}

		Dict_node *sdn = (Dict_node*) malloc(sizeof(Dict_node));
		memset(sdn, 0, sizeof(Dict_node));
		sdn->string = string_set_add(s, dict->string_set);
		sdn->exp = exp;
		sdn->right = dn;
		dn = sdn;
	}

	// make_expression(dict, argv[0], &exp);
	// assert(NULL != exp, "Failed expression %s", argv[0]);
	// exp->cost = 1.0;

	return dn;
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
