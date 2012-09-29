/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
/*                                                                       */
/*************************************************************************/

#include <string>
#include <vector>

using namespace std;

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
	LINK,   // e.g. Dmcn 
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
	private:
		AtomType _type;
};

/* A Node may be 
 -- a word (the string holds the word)
 -- a link (the string holds the link)
 -- a disjunct (the string holds the disjunct)
 -- etc.
 */
class Node : public Atom
{
	public:
		string name;
};

class Link : public Atom
{
	public:
		// Ooutgoing set
		vector<Atom*> oset;
};


