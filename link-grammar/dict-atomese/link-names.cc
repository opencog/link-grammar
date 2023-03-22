/*
 * link-names.cc
 *
 * Translate AtomSpace word-pairs to LG link names.
 *
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 */
#ifdef HAVE_ATOMESE

#include <cstdlib>
#include <opencog/atoms/truthvalue/CountTruthValue.h>
#include <opencog/atomspace/AtomSpace.h>
#include <opencog/nlp/types/atom_types.h>

#undef STRINGIFY

extern "C" {
#include "../link-includes.h"            // For Dictionary
#include "../memory-pool.h"              // For Pool_desc
#include "lookup-atomese.h"
};

//#include "dict-atomese.h"
#include "local-as.h"

using namespace opencog;

// ===============================================================
/// Mapping LG connectors to AtomSpace connectors, and v.v.
///
/// An LG connector is a string of capital letters. It's generated on
/// the fly. It's associated with `(List (Word ..) (Word ...))`. There
/// are two lookup tasks. In this blob of code, when given the List, we
/// need to find the matching LG string. For users of LG, we have the
/// inverse problem: given the LG string, what's the List?
///
/// There are several possible solutionhs:
///
///    (Edge (BondNode "ABC") (List (Word ..) (Word ...)))
///
///    (Edge (LgLinkNode "ABC") (List (Word ..) (Word ...)))
///
///    (Edge (Predicate "*-LG connector string-*")
///       (LgConnNode "ABC") (List (Word ..) (Word ...)))
///
///     (Grant (List (Word ...) (Word ...))
///         (Predicate "*-LG connector string-*")
///         (LgLinkNode "ASDF"))
///
/// The first seems to be the best, and is implemented here.
///
/// As of just right now, this pair is held only in the local AtomSpace;
/// it is never written back to storage. This is fine casual use with
/// a private local AtomSpace. Users working with a shared attached
/// AtomSpace will probably keep counts, and store the pair. This will
/// then require taking care to avoid issuing duplicate LG link strings.

/// int to base-26 capital letters. zero = 'A'.
static std::string idtostr(uint64_t aid)
{
	std::string s;
	aid ++;
	do
	{
		aid --;
		char c = (aid % 26) + 'A';
		// Slightly more human-readable than s.push_back()
		s.insert(s.begin(), c);
	}
	while (0 < (aid /= 26));

	// Generated strings always end with star.
	// s.push_back('*');
	return s;
}

/// Given a `(List (Word ...) (Word ...))` denoting a link, find the
/// corresponding BondNode, if it exists. The string name of that
/// BondNode is the name of the LG link for that word-pair.
Handle get_lg_conn(Local* local, const Handle& wpair)
{
	for (const Handle& edge : wpair->getIncomingSetByType(EDGE_LINK))
	{
		const Handle& bond = edge->getOutgoingAtom(0);
		if (bond != local->bany)
			return bond;
	}
	return Handle::UNDEFINED;
}

/// Return a cached LG-compatible link string.
/// Assigns a new name, if one does not exist.
/// The Handle `lnk` is an ordered pair, left-right, two words/classes.
/// That is, `lnk` is `(List (Word ...) (Word ...))`
///
std::string cached_linkname(Local* local, const Handle& lnk)
{
	// If we've already cached a connector string for this,
	// just return it.  Else build and cache a string.
	Handle lgc = get_lg_conn(local, lnk);
	if (lgc)
		return lgc->get_name();

	// Use a lock, to make sure that link ID's are issued only
	// once, ever.  The GrantLink could also be used for this;
	// the GrantLink was invented to solve this problem. But
	// GrantLink is awkward for edges, because the arguments are
	// "backwards". We really need a UniqueEdgeLink, instead.
	//
	// Also: FYI, we could use Atom::incremenetCount() instead
	// of doing the last_id++ but, again, for now, it seems
	// easier to do it as below.
	std::lock_guard<std::mutex> guard(local->pair_mutex);

	// Maybe it's in storage?
	if (local->stnp)
		local->stnp->fetch_incoming_by_type(lnk, EDGE_LINK);

	// Try again, under the lock, in case threads are racing.
	lgc = get_lg_conn(local, lnk);
	if (lgc)
		return lgc->get_name();

	// idtostr(1064) is "ANY" and we want to reserve "ANY"
	if (1064 == local->last_id) local->last_id++;
	store_link_id(local);

	std::string slnk = idtostr(local->last_id++);
	lgc = createNode(BOND_NODE, slnk);
	Handle he = local->asp->add_link(EDGE_LINK, lgc, lnk);

	// Store, if there is storage.
	if (local->stnp)
		local->stnp->store_atom(he);

	return slnk;
}

// ===============================================================

// Get the last issued link ID from the data store.
void fetch_link_id(Local* local)
{
	if (not local->using_external_as) return;

	// Location where issued link ID's are stored.
	if (local->stnp)
		local->stnp->fetch_atom(local->idanch);

	const TruthValuePtr& tv = local->idanch->getTruthValue();
	const CountTruthValuePtr ctv(CountTruthValueCast(tv));
	if (ctv)
		local->last_id = std::round(ctv->get_count());
}

// Record the last unissued id.
void store_link_id(Local* local)
{
	if (not local->using_external_as) return;

	TruthValuePtr tvp(createCountTruthValue(1, 0, local->last_id));
	local->asp->set_truthvalue(local->idanch, tvp);

	// Store, if there is storage.
	if (local->stnp)
		local->stnp->store_atom(local->idanch);
}

// ===============================================================
#endif /* HAVE_ATOMESE */
