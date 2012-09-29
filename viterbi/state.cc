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
#include "viterbi.h"

using namespace std;

namespace link_grammar {
namespace viterbi {

/* Current parse state */

Link * initialize_state(Dictionary dict)
{
	Node *left_wall = new Node(META, "LEFT-WALL");
	vector<Atom*> statev;
	statev.push_back(left_wall);
	Link *state = new Link(STATE, statev);

	return state;
}

void viterbi_parse(const char * sentence, Dictionary dict)
{
	// Initial state
	Link * state = initialize_state(dict);
cout <<"Hello world!"<<endl;

	Dict_node* dn = dictionary_lookup_list(dict, "LEFT-WALL");
	Exp* exp = dn->exp;
	print_expression(exp);
}

} // namespace viterbi
} // namespace link-grammar

void viterbi_parse(const char * sentence, Dictionary dict)
{
	link_grammar::viterbi::viterbi_parse(sentence, dict);
}

