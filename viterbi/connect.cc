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
 * was passed in ctor.  It is preseumed that left_sp is a single parse
 * state: it will contain no left-pointing connectors whatsoever.  This
 * routine will attempt to attach the right-pointing connectors to the 
 * left-pointers passed in the ctor.  A connection is considered to be
 * successful if *all* left-pointers in the ctor were attached (except
 * possibly for optionals).  The returned value is a set of all possible
 * alternative state pairs for which there is a connnection.
 *
 * Connectors must be satisfied sequentially: that is, the first connector
 * set in the sequence must be fully satisfied before a connection is made
 * to the second one in the sequence, etc.
 */
Set* Connect::try_connect(StatePair* left_sp)
{
// XXX passing only the state is .. wrong!?!?
	Seq* left_seq = left_sp->get_state();
	Set* bogo = try_connect(left_seq);
cout<<"duuude got bogo="<<bogo<<endl;
	if (bogo)
		return bogo;

	// If we are here, then nothing in the right cset was
	// able to attach to the left.  If the right cset has
	// no left-pointing links, that's just fine; add it
	// to the state.  If it does have left-pointing
	// links, then its a parse failure.
	WordCset* new_rcset = cset_trim_left_pointers(_right_cset);
	if (NULL == new_rcset)
	{
assert(0, "Parse fail, implement me");
	}

	// Append the connector set to the state
	Seq* old_state = left_sp->get_state();
	OutList state_vect = old_state->get_outgoing_set();
	state_vect.insert(state_vect.begin(), new_rcset);
	Seq* new_state = new Seq(state_vect);

	// In this situation, there is only one alternative: the new state
	// vector, together with the existing output.
	Set* alts = new Set(
		new StatePair(
			new_state,
			left_sp->get_output()
		)
	);

	return alts;
}

// this is being given only a single state
Set* Connect::try_connect(Seq* left_seq)
{
	_left_sequence = left_seq;

// XXX this is wrong, but works for just now .. 
// wrong because maybe we need next_connect in a loop!?
	WordCset* swc = dynamic_cast<WordCset*>(left_seq->get_outgoing_atom(0));
	Set* ling_set = next_connect(swc);
	if (NULL == ling_set)
		return NULL;

// This is wrong, since the state is empty!
// i.e. it is assuming that the connection emptied the state!
	OutList pair_list;
	for (int i=0; i < ling_set->get_arity(); i++)
	{
		Ling* output = dynamic_cast<Ling*>(ling_set->get_outgoing_atom(i));
		StatePair* result = new StatePair(new Seq(), new Seq(output));
		pair_list.push_back(result);
	}
	return new Set(pair_list);
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
cout<<"enter next_connect, state cset "<< left_a <<endl;
cout<<"in next_connect, word cset "<< right_a <<endl;

	// If the word connector set is a single solitary node, then
	// its not a set, its a single connecter.  Try to hook it up
	// to something on the left.
	Link* conn = conn_connect_aa(left_a, right_a);
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
Link* Connect::conn_connect_aa(Atom *latom, Atom* ratom)
{
	Connector* lnode = dynamic_cast<Connector*>(latom);
	if (lnode)
		return conn_connect_na(lnode, ratom);

	Link* llink = dynamic_cast<Link*>(latom);
	assert(llink, "Left side should be link");
	return conn_connect_ka(llink, ratom);
}

Link* Connect::conn_connect_na(Connector* lnode, Atom *ratom)
{
	Connector* rnode = dynamic_cast<Connector*>(ratom);
	if (rnode)
		return conn_connect_nn(lnode, rnode);

	Link* rlink = dynamic_cast<Link*>(ratom);
	assert(rlink, "Right side should be link");
	return conn_connect_nk(lnode, rlink);
}

// =============================================================
/**
 * Connect left_cset and _right_cset with an LG_LING
 * lnode and rnode are the two connecters that actually mate.
 */
Ling* Connect::conn_connect_nn(Connector* lnode, Connector* rnode)
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
Link* Connect::conn_connect_nk(Connector* lnode, Link* rlink)
{
cout<<"enter cnt_nk try match lnode="<<lnode->get_name()<<" to cset r="<< rlink << endl;

	// If the connector set is a disjunction, then try each of them, in turn.
	// Whenever something matches up, we add it to the list of possible
	// connection alternatives.
	OutList conn_alterns;
	if (OR == rlink->get_type())
	{
		// "conn_alterns" records the various different successful ways
		// that lnode and rlink can be mated together.
		for (int i = 0; i < rlink->get_arity(); i++)
		{
			Atom* a = rlink->get_outgoing_atom(i);
			Node* chinode = dynamic_cast<Node*>(a);
			if (chinode && chinode->get_name() == OPTIONAL_CLAUSE)
				continue;

			Link* conn = conn_connect_na(lnode, a);

			// If the shoe fits, wear it.
			if (conn)
				conn_alterns.push_back(conn);
		}
	}
	else
	{
cout <<"duuude explore the and link -- the lnode is "<<lnode<<endl;
		// For an AND connective, all connectors must connect.  This
		// means that we have to recurse through the state-pair system,
		// until we manage to connect all fo the required left-pointing
		// connectors in the right connector set.
		Atom* latom = lnode;
		OutList required_conn_list;
		for (int i = 0; i < rlink->get_arity(); i++)
		{
			Atom* a = rlink->get_outgoing_atom(i);
			Link* conn = conn_connect_aa(latom, a);
cout<<"and link="<<i<<" got a connction="<<conn<<endl;
// XXX start here .... 
			if (is_optional(a))
			{
				// ????
				if (conn)
					conn_alterns.push_back(conn);
			}
			else
			{
				if (conn) {
					required_conn_list.push_back(conn);
					// Now, recurse ...
					// trim off the connector, and the state...
					if (i+1 >= rlink->get_arity())
						break;
cout<<"i="<<i<<" left seq is ="<<_left_sequence<<endl;
					// try to link the next AND connector to the next
					// connector onthe left...

					// If there are no further left connector sets, then
					// everything remaining in the AND list had better be
					// optional!
					if (_left_sequence->get_arity() <= i+1)
					{
						for (int j = i+1; j < rlink->get_arity(); j++)
						{
							a = rlink->get_outgoing_atom(j);
							if (!is_optional(a))
								return NULL;
						}
						break;
					}

// XXX this assumes the next connector is left pointing .. what if its not?
					Atom* lwcsa = _left_sequence->get_outgoing_atom(i+1);
					WordCset* lwcs = dynamic_cast<WordCset*>(lwcsa);
					latom = lwcs->get_cset();
				}
				else
					return NULL;
			}
		}
Set* s = new Set(required_conn_list);
cout<<"ayyyyyyyyyyyyyyyyyyyyyyyyyyy done wi looooooooop "<<s<<endl;

// temp hack .. 
if(1 == required_conn_list.size()) return
dynamic_cast<Link*>(required_conn_list[0]);
	}

	if (0 == conn_alterns.size())
		return NULL;
	return new Set(flatten(conn_alterns));
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

Set* Connect::conn_connect_ka(Link* llink, Atom* ratom)
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
				Link* rv = conn_connect_na(chinode, ratom);
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
			Link* rv = conn_connect_ka(clink, ratom);
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
