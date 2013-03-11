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
	WORD_CSET,  // word, followed by a set of connectors for that word.
	WORD_DISJ,  // word, followed by a single disjunct for that word.
	LING,       // two connected connectors, (LGLINK) e.g. Dmcn w/o direction info
	STATE_PAIR, // Current state and corresponding output.
};

#define OPTIONAL_CLAUSE "0"

/* Base class for Nodes and Links */
/* Atoms are not mutable, except for the TV value. That is, you cannot
 * change the type of the atom.  In particular, all methods are const.
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
		const AtomType _type;
};

/* A Node may be 
 -- a word (the std::string holds the word)
 -- a link (the std::string holds the link)
 -- a disjunct (the std::string holds the disjunct)
 -- etc.
 * Nodes are immuatble; the name can be set but not changed.
 * Note: all methods are const.
 */
class Node : public Atom
{
	public:
		Node(AtomType t, const std::string& n)
			: Atom(t), _name(n) {}

		const std::string& get_name() const { return _name; }

		virtual bool operator==(const Atom*) const;
	protected:
		const std::string _name;
};

// Uhhhh ... 
typedef std::vector<Atom*> OutList;

/*
 * Links hold a bunch of atoms
 * Links are immutable; the outgoing set cannot be changed.
 * Note: all methods are const.
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
			: Atom(t), _oset(1, a)
		{}
		Link(AtomType t, Atom* a, Atom*b)
			: Atom(t), _oset(({OutList o(1,a); o.push_back(b); o;}))
		{}
		Link(AtomType t, Atom* a, Atom* b, Atom* c)
			: Atom(t), _oset(({OutList o(1,a); o.push_back(b);
			                   o.push_back(c); o;}))
		{}
		Link(AtomType t, Atom* a, Atom* b, Atom* c, Atom* d)
			: Atom(t), _oset(({OutList o(1,a); o.push_back(b);
			                   o.push_back(c); o.push_back(d); o;}))
		{}
		size_t get_arity() const { return _oset.size(); }
		Atom* get_outgoing_atom(size_t pos) const { return _oset.at(pos); }
		const OutList& get_outgoing_set() const { return _oset; }

		virtual bool operator==(const Atom*) const;
	protected:
		// Outgoing set is const, nnot modifiable.
		const OutList _oset;
};

std::ostream& operator<<(std::ostream& out, const Atom*);
std::ostream& operator<<(std::ostream& out, AtomType);


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_ATOM_H
