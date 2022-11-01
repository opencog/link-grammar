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
#include "lookup-atomese.h"
};

#include "dict-atomese.h"
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

		// Get the cached link-name for this pair.
		const std::string& slnk = cached_linkname(local, rawpr);

		// Direction is easy to determine: its either left or right.
		char cdir = '+';
		if (rawpr->getOutgoingAtom(1) == germ) cdir  = '-';

		// Create a connector, and make it optional.
		Exp* eee = make_connector_node(dict,
			dict->Exp_pool, slnk.c_str(), cdir, false);

		Exp* optex = make_optional_node(dict->Exp_pool, eee);

		// Use an OR-node to create a linked list of expressions.
		if (ortail)
		{
			if (nullptr == orhead)
				orhead = make_or_node(dict->Exp_pool, ortail, optex);
			else
				ortail->operand_next = optex;
		}
		ortail = optex;
	}

	return orhead;
}

#endif // HAVE_ATOMESE
