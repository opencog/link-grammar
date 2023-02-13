/*
 * sections.cc
 *
 * Translate AtomSpace Sections to LG Exp's.
 *
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 */
#ifdef HAVE_ATOMESE

#include <cstdlib>
#include <opencog/atomspace/AtomSpace.h>
#include <opencog/persist/api/StorageNode.h>
#include <opencog/persist/cog-storage/CogStorage.h>
#include <opencog/persist/file/FileStorage.h>
#include <opencog/persist/rocks/RocksStorage.h>
#include <opencog/nlp/types/atom_types.h>

#undef STRINGIFY

extern "C" {
#include "../link-includes.h"            // For Dictionary
#include "../dict-common/dict-common.h"  // for Dictionary_s
#include "../dict-common/dict-utils.h"   // for size_of_expression()
#include "../dict-ram/dict-ram.h"
#include "lookup-atomese.h"
};

#include "dict-atomese.h"
#include "local-as.h"

using namespace opencog;

// ===============================================================

/// Count the number of times that the `germ` appears in some
/// Section.
static size_t count_sections(Local* local, const Handle& germ)
{
	// Are there any Sections in the local atomspace?
	size_t nsects = germ->getIncomingSetSizeByType(SECTION);
	if (0 < nsects or nullptr == local->stnp) return nsects;

	local->stnp->fetch_incoming_by_type(germ, SECTION);
	local->stnp->barrier();
	nsects = germ->getIncomingSetSizeByType(SECTION);

	return nsects;
}

/// Return true if the given word can be found as the germ of some
/// Section, else return false. As a side-effect, Sections are loaded
/// from storage.
bool section_boolean_lookup(Dictionary dict, const char *s)
{
	Local* local = (Local*) (dict->as_server);
	Handle wrd = local->asp->add_node(WORD_NODE, s);

	// Are there any Sections for this word in the local atomspace?
	size_t nwrdsects = count_sections(local, wrd);

	// Does this word belong to any classes?
	size_t nclass = wrd->getIncomingSetSizeByType(MEMBER_LINK);
	if (0 == nclass and local->stnp)
	{
		local->stnp->fetch_incoming_by_type(wrd, MEMBER_LINK);
		local->stnp->barrier();
	}

	size_t nclssects = 0;
	for (const Handle& memb : wrd->getIncomingSetByType(MEMBER_LINK))
	{
		const Handle& wcl = memb->getOutgoingAtom(1);
		if (WORD_CLASS_NODE != wcl->get_type()) continue;
		nclssects += count_sections(local, wcl);
	}

	lgdebug(+D_SPEC+5,
		"section_boolean_lookup for >>%s<< found class=%lu nsects=%lu %lu\n",
		s, nclass, nwrdsects, nclssects);

	return 0 != (nwrdsects + nclssects);
}

// ===============================================================
/// Mapping LG connectors to AtomSpace connectors, and v.v.
///
/// An LG connector is a string of capital letters. It's generated on
/// the fly. It's associated with a `(Set (Word ..) (Word ...))`. There
/// are two lookup tasks. In this blob of code, when given the Set, we
/// need to find the matching LG string. For users of LG, we have the
/// inverse problem: given the LG string, what's the Set?
///
/// As of just right now, the best solution seems to be the traditional
/// one:
///
///    (Evaluation (Predicate "*-LG connector string-*")
///       (LgConnNode "ABC") (Set (Word ..) (Word ...)))
///
/// As of just right now, the above is held only in the local AtomSpace;
/// it is never written back to storage.

/// int to base-26 capital letters. Except it has gaps in that
/// sequence, and the order is reversed. Whatever. Doesn't matter.
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

/// Given a `(List (Word ...) (Word ...))` denoting a link, find the
/// corresponding LgConnNode, if it exists.
static Handle get_lg_conn(const Handle& wset, const Handle& key)
{
	for (const Handle& ev : wset->getIncomingSetByType(EVALUATION_LINK))
	{
		if (ev->getOutgoingAtom(0) == key)
			return ev->getOutgoingAtom(1);
	}
	return Handle::UNDEFINED;
}

/// Return a cached LG-compatible link string.
/// Assigns a new name, if one does not exist.
/// The Handle `lnk` is an ordered pair, left-right, two words/classes.
/// That is, `lnk` is `(List (Word ...) (Word ...))`
/// The link name is stored in an LgLinkNode, having the format
///
///     (Evaluation (Predicate "*-LG connector string-*")
///              (LgLinkNode "ASDF") (List (Word ...) (Word ...)))
///
std::string cached_linkname(Local* local, const Handle& lnk)
{
	// If we've already cached a connector string for this,
	// just return it.  Else build and cache a string.
	Handle lgc = get_lg_conn(lnk, local->linkp);
	if (lgc)
		return lgc->get_name();

	static uint64_t lid = 0;

	// idtostr(16562) is "ANY" and we want to reserve "ANY"
	if (16562 == lid) lid++;
	std::string slnk = idtostr(lid++);

	lgc = createNode(LG_LINK_NODE, slnk);
	local->asp->add_link(EVALUATION_LINK, local->linkp, lgc, lnk);
	return slnk;
}

/// Return a cached LG-compatible link string.
/// Assigns a new name, if one does not exist.
/// `germ` is the germ of the section, `ctcr` is one of the connectors.
static std::string get_linkname(Local* local, const Handle& germ,
                                const Handle& ctcr)
{
	// The connection target is the first Atom in the connector
	const Handle& tgt = ctcr->getOutgoingAtom(0);
	const Handle& dir = ctcr->getOutgoingAtom(1);
	const std::string& sdir = dir->get_name();

	// The link is the connection of both of these.
	if ('+' == sdir[0])
	{
		const Handle& lnk = local->asp->add_link(LIST_LINK, germ, tgt);
		return cached_linkname(local, lnk);
	}

	// Else direction is '-' and tgt comes before germ.
	const Handle& lnk = local->asp->add_link(LIST_LINK, tgt, germ);
	return cached_linkname(local, lnk);
}

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
		// Assign an upper-case name to the link.
		std::string slnk = get_linkname(local, germ, ctcr);

		const Handle& dir = ctcr->getOutgoingAtom(1);
		const std::string& sdir = dir->get_name();

		if (not first) printf("& ");
		first = false;
		printf("%s%c ", slnk.c_str(), sdir[0]);
	}
	printf("\n");
	fflush(stdout);
}

// ===============================================================

Exp* make_sect_exprs(Dictionary dict, const Handle& germ)
{
	Local* local = (Local*) (dict->as_server);
	Exp* orhead = nullptr;
	Exp* extras = nullptr;

	// Create some optional word-pair links; these may be nullptr's.
	if (0 < local->extra_pairs)
	{
// We need to restructure the code to pass in the sentence-words,
// and then also to not cahce the resultiong exprs, because they
// will be sentence-specific. This is doable, but just ... not right
// now. Maybe later, after we get the basics down.
HandleSeq sent_words;
assert(0, "Not supported yet!");
		extras = make_cart_pairs(dict, germ, nullptr, sent_words,
		                         local->extra_pairs,
		                         local->extra_any);
	}

	// Loop over all Sections on the word.
	HandleSeq sects = germ->getIncomingSetByType(SECTION);
	for (const Handle& sect: sects)
	{
		// Apparently, some sections are missing MI's. This is
		// most likey due to some data-processing bug, where the
		// MI's were not recomputed. For now, we will silently
		// ignore this issue, and assign a default to them.
		double mi = local->cost_default;

		const ValuePtr& mivp = sect->getValue(local->miks);
		if (mivp)
		{
			// MI is the second entry in the vector.
			const FloatValuePtr& fmivp = FloatValueCast(mivp);
			mi = fmivp->value()[local->cost_index];
		}

		// If the MI is too low, just skip this.
		if (mi < local->cost_cutoff)
			continue;

		Exp* andhead = nullptr;
		Exp* andtail = nullptr;

		// The connector sequence the second Atom.
		// Loop over the connectors in the connector sequence.
		// The atomspace stores connectors in word-order, while LG
		// stores them as distance-from given word.  Thus, we have
		// to reverse the order when building the expression.
		const Handle& conseq = sect->getOutgoingAtom(1);
		for (const Handle& ctcr : conseq->getOutgoingSet())
		{
			// The direction is the second Atom in the connector
			const Handle& dir = ctcr->getOutgoingAtom(1);
			char cdir = dir->get_name().c_str()[0];

			/* Assign an upper-case name to the link. */
			const std::string& slnk = get_linkname(local, germ, ctcr);

			Exp* eee = make_connector_node(dict,
				dict->Exp_pool, slnk.c_str(), cdir, false);

			if ('+' == cdir)
				and_enchain_right(dict->Exp_pool, andhead, andtail, eee);
			else if ('-' == cdir)
				and_enchain_left(dict->Exp_pool, andhead, andtail, eee);
		}

		// Sanity-check the Section - this should never-ever happen!
		if (nullptr == andhead)
		{
			prt_error("Warning: Empty connector sequence for %s\n",
				sect->to_short_string().c_str());
			continue;
		}

		// Tack on extra connectors, as configured.
		if (extras)
		{
			Exp* optex = make_optional_node(dict->Exp_pool, extras);
			and_enchain_left(dict->Exp_pool, andhead, andtail, optex);
			optex = make_optional_node(dict->Exp_pool, extras);
			and_enchain_right(dict->Exp_pool, andhead, andtail, optex);
		}

		// Optional: shorten the expression,
		// if there's only one connector in it.
		if (nullptr == andhead->operand_first->operand_next)
		{
			Exp *tmp = andhead->operand_first;
			andhead->operand_first = NULL;
			// pool_free(dict->Exp_pool, andhead); // not needed!?
			andhead = tmp;
		}

		double cost = (local->cost_scale * mi) + local->cost_offset;
		andhead->cost = cost;

		// Save the exp-section pairing in the AtomSpace.
		// XXX Why did we think this was needed ??
		// What were we going to do with it?
		// cache_disjunct_string(local, sect, andhead);

#if DEBUG
		print_section(dict, sect);
		const char* wrd = germ->get_name().c_str();
		printf("Word: '%s'  Exp: %s\n", wrd, lg_exp_stringify(andhead));
#endif

		// Add it to the linked list.
		or_enchain(dict->Exp_pool, orhead, andhead);
	}

	return orhead;
}

#endif /* HAVE_ATOMESE */
