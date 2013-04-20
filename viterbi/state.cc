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

// XXX is this really needed ???
void State::set_clean_state(Set* s)
{
	size_t sz = s->get_arity();
	// Clean it up, first, by removing empty state vectors.
	const OutList& alts = s->get_outgoing_set();
	OutList new_alts;
	for (int i = 0; i < sz; i++)
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
Set* State::stream_word_conset(WordCset* wrd_cset)
{
	// wrd_cset should be pointing at a word connector set:
	// WORD_CSET : 
	//   WORD : blah.v
	//   AND :
	//      CONNECTOR : Wd-  etc...

   DBG(cout << "--------- enter stream_word_conset ----------" << endl);
   DBG(cout << "Initial alternative_set:\n" << _alternatives << endl);
	Connect cnct(wrd_cset);

	// set_clean_state(new_state_set);
	return cnct.try_connect(_alternatives);
}

}} // end of namespaces
