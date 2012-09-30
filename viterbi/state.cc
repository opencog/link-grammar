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
	if (CONNECTOR_type == exp->type)
	{
		stringstream ss;
		if (exp->multi) ss << "@";
		ss << exp->u.string << exp->dir;
		Node *n = new Node(CONNECTOR, ss.str());
for(int i=0; i<cnt; i++) cout <<"  ";
cout<<"duude n="<<exp->u.string<<endl;
		return n;
	}

	E_list* el = exp->u.l;
	if (NULL == el)
	{
		stringstream ss;
		ss << "()";
		Node *n = new Node(CONNECTOR, ss.str());
for(int i=0; i<cnt; i++) cout <<"  ";
cout<<"duude n= null ()"<<endl;
		return n;
	}

for(int i=0; i<cnt; i++) cout <<"  ";
cout<<"exp=";
print_expression(exp);

	/* Handle optional expressions */
	// if (NULL == el->e->u.l)
	if ((OR_type == exp->type) and (NULL == el->e->u.l))
	{
for(int i=0; i<cnt; i++) cout <<"  ";
cout<<"duude n= opt "<<endl;
cnt++;
		el = el->next;
		Atom* a = lg_exp_to_atom(el->e);
cnt--;
		OutList oset;
		oset.push_back(a);
		Link* l = new Link(OPTIONAL, oset);
		return l;
	}

if (AND_type == exp->type)  {
for(int i=0; i<cnt; i++) cout <<"  ";
cout<<"duude AND "<<endl; }
else if (OR_type == exp->type){
for(int i=0; i<cnt; i++) cout <<"  ";
cout<<"duude OR "<<endl;  }
else cout<<"duude wtf not either!"<<endl;

	OutList alist;

assert(el != NULL, "unhandled null here");
	// operators are infixed, i.e. are always binary.
	// That is, for each type, theres an exper on the left, and on the right.
cnt++;
	Atom* a = lg_exp_to_atom(el->e);
cnt--;
	alist.push_back(a);
	el = el->next;

	while (exp->type == el->e->type)
	{
		el = el->e->u.l;
		Atom* a;
		if (el->e->u.l == NULL)
		{
for(int i=0; i<cnt; i++) cout <<"  ";
cout<<"duude loopy oooopt "<<endl;
			break;
		}
		else
		{
cnt++;
			a = lg_exp_to_atom(el->e);
cnt--;
		}
		alist.push_back(a);
		el = el->next;
	}
for(int i=0; i<cnt; i++) cout <<"  ";
cout<<"duude final "<<endl;
	if (el->e->u.l == NULL)
	{
for(int i=0; i<cnt; i++) cout <<"  ";
cout<<"duude final opt oooopt "<<endl;
cnt++;
		el = el->next;
		a = lg_exp_to_atom(el->e);
cnt--;
		OutList oset;
		oset.push_back(a);
		a = new Link(OPTIONAL, oset);
		alist.push_back(a);
	}
	else
	{
cnt++;
		a = lg_exp_to_atom(el->e);
		alist.push_back(a);
cnt--;
	}

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
	return wdj;
}

void Parser::initialize_state()
{
	const char * wall_word = "LEFT-WALL";

	Link *wall_disj = word_disjuncts(wall_word);

	OutList statev;
	statev.push_back(wall_disj);

	_state = new Link(STATE, statev);

	//cout <<"duuuude state="<< _state <<endl;
Atom *a=word_disjuncts("XXX");
cout<<a<<endl;
}

void viterbi_parse(Dictionary dict, const char * sentence)
{
	Parser pars(dict);

	// Initial state
	pars.initialize_state();
cout <<"Hello world!"<<endl;

//	pars.word_disjuncts("sing");
}

} // namespace viterbi
} // namespace link-grammar

void viterbi_parse(const char * sentence, Dictionary dict)
{
	link_grammar::viterbi::viterbi_parse(dict, sentence);
}

