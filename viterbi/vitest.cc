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
#define ALINK1(TYPE,A) (new Lynk(TYPE, A))
#define ALINK2(TYPE,A,B) (new Lynk(TYPE, A,B))
#define ALINK3(TYPE,A,B,C) (new Lynk(TYPE, A,B,C))

bool test_hello(const char *id, const char *dict_str)
{
	Dictionary dict = dictionary_create_from_utf8(dict_str);

	// print_dictionary_data(dict);

	Parser parser(dict);
	parser.streamin("Hello");

	Lynk* output = parser.get_output_set();

	Lynk* ans = 
	ALINK1(SET,
		ALINK3(LINK,
			ANODE(LINK_TYPE, "Wd"),
			ALINK2(WORD_DISJ,
   			ANODE(WORD, "LEFT-WALL"),
   			ANODE(CONNECTOR, "Wd+")
			),
			ALINK2(WORD_DISJ,
   			ANODE(WORD, "Hello"),
   			ANODE(CONNECTOR, "Wd-")
			)
		)
	);

	if (not (ans->operator==(output)))
	{
		cout << "Error: test failure on test " << id << endl;
		cout << "expecting " << ans << endl;
		cout << "got " << output << endl;
		return false;
	}
	cout<<"PASS: test_hello(" << id << ") " << endl;
	return true;
}

bool test_simplest()
{
	return test_hello ("test_simplest", 
		"LEFT-WALL: Wd+;"
		"Hello: Wd-;"
	);
}

bool test_simple_left_disj()
{
	return test_hello ("simple left disj",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"Hello: Wd-;"
	);
}

bool test_simple_right_disj()
{
	return test_hello ("simple right disj",
		"LEFT-WALL: Wd+;"
		"Hello: Wd- or Wi-;"
	);
}

int
main(int argc, char *argv[])
{
	size_t num_failures = 0;

	if (!test_simplest()) num_failures++;
	if (!test_simple_left_disj()) num_failures++;
	if (!test_simple_right_disj()) num_failures++;

	if (num_failures) exit(1);

	exit (0);
}

