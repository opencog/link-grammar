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

#ifndef _LG_VITERBI_COMPILE_H
#define _LG_VITERBI_COMPILE_H

#include "atom.h"

namespace link_grammar {
namespace viterbi {

// Classes that convert run-time atom types into compile-time static
// types, so that the compiler can check these for correctness.
// These are here purely for C++ programming convenience; the true
// structure that matters is the dynamic run-time (hyper-)graphs.


/// Unordered sequence
/// A Set inherits fom Link, and is an unordered set of zero or more
/// atoms.
class Set : public Link
{
	public:
		Set(const OutList& ol, const TV& tv = TV())
			: Link(SET, ol, tv)
		{}
		Set(Atom* singleton, const TV& tv = TV())
			: Link(SET, OutList(1, singleton), tv)
		{}
		Set(const TV& tv = TV())
			: Link(SET, tv)
		{}

		// See the C file for documentation
		Set* flatten() const { return new Set(flatset(), _tv); }
		Atom* super_flatten() const;

		// Add (append) other set  to this set.
		Set* add(const Set*);

      virtual Set* clone() const { return new Set(*this); }
protected:
		/// The sole purpose of this ctor is to allow inheritance.
		Set(AtomType t, const TV& tv = TV())
			: Link(t, tv)
		{}
		Set(AtomType t, const OutList& oset, const TV& tv = TV())
			: Link(t, oset, tv)
		{}
		OutList flatset() const;
};

/// Ordered sequence
/// Seq inherits from Set, and is an ordered sequence of zero or more
/// atoms.
class Seq : public Set
{
	public:
		Seq()
			: Set(SEQ)
		{}
		Seq(const OutList& ol, const TV& tv = TV())
			: Set(SEQ, ol, tv)
		{}
		Seq(Atom* singleton)
			: Set(SEQ, OutList(1, singleton))
		{}
		Seq(Atom* a, Atom* b)
			: Set(SEQ, ({OutList o(1,a); o.push_back(b); o;}))
		{}

		// See the Set class for documentation
		Seq* flatten() const { return new Seq(flatset(), _tv); }

      virtual Seq* clone() const { return new Seq(*this); }
protected:
		/// The sole purpose of this ctor is to allow inheritance.
		Seq(AtomType t)
			: Set(t)
		{}
		Seq(AtomType t, const OutList& oset, const TV& tv = TV())
			: Set(t, oset, tv)
		{}
};

/// Unordered OR of all children
class Or : public Set
{
	public:
		Or(const TV& tv = TV())
			: Set(OR, tv)
		{}
		Or(const OutList& ol, const TV& tv = TV())
			: Set(OR, ol, tv)
		{}
		Or(Atom* singleton, const TV& tv = TV())
			: Set(OR, OutList(1, singleton), tv)
		{}
		Or(Atom* a, Atom* b, const TV& tv = TV())
			: Set(OR, ({OutList o(1,a); o.push_back(b); o;}), tv)
		{}
		Or(Atom* a, Atom* b, Atom* c, const TV& tv = TV())
			: Set(OR, ({OutList o(1,a); o.push_back(b); o.push_back(c); o;}), tv)
		{}

		// Return disjunctive normal form (DNF)
		Atom* disjoin() const;

		// See the Set class for documentation
		Or* flatten() const { return new Or(flatset(), _tv); }

		// Remove repeated entries
		Or* uniq() const;

      virtual Or* clone() const { return new Or(*this); }
};

/// Ordered sequence
/// And inherits from Seq, since the order of the atoms in
/// its outgoing set is important.
class And : public Seq
{
	public:
		And()
			: Seq(AND)
		{}
		And(const OutList& ol, const TV& tv = TV())
			: Seq(AND, ol, tv)
		{}
		And(Atom* singleton, const TV& tv = TV())
			: Seq(AND, OutList(1, singleton), tv)
		{}
		And(Atom* a, Atom* b, const TV& tv = TV())
			: Seq(AND, ({OutList o(1,a); o.push_back(b); o;}), tv)
		{}
		And(Atom* a, Atom* b, Atom* c, const TV& tv = TV())
			: Seq(AND, ({OutList o(1,a); o.push_back(b); o.push_back(c); o;}), tv)
		{}


		// Return disjunctive normal form (DNF)
		// Does not modify this atom; just returns a new one.
		Atom* disjoin();

		// See the Set class for documentation
		And* flatten() const { return new And(flatset(), _tv); }

		/// Remove optional clauses.
		/// XXX Perhaps this should not be a method on this class...
		Atom* clean() const;

      virtual And* clone() const { return new And(*this); }
};


/// Given an atom of a some type, return the C++ class of that type.
Atom* upcast(Atom*);


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_COMPILE_H
