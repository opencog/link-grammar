/*************************************************************************/
/* Copyright (c) 2012 Linas Vepstas <linasvepstas@gmail.com>             */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the Viterbi parsing system is subject to the terms of the      */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "compile.h"
#include "disjoin.h"

namespace link_grammar {
namespace viterbi {

/**
 * Convert dictionary-normal form into disjunctive normal form.
 * That is, convert the mixed-form dictionary entries into a disjunction
 * of a list of conjoined connectors.  The goal of this conversion is to
 * simplify the parsing algorithm.
 */

Atom* disjoin(Atom* mixed_form)
{
	AtomType intype = mixed_form->get_type();
	if ((OR != intype) && (AND != intype))
		return mixed_form;

	Set* junct = dynamic_cast<Set*>(mixed_form);
	junct = junct->flatten();

	size_t sz = junct->get_arity();
	OutList new_oset;

	if (OR == intype)
	{
		// Just a recursive call, that's all.
		for(int i=0; i<sz; i++)
		{
			Atom* norm = disjoin(junct->get_outgoing_atom(i));
			new_oset.push_back(norm);
		}
	}
	else
	{
		// If we are here, the outgoing set is a conjunction of atoms.
		// Search for the first disjunction in that set, and distribute
		// over it.
		OutList front;
		int i;
		for(i=0; i<sz; i++)
		{
			Atom* a = junct->get_outgoing_atom(i);
			AtomType t = a->get_type();
			if (OR == t)
				break;
			front.push_back(a);
		}

		/* If no disjunctions found, we are done */
		if (i == sz)
			return mixed_form;

		Atom *orat = junct->get_outgoing_atom(i);

		OutList rest;
		for(; i<sz; i++)
		{
			Atom* a = junct->get_outgoing_atom(i);
			rest.push_back(a);
		}

		Or* orn = dynamic_cast<Or*>(orat);
		assert(orn, "Bad link type found during disjoin");

		// Distribute over the elements in OR-list
		sz = orn->get_arity();
		for (i=0; i<sz; i++)
		{
			OutList distrib;

			// Copy the front, without change.
			size_t jsz = front.size();
			for (int j=0; j<jsz; j++)
				distrib.push_back(front[j]);

			// insert one atom.
			distrib.push_back(orn->get_outgoing_atom(i));

			// Copy the rest.
			jsz = rest.size();
			for (int j=0; j<jsz; j++)
				distrib.push_back(rest[j]);

			And *andy = new And(distrib);
			new_oset.push_back(andy);
		}
	}
	Or* new_or = new Or(new_oset);
	return new_or->flatten();
}


} // namespace viterbi
} // namespace link-grammar

