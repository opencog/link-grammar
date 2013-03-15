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
#include "compress.h"
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

/// Unite two states into one. 
///
/// This is needed to implement zippering properly: The old set of
/// states is the collection of as-yet unconnected connectors; the 
/// new set of states is the collection that remains after connecting.
/// We have to peel off and discard some certain number of the old
/// states (as these are now connected), ad append in their place
/// the new states.  We also typically peel off one new one, as that
/// one will be used for trying new onnections.
static StatePair* unite(StatePair* old_sp, size_t old_peel_off,
                        StatePair* new_sp, size_t new_peel_off)
{
	OutList united_states;
	Seq* old_state = old_sp->get_state();
	Seq* new_state = new_sp->get_state();

	const OutList& oo = old_state->get_outgoing_set();
	united_states.insert(united_states.end(), 
	                     oo.begin() + old_peel_off, oo.end());

	const OutList& no = new_state->get_outgoing_set();
	united_states.insert(united_states.end(), 
	                     no.begin() + new_peel_off, no.end());

	// Unite the outputs too ... 
	// This is easy, just concatenate old and append new.
	OutList united_outputs;
	Seq* old_output = old_sp->get_output();
	Seq* new_output = new_sp->get_output();

	const OutList& ooo = old_output->get_outgoing_set();
	united_outputs.insert(united_outputs.end(), ooo.begin(), ooo.end());

	const OutList& noo = new_output->get_outgoing_set();
	united_outputs.insert(united_outputs.end(), noo.begin(), noo.end());

	return new StatePair(new Seq(united_states), new Seq(united_outputs));
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
// XXX most of this should probably be moved to State class
Set* Connect::try_connect(StatePair* left_sp)
{
	// Zipper up the zipper.
	// The left state can the thought of as a strand of zipper teeth,
	// or half of a strand of DNA, if you wish. Each of the zipper teeth
	// are right-pointing connectors.  These must be mated with the
	// left-pointing connectors from the right cset.  The reason its
	// zipper-like is that the right cset might connect to multiple csets
	// on the left.  The connectins must be made in order, and so we loop
	// through the left state, first trying to satisfy all connectors in
	// the first cset, then the second, and so on, until all of the
	// left-pointing connectors in the right cset have been connected,
	// or they're optional, or there is a failure to connect. 
	Seq* left_state = left_sp->get_state();
	Atom* a = left_state->get_outgoing_atom(0);
	WordCset* lwc = dynamic_cast<WordCset*>(a);
	Set* alternatives = next_connect(lwc);
cout<<"wtfffff got alts"<<alternatives<<endl;

	size_t lsz = left_state->get_arity();
	size_t lnext = 1;

	// OK, so do any of the alternatives include state with
	// left-pointing connectors?  If not, then we are done. If so,
	// then these need to be mated to the next word cset on the left.
	// If they can't be mated, then fail, and we are done.
	OutList filtered_alts;
	size_t sz = alternatives->get_arity();
cout<<"wtffff got arity="<<sz<<endl;
	for (size_t i=0; i<sz; i++)
	{
cout<<"wtffff enter i="<<i<<endl;
		Atom* a = alternatives->get_outgoing_atom(i);
		StatePair* new_sp = dynamic_cast<StatePair*>(a);
		Seq* new_state = new_sp->get_state();

		if (0 < new_state->get_arity())
		{
			a = new_state->get_outgoing_atom(0);
			WordCset* new_cset = dynamic_cast<WordCset*>(a);
			if (new_cset->has_left_pointers())
			{
				// The left-pointers are either mandatory or optional.
				// If they're mandatory and there is no state to zipper with,
				// then its a parse fail. Otherwise recurse.
// XX check for optional...
				if (lnext < lsz)
				{
					Atom* a = left_state->get_outgoing_atom(lnext);
				
cout <<"rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrecurse"<<endl;
cout << "old sp, rm "<<lnext<<": " << left_sp<<endl;
cout << "new sp rm 1:" << new_sp<<endl;
					// Recurse
					StatePair* united_sp = unite(left_sp, lnext, new_sp, 1);
cout << "United states:" << united_sp<<endl;
					Connect recurse(new_cset);
					Set* new_alts = recurse.try_connect(united_sp);
cout << "woot got this:" << new_alts<<endl;

					size_t nsz = new_alts->get_arity();
					for (size_t k = 0; k < nsz; k++)
					{
						Atom* a = new_alts->get_outgoing_atom(k);
						StatePair* asp = dynamic_cast<StatePair*>(a);
						StatePair* mrg = unite(united_sp, 1, asp, 0);
						filtered_alts.push_back(mrg);
cout << "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmerge result="<<mrg<<endl;
					}
					continue;
				}
			}
		}
		
		// Append old and new output and state, before pushing back.
		// XXX I think the below is correct, its untested.
cout<<"mooooooooooooooooooooooooooooooooooooooooooooore"<<endl;
		StatePair* mrg = unite(left_sp, lnext, new_sp, 0);
cout<<"left sp was "<<left_sp<<endl;
cout<<"new sp was "<<new_sp<<endl;
cout<<"mrg is "<<mrg<<endl;
		filtered_alts.push_back(mrg);
	}

cout<<"wtfffff returning"<<new Set(filtered_alts)<<endl;
	return new Set(filtered_alts);
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
	AtomType lt = left_a->get_type();
	if (CONNECTOR == lt or AND == lt)
		left_dnf = new Or(left_a);
	else
		left_dnf = dynamic_cast<Or*>(upcast(left_a));
	assert(left_dnf != NULL, "Left disjuncts not in DNF");

	Atom *right_a = _rcons;
cout<<"in next_connect, word cset dnf "<< right_a <<endl;
	Or* right_dnf = NULL;
	AtomType rt = right_a->get_type();
	if (CONNECTOR == rt or AND == rt)
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

	return compress_alternatives(new Set(alternatives));
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
		Seq* state = new Seq(rcset, lcset);
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
// XXX fixme ;;; if all left-pointers are opt, then OK to create state ...
	if (!conn)
		return NULL;

	// At this point, conn holds an LG link type, and the
	// two disjuncts that were mated.  Re-assemble these
	// into a pair of word_disjuncts (i.e. stick the word
	// back in there, as that is what later stages need).
	Seq* out = new Seq(conn);

	// The state is now everything else left in the disjunct.
	// We need to build this back up into WordCset.
	OutList remaining_cons = rand->get_outgoing_set();
	remaining_cons.erase(remaining_cons.begin());
	And* remaining_cj = new And(remaining_cons);
	Atom* rema = remaining_cj->super_flatten();
	WordCset* rem_cset = new WordCset(_right_cset->get_word(), rema);

	StatePair* sp = new StatePair(new Seq(rem_cset), out);
   DBG(cout<<"----- right multi-conn alternative created:\n" << sp << endl;);
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
