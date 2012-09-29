/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
/*                                                                       */
/*************************************************************************/

#include <iostream>
#include <string>
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

/* Current parse state */

Link * Parser::word_disjuncts(const string& word)
{
	// First atom at the from of the outgoing set is the word itself.
	vector<Atom*> dlist;
	Node *nword = new Node(WORD, word);
	dlist.push_back(nword);

	// See if we know about this word, or not.
	Dict_node* dn = dictionary_lookup_list(_dict, word.c_str());
	if (!dn) return NULL;

	Exp* exp = dn->exp;
	print_expression(exp);

	if (OR_type == exp->type)
	{
cout<<"duude or type"<<endl;
	}

	if (AND_type == exp->type)
	{
cout<<"duude and type"<<endl;
	}

	Link *dj = new Link(WORD_DISJ, dlist);
	return dj;
}

void Parser::initialize_state()
{
	const char * wall_word = "LEFT-WALL";

	Link *wall_disj = word_disjuncts(wall_word);

	vector<Atom*> statev;
	statev.push_back(wall_disj);

	_state = new Link(STATE, statev);
}

void viterbi_parse(Dictionary dict, const char * sentence)
{
	Parser pars(dict);

	// Initial state
	pars.initialize_state();
cout <<"Hello world!"<<endl;

}

} // namespace viterbi
} // namespace link-grammar

void viterbi_parse(const char * sentence, Dictionary dict)
{
	link_grammar::viterbi::viterbi_parse(dict, sentence);
}

