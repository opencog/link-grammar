/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
/*                                                                       */
/*************************************************************************/

#include <ctype.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "utilities.h"

#include "atom.h"
#include "connect.h"
#include "connector-utils.h"

using namespace std;

namespace link_grammar {
namespace viterbi {

#define OPTIONAL_CLAUSE "0"

/**
 * constructor: argument is a right-sided connector that this class
 * will try connecting to.
 */
Connect::Connect(Link* right_wconset)
	: _right_cset(right_wconset)
{
   // right_cset should be pointing at:
   // WORD_CSET :
   //   WORD : blah.v
   //   AND :
   //      CONNECTOR : Wd-  etc...

   assert(_right_cset, "Unexpected NULL dictionary entry!");
   assert(WORD_CSET == _right_cset->get_type(), "Unexpected link type word cset.");
   assert(2 == _right_cset->get_arity(), "Wrong arity for word connector set");

   _rcons = _right_cset->get_outgoing_atom(1);
}

/**
 * Try connecting this connector set, on the left, to what was passed
 * in ctor.
 */
Link* Connect::try_connect(Link* left_cset)
{
	assert(left_cset, "State word-connectorset is null");
	assert(2 == left_cset->get_arity(), "Wrong arity for state word conset");
	Atom* left = left_cset->get_outgoing_atom(1);

	Atom *right = _rcons;
cout<<"state con "<<left<<endl;
cout<<"word con "<<right<<endl;
	Node* lnode = dynamic_cast<Node*>(left);
	Node* rnode = dynamic_cast<Node*>(right);
	if (lnode and rnode)
		return conn_connect(left_cset, lnode, rnode);

	Link* llink = dynamic_cast<Link*>(left);
	if (llink and rnode)
	{
		Link *rv = conn_connect(left_cset, llink, rnode);
cout<<"got one it is "<<rv<<endl;
		return rv;
	}
	return NULL;
}

/**
 * Connect left_cset and _right_cset with an LG_LINK
 * lnode and rnode are the two connecters that actually mate.
 */
Link* Connect::conn_connect(Link* left_cset, Node* lnode, Node* rnode)
{
	assert(lnode->get_type() == CONNECTOR, "Expecting connector on left");
	assert(rnode->get_type() == CONNECTOR, "Expecting connector on right");
cout<<"try mathc l="<<lnode->get_name()<<" to r="<< rnode->get_name() << endl;
	if (!conn_match(lnode->get_name(), rnode->get_name()))
		return NULL;
	
cout<<"Yayyyyae  nodes match!"<<endl;
	string link_name = conn_merge(lnode->get_name(), rnode->get_name());
	OutList linkage;
	linkage.push_back(new Node(LINK_TYPE, link_name));
	linkage.push_back(left_cset);
	linkage.push_back(_right_cset);
	return new Link(LINK, linkage);
}

bool Connect::is_optional(Atom *a)
{
	AtomType ty = a->get_type();
	if (CONNECTOR == ty)
	{
		Node* n = dynamic_cast<Node*>(a);
		if (n->get_name() == OPTIONAL_CLAUSE)
			return true;
		return false;
	}
	assert (OR == ty or AND == ty, "Must be boolean junction");

	Link* l = dynamic_cast<Link*>(a);
	for (int i = 0; i < l->get_arity(); i++)
	{
		Atom *a = l->get_outgoing_atom(i);
		bool c = is_optional(a);
		if (OR == ty)
		{
			// If anything in OR is optional, the  whole clause is optional.
			if (c) return true;
		}
		else
		{
			// ty is AND
			// If anything in AND is isn't optional, then something is required
			if (!c) return false;
		}
	}
	
	// All disj were requied.
	if (OR == ty) return false;

	// All conj were optional.
	return true;
}

Link* Connect::conn_connect(Link* left_cset, Link* llink, Node* rnode)
{
cout<<"Enter recur l=" << llink->get_type()<<endl;
	// If llink is an AND, then the only way that rnode can
   // satisfy the connection is if at most one child is not
	// optional.
	if (AND == llink->get_type())
	{
	}

	bool clause_is_optional = false;
	for (int i = 0; i < llink->get_arity(); i++)
	{
		Atom *a = llink->get_outgoing_atom(i);
		Node* lnode = dynamic_cast<Node*>(a);
		if (lnode) 
		{
			if (lnode->get_name() == OPTIONAL_CLAUSE)
			{
				clause_is_optional = true;
				continue;
			}
			return conn_connect(left_cset, lnode, rnode);
		}

		Link* clink = dynamic_cast<Link*>(a);
		return conn_connect(left_cset, clink, rnode);
	}
	
	return NULL;
}

} // namespace viterbi
} // namespace link-grammar

