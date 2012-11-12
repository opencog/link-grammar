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
 * Convert mixed logical expressions into disjunctive normal form.
 * That is, convert a boolean logic expression (mixture of AND and OR
 * terms) into a disjunction of a list of conjoined terms.
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
		return new_or->flatten();
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
	return disjoin(new_or);
}


/**
 * Convert mixed logical expressions into conjunctive normal form.
 * That is, convert a boolean logic expression (mixture of AND and OR
 * terms) into a conjunction of a list of disjoined terms.
 *
 * The code below is almost the same as teh code above, but with AND
 * and OR exchanged with one-another.  (There are other differences
 * too, since optinal terms behave differently in such cases.)
 *
 * The primary user of this function is the parser, to convert the 
 * mixed-form dictionary entries into a simpler structure, thus
 * simplifying the parser algorithm.
 */
Atom* conjoin(Atom* mixed_form)
{
	AtomType intype = mixed_form->get_type();
	if ((OR != intype) && (AND != intype))
		return mixed_form;

	if (AND == intype)
	{
		And* junct = dynamic_cast<And*>(mixed_form);
		assert(junct, "conjoin: given a naked AND link!");

		junct = junct->flatten();

		size_t sz = junct->get_arity();
		OutList new_oset;

		// Just a recursive call, that's all.
		for(int i=0; i<sz; i++)
		{
			Atom* norm = conjoin(junct->get_outgoing_atom(i));
			new_oset.push_back(norm);
		}

		And* andy = new And(new_oset);
		andy = andy->flatten();
		return andy->clean();
	}

	Or* junct = dynamic_cast<Or*>(mixed_form);
	assert(junct, "conjoin: given a naked AND link!");

	junct = junct->flatten();

	// If we are here, the outgoing set is a conjunction of atoms.
	// Search for the first disjunction in that set, and distribute
	// over it.
	OutList front;
	size_t sz = junct->get_arity();
	int i;
	for(i=0; i<sz; i++)
	{
		Atom* a = conjoin(junct->get_outgoing_atom(i));
		AtomType t = a->get_type();
		if (AND == t)
			break;
		front.push_back(a);
	}

	/* If no disjunctions found, we are done */
	if (i == sz)
		return junct;

	Atom *andatom = junct->get_outgoing_atom(i);
	i++;

	OutList rest;
	for(; i<sz; i++)
	{
		Atom* norm = conjoin(junct->get_outgoing_atom(i));
		rest.push_back(norm);
	}

	And* anda = dynamic_cast<And*>(andatom);
	assert(anda, "Bad link type found during conjoin");

	// Distribute over the elements in OR-list
	OutList new_oset;
	sz = anda->get_arity();
	for (i=0; i<sz; i++)
	{
		OutList distrib;

		// Copy the front, without change.
		size_t jsz = front.size();
		for (int j=0; j<jsz; j++)
			distrib.push_back(front[j]);

		// insert one atom.
		distrib.push_back(anda->get_outgoing_atom(i));

		// Copy the rest.
		jsz = rest.size();
		for (int j=0; j<jsz; j++)
			distrib.push_back(rest[j]);

		Or *disj = new Or(distrib);
		new_oset.push_back(disj);
	}

	And* andy = new And(new_oset);
	andy = andy->flatten();
	andy = andy->clean();
	return conjoin(andy);
}


} // namespace viterbi
} // namespace link-grammar

