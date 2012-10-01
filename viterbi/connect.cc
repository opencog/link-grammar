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

/* Find matches...
 * state_cset is a single word-connectorset in the state
 * word_node is a word, and that's all.
 * right is the connector set for the word.
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
	{
		assert(lnode->get_type() == CONNECTOR, "Expecting connector on left");
		assert(rnode->get_type() == CONNECTOR, "Expecting connector on right");
		if (conn_match(lnode->get_name(), rnode->get_name()))
		{
cout<<"Yayyyyae  nodes match!"<<endl;
			string link_name = conn_merge(lnode->get_name(), rnode->get_name());
			OutList linkage;
			linkage.push_back(new Node(LINK_TYPE, link_name));
			linkage.push_back(left_cset);
			linkage.push_back(_right_cset);
			return new Link(LINK, linkage);
		}

		// No match, return NULL
		return NULL;
	}

	Link* llink = dynamic_cast<Link*>(left);
	if (llink and rnode)
	{
		conn_connect(llink, rnode);
	}
	return NULL;
}

Link* Connect::conn_connect(Node* lnode, Node* rnode)
{
	string link_name = conn_merge(lnode->get_name(), rnode->get_name());
	return NULL;
}

Link* Connect::conn_connect(Link* llink, Node* rnode)
{
	for (int i = 0; i < llink->get_arity(); i++)
	{
		Node *n = dynamic_cast<Node*>(llink->get_outgoing_atom(i));
	}
	
	return NULL;
}

} // namespace viterbi
} // namespace link-grammar

