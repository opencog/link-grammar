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
	LINK_TYPE,  // a pair of merged connectors
	// META,       // special-word, e.g. LEFT-WALL, RIGHT-WALL
	CONNECTOR,  // e.g. S+ 

	// Link types
	SET,        // unordered set of children
	AND,        // ordered AND of all children (order is important!)
	OR,         // unordered OR of all children
	// OPTIONAL,   // one child only, and its optional. XXX not used ATM
	WORD_CSET,  // word, followed by a set of connectors for that word.
	WORD_DISJ,  // word, followed by a single disjunct for that word.
	LINK,       // two connected connectors, e.g. Dmcn w/o direction info
	STATE
};
#endif

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
};

class Set : public Link
{
	public:
		Set(const OutList& ol)
      	: Link(SET, ol)
      {}
};


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_COMPILE_H
