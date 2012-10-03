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

using namespace std;

// using link_grammar::viterbi;

main()
{
	Dictionary dict = dictionary_create_from_utf8("LEFT-WALL: Wd+;");

	print_dictionary_data(dict);

	cout << "hellow world"<<endl;

	exit (0);
}

