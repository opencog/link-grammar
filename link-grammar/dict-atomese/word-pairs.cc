/*
 * word-pairs.cc
 *
 * Create disjuncts consisting of optionals, from word-pairs.
 *
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 */
#ifdef HAVE_ATOMESE

#include <opencog/atomspace/AtomSpace.h>
#undef STRINGIFY

extern "C" {
#include "../link-includes.h"            // For Dictionary
#include "../dict-common/dict-common.h"  // for Dictionary_s
#include "../dict-common/dict-utils.h"   // for size_of_expression()
#include "../dict-ram/dict-ram.h"
};

#include "local-as.h"

using namespace opencog;

static size_t fetch_pairs(Local* local, const Handle& germ)
{
	local->stnp->fetch_incoming_by_type(germ, LIST_LINK);
	local->stnp->barrier();

	size_t cnt = 0;
	HandleSeq rprs = germ->getIncomingSetByType(LIST_LINK);
	for (const Handle& rawpr : rprs)
	{
		local->stnp->fetch_incoming_by_type(rawpr, EVALUATION_LINK);

		// XXX TODO validate that the EvaluationLink has
		// (LgLinkNode "ANY") in the first location.
		cnt++;
	}
	return cnt;
}

/// Create a list of connectors, one for each available word pair
/// containing the word in the germ. These are simply OR'ed together.
Exp* make_pair_exprs(Dictionary dict, const Handle& germ)
{
	Local* local = (Local*) (dict->as_server);
	const AtomSpacePtr& asp = local->asp;
	Exp* orhead = nullptr;
	Exp* ortail = nullptr;

	fetch_pairs(local, germ);

	const Handle& hany = local->lany; // (LG_LINK_NODE, "ANY");

	HandleSeq rprs = germ->getIncomingSetByType(LIST_LINK);
	for (const Handle& rawpr : rprs)
	{
		Handle evpr = asp->get_link(EVALUATION_LINK, hany, rawpr);
		if (nullptr == evpr) continue;

		double cost = local->pair_default;
		const ValuePtr& mivp = evpr->getValue(local->mikp);
		if (mivp)
		{
			// MI is the second entry in the vector.
			const FloatValuePtr& fmivp = FloatValueCast(mivp);
			double mi = fmivp->value()[local->pair_index];
			cost = (local->pair_scale * mi) + local->pair_offset;
		}
		// If the cost is too high, just skip this.
		if (local->pair_cutoff <= cost)
			continue;

		// Get the cached link-name for this pair.
		const std::string& slnk = cached_linkname(local, rawpr);

		// Direction is easy to determine: its either left or right.
		char cdir = '+';
		if (rawpr->getOutgoingAtom(1) == germ) cdir  = '-';

		// Create the connector
		Exp* eee = make_connector_node(dict,
			dict->Exp_pool, slnk.c_str(), cdir, false);

		eee->cost = cost;

		// Use an OR-node to create a linked list of expressions.
		if (ortail)
		{
			if (nullptr == orhead)
				orhead = make_or_node(dict->Exp_pool, ortail, eee);
			else
				ortail->operand_next = eee;
		}
		ortail = eee;
	}

	return orhead;
}

// ===============================================================

/// Create exprs that consist of a Cartesian product of pairs.
/// Given a word, a lookup is made to find all word-pairs holding
/// that word. This is done by `make_pair_exprs()`, above. Then
/// this is ANDED against itself N times, and the result is returned.
/// The N is the `arity` argument.
///
/// For example, if `make_pair_exprs()` returns `(A+ or B- or C+)`
/// and arity is 3, then this will return `(A+ or B- or C+ or ())
/// and (A+ or B- or C+ or ()) and (A+ or B- or C+ or ())`. When
/// this is exploded into disjuncts, any combination is possible,
/// from size zero to three. That's why its a Cartesian product.
Exp* make_cart_pairs(Dictionary dict, const Handle& germ, int arity)
{
	Exp* andhead = nullptr;
	Exp* andtail = nullptr;

	Exp* epr = make_pair_exprs(dict, germ);
	if (nullptr == epr) return nullptr;

	Exp* optex = make_optional_node(dict->Exp_pool, epr);
	andhead = make_and_node(dict->Exp_pool, optex, NULL);
	andtail = andhead->operand_first;

	for (int i=1; i< arity; i++)
	{
		Exp* opt = make_optional_node(dict->Exp_pool, epr);
		andtail->operand_next = opt;
		andtail = opt;
	}

	// Could verify that it all multiplies out as expected.
	// lg_assert(arity * size_of_expression(epr) ==
	//           size_of_expression(andhead));

	return andhead;
}

// ===============================================================

/// Create exprs that are cartesian products of ANY links. The
/// corresponding disjuncts will have `arity` number of connectors.
/// If these are used all by themselves, the resulting parses will
/// be random planar graphs; i.e. will be equivalent to the `any`
/// language parses.
Exp* make_any_exprs(Dictionary dict, int arity)
{
	if (arity <= 0) return nullptr;

	// Create a pair of ANY-links that can connect either left or right.
	Exp* aneg = make_connector_node(dict, dict->Exp_pool, "ANY", '-', false);
	Exp* apos = make_connector_node(dict, dict->Exp_pool, "ANY", '+', false);

	Local* local = (Local*) (dict->as_server);
	aneg->cost = local->any_default;
	apos->cost = local->any_default;

	Exp* any = make_or_node(dict->Exp_pool, aneg, apos);
	Exp* optex = make_optional_node(dict->Exp_pool, any);

	Exp* andhead = nullptr;
	Exp* andtail = nullptr;

	andhead = make_and_node(dict->Exp_pool, optex, NULL);
	andtail = andhead->operand_first;

	for (int i=1; i< arity; i++)
	{
		Exp* opt = make_optional_node(dict->Exp_pool, any);
		andtail->operand_next = opt;
		andtail = opt;
	}

	return andhead;
}

#endif // HAVE_ATOMESE
