/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
/*                                                                       */
/*************************************************************************/

#include <iostream>
#include <string>
#include <assert.h>

#include "atom.h"

namespace link_grammar {
namespace viterbi {

using namespace std;

const string type_name(AtomType t)
{
	switch(t)
	{
		case WORD:       return "WORD";
		case LING_TYPE:  return "LING_TYPE";
		case CONNECTOR:  return "CONNECTOR";
		case SEQ:        return "SEQ";
		case SET:        return "SET";
		case OR:         return "OR";
		case AND:        return "AND";
		case WORD_CSET:  return "WORD_CSET";
		case WORD_DISJ:  return "WORD_DISJ";
		case LING:       return "LING";
		case STATE_PAIR: return "STATE_PAIR";
	}

	return "UNHANDLED_TYPE_NAME";
}

bool Atom::operator==(const Atom* other) const
{
	if (other == this) return true;
	if (other->_type == this->_type) return true;
	return false;
}

bool Node::operator==(const Atom* other) const
{
	if (other == this) return true;
	if (other->get_type() != this->get_type()) return false;
	const Node* nother = dynamic_cast<const Node*>(other);
	if (nother->_name != this->_name)  return false;
	return true;
}

bool Link::operator==(const Atom* other) const
{
	if (other == this) return true;
	if (other->get_type() != this->get_type()) return false;
	const Link* lother = dynamic_cast<const Link*>(other);
	if (lother->get_arity() != this->get_arity())  return false;
	for (size_t i=0; i<get_arity(); i++)
	{
		if (not (lother->_oset[i]->operator==(this->_oset[i]))) return false;
	}
	return true;
}


std::ostream& do_prt(std::ostream& out, const Atom* a, int ilvl)
{
	const char *indent_str = "  ";
	const Node* n = dynamic_cast<const Node*>(a);
	if (n)
	{
		for (int i=0; i<ilvl; i++) cout << indent_str;
		out << type_name(n->get_type()) <<" : ";
		out << n->get_name() << endl;
		return out;
	}
	const Link* l = dynamic_cast<const Link*>(a);
	if (l)
	{
		for (int i=0; i<ilvl; i++) cout << indent_str;
		out << type_name(l->get_type()) <<" :" << endl;

		ilvl++;
		for (int i=0; i < l->get_arity(); i++)
		{
			do_prt(out, l->get_outgoing_atom(i), ilvl);
		}
		return out;
	}

	out << "xxx-null-ptr-xxx";
	return out;
}

std::ostream& operator<<(std::ostream& out, const Atom* a)
{
	return do_prt(out, a, 0);
}

std::ostream& operator<<(std::ostream& out, AtomType t)
{
	out << type_name(t);
	return out;
}


} // namespace viterbi
} // namespace link-grammar
