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

#define Lynk link_grammar::viterbi::Link

#define ANODE(TYPE,NAME) (new Node(TYPE,NAME))
#define ALINK1(TYPE,A) ({OutList ol; \
                        ol.push_back(A); \
                        new Lynk(TYPE, ol); })
#define ALINK2(TYPE,A,B) ({OutList ol; \
                        ol.push_back(A); \
                        ol.push_back(B); \
                        new Lynk(TYPE, ol); })

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

	Lynk* output = parser.get_output_set();
	cout<<"duude final state="<<output<<endl;

	Lynk* ans = 
	ALINK2(WORD_DISJ,
   	ANODE(WORD, "Hello"),
   	ANODE(CONNECTOR, "Wd-")
	);

	cout <<"duude out="<<ans<<endl;

	exit (0);
}

