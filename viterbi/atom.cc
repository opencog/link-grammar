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
		case WORD:      return "WORD";
		case LINK_TYPE: return "LINK_TYPE";
		// case META:      return "META";
		case CONNECTOR: return "CONNECTOR";
		case SET:       return "SET";
		case OR:        return "OR";
		case AND:       return "AND";
		// case OPTIONAL:  return "OPTIONAL";
		case WORD_CSET: return "WORD_CSET";
		case WORD_DISJ: return "WORD_DISJ";
		case LINK:      return "LINK";
		case STATE:     return "STATE";
	}

	return "UNHANDLED_TYPE_NAME";
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
