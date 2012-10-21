/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
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
		Connector(const std::string& name)
			: Node(CONNECTOR, name)
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
		LingType(const std::string& name)
			: Node(LING_TYPE, name)
		{}
};

class Word : public Node
{
	public:
		Word(const std::string& name)
			: Node(WORD, name)
		{}
};

#if 0
// Atom types.  Right now an enum, but maybe should be dynamic!?
enum AtomType
{
	// Link types
	OR,         // unordered OR of all children
	WORD_DISJ,  // word, followed by a single disjunct for that word.
};
#endif


/// Ordered sequence
/// And could/should inherit fom Seq, since the order of the atoms in
/// its outgoing set is important.
class And : public Link
{
	public:
		And()
			: Link(AND)
		{}
		And(const OutList& ol)
			: Link(AND, ol)
		{}
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
			: Link(LING)
		{
			_oset.push_back(new LingType(str));
			_oset.push_back(a);
			_oset.push_back(b);
		}

		Ling(LingType* t, Atom* a, Atom *b)
			: Link(LING)
		{
			_oset.push_back(t);
			_oset.push_back(a);
			_oset.push_back(b);
		}

		LingType* get_ling_type()
		{
			return dynamic_cast<LingType*>(get_outgoing_atom(0));
		}

		Atom* get_left()
		{
			return get_outgoing_atom(1);
		}
		Atom* get_right()
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
};

/// Ordered sequence
/// Seq inherits fom Link, and is an ordered sequence of zero or more
/// atoms.
class Seq : public Link
{
	public:
		Seq()
			: Link(SEQ)
		{}
		Seq(const OutList& ol)
			: Link(SEQ, ol)
		{}
		Seq(Atom* singleton)
			: Link(SEQ, OutList(1, singleton))
		{}
};

/// Unordered sequence
/// A Set inherits fom Link, and is an unordered set of zero or more
/// atoms.
class Set : public Link
{
	public:
		Set(const OutList& ol)
			: Link(SET, ol)
		{}
		Set(Atom* singleton)
			: Link(SET, OutList(1, singleton))
		{}
};

/// A pair of two sequences.  The first sequence is the state, the
/// second sequence is the output.  That is, the link created is of the
/// form
///
///    STATE_PAIR :
///       SEQ
///           ...
///       SEQ
///           ...
///
class StatePair : public Link
{
	public:
		StatePair(Seq* stat, Seq* outp)
			: Link(STATE_PAIR)
		{
			_oset.push_back(stat);
			_oset.push_back(outp);
		}
		Seq* get_state() { return dynamic_cast<Seq*>(_oset.at(0)); }
		Seq* get_output() { return dynamic_cast<Seq*>(_oset.at(1)); }
};


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_COMPILE_H
