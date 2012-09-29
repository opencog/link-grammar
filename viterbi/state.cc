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

/* Current parse state */
Atom * Parser::lg_exp_to_atom(Exp* exp)
{
	if (CONNECTOR_type == exp->type)
	{
		stringstream ss;
		if (exp->multi) ss << "@";
		ss << exp->u.string << exp->dir;
cout<<"duude conn type "<<ss.str() <<endl;
		Node *n = new Node(CONNECTOR, ss.str());
		return n;
	}


	if ((AND_type == exp->type) || (OR_type == exp->type))
	{
		OutList alist;
		for (E_list* el = exp->u.l; el != NULL; el = el->next)
		{
			Atom* a = lg_exp_to_atom(el->e);
			alist.push_back(a);
		}

		if (AND_type == exp->type)
{ cout<<"duude and type"<<endl;
			return new Link(AND, alist);
}
		if (OR_type == exp->type)
{ cout<<"duude or type"<<endl;
			return new Link(OR, alist);
}
	}

	return NULL;
}

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

	Link *wdj = new Link(WORD_DISJ, dlist);
	return wdj;
}

void Parser::initialize_state()
{
	const char * wall_word = "LEFT-WALL";

	Link *wall_disj = word_disjuncts(wall_word);

	OutList statev;
	statev.push_back(wall_disj);

	_state = new Link(STATE, statev);

	cout <<"duuuude state="<< _state <<endl;
}

void viterbi_parse(Dictionary dict, const char * sentence)
{
	Parser pars(dict);

	// Initial state
	pars.initialize_state();
cout <<"Hello world!"<<endl;

	pars.word_disjuncts("sing");
}

} // namespace viterbi
} // namespace link-grammar

void viterbi_parse(const char * sentence, Dictionary dict)
{
	link_grammar::viterbi::viterbi_parse(dict, sentence);
}

