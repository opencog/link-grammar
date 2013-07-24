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

#include <iostream>
#include <string>
#include <assert.h>
#include <math.h>

#include "atom.h"

namespace atombase {

using namespace std;

const string type_name(AtomType t)
{
	switch(t)
	{
		case NODE:       return "NODE";

		case WORD:       return "WORD";
		case LING_TYPE:  return "LING_TYPE";
		case CONNECTOR:  return "CONNECTOR";

		// case LINK:       return "LINK";
		case SEQ:        return "SEQ";
		case SET:        return "SET";
		case OR:         return "OR";
		case AND:        return "AND";
		case WORD_CSET:  return "WORD_CSET";
		case WORD_DISJ:  return "WORD_DISJ";
		case LING:       return "LING";
		case STATE_TRIPLE: return "STATE_TRIPLE";

	}

	return "UNHANDLED_TYPE_NAME";
}

bool TV::operator==(const TV& other) const
{
	// The ULP for single-precision floating point is approx 1.0e-7.2
	if (fabs(other._strength - _strength) < 1.0e-6) return true;
	return false;
}

// ====================================================

bool Atom::operator==(const Atom* other) const
{
	if (!other) return false;
	if (other == this) return true;
	if (other->_type == this->_type and
	    other->_tv == this->_tv) return true;
	return false;
}

bool Node::operator==(const Atom* other) const
{
	if (!other) return false;
	if (other == this) return true;
	if (other->get_type() != this->get_type()) return false;
	if (not other->_tv.operator==(this->_tv)) return false;
	const Node* nother = dynamic_cast<const Node*>(other);
	if (nother->_name != this->_name)  return false;
	return true;
}

bool Link::operator==(const Atom* other) const
{
	if (!other) return false;
	if (other == this) return true;
	if (other->get_type() != this->get_type()) return false;
	if (not other->_tv.operator==(this->_tv)) return false;
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
	static const char *indent_str = "  ";
	const Node* n = dynamic_cast<const Node*>(a);
	if (n)
	{
		for (int i=0; i<ilvl; i++) cout << indent_str;
		out << type_name(n->get_type()) << " : "
		    << n->get_name();
		if (n->_tv._strength > 0.0f)
			out << "    (" << n->_tv._strength << ")";
		out << endl;
		return out;
	}
	const Link* l = dynamic_cast<const Link*>(a);
	if (l)
	{
		for (int i=0; i<ilvl; i++) cout << indent_str;
		out << type_name(l->get_type()) <<" :";
		if (l->_tv._strength > 0.0f)
			out << "     (" << l->_tv._strength << ")";
      out << endl;

		ilvl++;
		size_t lsz = l->get_arity();
		for (int i=0; i < lsz; i++)
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

} // namespace atom

