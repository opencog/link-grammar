/*************************************************************************/
/* Copyright (c) 2012, 2013 Linas Vepstas <linasvepstas@gmail.com>       */
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

#define DBG(X) X;

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
	_left_cset = left_cset;
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

		size_t rsz = right_dnf->get_arity();
		for (size_t j=0; j<rsz; j++)
		{
			Atom* rdj = right_dnf->get_outgoing_atom(j);

			StatePair* sp = try_alternative(ldj, rdj);
			if (sp)
				alternatives.push_back(sp);
		}
	}

	return new Set(alternatives);
}

// =============================================================

/// Try to connect the left and right disjuncts.
///
/// If the connection attempt is successful, then return a
/// StatePair given the emitted output, and the resulting state.
///
/// The implementation below is just a dispatcher for each of the
/// alternative handlers, depending on whether the arguments are
/// single or multi-connectors.
StatePair* Connect::try_alternative(Atom* ldj, Atom* rdj)
{
	Connector* lcon = dynamic_cast<Connector*>(ldj);
	Connector* rcon = dynamic_cast<Connector*>(rdj);

	// Left disjunct is a single connector
	if (lcon)
	{
		// Right disjunct is a single connector
		if (rcon)
			return alternative(lcon, rcon);
		else
		{
			// Right disunct better be a multi-connector
			And* rand = dynamic_cast<And*>(upcast(rdj));
			assert(rand, "Right dj not a disjunct");
			return alternative(lcon, rand);
		}
	}
	else
	{
		// Left disjunct better be a multi-connector.
		And* land = dynamic_cast<And*>(upcast(ldj));
		assert(land, "Left dj not a disjunct");

		// Right disjunct is a single connector
		if (rcon)
			return alternative(land, rcon);
		else
		{
			// Right disunct better be a multi-connector
			And* rand = dynamic_cast<And*>(upcast(rdj));
			assert(rand, "Right dj not a disjunct");

			return alternative(land, rand);
		}
	}

	return NULL; // Not reached.
}

// =============================================================
/// Try connecting the left and right disjuncts.
///
/// If a connection was made, return the resulting state pair.
/// If no connection is possible, return NULL.
///
/// The state pair will contain the output generated (if any) and the
/// final state (if any) after the connection is made.
///
/// There are four distinct methods below, depending on whether
/// each disjunct is a single or a multi connector.  Multi-connectors
/// are just a list of conjoind (AND) single-connectors.  A multi-
/// connector is also called a "disjunct" because it is one of the
/// terms in a connector set that has been expanded into dsjunctive
/// normal form. Viz. a single disjunct is a conjoined set of
/// connectors.
//
StatePair* Connect::alternative(Connector* lcon, Connector* rcon)
{
	Ling* conn = conn_connect_nn(lcon, rcon);
	if (!conn)
	{
		// If we are here, then no connection was possible. It may
		// be the case that rcon was right-pointing, in which case,
		// it can be added to the state.
		char dir = rcon->get_direction();
		if ('-' == dir)
			return NULL;

		Word* lword = _left_cset->get_word();
		Word* rword = _right_cset->get_word();
		WordCset* lcset = new WordCset(lword, lcon);
		WordCset* rcset = new WordCset(rword, rcon);
		Seq* state = new Seq(lcset, rcset);
		StatePair* sp = new StatePair(state, new Seq());
		DBG(cout<<"------ Empty-output alternative created:\n" << sp << endl;);
		return sp;
	}

	// At this point, conn holds an LG link type, and the
	// two disjuncts that were mated.  Re-assemble these
	// into a pair of word_disjuncts (i.e. stick the word
	// back in there, as that is what later stages need).
	Seq* out = new Seq(conn);

	// Meanwhile, we exhausted the state, so that's empty.
	StatePair* sp = new StatePair(new Seq(), out);
   DBG(cout<<"----- single-connector alternative created:\n" << sp << endl;);
	return sp;
}

// See docs above
StatePair* Connect::alternative(Connector* lcon, And* rand)
{
	if (0 == rand->get_arity())
		return NULL;

cout<<"duuude rand="<<rand<<endl;
	Atom* rfirst = rand->get_outgoing_atom(0);
	Connector* rfc = dynamic_cast<Connector*>(rfirst);
	assert(rfc, "Exepcting a connector in the right disjunct");

	Ling* conn = conn_connect_nn(lcon, rfc);
	if (!conn)
		return NULL;
cout<<"super got one it is "<<conn<<endl;

	// At this point, conn holds an LG link type, and the
	// two disjuncts that were mated.  Re-assemble these
	// into a pair of word_disjuncts (i.e. stick the word
	// back in there, as that is what later stages need).
	Seq* out = new Seq(conn);

	// The right cset better not have any left-pointing
	// links, because if we are here, these cannot be
	// satisfied ...
// XXX should use the ttrim fun here
// xxxxxxx
	size_t rsz = rand->get_arity();
	bool unmatched_left_pointer = false;
	for (size_t k=1; k<rsz; k++)
	{
		Atom* ra = rand->get_outgoing_atom(k);
		Connector* rc = dynamic_cast<Connector*>(ra);
		assert(rc, "Exepcting a connector in the right disjunct");

		char dir = rc->get_direction();
		if ('-' == dir)
		{
			unmatched_left_pointer = true;
			break;
		}
	}

	// If unmatched, fail, and try again.
	if (unmatched_left_pointer)
		return NULL;

	// The state is now everything else left in the disjunct.
	// We need to build this back up into WordCset.
	OutList remaining_cons = rand->get_outgoing_set();
	remaining_cons.erase(remaining_cons.begin());
	And* remaining_cj = new And(remaining_cons);
	WordCset* rem_cset = new WordCset(_right_cset->get_word(), remaining_cj);

	StatePair* sp = new StatePair(new Seq(rem_cset), out);
cout<<"=====================> randy state pair just crated: "<<sp<<endl;
	return sp;
}

// See docs above
StatePair* Connect::alternative(And* land, Connector* rcon)
{
	Atom* lfirst = land->get_outgoing_atom(0);
	Connector* lfc = dynamic_cast<Connector*>(lfirst);
	assert(lfc, "Exepcting a connector in the left disjunct");

	Ling* conn = conn_connect_nn(lfc, rcon);
	if (!conn)
		return NULL;
cout<<"yah got one it is "<<conn<<endl;

	// At this point, conn holds an LG link type, and the
	// two disjuncts that were mated.  Re-assemble these
	// into a pair of word_disjuncts (i.e. stick the word
	// back in there, as that is what later stages need).
	Seq* out = new Seq(conn);

	// The state is now everything left in the disjunct.
	// We need to build this back up into WordCset.
	OutList remaining_cons = land->get_outgoing_set();
	remaining_cons.erase(remaining_cons.begin());
	And* remaining_cj = new And(remaining_cons);
	WordCset* rem_cset = new WordCset(_left_cset->get_word(), remaining_cj);

	StatePair* sp = new StatePair(new Seq(rem_cset), out);
cout<<"=====================> state pair just crated: "<<sp<<endl;
	return sp;
}

// See docs above
StatePair* Connect::alternative(And* land, And* rand)
{
cout<<"duude land="<<land<<endl;
cout<<"duude rand="<<rand<<endl;

	OutList outputs;
	size_t m = 0;
	size_t rsz = rand->get_arity();
	size_t lsz = land->get_arity();
	size_t sz = (lsz<rsz) ? lsz : rsz;
	while (m < sz)
	{
		Atom* rfirst = rand->get_outgoing_atom(m);
		Connector* rfc = dynamic_cast<Connector*>(rfirst);
		assert(rfc, "Exepecting a connector in the right disjunct");

		Atom* lfirst = land->get_outgoing_atom(m);
		Connector* lfc = dynamic_cast<Connector*>(lfirst);
		assert(lfc, "Exepecting a connector in the left disjunct");


		Ling* conn = conn_connect_nn(lfc, rfc);
		if (!conn)
			break;
cout<<"whoa super got one for "<< m <<" it is "<<conn<<endl;
		m++;

		// At this point, conn holds an LG link type, and the
		// two disjuncts that were mated.  Re-assemble these
		// into a pair of word_disjuncts (i.e. stick the word
		// back in there, as that is what later stages need).
		outputs.push_back(conn);
	}
cout<<"duude matched a total of m="<<m<<endl;
	if (0 == m)
		return NULL;

	// Add the un-connected parts of the left and right csets
	// to the state.  But first, check to make sure that the
	// right cset does not have any (non-optional)
	// left-pointers, because these will never be fulfilled.
	// Lets start with the right cset.
	// We need to build this back up into WordCset.
	OutList remaining_cons = rand->get_outgoing_set();
	for (size_t k = 0; k<m; k++)
		remaining_cons.erase(remaining_cons.begin());

	And* remaining_cj = new And(remaining_cons);
	WordCset* rem_cset = new WordCset(_right_cset->get_word(), remaining_cj);
	rem_cset = cset_trim_left_pointers(rem_cset);
	if (NULL == rem_cset)
		return NULL;

	// If we are here, the remaining right connectors all
	// point right.  Put them into the state.
	OutList statel;
	statel.push_back(rem_cset);

	// And now repeat for the left cset.
	remaining_cons = land->get_outgoing_set();
	for (size_t k = 0; k<m; k++)
		remaining_cons.erase(remaining_cons.begin());

	remaining_cj = new And(remaining_cons);
	rem_cset = new WordCset(_left_cset->get_word(), remaining_cj);
	statel.push_back(rem_cset);

	Seq* state = new Seq(statel);

	Seq* out = new Seq(outputs);

	StatePair* sp = new StatePair(state, out);
cout<<"=====================> multi state pair just created: "<<sp<<endl;
	return sp;
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
 * Try to connect the left and right connectors. If they do connect,
 * then return an LG_LING structure linking them.
 */
Ling* Connect::conn_connect_nn(Connector* lnode, Connector* rnode)
{
cout<<"try match connectors l="<<lnode->get_name()<<" to r="<< rnode->get_name() << endl;
	if (lnode->is_optional()) return NULL;
	if (rnode->is_optional()) return NULL;
	if (!conn_match(lnode->get_name(), rnode->get_name()))
		return NULL;

cout<<"Yayyyyae connectors match!"<<endl;
	string link_name = conn_merge(lnode->get_name(), rnode->get_name());
	Ling* ling = new Ling(link_name, lnode, rnode);

	ling = reassemble(ling, _left_cset, _right_cset);
	return ling;
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
