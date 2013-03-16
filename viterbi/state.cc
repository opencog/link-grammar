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
#include <sstream>
#include <vector>

#include "atom.h"
#include "compile.h"
#include "connect.h"
#include "connector-utils.h"
#include "state.h"
#include "viterbi.h"

using namespace std;

// #define DBG(X) X;
#define DBG(X) 

namespace link_grammar {
namespace viterbi {

State::State(Set* initial_state_set)
	:  _alternatives(NULL)
{
	set_clean_state(initial_state_set);
}

// ===================================================================
/** convenience wrapper */
Set* State::get_alternatives()
{
	return _alternatives;
}

// XXX is this really needed ???
void State::set_clean_state(Set* s)
{
	// Clean it up, first, by removing empty state vectors.
	const OutList& alts = s->get_outgoing_set();
	OutList new_alts;
	for (int i = 0; i < alts.size(); i++)
	{
		StatePair* s = dynamic_cast<StatePair*>(alts[i]);
		Seq* state = s->get_state();
		Seq* output = s->get_output();
		if (state->get_arity() != 0)
			new_alts.push_back(s);
	}
	
	_alternatives = new Set(new_alts);
}

// ===================================================================
/**
 * Add a single dictionary entry to the parse state, if possible.
 */
void State::stream_word_conset(WordCset* wrd_cset)
{
	// wrd_cset should be pointing at:
	// WORD_CSET : 
	//   WORD : blah.v
	//   AND :
	//      CONNECTOR : Wd-  etc...

   DBG(cout << "--------- enter stream_word_conset ----------" << endl);
   DBG(cout << "Initial alternative_set:\n" << get_alternatives() << endl);
	Connect cnct(wrd_cset);

	// The state set consists of a bunch of sequences; each sequence
	// being a single parse state.  Each parse state is a sequence of
	// unsatisfied right-pointing links.
	Set* new_alts = new Set();
	Set* alts = get_alternatives();
	for (int i = 0; i < alts->get_arity(); i++)
	{
		StatePair* sp = dynamic_cast<StatePair*>(alts->get_outgoing_atom(i));
		assert(sp, "Missing state");

		// Each state sequence consists of a sequence of right-pointing
		// links. These must be sequentially satisfied: This is the
		// viterbi equivalent of "planar graph" or "no-crossing-links"
		// in the classical link-grammar parser.  That is, a new word
		// must link to the first sequence element that has dangling
		// right-pointing connectors.
		Set* next_alts = cnct.try_connect(sp);
		new_alts = new_alts->add(next_alts);
	}
	_alternatives = new_alts;

	// set_clean_state(new_state_set);
}

}} // end of namespaces
