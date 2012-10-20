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
	:  _state(NULL), _output(NULL)
{
	set_clean_state(initial_state_set);
}

// ===================================================================
/** convenience wrapper */
Set* State::get_state()
{
	return _state;
}

void State::set_clean_state(Set* s)
{
	// Clean it up, first, by removing empty state vectors.
	const OutList& states = s->get_outgoing_set();
	OutList new_states;
	for (int i = 0; i < states.size(); i++)
	{
		Link* l = dynamic_cast<Link*>(states[i]);
		if (l->get_arity() != 0)
			new_states.push_back(l);
	}
	
	_state = new Set(new_states);
}

Set* State::get_output_set()
{
	return _output;
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
   DBG(cout << "Initial state:\n" << get_state() << endl);
	Set* state_set = get_state();
	Set* new_state_set = state_set;
	Connect cnct(wrd_cset);

	// The state set consists of a bunch of sequences; each sequence
	// being a single parse state.  Each parse state is a sequence of
	// unsatisfied right-pointing links.
	for (int i = 0; i < state_set->get_arity(); i++)
	{
		Seq* seq = dynamic_cast<Seq*>(state_set->get_outgoing_atom(i));
		assert(seq, "Missing state");

		OutList state_vect = seq->get_outgoing_set();
		bool state_vect_modified = false;

		// Each state sequence consists of a sequence of right-pointing
		// links. These must be sequentially satisfied: This is the
		// viterbi equivalent of "planar graph" or "no-crossing-links"
		// in the classical lnk-grammar parser.  That is, a new word
		// must link to the first sequence element that has dangling
		// right-pointing connectors.
		Set* alternatives = cnct.try_connect(seq);

		if (alternatives)
		{
cout<<"Got linkage "<< alternatives <<endl;
			OutList alt_links;
			OutList danglers;
			for (int j = 0; j < alternatives->get_arity(); j++)
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
		}
		else
		{
			// If we are here, then nothing in the new word was
			// able to attach to the left.  If the new word has
			// no left-pointing links, that's just fine; add it
			// to the state.  If it does have left-pointing
			// links, then its a parse failure.
			WordCset* new_wcset = cset_trim_left_pointers(wrd_cset);
			if (NULL == new_wcset)
			{
assert(0, "Parse fail, implement me");
			}
			state_vect.insert(state_vect.begin(), new_wcset);
			state_vect_modified = true;
		}

		// If the state vector was modified, then record it.
		if (state_vect_modified)
		{
			Seq* new_vect = new Seq(state_vect);

			OutList nssv = new_state_set->get_outgoing_set();
			nssv[i] = new_vect;
			new_state_set = new Set(nssv);
		}
	}

	set_clean_state(new_state_set);
}

// ===================================================================

/// Trim away all optional left pointers (connectors with - direction)
/// If there are any non-optional left-pointers, then return NULL.
///
/// XXX Wait, I'm confused: it could also be null if everything
/// was optional, and everything was left-facing, so everything got
/// trimmed away... but htat's OK, because ... ermmmm
WordCset* State::cset_trim_left_pointers(WordCset* wrd_cset)
{
	Atom* trimmed = trim_left_pointers(wrd_cset->get_cset());
	if (!trimmed)
		return NULL;
	return new WordCset(wrd_cset->get_word(), trimmed);
}

Atom* State::trim_left_pointers(Atom* a)
{
	Connector* ct = dynamic_cast<Connector*>(a);
	if (ct)
	{
		if (ct->is_optional())
			return a;
		char dir = ct->get_direction();
		if ('-' == dir) return NULL;
		if ('+' == dir) return a;
		assert(0, "Bad word direction");
	}

	AtomType ty = a->get_type();
	assert (OR == ty or AND == ty, "Must be boolean junction");

	if (OR == ty)
	{
		OutList new_ol;
		const Link* l = dynamic_cast<Link*>(a);
		for (int i = 0; i < l->get_arity(); i++)
		{
			Atom* ota = l->get_outgoing_atom(i);
			Atom* new_ota = trim_left_pointers(ota);
			if (new_ota)
				new_ol.push_back(new_ota);
		}
		if (0 == new_ol.size())
			return NULL;

		// The result of trimming may be multiple empty nodes. 
		// Remove all but one of them.
		bool got_opt = false;
		for (int i = 0; i < new_ol.size(); i++)
		{
			Connector* c = dynamic_cast<Connector*>(new_ol[i]);
			if (c and c->is_optional())
			{
				if (!got_opt)
					got_opt = true;
				else
					new_ol.erase(new_ol.begin() + i);
			}
		}

		if (1 == new_ol.size())
		{
			// If the entire OR-list was pruned down to one connector,
			// and that connector is the empty connector, then it
			// "connects to nothing" on the left, and should be removed.
			Connector* c = dynamic_cast<Connector*>(new_ol[0]);
			if (c and c->is_optional())
				return NULL;
			return new_ol[0];
		}

		return new Link(OR, new_ol);
	}

	// If we are here, then it an andlist, and all conectors are
	// mandatory, unless they are optional.  So fail, if the
	// connectors that were trimmed were not optional.
	OutList new_ol;
	Link* l = dynamic_cast<Link*>(a);
	for (int i = 0; i < l->get_arity(); i++)
	{
		Atom* ota = l->get_outgoing_atom(i);
		Atom* new_ota = trim_left_pointers(ota);
		if (new_ota)
			new_ol.push_back(new_ota);
		else
			if (!is_optional(ota))
				return NULL;
	}

	if (0 == new_ol.size())
		return NULL;

	if (1 == new_ol.size())
		return new_ol[0];

	return new Link(AND, new_ol);
}

}} // end of namespaces
