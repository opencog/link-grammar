/*************************************************************************/
/* Copyright (c) 2012, 2013 Linas Vepstas <linasvepstas@gmail.com>       */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the Viterbi parsing system is subject to the terms of the      */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

/// This file provides a unit test for the operation of the viterbi parser.

#include <stdlib.h>
#include <iostream>

#include "parser.h"

using namespace std;
using namespace link_grammar::viterbi;

#define Lynk link_grammar::viterbi::Link

#define ANODE(TYPE,NAME) (new Node(TYPE,NAME))
#define ALINK0(TYPE) (new Lynk(TYPE))
#define ALINK1(TYPE,A) (new Lynk(TYPE, A))
#define ALINK2(TYPE,A,B) (new Lynk(TYPE, A,B))
#define ALINK3(TYPE,A,B,C) (new Lynk(TYPE, A,B,C))
#define ALINK4(TYPE,A,B,C,D) (new Lynk(TYPE, A,B,C,D))
#define ALINK5(TYPE,A,B,C,D,E) (new Lynk(TYPE, A,B,C,D,E))

#define ANODEC(TYPE,NAME,COST) (new Node(TYPE,NAME,COST))
#define ALINK0C(TYPE,COST) (new Lynk(TYPE,COST))
#define ALINK1C(TYPE,A,COST) (new Lynk(TYPE, A,COST))
#define ALINK2C(TYPE,A,B,COST) (new Lynk(TYPE, A,B,COST))
#define ALINK3C(TYPE,A,B,C,COST) (new Lynk(TYPE, A,B,C,COST))
#define ALINK4C(TYPE,A,B,C,D,COST) (new Lynk(TYPE, A,B,C,D,COST))
#define ALINK5C(TYPE,A,B,C,D,E,COST) (new Lynk(TYPE, A,B,C,D,E,COST))


#define CHECK(NAME, EXPECTED, COMPUTED)                              \
	total_tests++;                                                    \
	if (not (EXPECTED->operator==(COMPUTED)))                         \
	{                                                                 \
		cout << "Error: test failure on " << NAME << endl;             \
		cout << "=== Expecting:\n" << EXPECTED << endl;                \
		cout << "=== Got:\n" << COMPUTED << endl;                      \
		return false;                                                  \
	}                                                                 \
	cout<<"PASS: " << NAME << endl;                                   \
	return true;


#define CHECK_NE(NAME, EXPECTED, COMPUTED)                           \
	total_tests++;                                                    \
	if (EXPECTED->operator==(COMPUTED))                               \
	{                                                                 \
		cout << "Error: test failure on " << NAME << endl;             \
		cout << "=== Expecting:\n" << EXPECTED << endl;                \
		cout << "=== To differ from:\n" << COMPUTED << endl;           \
		return false;                                                  \
	}                                                                 \
	cout<<"PASS: " << NAME << endl;                                   \
	return true;


int total_tests = 0;

// ==================================================================

void report(int num_failures, bool exit_on_fail)
{
	if (num_failures)
	{
		cout << "Test failures = " << num_failures
		     << " out of " << total_tests
		     << " total, so far." << endl;
		if (exit_on_fail)
			exit(1);
	}

	cout << "All " << total_tests
	     << " tests so far pass." << endl;
}

