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

#include "utilities.h"  // needed for assert

#include "atom.h"

namespace link_grammar {
namespace viterbi {

// Classes that convert run-time atom types into compile-time static
// types, so that the compiler can check these for correctness.
// These are here purely for C++ programming convenience; the true
// structure that matters is the dynamic run-time (hyper-)graphs.

class Connector : public Node
{
	public:
		// Last letter of the connector must be + or -
		// indicating the direction of the connector.
		Connector(const std::string& name, const TV& tv = TV())
			: Node(CONNECTOR, name, tv)
		{
			if (name == OPTIONAL_CLAUSE)
				return;
			char dir = *name.rbegin();
			assert (('+' == dir) or ('-' == dir), "Bad direction");
		}

		bool is_optional() const
		{
			return _name == OPTIONAL_CLAUSE;
		}

		char get_direction() const
		{
			return *_name.rbegin();
		}
};

class LingType : public Node
{
	public:
		LingType(const std::string& name, const TV& tv = TV())
			: Node(LING_TYPE, name, tv)
		{}
};

class Word : public Node
{
	public:
		Word(const std::string& name, const TV& tv = TV())
			: Node(WORD, name, tv)
		{}
};

#if 0
// Atom types.  Right now an enum, but maybe should be dynamic!?
enum AtomType
{
	// Link types
	WORD_DISJ,  // word, followed by a single disjunct for that word.
};
#endif


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
};

/// Create a ling-grammar link. This will be of the form:
///     LING:
///        Ling_TYPE "MVa"
///        Atom ...
///        Atom ...
/// where the Atoms are typically either connectors, or WORD_DISJ
///
class Ling : public Link
{
	public:
		Ling(const OutList& ol)
			: Link(LING, ol)
		{
			assert(3 == ol.size(), "LG link wrong size");
			assert(ol[0]->get_type() == LING_TYPE, "LG link has bad first node");
		}
		Ling(const std::string& str, Atom* a, Atom *b)
			: Link(LING, new LingType(str), a, b) {}

		Ling(LingType* t, Atom* a, Atom *b)
			: Link(LING, t, a, b) {}

		LingType* get_ling_type() const
		{
			return dynamic_cast<LingType*>(get_outgoing_atom(0));
		}

		Atom* get_left() const
		{
			return get_outgoing_atom(1);
		}
		Atom* get_right() const
		{
			return get_outgoing_atom(2);
		}
};


class WordCset : public Link
{
	public:
		WordCset(Word* a, Atom* b)
			: Link(WORD_CSET, a, b)
		{
			// this should be pointing at:
			// WORD_CSET :
			//   WORD : blah.v
			//   AND :
			//      CONNECTOR : Wd-  etc...

			assert(a->get_type() == WORD, "CSET is expecting WORD as first arg");
			bool ok = false;
			ok = ok or b->get_type() == CONNECTOR;
			ok = ok or b->get_type() == AND;
			ok = ok or b->get_type() == OR;
			assert(ok, "CSET is expecting connector set as second arg");
		}

		Word* get_word() const
		{
			return dynamic_cast<Word*>(_oset[0]);
		}
		Atom* get_cset() const
		{
			return _oset[1];
		}
		bool has_left_pointers() const;
		WordCset* flatten();
};

/// A triple of three sequences.  The first sequence is a sequence of 'input'
/// words (a sentence or phrase that has not yet been parsed).  The second
/// sequence is the current parse state.  The third sequence is the 'output'
/// of the parse, i.e. a set of connected words.
///
/// It of the form
///
///    STATE_TRIPLE :
///       SEQ
///           WORD
///           WORD
///       SEQ
///           WORD_CSET ...
///           WORD_CSET ...
///       SET
///           LING ...
///           LING ...
///
class StateTriple : public Link
{
	public:
		StateTriple(Seq* input, Seq* state, Set* output)
			: Link(STATE_TRIPLE, input, state, output) {}
		Seq* get_input() const { return dynamic_cast<Seq*>(_oset.at(0)); } 
		Seq* get_state() const { return dynamic_cast<Seq*>(_oset.at(1)); } 
		Set* get_output() const { return dynamic_cast<Set*>(_oset.at(2)); }
};

/// Given an atom of a some type, return the C++ class of that type.
Atom* upcast(Atom*);


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_COMPILE_H
