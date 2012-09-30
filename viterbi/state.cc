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
int cnt = 0;

/* Current parse state */
Atom * Parser::lg_exp_to_atom(Exp* exp, bool more)
{
assert(exp, "wtf bad arg");
	if (CONNECTOR_type == exp->type)
	{
		stringstream ss;
		if (exp->multi) ss << "@";
		ss << exp->u.string << exp->dir;
for(int i=0; i<cnt; i++) cout <<"  ";
cout<<"duude n="<<ss<<endl;
		return new Node(CONNECTOR, ss.str());
	}

	// Whenever a null appears in an OR-list, it means the 
	// entire OR-list is optional.  A null can never appear
	// in an AND-list.
	E_list* el = exp->u.l;
	if (NULL == el)
	{
for(int i=0; i<cnt; i++) cout <<"  ";
cout<<"duude n= null ()"<<endl;
		return new Node(CONNECTOR, "0");
	}

for(int i=0; i<cnt; i++) cout <<"  ";
cout<<"exp=";
print_expression(exp);

if (AND_type == exp->type)  {
for(int i=0; i<cnt; i++) cout <<"  ";
cout<<"duude AND "<<endl; }
else if (OR_type == exp->type){
for(int i=0; i<cnt; i++) cout <<"  ";
cout<<"duude OR "<<endl;  }
else cout<<"duude wtf not either!"<<endl;


assert(el != NULL, "unhandled null here");
	// The data structure that link-grammar uses for connector
	// expressions is totally insane, as witnessed by the loop below.
	// Anyway: operators are infixed, i.e. are always binary,
	// with exp->u.l->e being the left hand side, and
	//      exp->u.l->next->e being the right hand side.
	// This means that exp->u.l->next->next is always null.
cnt++;
	OutList alist;
	alist.push_back(lg_exp_to_atom(el->e));
cnt--;
	el = el->next;

if (el) assert(el->e != NULL, "unhandled crazy null here");

	while (el && exp->type == el->e->type)
	{
		el = el->e->u.l;
assert(el != NULL, "unhandled walk null here");
assert(el->e != NULL, "unhandled walk more null here");
cnt++;
		alist.push_back(lg_exp_to_atom(el->e));
cnt--;
		el = el->next;
// assert(el != NULL, "unhandled walk right null here");
// assert(el->e != NULL, "unhandled walk more right null here");
	}
for(int i=0; i<cnt; i++) cout <<"  ";
cout<<"duude final "<<endl;

if(el) assert(el->e != NULL, "unhandled final  null here");
	if (el)
		alist.push_back(lg_exp_to_atom(el->e));

	if (AND_type == exp->type)
		return new Link(AND, alist);

	if (OR_type == exp->type)
		return new Link(OR, alist);

	assert(0, "Not reached");
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
cout <<"duuuude word disj=\n"<< wdj <<endl;
	return wdj;
}

void Parser::initialize_state()
{
	const char * wall_word = "LEFT-WALL";

	Link *wall_disj = word_disjuncts(wall_word);

	OutList statev;
	statev.push_back(wall_disj);

	_state = new Link(STATE, statev);

word_disjuncts("XXX");
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

