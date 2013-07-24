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

#include "compile-base.h"
#include "utilities.h"  // needed for assert

namespace atombase {


/// Flatten a set.  That is, remove the nested, recursive
/// structure of the set, and return all elements as just
/// one single set. For example, the flattened version of
/// {a, {b,c}} is {a, b, c}
///
/// Any costs (i.e. truth values) associated with the removed
/// levels are accumulated onto the children.
///
/// See also super_flatten(), which recursively flattens
/// anything deriving from class Set, recursively.
///
// Note that this algo, as currently implemented, is order-preserving.
// This is important for the Seq class, and the And class (since the
// link-grammar AND must be an ordered sequence, to preserve planarity
// of the parses.)
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
		const TV& cost = a->_tv;
		Set* ora = dynamic_cast<Set*>(upcast(a));
		OutList fora = ora->flatset();
		size_t osz = fora.size();
		for (int j=0; j<osz; j++)
		{
			fora[j]->_tv += cost;
			newset.push_back(fora[j]);
		}
	}

	return newset;
}

/// Recursively flatten everything that inherits from set.
/// This does preserve the type hierarchy: that is, types are respected,
/// and the flattening only happens within a single type.  The only
/// exception to this is if the set contains just a single element,
/// in which case this returns that one element.
//
Atom* Set::super_flatten() const
{
	size_t sz = get_arity();

	// If its a singleton, just return that. But super-flatten
	// it first!  Push the cost of this node down onto the child.
	if (1 == sz)
	{
		Atom* a = get_outgoing_atom(0);
		Set* set = dynamic_cast<Set*>(upcast(a));
		if (set)
			a = set->super_flatten();
		a->_tv += _tv;
		return a;
	}

	OutList newset;
	for (size_t i=0; i<sz; i++)
	{
		Atom* a = get_outgoing_atom(i);
		Set* set = dynamic_cast<Set*>(upcast(a));
		if (NULL == set)
		{
			newset.push_back(a);
			continue;
		}

		/* Copy without change, if types differ. */
		/* But flatten it first, if it inherits from set. */
		if (get_type() != a->get_type())
		{
			newset.push_back(set->super_flatten());
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

		// perform the actual flattening, distributing the cost
		// of the deleted atom onto its children.
		size_t csz = chld->get_arity();
		for (int j=0; j<csz; j++)
		{
			Atom* c = chld->get_outgoing_atom(j);
			c->_tv += a->_tv;
			newset.push_back(c);
		}
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
/// Costs are distributed over disjuncts.
///
/// XXX Note: this somewhat duplicates the function of the
/// disjoin() subroutine defined in disjoin.cc ...
/// Note: this one is unit-tested, the other is not.
/// Note, however, that one handles optional clauses; this does not.
Atom* Or::disjoin() const
{
	// Trying to disjoin anything that is not in a flattened
	// form is crazy-making.
	Atom* sfl = super_flatten();
	Set* fl = dynamic_cast<Set*>(sfl);
	if (NULL == fl) return sfl;

	// If the flattening discarded the top-level Or, deal with it.
	And* afl = dynamic_cast<And*>(sfl);
	if (afl) return afl->disjoin();

	// If we are not an Or link, then wtf!?
	assert(dynamic_cast<Or*>(sfl), "We are deeply confused disjoining!");

	OutList dnf;
	size_t sz = fl->get_arity();
	for (size_t i=0; i<sz; i++)
	{
		Atom* a = fl->get_outgoing_atom(i);
		AtomType ty = a->get_type();
		if (AND == ty)
		{
			And* al = dynamic_cast<And*>(upcast(a));
			Atom* a = al->disjoin();
			Link* l = dynamic_cast<Link*>(a);
			if (l)
			{
				for (size_t j=0; j<l->get_arity(); j++)
					dnf.push_back(l->get_outgoing_atom(j));
			}
			else
			{
				dnf.push_back(a);
			}
		}
		else if (OR == ty)
		{
assert(0, "not expecting Or after flattening");
#if 0
			Or* ol = dynamic_cast<Or*>(upcast(a));
			Or* l = ol->disjoin();
			TV cost = l->_tv;
			for (size_t j=0; j<l->get_arity(); j++)
			{
				// XXX We've got to distribute the cost, somehow, but
				// I don't really like bumping it like this ... its not
				// a pure play.
				Atom* aol = l->get_outgoing_atom(j);
				aol->_tv += cost;
				dnf.push_back(aol);
			}
#endif
		}
		else
			dnf.push_back(a);
	}
	return new Or(dnf, fl->_tv);
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
Atom* And::disjoin()
{
	// Trying to disjoin anything that is not in a flattened
	// form is crazy-making.
	Atom* sfl = super_flatten();
	Set* fl = dynamic_cast<Set*>(sfl);
	if (NULL == fl) return sfl;

	// If the flattening discarded the top-level And, deal with it.
	Or* orfl = dynamic_cast<Or*>(sfl);
	if (orfl) return orfl->disjoin();

	// If we are not an And link, then wtf!?
	assert(dynamic_cast<And*>(sfl), "And we are deeply confused!");

	size_t sz = fl->get_arity();
	if (0 == sz) return new Or(fl->_tv);
	if (1 == sz) return fl->get_outgoing_atom(0);

	// Perhaps there is nothing to be done, because none
	// of the children are boolean operators.
	bool done = true;
	bool needs_flattening = false;
	for (size_t i=0; i<sz; i++)
	{
		AtomType ty = fl->get_outgoing_atom(i)->get_type();
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
		return fl;
	}

	// First, disjoin any child nodes
	OutList* ol = new OutList(fl->get_outgoing_set());
#if 0
	for (size_t i=0; i<sz; i++)
	{
		Atom* a = ol->at(i);
		AtomType ty = a->get_type();
		if (OR == ty)
		{
			Or* oo = dynamic_cast<Or*>(upcast(a));
			(*ol)[i] = oo->disjoin();
		}
		else if (AND == ty)
		{
			And* aa = dynamic_cast<And*>(upcast(a));
			(*ol)[i] = aa->disjoin();
		}
	}
#endif

	// Next, flatten out any nested And's
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
	Atom* stumper = shorter.disjoin();
	Or* stumpy = dynamic_cast<Or*>(stumper);

	if (!stumpy)
	{
		// If we are here, then the front was just a single atom,
		// and the last element was (possibly) an OR-list. So
		// distribute the front onto the tail. (using AND, of course)
		OutList dnf;

		if (OR != last->get_type())
			last = new Or(last);

		// Costs distribute additively: AND over OR.
		TV cost = fl->_tv + last->_tv;

		Link* ll = dynamic_cast<Link*>(last);
		size_t jsz = ll->get_arity();
		for (size_t j=0; j<jsz; j++)
		{
			Atom* tail = ll->get_outgoing_atom(j);
			dnf.push_back(new And(stumper, tail, cost));
		}
		return new Or(dnf);
	}

	// finally, distribute last elt back onto the end.
	OutList dnf;

	if (OR != last->get_type())
		last = new Or(last);

	// Costs distribute additively: AND over OR.
	TV cost = stumpy->_tv + fl->_tv + last->_tv;

	Link* ll = dynamic_cast<Link*>(last);
	size_t jsz = ll->get_arity();
	size_t ssz = stumpy->get_arity();
	for (size_t j=0; j<jsz; j++)
	{
		Atom* tail = ll->get_outgoing_atom(j);

		for (size_t i=0; i<ssz; i++)
		{
			Atom* a = stumpy->get_outgoing_atom(i);
			AtomType ty = a->get_type();
			if (AND == ty)
			{
				Link* l = dynamic_cast<Link*>(a);
				OutList al = l->get_outgoing_set();
				al.push_back(tail);
				dnf.push_back(new And(al, cost + a->_tv));
			}
			else
			{
				dnf.push_back(new And(a, tail, cost));
			}
		}
	}
	return new Or(dnf);
}

} // namespace atombase
