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
#include <sstream>
#include <vector>

#include "atom.h"
#include "compile.h"
#include "connect.h"
#include "connector-utils.h"
#include "state.h"
#include "viterbi.h"

using namespace std;

#define DBG(X) X;

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
	Set* alts = get_alternatives();
	Set* new_alts = alts;
	Connect cnct(wrd_cset);

	// The state set consists of a bunch of sequences; each sequence
	// being a single parse state.  Each parse state is a sequence of
	// unsatisfied right-pointing links.
	for (int i = 0; i < alts->get_arity(); i++)
	{
		StatePair* sp = dynamic_cast<StatePair*>(alts->get_outgoing_atom(i));
		assert(sp, "Missing state");

		Seq* state_seq = sp->get_state();
		OutList state_vect = state_seq->get_outgoing_set();
		bool state_vect_modified = false;

		// Each state sequence consists of a sequence of right-pointing
		// links. These must be sequentially satisfied: This is the
		// viterbi equivalent of "planar graph" or "no-crossing-links"
		// in the classical lnk-grammar parser.  That is, a new word
		// must link to the first sequence element that has dangling
		// right-pointing connectors.
		Set* new_alternatives = cnct.try_connect(sp);

		if (new_alternatives)
		{
cout<<"Got alternatives "<< new_alternatives <<endl;
_alternatives = new_alternatives;
#if 0
			OutList alt_links;
			OutList danglers;
			for (int j = 0; j <new_alternatives->get_arity(); j++)
			{
				Atom* a = alternatives->get_outgoing_atom(j);
				Ling* lg_link = dynamic_cast<Ling*>(a);
				if (lg_link)
					alt_links.push_back(lg_link);
				else
					danglers.push_back(a);
			}
			_output = new Set(alt_links);

			// If all links were satisfied, then remove the state
			if (0 == danglers.size())
			{
				state_vect.erase(state_vect.begin());
				state_vect_modified = true;
			}
			// If there are any danglers...
assert(0 == danglers.size(), "Implement dangler state");
#endif
		}
	}

	// set_clean_state(new_state_set);
}

}} // end of namespaces
