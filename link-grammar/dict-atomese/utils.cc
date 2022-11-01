/*
 * utils.cc
 *
 * Assorted AtomSpace and LG Exp utils.
 *
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 */
#ifdef HAVE_ATOMESE

#include <cstdlib>
#include <opencog/atomspace/AtomSpace.h>
#include <opencog/nlp/types/atom_types.h>

#undef STRINGIFY

extern "C" {
#include "../link-includes.h"            // For Dictionary
#include "../dict-common/dict-common.h"  // for Dictionary_s
#include "../dict-common/dict-utils.h"   // for size_of_expression()
#include "lookup-atomese.h"
};

#include "local-as.h"

using namespace opencog;


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

#endif /* HAVE_ATOMESE */
