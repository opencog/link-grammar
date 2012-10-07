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

/**
 * constructor: argument is a right-sided connector that this class
 * will try connecting to.
 */
Connect::Connect(WordCset* right_wconset)
	: _right_cset(right_wconset)
{
	// right_cset should be pointing at:
	// WORD_CSET :
	//   WORD : blah.v
	//   AND :
	//      CONNECTOR : Wd-  etc...

	assert(_right_cset, "Unexpected NULL dictionary entry!");
	assert(WORD_CSET == _right_cset->get_type(), "Expecting right word cset.");
	assert(2 == _right_cset->get_arity(), "Wrong arity for word connector set");

	_rcons = _right_cset->get_outgoing_atom(1);
}

/**
 * Try connecting this connector set, from the left, to what was passed
 * in ctor.
 */
Set* Connect::try_connect(WordCset* left_cset)
{
	assert(left_cset, "State word-connectorset is null");
	assert(WORD_CSET == left_cset->get_type(), "Expecting left word cset.");
	assert(2 == left_cset->get_arity(), "Wrong arity for state word conset");
	Atom* left_a = left_cset->get_outgoing_atom(1);

	Atom *right_a = _rcons;
cout<<"state cset "<<left<<endl;
cout<<"word cset "<<right<<endl;

	// If the word connector set is a single solitary node, then
	// its not a set, its a single connecter.  Try to hook it up
	// to something on the left.
	Link* conn = conn_connect_aa(left_cset, left_a, right_a);
	if (!conn)
		return NULL;
cout<<"got one it is "<<conn<<endl;

	// At this point, conn holds an LG link type, and the
	// two disjuncts that were mated.  Re-assemble these
	// into a pair of word_disjuncts (i.e. stick the word
	// back in there, as that is what later stages need).
	Set* alts = dynamic_cast<Set*>(conn);
	if (alts)
		return reassemble(alts, left_cset, _right_cset);

	Ling* lg_link = dynamic_cast<Ling*>(conn);
	if (lg_link)
		return new Set(reassemble(lg_link, left_cset, _right_cset));
}

// =============================================================

// At this point, conn holds an LG link type, and the
// two disjuncts that were mated.  Re-assemble these
// into a pair of word_disjuncts (i.e. stick the word
// back in there, as that is what later stages need).
//
// The left_cset and right_cset are assumed to be the word-connector
// sets that matched. These are needed, only to extract the words;
// the rest is dicarded.
Ling* Connect::reassemble(Ling* conn, WordCset* left_cset, WordCset* right_cset)
{
	OutList lwdj;
	lwdj.push_back(left_cset->get_outgoing_atom(0));  // the word
	lwdj.push_back(conn->get_outgoing_atom(1));       // the connector
	Link *lwordj = new Link(WORD_DISJ, lwdj);

	OutList rwdj;
	rwdj.push_back(right_cset->get_outgoing_atom(0));   // the word
	rwdj.push_back(conn->get_outgoing_atom(2));         // the connector
	Link *rwordj = new Link(WORD_DISJ, rwdj);

	OutList lo;
	lo.push_back(conn->get_outgoing_atom(0));
	lo.push_back(lwordj);
	lo.push_back(rwordj);
	Ling *lg_link = new Ling(lo);

cout<<"normalized into "<<lg_link<<endl;
	return lg_link;
}

Set* Connect::reassemble(Set* conn, WordCset* left_cset, WordCset* right_cset)
{
	OutList alternatives;
	for (int i = 0; i < conn->get_arity(); i++)
	{
		Ling* alt = dynamic_cast<Ling*>(conn->get_outgoing_atom(i));
		assert(alt, "Unexpected type in alternative set");
		Ling* normed_alt = reassemble(alt, left_cset, right_cset);
		alternatives.push_back(normed_alt);
	}

	return new Set(alternatives);
}

// =============================================================
/**
 * Dispatch appropriatly, depending on whether left atom is node or link
 */
Link* Connect::conn_connect_aa(WordCset* left_cset, Atom *latom, Atom* ratom)
{
	Node* lnode = dynamic_cast<Node*>(latom);
	if (lnode)
		return conn_connect_na(left_cset, lnode, ratom);

	Link* llink = dynamic_cast<Link*>(latom);
	return conn_connect_ka(left_cset, llink, ratom);
}

Link* Connect::conn_connect_na(WordCset* left_cset, Node* lnode, Atom *ratom)
{
	assert(lnode->get_type() == CONNECTOR, "Expecting connector on left");
	Node* rnode = dynamic_cast<Node*>(ratom);
	if (rnode)
		return conn_connect_nn(left_cset, lnode, rnode);

	Link* rlink = dynamic_cast<Link*>(ratom);
	return conn_connect_nk(left_cset, lnode, rlink);
}

// =============================================================
/**
 * Connect left_cset and _right_cset with an LG_LING
 * lnode and rnode are the two connecters that actually mate.
 */
Ling* Connect::conn_connect_nn(WordCset* left_cset, Node* lnode, Node* rnode)
{
	assert(lnode->get_type() == CONNECTOR, "Expecting connector on left");
cout<<"try match connectors l="<<lnode->get_name()<<" to r="<< rnode->get_name() << endl;
	if (!conn_match(lnode->get_name(), rnode->get_name()))
		return NULL;
	
cout<<"Yayyyyae connectors match!"<<endl;
	string link_name = conn_merge(lnode->get_name(), rnode->get_name());
	OutList linkage;
	linkage.push_back(new Node(LING_TYPE, link_name));
	linkage.push_back(lnode);
	linkage.push_back(rnode);
	return new Ling(linkage);
}

/**
 * Connect left_cset and _right_cset with a LING
 * lnode is a connecter we hope to mate with something from rlink.
 */
Link* Connect::conn_connect_nk(WordCset* left_cset, Node* lnode, Link* rlink)
{
	assert(lnode->get_type() == CONNECTOR, "Expecting connector on left");
cout<<"try match con l="<<lnode->get_name()<<" to cset r="<< rlink << endl;

	// If the connector set is a disjunction, then try each of them, in turn.
	OutList alternatives;
	if (OR == rlink->get_type())
	{
		// "alternatives" records the various different successful ways
		// that lnode and rlink can be mated together.
		for (int i = 0; i < rlink->get_arity(); i++)
		{
			Atom* a = rlink->get_outgoing_atom(i);
			Node* chinode = dynamic_cast<Node*>(a);
			if (chinode && chinode->get_name() == OPTIONAL_CLAUSE)
				continue;

			Link* conn = conn_connect_na(left_cset, lnode, a);

			// If the shoe fits, wear it.
			if (conn)
				alternatives.push_back(conn);
		}
	}
	else
	{
		// For an AND connective, all connectors must connect.  The
		// only way that this can happen for an and-link is if zero
		// or one connectors are required, and all other connectors in
		// the and-list are optional.
		Link* req_conn = NULL;
		for (int i = 0; i < rlink->get_arity(); i++)
		{
			Atom* a = rlink->get_outgoing_atom(i);
			Link* conn = conn_connect_na(left_cset, lnode, a);
			if (is_optional(a))
			{
				if (conn)
					alternatives.push_back(conn);
			}
			else
			{
				// check for more than one required clause.
				// If there is more than one, then connection fails.
				if (req_conn) return NULL;
				req_conn = conn;
			}
		}

		// If we found exactly one required connector, and it
		// connects, then it is the only possible connection.
		if (req_conn)
			return req_conn;
	}

	if (0 == alternatives.size())
		return NULL;
	return new Set(flatten(alternatives));
}

// =============================================================
/// Attempt to connect one connector in the llink cset to one connector 
/// in the ratom cset.  If no linkage is possible, return NULL.
/// Otheriwse, return a set of graphs of the form:
/// (for example):
///
///   LING :
///     LING_TYPE : Wd
///     CONNECTOR : Wd+
///     CONNECTOR : Wd-

Set* Connect::conn_connect_ka(WordCset* left_cset, Link* llink, Atom* ratom)
{
cout<<"Enter recur l=" << llink->get_type()<<endl;

	// "alternatives" records the various different successful ways
	// that llink and ratom can be mated together.
	OutList alternatives;
	for (int i = 0; i < llink->get_arity(); i++)
	{
		Atom* a = llink->get_outgoing_atom(i);
		Node* chinode = dynamic_cast<Node*>(a);
		if (chinode) 
		{
			// XXX TODO -- I think this is the right response here, not sure...
			if (chinode->get_name() == OPTIONAL_CLAUSE)
				continue;

			// Only one needs to be satisfied for OR clause
			AtomType op = llink->get_type();
			if (OR == op)
			{
				Link* rv = conn_connect_na(left_cset, chinode, ratom);
				if (rv)
					alternatives.push_back(rv);
			}
			else
			{
				// If we are here, then its an AND. 
				assert(AND == op, "Bad connective.");
cout<<"duuude lefty is "<<chinode<<endl;
				// All connectors must be satsisfied.
				assert(0, "Implement me cnode AND");
			}
		}
		else
		{	
			Link* clink = dynamic_cast<Link*>(a);
			Link* rv = conn_connect_ka(left_cset, clink, ratom);
			if (rv)
				alternatives.push_back(rv);
		}
	}

	if (0 == alternatives.size())
		return NULL;

	return new Set(flatten(alternatives));
}

// =============================================================

// Collapse singleton sets, if any.  This is the price we pay
// for otherwise being able to ignore the difference between
// singleton sets, and their elements.
const OutList& Connect::flatten(OutList& alternatives)
{
	size_t asize = alternatives.size();

	// If its a singleton, and its already a set ...
	if (1 == asize)
	{
		Set* set = dynamic_cast<Set*>(alternatives[0]);
		if (set)
			return set->get_outgoing_set();
	}

	for (int i = 0; i < asize; i++)
	{
		Set* set = dynamic_cast<Set*>(alternatives[i]);
		if (set and (1 == set->get_arity()))
		{
			alternatives[i] = set->get_outgoing_atom(0);
		}
	}
	return alternatives;
}

} // namespace viterbi
} // namespace link-grammar
