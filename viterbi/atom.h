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

#ifndef _ATOMBASE_ATOM_H
#define _ATOMBASE_ATOM_H

#include <iostream>
#include <string>
#include <vector>
#include <gc/gc_allocator.h>
#include <gc/gc_cpp.h>

namespace atombase {

// Classes generally resembling those of the OpenCog AtomSpace
// These are tailored for use for parsing.

/**
 * TV (truth value): strength or likelihood of a link.
 *
 * Actually, we store the log-likelihood here, in units of bits,
 * rather than the probability.  This makes the numbers more
 * comprehensible and easier to read and debug.  To obtain the
 * probability (likelihood), just raise 2 to minus this value.
 *
 * Measuring in bits allows us to conflate ideas of energy, entropy,
 * complexity, cost.  In particular, long linkages will get a complexity
 * cost, whereas certain disjuncts have an innate cost, obtained from
 * entropy principles. These can be added together; they'e on the same
 * scale.
 */
class TV
{
	public:
		TV(float likeli=0.0f) : _strength(likeli) {}
		float _strength;
		bool operator==(const TV&) const;

		/// Log-likelihoods (costs, energies, entropies) add.
		TV& operator+=(const TV& other)
		{
			_strength += other._strength;
			return *this;
		}

		const TV operator+(const TV& other) const
		{
			return TV(*this) += other;
		}
};

// Atom types.  Right now an enum, but maybe should be dynamic!?
enum AtomType
{
	// Node types
	NODE = 1,
	WORD,       // a word
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
	STATE_TRIPLE, // Current pending input, parse state and corresponding output.

	RULE,       // Base class for graph re-write rules
};

/* Base class for Nodes and Links */
/**
 * Atoms are not mutable, except for the TV value. That is, you cannot
 * change the type of the atom.  In particular, all methods are const.
 *
 * The mutable TV value can cause problems.  In particular, when
 * propagating costs upwards when putting mixed expressions into DNF,
 * this mutability can mess things up.  The work-around for this is to
 * have a clone() function.  I'm not sure I like this.  Its ugly, because
 * of course, once an atom is in the atom space, its unique, and not clonable.
 * Ick.  Perhaps TV should not be mutable??
 *
 * All atoms are automatically garbage-collected.
 */
class Atom : public gc
{
	public:
		Atom(AtomType type, const TV& tv = TV()) :
			_tv(tv), _type(type)
		{
			// Marking stubborn, since its immutable.
			GC_change_stubborn(this);
		}
		AtomType get_type() const { return _type; }
		TV _tv;
		virtual bool operator==(const Atom*) const;
		virtual Atom* clone() const = 0;
		virtual ~Atom() {}
		Atom* upcaster();
	protected:
		const AtomType _type;
};

/// Given an atom of a given type, return the C++ class of that type.
template<typename T>
T upcast(Atom* a)
{
	T t = dynamic_cast<T>(a);
	if (t) return t;
	return dynamic_cast<T>(a->upcaster());
}

/**
 * A Node may be
 * -- a word (the std::string holds the word)
 * -- a link (the std::string holds the link)
 * -- a disjunct (the std::string holds the disjunct)
 * -- etc.
 * Nodes are immuatble; the name can be set but not changed.
 * Note: all methods are const.
 */
class Node : public Atom
{
	public:
		Node(const std::string& n, const TV& tv = TV())
			: Atom(NODE, tv), _name(n) {}

		Node(AtomType t, const std::string& n, const TV& tv = TV())
			: Atom(t, tv), _name(n) {}

		const std::string& get_name() const { return _name; }

		virtual bool operator==(const Atom*) const;
		virtual Node* clone() const { return new Node(*this); }
	protected:
		const std::string _name;
};

/// All outgoing lists will be handled as vectors.
// Must use the bdw-gc allocator to track these pointers.
// If this is not done, the GC will fail to see the pointers here.
typedef std::vector<Atom*, gc_allocator<Atom*> > OutList;

/**
 * Links hold a bunch of atoms
 * Links are immutable; the outgoing set cannot be changed.
 * Note: all methods are const.
 */
class Link : public Atom
{
	public:
		// The main ctor
		Link(AtomType t, const OutList& oset, const TV& tv = TV())
			: Atom(t, tv), _oset(oset) {}
		Link(AtomType t, const TV& tv = TV())
			: Atom(t, tv)
		{}
		Link(AtomType t, Atom* a, const TV& tv = TV())
			: Atom(t, tv), _oset(1, a)
		{}
		Link(AtomType t, Atom* a, Atom*b, const TV& tv = TV())
			: Atom(t, tv), _oset(({OutList o(1,a); o.push_back(b); o;}))
		{}
		Link(AtomType t, Atom* a, Atom* b, Atom* c, const TV& tv = TV())
			: Atom(t, tv), _oset(({OutList o(1,a); o.push_back(b);
			                      o.push_back(c); o;}))
		{}
		Link(AtomType t, Atom* a, Atom* b, Atom* c, Atom* d, const TV& tv = TV())
			: Atom(t, tv), _oset(({OutList o(1,a); o.push_back(b);
			                      o.push_back(c); o.push_back(d); o;}))
		{}
		Link(AtomType t, Atom* a, Atom* b, Atom* c, Atom* d, Atom* e, const TV& tv = TV())
			: Atom(t, tv), _oset(({OutList o(1,a); o.push_back(b);
			                      o.push_back(c); o.push_back(d);
			                      o.push_back(e); o;}))
		{}
		size_t get_arity() const { return _oset.size(); }
		Atom* get_outgoing_atom(size_t pos) const { return _oset.at(pos); }
		const OutList& get_outgoing_set() const { return _oset; }

		Link* append(Atom* a) const;

		virtual bool operator==(const Atom*) const;
		virtual Link* clone() const { return new Link(*this); }
	protected:
		// Outgoing set is const, not modifiable.
		const OutList _oset;
};

// An unhygenic for-each loop, to simplify iterating over
// the outgoing set.  I don't see a more elegant way to do this,
// just right now...
// Anyway, this implements the semantics "foreach VAR of TYPENAME in LNK"
#define foreach_outgoing(TYPENAME,VAR,LNK) \
	const atombase::Link* _ll_##VAR; \
	size_t _ii_##VAR, _ee_##VAR; \
	atombase::Atom* _aa_##VAR; \
	TYPENAME VAR; \
	for (_ll_##VAR = (LNK), _ii_##VAR = 0, \
	     _ee_##VAR = _ll_##VAR->get_arity(); \
	     _aa_##VAR = _ii_##VAR < _ee_##VAR ? \
	        _ll_##VAR->get_outgoing_atom(_ii_##VAR) : 0x0, \
	     VAR = dynamic_cast<TYPENAME>(_aa_##VAR), \
	     _ii_##VAR < _ee_##VAR; \
	     _ii_##VAR++)

std::ostream& operator<<(std::ostream& out, const Atom*);
std::ostream& operator<<(std::ostream& out, AtomType);

} // namespace atombase

#endif // _ATOMBASE_ATOM_H
