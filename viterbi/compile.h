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

#if 0
// Atom types.  Right now an enum, but maybe should be dynamic!?
enum AtomType
{
	// Node types
	WORD = 1,
	LING_TYPE,  // a pair of merged connectors
	// META,       // special-word, e.g. LEFT-WALL, RIGHT-WALL
	CONNECTOR,  // e.g. S+ 

	// Link types
	SET,        // unordered set of children
	AND,        // ordered AND of all children (order is important!)
	OR,         // unordered OR of all children
	// OPTIONAL,   // one child only, and its optional. XXX not used ATM
	WORD_CSET,  // word, followed by a set of connectors for that word.
	WORD_DISJ,  // word, followed by a single disjunct for that word.
	LING,       // two connected connectors, e.g. Dmcn w/o direction info
	STATE
};
#endif

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
};


class WordCset : public Link
{
	public:
		WordCset(Node* a, Atom*b)
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

		Word* get_word()
		{
			return dynamic_cast<Word*>(_oset[0]);
		}
		Atom* get_cset()
		{
			return _oset[1];
		}
};

// Ordered sequence
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

// Unordered sequence
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
