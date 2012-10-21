/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
/*                                                                       */
/*************************************************************************/

#ifndef _LG_VITERBI_ATOM_H
#define _LG_VITERBI_ATOM_H

#include <iostream>
#include <string>
#include <vector>

namespace link_grammar {
namespace viterbi {

// Classes generally resembling those of the OpenCog AtomSpace
// These are tailored for use for parsing.

/* TV: strength or likelihood of a link */
class TV
{
	public:
		float strength;
};

// Atom types.  Right now an enum, but maybe should be dynamic!?
enum AtomType
{
	// Node types
	WORD = 1,
	LING_TYPE,  // a pair of merged connectors (LG LINK TYPE)
	// META,       // special-word, e.g. LEFT-WALL, RIGHT-WALL
	CONNECTOR,  // e.g. S+ 

	// Link types
	SET,        // unordered set of children
	SEQ,        // ordered sequence of children
	AND,        // ordered AND of all children (order is important!)
	OR,         // unordered OR of all children
	// OPTIONAL,   // one child only, and its optional. XXX not used ATM
	WORD_CSET,  // word, followed by a set of connectors for that word.
	WORD_DISJ,  // word, followed by a single disjunct for that word.
	LING,       // two connected connectors, (LGLINK) e.g. Dmcn w/o direction info
	STATE_PAIR, // Current state and corresponding output.
};

#define OPTIONAL_CLAUSE "0"

/* Base class for Nodes and Links */
/* Atoms are not mutable, except for the TV value. That is, you cannot
 * change the type of the atom.
 */
class Atom
{
	public:
		Atom(AtomType type) : _type(type) {}
		AtomType get_type() const { return _type; }
		TV tv;
		virtual bool operator==(const Atom*) const;
		virtual ~Atom() {}
	protected:
		AtomType _type;
};

/* A Node may be 
 -- a word (the std::string holds the word)
 -- a link (the std::string holds the link)
 -- a disjunct (the std::string holds the disjunct)
 -- etc.
 * Nodes are immuatble; the name can be set but not changed.
 */
class Node : public Atom
{
	public:
		Node(AtomType t, const std::string& n)
			: Atom(t), _name(n) {}

		const std::string& get_name() const { return _name; }

		virtual bool operator==(const Atom*) const;
	protected:
		std::string _name;
};

// Uhhhh ... 
typedef std::vector<Atom*> OutList;

/*
 * Links hold a bunch of atoms
 * Links are immutable; the outgoing set cannot be changed.
 */
class Link : public Atom
{
	public:
		// The main ctor
		Link(AtomType t, const OutList& oset)
			: Atom(t), _oset(oset) {}
		Link(AtomType t)
			: Atom(t)
		{}
		Link(AtomType t, Atom* a)
			: Atom(t)
		{
			_oset.push_back(a);
		}
		Link(AtomType t, Atom* a, Atom*b)
			: Atom(t)
		{
			_oset.push_back(a);
			_oset.push_back(b);
		}
		Link(AtomType t, Atom* a, Atom*b, Atom* c)
			: Atom(t)
		{
			_oset.push_back(a);
			_oset.push_back(b);
			_oset.push_back(c);
		}
		size_t get_arity() const { return _oset.size(); }
		Atom* get_outgoing_atom(size_t pos) const { return _oset.at(pos); }
		const OutList& get_outgoing_set() const { return _oset; }

		virtual bool operator==(const Atom*) const;
	protected:
		// Outgoing set
		OutList _oset;
};

std::ostream& operator<<(std::ostream& out, const Atom*);
std::ostream& operator<<(std::ostream& out, AtomType);


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_ATOM_H
