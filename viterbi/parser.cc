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

#include <link-grammar/link-includes.h>
#include "api-types.h"
#include "read-dict.h"
#include "structures.h"

#include "atom.h"
#include "compile.h"
#include "disjoin.h"
#include "garbage.h"
#include "parser.h"
#include "viterbi.h"
#include "word-monad.h"

using namespace std;

#define DBG(X) X;

namespace link_grammar {
namespace viterbi {

Parser::Parser(Dictionary dict)
	: _dict(dict), _alternatives(NULL)
{
	DBG(cout << "=============== Parser ctor ===============" << endl);
	lg_init_gc();
	initialize_state();
}

// ===================================================================
/**
 * Convert LG dictionary expression to atomic formula.
 *
 * The returned expression is in the form of an opencog-style
 * prefix-notation boolean expression.  Note that it is not in any
 * particular kind of normal form.  In particular, some AND nodes
 * may have only one child: these can be removed.
 *
 * Note that the order of the connectors is important: while linking,
 * these must be satisfied in left-to-right (nested!?) order.
 *
 * Optional clauses are indicated by OR-ing with null, where "null"
 * is a CONNECTOR Node with string-value "0".  Optional clauses are
 * not necessarily in any sort of normal form; the null connector can
 * appear anywhere.
 */
Atom * Parser::lg_exp_to_atom(Exp* exp)
{
	// log-likelihood is identical to the link-grammar cost.
	float likli = exp->cost;

	if (CONNECTOR_type == exp->type)
	{
		stringstream ss;
		if (exp->multi) ss << "@";
		ss << exp->u.string << exp->dir;

		return new Connector(ss.str(), likli);
	}

	// Whenever a null appears in an OR-list, it means the
	// entire OR-list is optional.  A null can never appear
	// in an AND-list.
	E_list* el = exp->u.l;
	if (NULL == el)
		return new Connector(OPTIONAL_CLAUSE, likli);

	// The C data structure that link-grammar uses for connector
	// expressions is totally insane, as witnessed by the loop below.
	// Anyway: operators are infixed, i.e. are always binary,
	// with exp->u.l->e being the left hand side, and
	//      exp->u.l->next->e being the right hand side.
	// This means that exp->u.l->next->next is always null.
	OutList alist;
	alist.push_back(lg_exp_to_atom(el->e));
	el = el->next;

	while (el && exp->type == el->e->type)
	{
		el = el->e->u.l;
		alist.push_back(lg_exp_to_atom(el->e));
		el = el->next;
	}

	if (el)
		alist.push_back(lg_exp_to_atom(el->e));

	if (AND_type == exp->type)
		return new And(alist, likli);

	if (OR_type == exp->type)
		return new Or(alist, likli);

	assert(0, "Not reached");
}

// ===================================================================
/**
 * Return atomic formula connector expression for the given word.
 *
 * This looks up the word in the link-grammar dictionary, and converts
 * the resulting link-grammar connective expression into an formula
 * composed of atoms.
 */
Set * Parser::word_consets(const string& word)
{
	// See if we know about this word, or not.
	Dict_node* dn_head = dictionary_lookup_list(_dict, word.c_str());
	if (!dn_head) return NULL;

	OutList djset;
	for (Dict_node*dn = dn_head; dn; dn= dn->right)
	{
		Exp* exp = dn->exp;
		DBG({cout << "=============== Parser word: " << dn->string << ": ";
			print_expression(exp); });

		Atom *dj = lg_exp_to_atom(exp);
		dj = disjoin(dj);

		// First atom at the front of the outgoing set is the word itself.
		// Second atom is the first disjuct that must be fulfilled.
		Word* nword = new Word(dn->string);
		djset.push_back(new WordCset(nword, dj));
	}
	free_lookup_list(dn_head);
	return new Set(djset);
}

// ===================================================================
/**
 * Set up initial viterbi state for the parser
 */
void Parser::initialize_state()
{
	const char * wall_word = "LEFT-WALL";

	Set *wall_disj = word_consets(wall_word);

	// We are expecting the initial wall to be unique.
	assert(wall_disj->get_arity() == 1, "Unexpected wall structure");
	OutList state_vec;
	Atom* wall_cset = wall_disj->get_outgoing_atom(0);

	// Initial state: no output, and the wall cset.
	_alternatives = new Set(
		new StateTriple(
			new Seq(),
			new Seq(wall_cset),
			new Set()
		)
	);
}

// ===================================================================
/**
 * Add a single word to the parse.
 */
void Parser::stream_word(const string& word)
{
	// Look up the dictionary entries for this word.
	Set *djset = word_consets(word);
	if (!djset)
	{
		cout << "Unhandled error; word not in dict: >>" << word << "<<" << endl;
		assert (0, "word not in dict");
		return;
	}

	// Try to add each dictionary entry to the parse state.
	Set* new_alts = new Set();
	for (int i = 0; i < djset->get_arity(); i++)
	{
		Atom* cset = djset->get_outgoing_atom(i);
		WordCset* wrd_cset = dynamic_cast<WordCset*>(cset);

		WordMonad cnct(wrd_cset);
		Set* alts = cnct(_alternatives);
		new_alts = new_alts->add(alts);
	}
	_alternatives = new_alts;
}

// ===================================================================
/** convenience wrapper */
Set* Parser::get_alternatives()
{
	return _alternatives;
}

// ===================================================================
/**
 * Add a stream of text to the input.
 *
 * No particular assumptiions are made about the input, other than
 * that its space-separated words (i.e. no HTML markup or other junk)
 */
void Parser::streamin(const string& text)
{
	// A trivial tokenizer
	size_t pos = 0;
	while(true)
	{
		size_t wend = text.find(' ', pos); // wend == word end
		if (wend != string::npos)
		{
			const string word = text.substr(pos, wend-pos);
			stream_word(word);
			pos = wend + 1; // skip over space
			while (' ' == text[pos])
				pos++;
		}
		else
		{
			const string word = text.substr(pos);
			if (0 < word.size())
				stream_word(word);
			break;
		}
	}
}

// Send in the right wall -- the traditional link-grammar
// design wants this to terminate sentences.
void Parser::stream_end()
{
	const char * right_wall_word = "RIGHT-WALL";
	Set *wall_disj = word_consets(right_wall_word);

	// We are expecting the initial wall to be unique.
	assert(wall_disj->get_arity() == 1, "Unexpected wall structure");
	Atom* wall_cset = wall_disj->get_outgoing_atom(0);
	WordCset* rwcs = dynamic_cast<WordCset*>(wall_cset);

	WordMonad cnct(rwcs);
	_alternatives = cnct(_alternatives);
}

void viterbi_parse(Dictionary dict, const char * sentence)
{
	Parser pars(dict);

	pars.streamin(sentence);

	// The old link-grammar design insists on  having a RIGHT-WALL,
	// so provide one.
	pars.stream_end();

	Link* alts = pars.get_alternatives();

	/* Print some diagnostic outputs ... for now. Remove when finished. */
	size_t num_alts = alts->get_arity();
	printf("Found %lu alternatives\n", num_alts);
	for (size_t i=0; i<num_alts; i++)
	{
		Atom* a = alts->get_outgoing_atom(i);
		StateTriple* sp = dynamic_cast<StateTriple*>(a);
		Seq* state = sp->get_state();
		size_t state_sz = state->get_arity();
		if (0 == state_sz)
		{
			cout << "\nAlternative =============== " << i << endl;
			cout << sp->get_output() << endl;
		}
		else
		{
			cout << "\nIncomplete parse =============== " << i << endl;
			cout << sp->get_output() << endl;
			cout << "\n---- state for ----" << i << endl;
			cout << sp->get_state() << endl;
		}
	}
}

} // namespace viterbi
} // namespace link-grammar

// ===================================================================

// Wrapper to escape out from C++
void viterbi_parse(const char * sentence, Dictionary dict)
{
	link_grammar::viterbi::viterbi_parse(dict, sentence);
}

