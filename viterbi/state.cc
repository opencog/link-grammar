/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
/*                                                                       */
/*************************************************************************/

#include <iostream>
#include <string>
#include <vector>

#include <link-grammar/link-includes.h>
#include "api-types.h"
#include "read-dict.h"
#include "structures.h"

#include "viterbi.h"

using namespace std;

namespace link_grammar {
namespace viterbi {

/* Current parse state */

/* TV: strength or likelihood of a link */
class TV
{
	public:
		float strength;
};

// Atom types.  Right now an enum, but maybe should be dynamic!?
enum AtomType
{
	WORD = 1,
	META,       // special-word, e.g. LEFT-WALL, RIGHT-WALL
	CONNECTOR,  // e.g. S+ 
	LINK,       // e.g. Dmcn 
	STATE
};


/* Base class for Nodes and Links */
/* Atoms are not mutable, except for the TV value. That is, you cannot
 * change the type of the atom.
 */
class Atom
{
	public:
		Atom(AtomType type) : _type(type) {}
		TV tv;
	protected:
		AtomType _type;
};

/* A Node may be 
 -- a word (the string holds the word)
 -- a link (the string holds the link)
 -- a disjunct (the string holds the disjunct)
 -- etc.
 * Nodes are immuatble; the name can be set but not changed.
 */
class Node : public Atom
{
	public:
		Node(AtomType t, const string& n)
			: Atom(t), _name(n) {}
	private:
		string _name;
};

/*
 * Links hold a bunch of atoms
 * Links are immutable; the outgoing set cannot be changed.
 */
class Link : public Atom
{
	public:
		Link(AtomType t, const vector<Atom*> oset)
			: Atom(t), _oset(oset) {}
	private:
		// Outgoing set
		vector<Atom*> _oset;
};

Link * initialize_state()
{
	Node *left_wall = new Node(META, "LEFT-WALL");
	vector<Atom*> statev;
	statev.push_back(left_wall);
	Link *state = new Link(STATE, statev);

	return state;
}

void viterbi_parse(const char * sentence, Dictionary dict)
{
	// Initial state
	Link * state = initialize_state();
cout <<"Hello world!"<<endl;

	Dict_node* dn = dictionary_lookup_list(dict, "LEFT-WALL");
	Exp* exp = dn->exp;
	print_expression(exp);
}

} // namespace viterbi
} // namespace link-grammar

void viterbi_parse(const char * sentence, Dictionary dict)
{
	link_grammar::viterbi::viterbi_parse(sentence, dict);
}

