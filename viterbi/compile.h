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

#if 0
// Atom types.  Right now an enum, but maybe should be dynamic!?
enum AtomType
{
	// Link types
	AND,        // ordered AND of all children (order is important!)
	OR,         // unordered OR of all children
	WORD_DISJ,  // word, followed by a single disjunct for that word.
};
#endif

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
			: Link(LING, ({OutList ol;
				ol.push_back(new LingType(str));
				ol.push_back(a);
				ol.push_back(b);
				ol; }))
		{}

		Ling(LingType* t, Atom* a, Atom *b)
			: Link(LING, ({OutList ol;
				ol.push_back(t);
				ol.push_back(a);
				ol.push_back(b);
				ol; }))
		{}

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


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_COMPILE_H
