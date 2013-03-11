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

namespace link_grammar {
namespace viterbi {


/// Flatten a set.  That is, remove the nested, recursive
/// structure of the set, and return all elements as just
/// one single set. For example, the flattened version of
/// {a, {b,c}} is {a, b, c}
///
// Note that this algo, as currently implemented, is order-preserving.
// This is important for the Seq class, and the And class (since the
// link-grammar AND must be an ordered sequence, to preserve planarity
// of the parses.  
OutList Set::flatset() const
{
	size_t sz = _oset.size();
	OutList newset;
	for (int i=0; i<sz; i++)
	{
		/* Copy without change, if types differ. */
		if (_type != _oset[i]->get_type())
		{
			newset.push_back(_oset[i]);
			continue;
		}

		/* Get rid of a level */
		Set* ora = dynamic_cast<Set*>(_oset[i]);
		OutList fora = ora->flatset();
		size_t osz = fora.size();
		for (int j=0; j<osz; j++)
			newset.push_back(fora[j]);
	}

	return newset;
}

/// Remove optional connectors.
///
/// It doesn't make any sense at all to have an optional connector
/// in an AND-clause, so just remove it.  (Well, OK, it "makes sense",
/// its just effectively a no-op, and so doesn't have any effect.  So,
/// removing it here simplifies logic in other places.)
And* And::clean() const
{
	OutList cl;
	size_t sz = _oset.size();
	for (int i=0; i<sz; i++)
	{
		Connector* cn = dynamic_cast<Connector*>(_oset[i]);
		if (cn and cn->is_optional())
			continue;

		cl.push_back(_oset[i]);
	}

	return new And(cl);
}

/// Return disjunctive normal form (DNF)
///
/// Presumes that the oset is a nested list consisting
/// of And and Or nodes.  If the oset contains non-boolean
/// terms, these are left in place, unmolested.
Or* And::disjoin()
{
	size_t sz = get_arity();
	if (0 == sz) return new Or();
	if (1 == sz) return new Or(_oset);

	// Perhaps there is nothing to be done, because none
	// of the children are boolean operators.
	bool done = true;
	bool needs_flattening = false;
	for (int i=0; i<sz; i++)
	{
		AtomType ty = _oset[i]->get_type();
		if (AND == ty)
		{
			done = false;
			needs_flattening = true;
		}
		else if (OR == ty)
			done = false;
	}
	if (done)
	{
		return new Or(this);
	}

	// First, flatten out any nested And's
	OutList* ol = new OutList(_oset);
	while (needs_flattening)
	{
		bool did_flatten = false;
		OutList* flat = new OutList();
		for (int i=0; i<sz; i++)
		{
			Atom* a = ol->at(i);
			AtomType ty = a->get_type();
			if (AND == ty)
			{
				did_flatten = true;
				Link* l = dynamic_cast<Link*>(a);
				for (size_t j=0; j<l->get_arity(); j++)
					flat->push_back(l->get_outgoing_atom(j));
			}
			else
				flat->push_back(a);
		}

		if (not did_flatten) break;
		ol = flat;
	}

	// Get the last element off of the list of and'ed terms
	Atom* last = *(ol->rend());
	ol->pop_back();
	And shorter(*ol);

	// recurse ...
	Or* stumpy = shorter.disjoin();

	// finally, distribute last elt back onto the end.
	OutList dnf;
	sz = stumpy->get_arity();
	for (int i=0; i<sz; i++)
	{
		Atom* a = stumpy->get_outgoing_atom(i);
		AtomType ty = a->get_type();
		if (AND == ty)
		{
			Link* l = dynamic_cast<Link*>(a);
			OutList al = l->get_outgoing_set();
			al.push_back(last);
			dnf.push_back(new And(al));
		}
		else
			dnf.push_back(a);
	}
	return new Or(dnf);
}


Atom* upcast(const Atom* a)
{
	const Node* n = dynamic_cast<const Node*>(a);
	const Link* l = dynamic_cast<const Link*>(a);

	switch (a->get_type())
	{
		// Links
		case AND:
			return new And(l->get_outgoing_set());
		case OR:
			return new Or(l->get_outgoing_set());

		// Nodes
		case CONNECTOR:
			return new Connector(n->get_name());

		default:
			assert(0, "upcast: implement me!");
	}
}

} // namespace viterbi
} // namespace link-grammar
