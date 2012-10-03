/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
/*                                                                       */
/*************************************************************************/

#include <stdlib.h>

#include <iostream>

#include <link-grammar/link-includes.h>
#include "read-dict.h"

#include "parser.h"

using namespace std;
using namespace link_grammar::viterbi;

// using link_grammar::viterbi;

main()
{
	Dictionary dict = dictionary_create_from_utf8(
		"LEFT-WALL: Wd+;"
		"Hello: Wd-;"
	);

	print_dictionary_data(dict);

	Parser parser(dict);
	parser.streamin("Hello");

	cout << "hello world"<<endl;

	link_grammar::viterbi::Link* state = parser.get_state_set();
	cout<<"duude final state="<<state<<endl;

	exit (0);
}

