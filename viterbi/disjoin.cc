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

/// Flatten a set by pulling up all singletons.
/// This is a valid operation for link-grammar AND, OR sequences/sets.
/// Since such singleton sets are the same as the contained element.
static Atom* super_flatten(Atom* in)
{
	Set* set = dynamic_cast<Set*>(in);
	if (!set)
		return in;

	size_t sz = set->get_arity();
	if (1 == sz) return set->get_outgoing_atom(0);

	OutList newlist;
	for (int i=0; i<sz; i++)
	{
		Atom* a = set->get_outgoing_atom(i);
		Link* l = dynamic_cast<Link*>(a);
		if (l and (1 == l->get_arity()))
			newlist.push_back(l->get_outgoing_atom(0));
		else
			newlist.push_back(a);
	}
	return upcast(new Link(set->get_type(), newlist));
}

/**
 * Convert mixed connector expressions into disjunctive normal form.
 * The final form will consist of disjunction of conjunctions of
 * connectors.
 *
 * Note that connector expressions resemble Boolean logic terms, but
 * they are not actually Boolean logic; each connector can be used
 * once, and only once.  Thus, connector expressions are OR-distributive,
 * but not AND-distributive.  Thus, (A & (B or C)) = ((A & B) or (A & C))
 * but it is not at all the case that (A or (B & C)) is the same as
 * ((A or B) & (A or C)) because connectors cannot be duplicated!
 * That is, the logic of link-grammar connectors is that of linear
 * logic (monoidal categories) not boolean logic (cartesian categories).
 *
 * The primary user of this function is the parser, to convert the 
 * mixed-form dictionary entries into a simpler structure, thus
 * simplifying the parser algorithm.
 */

Atom* disjoin(Atom* mixed_form)
{
	AtomType intype = mixed_form->get_type();
	if ((OR != intype) && (AND != intype))
		return mixed_form;

	if (OR == intype)
	{
		Or* junct = dynamic_cast<Or*>(mixed_form);
		assert(junct, "disjoin: given a naked OR link!");

		junct = junct->flatten();

		size_t sz = junct->get_arity();
		OutList new_oset;

		// Just a recursive call, that's all.
		for(int i=0; i<sz; i++)
		{
			Atom* norm = disjoin(junct->get_outgoing_atom(i));
			new_oset.push_back(norm);
		}

		Or* new_or = new Or(new_oset);
		return super_flatten(new_or->flatten());
	}

	And* junct = dynamic_cast<And*>(mixed_form);
	assert(junct, "disjoin: given a naked AND link!");

	junct = junct->flatten();

	// If we are here, the outgoing set is a conjunction of atoms.
	// Search for the first disjunction in that set, and distribute
	// over it.
	OutList front;
	size_t sz = junct->get_arity();
	int i;
	for(i=0; i<sz; i++)
	{
		Atom* a = disjoin(junct->get_outgoing_atom(i));
		AtomType t = a->get_type();
		if (OR == t)
			break;
		front.push_back(a);
	}

	/* If no disjunctions found, we are done */
	if (i == sz)
		return junct;

	Atom *orat = junct->get_outgoing_atom(i);
	i++;

	OutList rest;
	for(; i<sz; i++)
	{
		Atom* norm = disjoin(junct->get_outgoing_atom(i));
		rest.push_back(norm);
	}

	Or* orn = dynamic_cast<Or*>(orat);
	assert(orn, "Bad link type found during disjoin");

	// Distribute over the elements in OR-list
	OutList new_oset;
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
		andy = andy->clean();
		new_oset.push_back(andy);
	}

	Or* new_or = new Or(new_oset);
	new_or = new_or->flatten();
	return super_flatten(disjoin(new_or));
}


} // namespace viterbi
} // namespace link-grammar

