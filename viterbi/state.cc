/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
/*                                                                       */
/*************************************************************************/


#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <link-grammar/link-includes.h>
#include "api-types.h"
#include "read-dict.h"
#include "structures.h"

#include "atom.h"
#include "parser.h"
#include "viterbi.h"

using namespace std;

namespace link_grammar {
namespace viterbi {

Parser::Parser(Dictionary dict)
	: _dict(dict), _state(NULL)
{
	initialize_state();
}

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
	if (CONNECTOR_type == exp->type)
	{
		stringstream ss;
		if (exp->multi) ss << "@";
		ss << exp->u.string << exp->dir;
		return new Node(CONNECTOR, ss.str());
	}

	// Whenever a null appears in an OR-list, it means the 
	// entire OR-list is optional.  A null can never appear
	// in an AND-list.
	E_list* el = exp->u.l;
	if (NULL == el)
		return new Node(CONNECTOR, "0");

	// The data structure that link-grammar uses for connector
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
		return new Link(AND, alist);

	if (OR_type == exp->type)
		return new Link(OR, alist);

	assert(0, "Not reached");
}

/**
 * Return atomic formula connector expression for the given word.
 *
 * This looks up the word in the link-grammar dictionary, and converts
 * the resulting link-grammar connective expression into an atomic
 * formula.
 */
Link * Parser::word_disjuncts(const string& word)
{
	// See if we know about this word, or not.
	Dict_node* dn = dictionary_lookup_list(_dict, word.c_str());
	if (!dn) return NULL;

	Exp* exp = dn->exp;
print_expression(exp);

	Atom *dj = lg_exp_to_atom(exp);

	// First atom at the from of the outgoing set is the word itself.
	// Second atom is the first disjuct that must be fulfilled.
	OutList dlist;
	Node *nword = new Node(WORD, word);
	dlist.push_back(nword);
	dlist.push_back(dj);

	return new Link(WORD_DISJ, dlist);
}

/**
 * Set up initial viterbi state for the parser
 */
void Parser::initialize_state()
{
	const char * wall_word = "LEFT-WALL";

	Link *wall_disj = word_disjuncts(wall_word);

	OutList statev;
	statev.push_back(wall_disj);

	_state = new Link(STATE, statev);
}

/**
 * Add a single word to the parse.
 */
void Parser::stream_word(const string& word)
{
	Link *disj = word_disjuncts(word);
	if (!disj)
	{
		cout << "Unhandled error; word not in dict: " << word << endl;
		return;
	}
}

/**
 * Add a stream of text to the input.
 *
 * No particular assumptiions are made about the input, other than
 * that its space-separated words (i.e. no HTML markup or other junk)
 */
void Parser::streamin(const string& text)
{
	stream_word(text);
}

void viterbi_parse(Dictionary dict, const char * sentence)
{
	Parser pars(dict);

	pars.streamin(sentence);
}

} // namespace viterbi
} // namespace link-grammar

// Wrapper to escape out from C++
void viterbi_parse(const char * sentence, Dictionary dict)
{
	link_grammar::viterbi::viterbi_parse(dict, sentence);
}

