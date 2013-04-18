/*************************************************************************/
/* Copyright (c) 2012, 2013 Linas Vepstas <linasvepstas@gmail.com>       */
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
	OutList newset;
	size_t sz = get_arity();
	for (size_t i=0; i<sz; i++)
	{
		Atom* a = get_outgoing_atom(i);
		/* Copy without change, if types differ. */
		if (_type != a->get_type())
		{
			newset.push_back(a);
			continue;
		}

		/* Get rid of a level */
		Set* ora = dynamic_cast<Set*>(a);
		OutList fora = ora->flatset();
		size_t osz = fora.size();
		for (int j=0; j<osz; j++)
			newset.push_back(fora[j]);
	}

	return newset;
}

/// Recursively flatten everything that inherits from set.
/// This does preserve the type hierarchy: that is, types are respected, 
/// and the flattening only happens within a single type.  The only
/// exception to this is if the set contains just a single element,
/// in which case this returns that one element.
//
// XXX TODO: handling of TV's during flattening is probably broken !?!
Atom* Set::super_flatten() const
{
	size_t sz = get_arity();

	// If its a singleton, just return that. But super-flatten
	// it first!
	if (1 == sz)
	{
		Atom* a = get_outgoing_atom(0);
		Set* set = dynamic_cast<Set*>(a);
		if (!set)
			return a;
		return set->super_flatten();
	}

	OutList newset;
	for (size_t i=0; i<sz; i++)
	{
		Atom* a = get_outgoing_atom(i);
		Set* set = dynamic_cast<Set*>(a);

		/* Copy without change, if types differ. */
		/* But flatten it first, if it inherits from set. */
		if (get_type() != a->get_type())
		{
			if (set)
				newset.push_back(set->super_flatten());
			else
				newset.push_back(a);
			continue;
		}

		// type if this equals type of children.
		/* Get rid of a level */
		Atom* achld = set->super_flatten();
		Set* chld = dynamic_cast<Set*>(achld);
		if (!chld)
		{
			newset.push_back(achld);
			continue;
		}

		size_t csz = chld->get_arity();
		for (int j=0; j<csz; j++)
			newset.push_back(chld->get_outgoing_atom(j));
	}

	return  upcast(new Link(get_type(), newset, _tv));
}

/// Add (append) other set to this set.
Set* Set::add(const Set* other)
{
	if (!other) return this;
	if (0 == other->get_arity()) return this;

	OutList o = get_outgoing_set();
	const OutList& oth = other->get_outgoing_set();
	o.insert(o.end(), oth.begin(), oth.end());
	return new Set(o);
}

// ============================================================
/// Remove repeated entries.
Or* Or::uniq() const
{
	OutList uniq;
	size_t sz = get_arity();
	for (size_t i=0; i<sz; i++)
	{
		Atom* a = get_outgoing_atom(i);
		bool is_uniq = true;
		for (size_t j=i+1; j<sz; j++)
		{
			Atom* b = get_outgoing_atom(j);
			if (a->operator==(b))
			{
				is_uniq = false;
				break;
			}
		}

		if (is_uniq)
			uniq.push_back(a);
	}
	return new Or(uniq);
}

// ============================================================
/// Return disjunctive normal form (DNF)
///
/// Presumes that the oset is a nested list consisting
/// of And and Or nodes.  If the oset contains non-boolean
/// terms, these are left in place, unmolested.
///
/// XXX Note: this somewhat duplicates the function of the
/// disjoin() subroutine defined in disjoin.cc ...
/// Note: this one is unit-tested, the other is not.
/// Note, however, that one handles optional clauses; this does not.
Or* Or::disjoin() const
{
	OutList dnf;
	size_t sz = get_arity();
	for (size_t i=0; i<sz; i++)
	{
		Atom* a = get_outgoing_atom(i);
		AtomType ty = a->get_type();
		if (AND == ty)
		{
			And* al = dynamic_cast<And*>(upcast(a));
			Or* l = al->disjoin();
			for (size_t j=0; j<l->get_arity(); j++)
				dnf.push_back(l->get_outgoing_atom(j));
		}
		else if (OR == ty)
		{
			Or* ol = dynamic_cast<Or*>(upcast(a));
			Or* l = ol->disjoin();
			for (size_t j=0; j<l->get_arity(); j++)
				dnf.push_back(l->get_outgoing_atom(j));
		}
		else
			dnf.push_back(a);
	}
	return new Or(dnf);
}

/// Return disjunctive normal form (DNF)
///
/// Presumes that the oset is a nested list consisting
/// of And and Or nodes.  If the oset contains non-boolean
/// terms, these are left in place, unmolested.
///
/// Costs are distributed over disjuncts.
///
/// XXX Note: this somewhat duplicates the function of the
/// disjoin() subroutine defined in disjoin.cc ...
/// Note: this one is unit-tested, the other is not.
/// Note, however, that one handles optional clauses; this does not.
Or* And::disjoin()
{
	size_t sz = get_arity();
	if (0 == sz) return new Or(_tv);
	if (1 == sz)
	{
		if (OR == _oset[0]->get_type())
			return dynamic_cast<Or*>(upcast(_oset[0]));
		return new Or(_oset, _tv);
	}

	// Perhaps there is nothing to be done, because none
	// of the children are boolean operators.
	bool done = true;
	bool needs_flattening = false;
	for (size_t i=0; i<sz; i++)
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
		for (size_t i=0; i<sz; i++)
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
	Atom* last = *(ol->rbegin());
	ol->pop_back();
	And shorter(*ol);

	// recurse ...
	Or* stumpy = shorter.disjoin();

	// finally, distribute last elt back onto the end.
	OutList dnf;

	if (OR != last->get_type())
		last = new Or(last);

	Link* ll = dynamic_cast<Link*>(last);
	size_t jsz = ll->get_arity();
	for (size_t j=0; j<jsz; j++)
	{
		Atom* tail = ll->get_outgoing_atom(j);
		sz = stumpy->get_arity();

		// Costs distribute additively: AND over OR.
		TV cost = stumpy->_tv + _tv;
		for (size_t i=0; i<sz; i++)
		{
			Atom* a = stumpy->get_outgoing_atom(i);
			AtomType ty = a->get_type();
			if (AND == ty)
			{
				Link* l = dynamic_cast<Link*>(a);
				OutList al = l->get_outgoing_set();
				al.push_back(tail);
				dnf.push_back(new And(al, cost));
			}
			else
			{
				dnf.push_back(new And(a, tail, cost));
			}
		}
	}
	return new Or(dnf);
}

// ============================================================
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

// ============================================================

// Helper function for below.
static bool has_lefties(Atom* a)
{
	Connector* c = dynamic_cast<Connector*>(a);
	if (c)
	{
		if ('-' == c->get_direction())
			return true;
		return false;
	} 

	// Verify we've got a valid disjunct
	AtomType at = a->get_type();
	assert ((at == OR) or (at == AND), "Disjunct, expecting OR or AND");

	// Recurse down into disjunct
	Link* l = dynamic_cast<Link*>(a);
	size_t sz = l->get_arity();
	for (size_t i=0; i<sz; i++)
	{
		if (has_lefties(l->get_outgoing_atom(i)))
			return true;
	}
	return false;
}

/// Return true if any of the connectors in the cset point to the left.
bool WordCset::has_left_pointers() const
{
	return has_lefties(get_cset());
}

/// Simplify any gratuituousnesting in the cset.
WordCset* WordCset::flatten()
{
	// AND and OR inherit from Set
	Set* s = dynamic_cast<Set*>(get_cset());
	if (NULL == s)
		return this;

	Atom* flat = s->super_flatten();

	// If there is nothing left after flattening, return NULL.
	const Link* fl = dynamic_cast<const Link*>(flat);
	if (fl && 0 == fl->get_arity())
		return NULL;

	return new WordCset(get_word(), flat);
}

// ============================================================
Atom* upcast(const Atom* a)
{
	const Node* n = dynamic_cast<const Node*>(a);
	const Link* l = dynamic_cast<const Link*>(a);

	switch (a->get_type())
	{
		// Links
		case AND:
			return new And(l->get_outgoing_set(), l->_tv);
		case OR:
			return new Or(l->get_outgoing_set(), l->_tv);

		// Nodes
		case CONNECTOR:
			return new Connector(n->get_name(), n->_tv);

		default:
			assert(0, "upcast: implement me!");
	}
}

} // namespace viterbi
} // namespace link-grammar
