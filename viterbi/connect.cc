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
 *	   OR :
 *       CONNECTOR : Wd-
 *       CONNECTOR : Abc+ etc...
 * 
 * In particular, it is assumed to be in DNF (disjunctive normal form).
 */
Connect::Connect(WordCset* right_wconset)
	: _right_cset(right_wconset)
{
	assert(_right_cset, "Unexpected NULL dictionary entry!");
	_rcons = _right_cset->get_cset();
cout<<"------------------------- duuude rwcset=\n"<<_right_cset<<endl;
cout<<"------------------------- duuude rcons=\n"<<_rcons<<endl;
}

/**
 * Try connecting this connector set sequence, from the left, to what
 * was passed in ctor.  It is preseumed that left_sp is a single parse
 * state: it should contain no left-pointing connectors whatsoever.  This
 * routine will attempt to attach the right-pointing connectors to the 
 * left-pointers passed in the ctor.  A connection is considered to be
 * successful if *all* left-pointers in the ctor were attached (except
 * possibly for optionals).  The returned value is a set of all possible
 * alternative state pairs for which there is a connnection.
 *
 * Connectors must be satisfied sequentially: that is, the first connector
 * set in the sequence must be fully satisfied before a connection is made
 * to the second one in the sequence, etc. (counting from zero).
 */
Set* Connect::try_connect(StatePair* left_sp)
{
	Set* bogo = try_connect_a(left_sp);
	if (bogo)
		return bogo;

assert(0, "Parse fail, implement me");
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

/// Same as try_connect(), except that it doesn't handle the cases
/// where the right set doesn't connect to the left.
Set* Connect::try_connect_a(StatePair* left_sp)
{
	Seq* left_seq = left_sp->get_state();

// XXX this is wrong, but works for just now .. 
// wrong because maybe we need next_connect in a loop!?
// wrong thecause the sequence may be a list of several unconnected
// words and we have to loop over all of them.
	WordCset* swc = dynamic_cast<WordCset*>(left_seq->get_outgoing_atom(0));
	Set* pair_set = next_connect(swc);

	return pair_set;
}

/**
 * Try connecting this connector set, from the left, to what was passed
 * in ctor.  This returns a set of alternative connections, as state
 * pairs: each alternative will consist of new state, and the links that
 * were issued.
 */
Set* Connect::next_connect(WordCset* left_cset)
{
	assert(left_cset, "State word-connectorset is null");
	Atom* left_a = left_cset->get_cset();

cout<<"enter next_connect, state cset dnf "<< left_a <<endl;

	// Wrap bare connector with OR; this simplifie the nested loop below.
	Or* left_dnf = NULL;
	if (CONNECTOR == left_a->get_type())
		left_dnf = new Or(left_a);
	else
		left_dnf = dynamic_cast<Or*>(upcast(left_a));
	assert(left_dnf != NULL, "Left disjuncts not in DNF");

	Atom *right_a = _rcons;
cout<<"in next_connect, word cset dnf "<< right_a <<endl;
	Or* right_dnf = NULL;
	if (CONNECTOR == right_a->get_type())
		right_dnf = new Or(right_a);
	else
		right_dnf = dynamic_cast<Or*>(upcast(right_a));
	assert(right_dnf != NULL, "Right disjuncts not in DNF");


	// At this point, both left_dnf and right_dnf should be in
	// disjunctive normal form.  Perform a nested loop over each
	// of them, connecting each to each.

	// "alternatives" records the various different successful ways
	// that connectors can be mated.  Its a list of state pairs.
	OutList alternatives;

	size_t lsz = left_dnf->get_arity();
	for (size_t i=0; i<lsz; i++)
	{
		Atom* ldj = left_dnf->get_outgoing_atom(i);
		Connector* lcon = dynamic_cast<Connector*>(ldj);

		if (lcon)
		{
			size_t rsz = right_dnf->get_arity();
			for (size_t j=0; j<rsz; j++)
			{
				Atom* rdj = right_dnf->get_outgoing_atom(j);
				Connector* rcon = dynamic_cast<Connector*>(rdj);		

				if (rcon)
				{
					Ling* conn = conn_connect_nn(lcon, rcon);
					if (!conn)
						continue;
cout<<"got one it is "<<conn<<endl;

					// At this point, conn holds an LG link type, and the
					// two disjuncts that were mated.  Re-assemble these
					// into a pair of word_disjuncts (i.e. stick the word
					// back in there, as that is what later stages need).
					Seq* out = new Seq(reassemble(conn, left_cset, _right_cset));

					// Meanwhile, we exhausted the state, so that's empty.
					StatePair* sp = new StatePair(new Seq(), out);
					alternatives.push_back(sp);
				}
				else
				{
					And* rand = dynamic_cast<And*>(upcast(rdj));
					assert(rand, "Right dj not a conjunction");

					Atom* rfirst = rand->get_outgoing_atom(0);
					Connector* rfc = dynamic_cast<Connector*>(rfirst);
					assert(rfc, "Exepcting a connector in the right conjunct");

					Ling* conn = conn_connect_nn(lcon, rfc);
					if (!conn)
						continue;
cout<<"super got one it is "<<conn<<endl;

					// At this point, conn holds an LG link type, and the
					// two disjuncts that were mated.  Re-assemble these
					// into a pair of word_disjuncts (i.e. stick the word
					// back in there, as that is what later stages need).
					Seq* out = new Seq(reassemble(conn, left_cset, _right_cset));

					// The right cset bettter not have any left-pointing 
               // links, since these cannot be satisfied ...
assert(0, "not done yet");
#if 0
					OutList state = land->get_outgoing_set();
					state.erase(state.begin());
					StatePair* sp = new StatePair(new Seq(state), out);
					alternatives.push_back(sp);
cout<<"state pair just crated: "<<sp<<endl;
#endif
				}
			}
		}
		else
		{
			And* land = dynamic_cast<And*>(upcast(ldj));
			assert(land, "Left dj not a conjunction");
			size_t rsz = right_dnf->get_arity();
			for (size_t j=0; j<rsz; j++)
			{
				Atom* rdj = right_dnf->get_outgoing_atom(j);
				Connector* rcon = dynamic_cast<Connector*>(rdj);		

				if (rcon)
				{
					Atom* lfirst = land->get_outgoing_atom(0);
					Connector* lfc = dynamic_cast<Connector*>(lfirst);
					assert(lfc, "Exepcting a connector in the left conjunct");

					Ling* conn = conn_connect_nn(lfc, rcon);
					if (!conn)
						continue;
cout<<"yah got one it is "<<conn<<endl;

					// At this point, conn holds an LG link type, and the
					// two disjuncts that were mated.  Re-assemble these
					// into a pair of word_disjuncts (i.e. stick the word
					// back in there, as that is what later stages need).
					Seq* out = new Seq(reassemble(conn, left_cset, _right_cset));

					// The state is now everything left in the conjunct.
					// We need top build this back up into WordCset.
					OutList remaining_cons = land->get_outgoing_set();
					remaining_cons.erase(remaining_cons.begin());
               And* remaining_cj = new And(remaining_cons);
					WordCset* rem_cset = new WordCset(left_cset->get_word(), remaining_cj);
 
					StatePair* sp = new StatePair(new Seq(rem_cset), out);
					alternatives.push_back(sp);
cout<<"state pair just crated: "<<sp<<endl;
				}
				else
				{
					And* rand = dynamic_cast<And*>(upcast(rdj));
					assert(rand, "Right dj not a conjunction");

					while (1)
					{
						Atom* rfirst = rand->get_outgoing_atom(0);
						Connector* rfc = dynamic_cast<Connector*>(rfirst);
						assert(rfc, "Exepcting a connector in the right conjunct");

						Atom* lfirst = land->get_outgoing_atom(0);
						Connector* lfc = dynamic_cast<Connector*>(lfirst);
						assert(lfc, "Exepcting a connector in the left conjunct");

cout<<"duude land="<<land<<endl;
cout<<"duude rand="<<rand<<endl;
						Ling* conn = conn_connect_nn(lfc, rfc);
						if (!conn)
							break;
cout<<"super got one it is "<<conn<<endl;

						// At this point, conn holds an LG link type, and the
						// two disjuncts that were mated.  Re-assemble these
						// into a pair of word_disjuncts (i.e. stick the word
						// back in there, as that is what later stages need).
						Seq* out = new Seq(reassemble(conn, left_cset, _right_cset));

						// The right cset bettter not have any left-pointing 
               	// links, since these cannot be satisfied ...
assert(0, "not done yet");
#if 0
						OutList state = land->get_outgoing_set();
						state.erase(state.begin());
						StatePair* sp = new StatePair(new Seq(state), out);
						alternatives.push_back(sp);
cout<<"state pair just crated: "<<sp<<endl;
#endif
					}
				}
			}
		}
	}

	return new Set(alternatives);
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
	assert(conn, "Bad cast to Ling");

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
cout<<"duude come from aa"<<endl;
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

	assert(0, "Ohhh no mr billl");
	return NULL;
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
cout<<"duude looping over ith term="<<i<< " term is "<<a<<endl;
		Connector* chinode = dynamic_cast<Connector*>(a);
		if (chinode) 
		{
			// XXX TODO -- I think this is the right response here, not sure...
			if (chinode->get_name() == OPTIONAL_CLAUSE)
				continue;

cout<<"duude got the chinode "<<endl;
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
cout<<"duuude righty is "<<ratom<<endl;
				Link* rv = conn_connect_na(chinode, ratom);

				// All connectors must be satsisfied.
				assert(0, "Implement me cnode AND");
			}
		}
		else
		{	
cout<<"duude not the chinode!"<<endl;
			Link* clink = dynamic_cast<Link*>(a);
			assert(clink, "Child should be link");
			Link* rv = conn_connect_ka(clink, ratom);
			if (rv)
				alternatives.push_back(rv);
		}
	}

cout<< "duude done with recur, alter sz="<<alternatives.size()<<endl;
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
