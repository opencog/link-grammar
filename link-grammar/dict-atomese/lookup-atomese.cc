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
#include "../dict-ram/dict-ram.h"
#include "lookup-atomese.h"
};

using namespace opencog;

class Local
{
public:
	const char* url;
	AtomSpacePtr asp;
	StorageNodePtr stnp;
	Handle linkp;
};

/// Open a connection to a CogServer located at url.
void as_open(Dictionary dict, const char* url)
{
	Local* local = new Local;
printf("duuude its %s\n", url);

	local->url = string_set_add(url, dict->string_set);

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

	// Clear the cache as well
	free_dictionary_root(dict);
	dict->num_entries = 0;
}

// ===============================================================

/// Return true if the given word can be found in the dictionary,
/// else return false.
bool as_boolean_lookup(Dictionary dict, const char *s)
{
	bool found = dict_node_exists_lookup(dict, s);
	if (found) return true;

	if (0 == strcmp(s, LEFT_WALL_WORD))
		s = "###LEFT-WALL###";

	Local* local = (Local*) (dict->as_server);
	Handle wrd = local->asp->add_node(WORD_NODE, s);

	// Are there any Sections in the local atomspace?
	size_t nsects = wrd->getIncomingSetSizeByType(SECTION);
	if (0 == nsects)
	{
		local->stnp->fetch_incoming_by_type(wrd, SECTION);
		local->stnp->barrier();
	}

	nsects = wrd->getIncomingSetSizeByType(SECTION);
printf("duuude as_boolean_lookup for >>%s<< found sects=%lu\n", s, nsects);
	return 0 != nsects;
}

// ===============================================================

/// int to base-26 capital letters.
static std::string idtostr(uint64_t aid)
{
	std::string s;
	do
	{
		char c = (aid % 26) + 'A';
		s.push_back(c);
	}
	while (0 < (aid /= 26));

	return s;
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

	static uint64_t lid = 0;
	std::string slnk = idtostr(lid++);
	vname = createStringValue(slnk);
	lnk->setValue(local->linkp, ValueCast(vname));
	return slnk;
}

// Cheap hack until c++20 ranges are generally available.
template<typename T>
class reverse {
private:
  T& iterable_;
public:
  explicit reverse(T& iterable) : iterable_{iterable} {}
  auto begin() const { return std::rbegin(iterable_); }
  auto end() const { return std::rend(iterable_); }
};

// Debugging utility
void print_section(Dictionary dict, const Handle& sect)
{
	Local* local = (Local*) (dict->as_server);

	const Handle& germ = sect->getOutgoingAtom(0);
	printf("(%s: ", germ->get_name().c_str());

	const Handle& conseq = sect->getOutgoingAtom(1);
	for (const Handle& ctcr : conseq->getOutgoingSet())
	{
		// The connection target is the first Atom in the connector
		const Handle& tgt = ctcr->getOutgoingAtom(0);
		const Handle& dir = ctcr->getOutgoingAtom(1);
		const std::string& sdir = dir->get_name();
		printf("%s%c ", tgt->get_name().c_str(), sdir[0]);
	}
	printf(") == ");

	bool first = true;
	for (const Handle& ctcr : conseq->getOutgoingSet())
	{
		// The connection target is the first Atom in the connector
		const Handle& tgt = ctcr->getOutgoingAtom(0);
		const Handle& dir = ctcr->getOutgoingAtom(1);

		// The link is the connection of both of these.
		const Handle& lnk = local->asp->add_link(SET_LINK, germ, tgt);

		// Assign an upper-case name to the link.
		std::string slnk = get_linkname(local, lnk);
		const std::string& sdir = dir->get_name();
		if (not first) printf("& ");
		first = false;
		printf("%s%c ", slnk.c_str(), sdir[0]);
	}
	printf("\n");
	fflush(stdout);
}

// ===============================================================

#define INNER_LOOP                                                \
	/* The connection target is the first Atom in the connector */ \
	const Handle& tgt = ctcr->getOutgoingAtom(0);                  \
	                                                               \
	/* The link is the connection of both of these. */             \
	const Handle& lnk = local->asp->add_link(SET_LINK, wrd, tgt);  \
	                                                               \
	/* Assign an upper-case name to the link. */                   \
	std::string slnk = get_linkname(local, lnk);                   \
	                                                               \
	Exp* e = make_connector_node(dict, dict->Exp_pool,             \
	                 slnk.c_str(), cdir, false);                   \
	if (nullptr == andex) {                                        \
		andex = e;                                                  \
		continue;                                                   \
	}                                                              \
	andex = make_and_node(dict->Exp_pool, e, andex);

Dict_node * as_lookup_list(Dictionary dict, const char *s)
{
	// Do we already have this word cached? If so, pull from
	// the cache.
	Dict_node * dn = dict_node_lookup(dict, s);

	if (dn) return dn;

	const char* ssc = string_set_add(s, dict->string_set);
	Local* local = (Local*) (dict->as_server);

	if (0 == strcmp(s, LEFT_WALL_WORD))
		s = "###LEFT-WALL###";

	Handle wrd = local->asp->get_node(WORD_NODE, s);
	if (nullptr == wrd) return nullptr;

	Exp* exp = nullptr;

	// Loop over all Sections on the word.
	HandleSeq sects = wrd->getIncomingSetByType(SECTION);
	for (const Handle& sect: sects)
	{
		Exp* andex = nullptr;

		// The connector sequence the second Atom.
		// Loop over the connectors in the connector sequence.
		// Loop twice, because the atomspace stores connectors
		// in word-order, while LG stores them as distance-from
		// given word.  So we have to reverse the order when
		// building the disjunct.
		const Handle& conseq = sect->getOutgoingAtom(1);
		for (const Handle& ctcr : conseq->getOutgoingSet())
		{
			// The direction is the second Atom in the connector
			const Handle& dir = ctcr->getOutgoingAtom(1);
			char cdir = dir->get_name().c_str()[0];
			if ('+' == cdir) continue;
			INNER_LOOP;
		}
		for (const Handle& ctcr : reverse(conseq->getOutgoingSet()))
		{
			// The direction is the second Atom in the connector
			const Handle& dir = ctcr->getOutgoingAtom(1);
			char cdir = dir->get_name().c_str()[0];
			if ('-' == cdir) continue;
			INNER_LOOP;
		}
		print_section(dict, sect);
		printf("Word %s expression %s\n", ssc, lg_exp_stringify(andex));

		if (nullptr == exp)
			exp = andex;
		else
			exp = make_or_node(dict->Exp_pool, exp, andex);
	}

	dn = (Dict_node*) malloc(sizeof(Dict_node));
	memset(dn, 0, sizeof(Dict_node));
	dn->string = ssc;
	dn->exp = exp;

	// Cache the result; avoid repeated lookups.
	dict->root = dict_node_insert(dict, dict->root, dn);
	dict->num_entries++;
printf("duuude as_lookup_list %d for >>%s<< had=%lu\n",
dict->num_entries, ssc, sects.size());

	// Rebalance the tree every now and then.
	if (0 == dict->num_entries% 30)
	{
		dict->root = dsw_tree_to_vine(dict->root);
		dict->root = dsw_vine_to_tree(dict->root, dict->num_entries);
	}

	// Perform the lookup. We cannot return the dn above, as the
	// as_free_llist() below will delete it, leading to mem corruption.
	dn = dict_node_lookup(dict, ssc);
	return dn;
}

// This is supposed to provide a wild-card lookup.  However,
// There is currently no way to support a wild-card lookup in the
// atomspace: there is no way to ask for all WordNodes that match
// a given regex.  There's no regex predicate... this can be hacked
// around in various elegant and inelegant ways, e.g. adding a
// regex predicate to the AtomSpace. Punt for now. This is used
// only for the !! command in the parser command-line tool.
// XXX FIXME. But low priority.
Dict_node * as_lookup_wild(Dictionary dict, const char *s)
{
	Dict_node * dn = dict_node_wild_lookup(dict, s);
	if (dn) return dn;

	as_lookup_list(dict, s);
	return dict_node_wild_lookup(dict, s);
}

// Zap all the Dict_nodes that we've added earlier.
// This clears out everything hanging on dict->root
// as well as the expression pool.
// And also the local AtomSpace.
//
void as_clear_cache(Dictionary dict)
{
	Local* local = (Local*) (dict->as_server);
	printf("Prior to clear, dict has %d entries, Atomspace has %lu Atoms\n",
		dict->num_entries, local->asp->get_size());

	dict->Exp_pool = pool_new(__func__, "Exp", /*num_elements*/4096,
	                             sizeof(Exp), /*zero_out*/false,
	                             /*align*/false, /*exact*/false);

	// Clear the local AtomSpace too.
	// Easiest way to do this is to just close and reopen
	// the connection.
	const char* url = local->url;
	as_close(dict);
	as_open(dict, url);
	as_boolean_lookup(dict, LEFT_WALL_WORD);
}
#endif /* HAVE_ATOMESE */
