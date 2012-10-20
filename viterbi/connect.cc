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

#include "utilities.h" // From base link-grammar

#include "atom.h"
#include "connect.h"
#include "connector-utils.h"

using namespace std;

namespace link_grammar {
namespace viterbi {

/**
 * constructor: argument is a connector set (typically, for a single
 * word) that will be used to try connections. This connector set is
 * assumed to be to the right of the argument to the try_connect()
 * method.
 *
 * To be precise, the right_wconset is presumed to be of the form:
 *	WORD_CSET :
 *    WORD : blah-blah.v
 *	   AND :
 *       CONNECTOR : Wd-
 *       CONNECTOR : Abc+ etc...
 */
Connect::Connect(WordCset* right_wconset)
	: _right_cset(right_wconset)
{
	assert(_right_cset, "Unexpected NULL dictionary entry!");
	_rcons = _right_cset->get_cset();
}

/**
 * Try connecting this connector set sequence, from the left, to what
 * was passed in ctor.  It is preseumed that left_seq is a single parse
 * state: it will contain no left-pointing connectors whatsoever.  This
 * routine will attempt to attach the right-pointing connectors to the 
 * left-pointers passed in the ctor.  A connection is considered to be
 * successful if *all* left-pointers in the ctor were attached (except
 * possibly for optionals).  The returned value is a set of all possible
 * alternative ways of making the connections.
 *
 * Connectors must be satisfied sequentially: that is, the first connector
 * set in the sequence must be fully satisfied before a connection is made
 * to the second one in the sequence, etc.
 */
Set* Connect::try_connect(Seq* left_seq)
{
	_left_sequence = left_seq;

	WordCset* swc = dynamic_cast<WordCset*>(left_seq->get_outgoing_atom(0));
	return next_connect(swc);
}
/**
 * Try connecting this connector set, from the left, to what was passed
 * in ctor.
 */
Set* Connect::next_connect(WordCset* left_cset)
{
	assert(left_cset, "State word-connectorset is null");
	Atom* left_a = left_cset->get_cset();

	Atom *right_a = _rcons;
cout<<"enter try_connect, state cset "<< left_a <<endl;
cout<<"enter try_connect, word cset "<< right_a <<endl;

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
	lwdj.push_back(left_cset->get_word());    // the word
	lwdj.push_back(conn->get_left());         // the connector
	Link *lwordj = new Link(WORD_DISJ, lwdj);

	OutList rwdj;
	rwdj.push_back(right_cset->get_word());   // the word
	rwdj.push_back(conn->get_right());        // the connector
	Link *rwordj = new Link(WORD_DISJ, rwdj);

	Ling *lg_link = new Ling(conn->get_ling_type(), lwordj, rwordj);

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
	Connector* lnode = dynamic_cast<Connector*>(latom);
	if (lnode)
		return conn_connect_na(left_cset, lnode, ratom);

	Link* llink = dynamic_cast<Link*>(latom);
	assert(llink, "Left side should be link");
	return conn_connect_ka(left_cset, llink, ratom);
}

Link* Connect::conn_connect_na(WordCset* left_cset, Connector* lnode, Atom *ratom)
{
	Connector* rnode = dynamic_cast<Connector*>(ratom);
	if (rnode)
		return conn_connect_nn(left_cset, lnode, rnode);

	Link* rlink = dynamic_cast<Link*>(ratom);
	assert(rlink, "Right side should be link");
	return conn_connect_nk(left_cset, lnode, rlink);
}

// =============================================================
/**
 * Connect left_cset and _right_cset with an LG_LING
 * lnode and rnode are the two connecters that actually mate.
 */
Ling* Connect::conn_connect_nn(WordCset* left_cset, Connector* lnode, Connector* rnode)
{
cout<<"try match connectors l="<<lnode->get_name()<<" to r="<< rnode->get_name() << endl;
	if (!conn_match(lnode->get_name(), rnode->get_name()))
		return NULL;
	
cout<<"Yayyyyae connectors match!"<<endl;
	string link_name = conn_merge(lnode->get_name(), rnode->get_name());
	return new Ling(link_name, lnode, rnode);
}

/**
 * Connect left_cset and _right_cset with a LING
 * lnode is a connecter we hope to mate with something from rlink.
 */
Link* Connect::conn_connect_nk(WordCset* left_cset, Connector* lnode, Link* rlink)
{
cout<<"enter cnt_nk try match lnode="<<lnode->get_name()<<" to cset r="<< rlink << endl;

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
cout <<"duuude explore the and link"<<endl;
		// For an AND connective, all connectors must connect.  The
		// only way that this can happen for an and-link is if zero
		// or one connectors are required, and all other connectors in
		// the and-list are optional.
		Link* req_conn = NULL;
		for (int i = 0; i < rlink->get_arity(); i++)
		{
			Atom* a = rlink->get_outgoing_atom(i);
			Link* conn = conn_connect_na(left_cset, lnode, a);
cout<<"and link="<<i<<" con="<<conn<<endl;
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
if (req_conn)
cout <<"duuude got require conn"<<req_conn<<endl;

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
		Connector* chinode = dynamic_cast<Connector*>(a);
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
			assert(clink, "Child should be link");
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
