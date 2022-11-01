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
#include <opencog/persist/sexpr/Sexpr.h>
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

/// Return true if the given word can be found in the dictionary,
/// else return false.
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

#if 0
// Unused. This can walk a HandleSeq backwards.
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
#endif

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

#if 0
// Unused just right now.

// This is a minimalist version of `lg_exp_stringify()` because
// we want the string, without the costs on it. We also know that
// it's just a list of and's with connectors.
static std::string prt_andex(Exp* e)
{
	std::string str;

	Exp* esave = e;

	// Validate that either it's a single connector or an AND-list.
	if (AND_type != e->type and CONNECTOR_type != e->type)
	{
		prt_error("Error: Unexpected expression type %s\n",
			lg_exp_stringify(e));
		return "";
	}

	e = e->operand_first;
	while (e)
	{
		// We expect a linked list of connectors to follow.
		if (CONNECTOR_type == e->type)
		{
			str += e->condesc->string;
			str += e->dir;
		}

		else if (OR_type == e->type)
		{
			str += "{";
			Exp* con = e->operand_first->operand_next;
			str += con->condesc->string;
			str += con->dir;
			str += "}";
		}
		else if (AND_type == e->type)
		{
			prt_error("Error: Unexpected AND expression %s\n",
				lg_exp_stringify(esave));
			return "";
		}

		// Walk the linked list.
		e = e->operand_next;
		if (e) str += " & ";
	}

	return str;
}

// XXX Unused right now. What was the plan, here?
static void cache_disjunct_string(Local* local,
                                  const Handle& sect, Exp* andex)
{
	local->asp->add_link(
		EVALUATION_LINK, local->djp,
		createNode(ITEM_NODE, prt_andex(andex)),
		sect);
}
#endif

// ===============================================================

Exp* make_sect_exprs(Dictionary dict, const Handle& germ)
{
	Local* local = (Local*) (dict->as_server);
	Exp* orhead = nullptr;
	Exp* ortail = nullptr;

// #define OPTIONAL_ANY_LINK
#ifdef OPTIONAL_ANY_LINK
	// Create a pair of optional ANY-links that can connect both left
	// and right. This is a stop-gap measure the case where there are
	// no suitable disjuncts on the word. Disabled for now, because this
	// is too blunt a tool; using explicit word-pairs is a better deal.
	Exp* an = make_connector_node(dict, dict->Exp_pool, "ANY", '-', false);
	Exp* on = make_optional_node(dict->Exp_pool, an);
	Exp* ap = make_connector_node(dict, dict->Exp_pool, "ANY", '+', false);
	Exp* op = make_optional_node(dict->Exp_pool, ap);
	ortail = make_and_node(dict->Exp_pool, on, op);
#endif // OPTIONAL_ANY_LINK

#define OPTIONAL_PAIRS
#ifdef OPTIONAL_PAIRS
	Exp* epr = make_pair_exprs(dict, germ);
	Exp* optex = make_optional_node(dict->Exp_pool, epr);
	Exp* optey = make_optional_node(dict->Exp_pool, epr);
	Exp* optez = make_optional_node(dict->Exp_pool, epr);
	ortail = make_and_node(dict->Exp_pool, optex, optey);
	optey->operand_next = optez;
printf("duuuude got %d pair exprs for %s\n", size_of_expression(epr),
germ->get_name().c_str());
#endif // OPTIONAL_PAIRS

	// Loop over all Sections on the word.
	HandleSeq sects = germ->getIncomingSetByType(SECTION);
	for (const Handle& sect: sects)
	{
		// Apprently, some sections are missing costs. This is
		// most likey due to some data-processing bug, where the
		// MI's were not recomputed. For nw, we will silently
		// ignore this issue, and assign a default to them.
		double cost = local->cost_default;

		const ValuePtr& mivp = sect->getValue(local->miks);
		if (mivp)
		{
			// MI is the second entry in the vector.
			const FloatValuePtr& fmivp = FloatValueCast(mivp);
			double mi = fmivp->value()[local->cost_index];
			cost = (local->cost_scale * mi) + local->cost_offset;
		}

		// If the cost is too high, just skip this.
		if (local->cost_cutoff <= cost)
			continue;

		Exp* andhead = nullptr;
		Exp* andtail = nullptr;

#ifdef OPTIONAL_ANY_LINK
		Exp* an = make_connector_node(dict, dict->Exp_pool, "ANY", '-', false);
		Exp* on = make_optional_node(dict->Exp_pool, an);
		Exp* ap = make_connector_node(dict, dict->Exp_pool, "ANY", '+', false);
		Exp* op = make_optional_node(dict->Exp_pool, ap);
		andhead = make_and_node(dict->Exp_pool, on, op);
		andtail = op;
#endif // OPTIONAL_ANY_LINK

#ifdef EXTRA_OPTIONAL_PAIRS
		Exp* optex = make_optional_node(dict->Exp_pool, epr);
		Exp* optey = make_optional_node(dict->Exp_pool, epr);
		// andhead = make_and_node(dict->Exp_pool, optex, NULL);
		andhead = make_and_node(dict->Exp_pool, optex, optey);
		andtail = optey;
#endif // EXTRA_OPTIONAL_PAIRS

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

			if (nullptr == andhead)
				andhead = make_and_node(dict->Exp_pool, eee, NULL);
			else
			if ('+' == cdir)
			{
				/* Link new connectors to the tail */
				if (nullptr == andtail)
				{
					andtail = andhead;
					eee->operand_next = andhead->operand_first;
					andhead->operand_first = eee;
				}
				else
				{
					andtail->operand_next = eee;
					andtail = eee;
				}
			}
			else if ('-' == cdir)
			{
				/* Link new connectors to the head */
				eee->operand_next = andhead->operand_first;
				andhead->operand_first = eee;

				if (nullptr == andtail)
					andtail = eee->operand_next;
			}
		}

		// This should never-ever happen!
		if (nullptr == andhead)
		{
			prt_error("Warning: Empty connector sequence for %s\n",
				sect->to_short_string().c_str());
			continue;
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

		// If there are two or more and-expressions,
		// they get appended to a list hanging on a single OR-node.
		if (ortail)
		{
			if (nullptr == orhead)
				orhead = make_or_node(dict->Exp_pool, ortail, andhead);
			else
				ortail->operand_next = andhead;
		}
		ortail = andhead;
	}

	if (nullptr == orhead) return ortail;
	return orhead;
}

#if LATER
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

	// Get expressions, where the word itself is the germ.
	Exp* exp = make_exprs(dict, wrd);

	// Get expressions, where the word is in some class.
	for (const Handle& memb : wrd->getIncomingSetByType(MEMBER_LINK))
	{
		const Handle& wcl = memb->getOutgoingAtom(1);
		if (WORD_CLASS_NODE != wcl->get_type()) continue;

		Exp* clexp = make_exprs(dict, wcl);
		if (nullptr == clexp) continue;

		lgdebug(+D_SPEC+5, "as_lookup_list class for >>%s<< nexpr=%d\n",
			ssc, size_of_expression(clexp));

		if (nullptr == exp)
			exp = clexp;
		else
			exp = make_or_node(dict->Exp_pool, exp, clexp);
	}

	if (nullptr == exp)
		return nullptr;

	dn = (Dict_node*) malloc(sizeof(Dict_node));
	memset(dn, 0, sizeof(Dict_node));
	dn->string = ssc;
	dn->exp = exp;

	// Cache the result; avoid repeated lookups.
	dict->root = dict_node_insert(dict, dict->root, dn);
	dict->num_entries++;

	lgdebug(+D_SPEC+5, "as_lookup_list %d for >>%s<< nexpr=%d\n",
		dict->num_entries, ssc, size_of_expression(exp));

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
#endif // LATER

#endif /* HAVE_ATOMESE */
