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

#include <link-grammar/link-includes.h>
#include "read-dict.h"

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
// Test some basic atomic functions.

bool test_operator_equals()
{
	Node* na = new Node("abc");
	Node* nb = new Node("abc", 0.8f);
	CHECK_NE(__FUNCTION__, na, nb);
}

bool test_operator_equals2()
{
	Node* na = new Node("abcd", 0.8f*0.8f);
	Node* nb = new Node("abcd", 0.6400001f);
	CHECK(__FUNCTION__, na, nb);
}

int ntest_core()
{
	size_t num_failures = 0;
	if (!test_operator_equals()) num_failures++;
	if (!test_operator_equals2()) num_failures++;
	return num_failures;
}

// ==================================================================
// Test the flatten function

bool test_flatten()
{
	Or* or_right = new Or(
  		ANODE(WORD, "AA1"),
		ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"))
	);
	Or* computed = or_right->flatten();

	Lynk* expected =
	ALINK3(OR,
		ANODE(WORD, "AA1"),
		ANODE(WORD, "BB2"),
		ANODE(WORD, "CC3")
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_flatten_rec()
{
	Or* or_right = new Or(
		ALINK2(OR,
			ANODE(WORD, "AA1"),
  		 	ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")))
	);
	Or* computed = or_right->flatten();

	Lynk* expected =
	ALINK3(OR,
		ANODE(WORD, "AA1"),
		ANODE(WORD, "BB2"),
		ANODE(WORD, "CC3")
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_flatten_nest()
{
	And* and_right = new And(
		ALINK2(OR,
			ANODE(WORD, "AA1"),
  		 	ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")))
	);
	Atom* computed = and_right->super_flatten();

	Lynk* expected =
	ALINK3(OR,
		ANODE(WORD, "AA1"),
		ANODE(WORD, "BB2"),
		ANODE(WORD, "CC3")
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_flatten_nest_deep()
{
	And* and_right = new And(
		ALINK3(OR,
			ANODE(WORD, "AA1"),
  		 	ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")),
			ALINK3(AND,
				ANODE(WORD, "XAA1"),
  		 		ALINK2(AND, ANODE(WORD, "XBB2"), ANODE(WORD, "XCC3")),
  		 		ALINK2(AND, ANODE(WORD, "XDD4"), ANODE(WORD, "XEE5"))
			)
		)
	);
	Atom* computed = and_right->super_flatten();

	Lynk* expected =
	ALINK4(OR,
		ANODE(WORD, "AA1"),
		ANODE(WORD, "BB2"),
		ANODE(WORD, "CC3"),
		ALINK5(AND,
			ANODE(WORD, "XAA1"),
			ANODE(WORD, "XBB2"),
			ANODE(WORD, "XCC3"),
			ANODE(WORD, "XDD4"),
			ANODE(WORD, "XEE5")
		)
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_cost_flatten()
{
	Or* or_right = new Or(
  		ANODEC(WORD, "AA1", 0.01),
		ALINK2C(OR, ANODEC(WORD, "BB2", 0.02), ANODEC(WORD, "CC3", 0.03), 0.001),
	0.1f);
	Or* computed = or_right->flatten();

	Lynk* expected =
	ALINK3C(OR,
		ANODEC(WORD, "AA1", 0.01f),
		ANODEC(WORD, "BB2", 0.021f),
		ANODEC(WORD, "CC3", 0.031f),
	0.1f);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_cost_flatten_rec()
{
	Or* or_right = new Or(
		ALINK2C(OR,
			ANODEC(WORD, "AA1", 0.01f),
  		 	ALINK2C(OR,
				ANODEC(WORD, "BB2", 0.02f),
				ANODEC(WORD, "CC3",0.03f),
			0.003f),
		0.0004f),
	0.1f);
	Or* computed = or_right->flatten();

	Lynk* expected =
	ALINK3C(OR,
		ANODEC(WORD, "AA1", 0.0104f),
		ANODEC(WORD, "BB2", 0.0234f),
		ANODEC(WORD, "CC3", 0.0334f),
	0.1f);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_cost_flatten_nest()
{
	And* and_right = new And(
		ALINK2C(OR,
			ANODEC(WORD, "AA1", 0.01f),
  		 	ALINK2C(OR,
				ANODEC(WORD, "BB2", 0.02f),
				ANODEC(WORD, "CC3", 0.03f),
			0.003f),
		0.0004f),
	0.1f);
	Atom* computed = and_right->super_flatten();

	Lynk* expected =
	ALINK3C(OR,
		ANODEC(WORD, "AA1", 0.01f),
		ANODEC(WORD, "BB2", 0.023f),
		ANODEC(WORD, "CC3", 0.033f),
	0.1004f);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_cost_flatten_nest_deep()
{
	And* and_right = new And(
		ALINK3C(OR,
			ANODEC(WORD, "AA1", 0.01f),
  		 	ALINK2C(OR,
				ANODEC(WORD, "BB2", 0.02f),
				ANODEC(WORD, "CC3", 0.03f),
			0.003f),
			ALINK3C(AND,
				ANODEC(WORD, "XAA1", 0.00001f),
  		 		ALINK2C(AND,
					ANODEC(WORD, "XBB2", 0.00002f),
					ANODEC(WORD, "XCC3", 0.00003f),
				0.000003f),
  		 		ALINK2C(AND,
					ANODEC(WORD, "XDD4", 0.00004f),
					ANODEC(WORD, "XEE5", 0.00005f),
				0.000006f),
			0.5f),
		0.00007f),
	0.1f);
	Atom* computed = and_right->super_flatten();

	Lynk* expected =
	ALINK4C(OR,
		ANODEC(WORD, "AA1", 0.01f),
		ANODEC(WORD, "BB2", 0.023f),
		ANODEC(WORD, "CC3", 0.033f),
		ALINK5C(AND,
			ANODEC(WORD, "XAA1", 0.00001f),
			ANODEC(WORD, "XBB2", 0.000023f),
			ANODEC(WORD, "XCC3", 0.000033f),
			ANODEC(WORD, "XDD4", 0.000046f),
			ANODEC(WORD, "XEE5", 0.000056f),
		0.5f),
	0.10007f);

	CHECK(__FUNCTION__, expected, computed);
}

int ntest_flatten()
{
	size_t num_failures = 0;
	if (!test_flatten()) num_failures++;
	if (!test_flatten_rec()) num_failures++;
	if (!test_flatten_nest()) num_failures++;
	if (!test_flatten_nest_deep()) num_failures++;

	if (!test_cost_flatten()) num_failures++;
	if (!test_cost_flatten_rec()) num_failures++;
	if (!test_cost_flatten_nest()) num_failures++;
	if (!test_cost_flatten_nest_deep()) num_failures++;
	return num_failures;
}

// ==================================================================
// Make sure that the disjoined functions actually work.

bool test_and_dnf_single()
{
	And* and_singleton = new And(ANODE(WORD, "AA1"));
	Atom* computed = and_singleton->disjoin();

	Atom* expected = ANODE(WORD, "AA1");

	CHECK(__FUNCTION__, expected, computed);
}

bool test_and_dnf_double()
{
	And* and_two = new And(ANODE(WORD, "AA1"), ANODE(WORD, "BB2"));
	Atom* computed = and_two->disjoin();

	Lynk* expected =
	ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2"));

	CHECK(__FUNCTION__, expected, computed);
}

bool test_and_distrib_left()
{
	And* and_right = new And(
		ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")),
		ANODE(WORD, "RR1"));
	Atom* computed = and_right->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK2(AND, ANODE(WORD, "BB2"), ANODE(WORD, "RR1")),
		ALINK2(AND, ANODE(WORD, "CC3"), ANODE(WORD, "RR1"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_and_distrib_right()
{
	And* and_right = new And(ANODE(WORD, "AA1"),
		ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")));
	Atom* computed = and_right->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2")),
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_and_distrib_middle()
{
	And* and_mid = new And(ANODE(WORD, "AA1"),
		ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")),
		ANODE(WORD, "DD4"));
	Atom* computed = and_mid->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK3(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2"), ANODE(WORD, "DD4")),
		ALINK3(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3"), ANODE(WORD, "DD4"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_and_distrib_quad()
{
	And* and_mid = new And(
		ALINK2(OR, ANODE(WORD, "AA1"), ANODE(WORD, "BB2")),
		ALINK2(OR, ANODE(WORD, "CC3"), ANODE(WORD, "DD4")));
	Atom* computed = and_mid->disjoin();

	Lynk* expected =
	ALINK4(OR,
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3")),
		ALINK2(AND, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")),
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "DD4")),
		ALINK2(AND, ANODE(WORD, "BB2"), ANODE(WORD, "DD4"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_and_distrib_quad_right()
{
	And* and_mid = new And(
		ALINK2(OR, ANODE(WORD, "AA1"), ANODE(WORD, "BB2")),
		ALINK2(OR, ANODE(WORD, "CC3"), ANODE(WORD, "DD4")),
		ANODE(WORD, "EE5")
	);
	Atom* computed = and_mid->disjoin();

	Lynk* expected =
	ALINK4(OR,
		ALINK3(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3"), ANODE(WORD, "EE5")),
		ALINK3(AND, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"), ANODE(WORD, "EE5")),
		ALINK3(AND, ANODE(WORD, "AA1"), ANODE(WORD, "DD4"), ANODE(WORD, "EE5")),
		ALINK3(AND, ANODE(WORD, "BB2"), ANODE(WORD, "DD4"), ANODE(WORD, "EE5"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_and_distrib_quad_left()
{
	And* and_mid = new And(
		ANODE(WORD, "EE5"),
		ALINK2(OR, ANODE(WORD, "AA1"), ANODE(WORD, "BB2")),
		ALINK2(OR, ANODE(WORD, "CC3"), ANODE(WORD, "DD4")));
	Atom* computed = and_mid->disjoin();

	Lynk* expected =
	ALINK4(OR,
		ALINK3(AND, ANODE(WORD, "EE5"), ANODE(WORD, "AA1"), ANODE(WORD, "CC3")),
		ALINK3(AND, ANODE(WORD, "EE5"), ANODE(WORD, "BB2"), ANODE(WORD, "CC3")),
		ALINK3(AND, ANODE(WORD, "EE5"), ANODE(WORD, "AA1"), ANODE(WORD, "DD4")),
		ALINK3(AND, ANODE(WORD, "EE5"), ANODE(WORD, "BB2"), ANODE(WORD, "DD4"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_or_dnf_single()
{
	Or* or_singleton = new Or(ANODE(WORD, "AA1"));
	Atom* computed = or_singleton->disjoin();

	Atom* expected = ANODE(WORD, "AA1");

	CHECK(__FUNCTION__, expected, computed);
}

bool test_or_dnf_double()
{
	Or* or_two = new Or(ANODE(WORD, "AA1"), ANODE(WORD, "BB2"));
	Atom* computed = or_two->disjoin();

	Lynk* expected =
	ALINK2(OR, ANODE(WORD, "AA1"), ANODE(WORD, "BB2"));

	CHECK(__FUNCTION__, expected, computed);
}

bool test_or_distrib_left()
{
	Or* or_right = new Or(
		ALINK2(AND,
			ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")),
			ANODE(WORD, "RR1"))
	);
	Atom* computed = or_right->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK2(AND, ANODE(WORD, "BB2"), ANODE(WORD, "RR1")),
		ALINK2(AND, ANODE(WORD, "CC3"), ANODE(WORD, "RR1"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_or_distrib_right()
{
	Or* or_right = new Or(
		ALINK2(AND,
			ANODE(WORD, "AA1"),
			ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")))
	);
	Atom* computed = or_right->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2")),
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_or_distrib_nest()
{
	Or* or_right = new Or(
		ALINK1(OR,
			ALINK2(AND,
				ANODE(WORD, "AA1"),
	  		 	ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"))))
	);
	Atom* computed = or_right->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2")),
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_or_distrib_nest2()
{
	Or* or_right = new Or(
		ALINK3(OR,
			ANODE(WORD, "DD4"),
			ALINK2(AND,
				ANODE(WORD, "AA1"),
	  		 	ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"))),
			ANODE(WORD, "EE5"))
	);
	Atom* computed = or_right->disjoin();

	Lynk* expected =
	ALINK4(OR,
		ANODE(WORD, "DD4"),
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2")),
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3")),
		ANODE(WORD, "EE5")
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_or_distrib_nest3()
{
	Or* or_right = new Or(
		ANODE(WORD, "AA1"),
  	 	ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")));
	Atom* computed = or_right->disjoin();

	Lynk* expected =
	ALINK3(OR,
		ANODE(WORD, "AA1"),
		ANODE(WORD, "BB2"),
		ANODE(WORD, "CC3")
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_or_distrib_nest4()
{
	Or* or_right = new Or(
		ALINK2(OR,
			ANODE(WORD, "AA1"),
  		 	ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")))
	);
	Atom* computed = or_right->disjoin();

	Lynk* expected =
	ALINK3(OR,
		ANODE(WORD, "AA1"),
		ANODE(WORD, "BB2"),
		ANODE(WORD, "CC3")
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_or_distrib_nest5()
{
	And* and_right = new And(
		ALINK2(OR,
			ANODE(WORD, "AA1"),
  		 	ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")))
	);
	Atom* computed = and_right->disjoin();

	Lynk* expected =
	ALINK3(OR,
		ANODE(WORD, "AA1"),
		ANODE(WORD, "BB2"),
		ANODE(WORD, "CC3")
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_or_distrib_nest6()
{
	Or* or_right = new Or(
		ALINK1(AND,
			ALINK2(OR,
				ANODE(WORD, "AA1"),
	  		 	ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")))
			)
	);
	Atom* computed = or_right->disjoin();

	Lynk* expected =
	ALINK3(OR,
		ANODE(WORD, "AA1"),
		ANODE(WORD, "BB2"),
		ANODE(WORD, "CC3")
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_or_distrib_nest7()
{
	Or* or_right = new Or(
		ALINK3(AND,
			ANODE(WORD, "DD4"),
			ALINK2(OR,
				ANODE(WORD, "AA1"),
	  		 	ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"))),
			ANODE(WORD, "EE5"))
	);
	Atom* computed = or_right->disjoin();

	Lynk* expected =
	ALINK3(OR,
		ALINK3(AND, ANODE(WORD, "DD4"), ANODE(WORD, "AA1"), ANODE(WORD, "EE5")),
		ALINK3(AND, ANODE(WORD, "DD4"), ANODE(WORD, "BB2"), ANODE(WORD, "EE5")),
		ALINK3(AND, ANODE(WORD, "DD4"), ANODE(WORD, "CC3"), ANODE(WORD, "EE5"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

int ntest_disjoin()
{
	size_t num_failures = 0;
	if (!test_and_dnf_single()) num_failures++;
	if (!test_and_dnf_double()) num_failures++;
	if (!test_and_distrib_left()) num_failures++;
	if (!test_and_distrib_right()) num_failures++;
	if (!test_and_distrib_middle()) num_failures++;
	if (!test_and_distrib_quad()) num_failures++;
	if (!test_and_distrib_quad_right()) num_failures++;
	if (!test_and_distrib_quad_left()) num_failures++;

	if (!test_or_dnf_single()) num_failures++;
	if (!test_or_dnf_double()) num_failures++;
	if (!test_or_distrib_left()) num_failures++;
	if (!test_or_distrib_right()) num_failures++;

	if (!test_or_distrib_nest()) num_failures++;
	if (!test_or_distrib_nest2()) num_failures++;
	if (!test_or_distrib_nest3()) num_failures++;
	if (!test_or_distrib_nest4()) num_failures++;
	if (!test_or_distrib_nest5()) num_failures++;
	if (!test_or_distrib_nest6()) num_failures++;
	if (!test_or_distrib_nest7()) num_failures++;
	return num_failures;
}


// ==================================================================
// Make sure that the disjoined functions actually work.
// Identical to the above, except this time, there are costs involved.

bool test_costly_and_dnf_single()
{
	And* and_singleton = new And(ANODEC(WORD, "AA1", 1.5f));
	Atom* computed = and_singleton->disjoin();

	Atom* expected = ANODEC(WORD, "AA1", 1.5f);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_costly_and_dnf_single_ne()
{
	And* and_singleton = new And(ANODEC(WORD, "AA1", 1.5f));
	Atom* computed = and_singleton->disjoin();

	Atom* expected = ANODEC(WORD, "AA1", 31.6f);

	CHECK_NE(__FUNCTION__, expected, computed);
}

bool test_costly_and_dnf_single_sum()
{
	And* and_singleton = new And(ANODEC(WORD, "AA1", 1.5f), 0.3f);
	Atom* computed = and_singleton->disjoin();

	Atom* expected = ANODEC(WORD, "AA1", 1.8f);

	CHECK(__FUNCTION__, expected, computed);
}

// -----------------------------------------------
bool test_costly_and_dnf_double()
{
	And* and_two = new And(ANODE(WORD, "AA1"), ANODE(WORD, "BB2"), 1.6f);
	Atom* computed = and_two->disjoin();

	Lynk* expected =
	ALINK2C(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2"), 1.6f);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_costly_and_dnf_double_w()
{
	And* and_two = new And(ANODE(WORD, "AA1"), ANODEC(WORD, "BB2", 2.8f));
	Atom* computed = and_two->disjoin();

	Lynk* expected =
	ALINK2(AND, ANODE(WORD, "AA1"), ANODEC(WORD, "BB2", 2.8f));

	CHECK(__FUNCTION__, expected, computed);
}

// -----------------------------------------------
bool test_costly_and_distrib_left()
{
	And* and_right = new And(
		ALINK2C(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"), 1.1f),
		ANODE(WORD, "RR1"));
	Atom* computed = and_right->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK2C(AND, ANODE(WORD, "BB2"), ANODE(WORD, "RR1"), 1.1f),
		ALINK2C(AND, ANODE(WORD, "CC3"), ANODE(WORD, "RR1"), 1.1f)
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_costly_and_distrib_left_sum()
{
	And* and_right = new And(
		ALINK2C(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"), 1.1f),
		ANODE(WORD, "RR1"), 0.8f);
	Atom* computed = and_right->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK2C(AND, ANODE(WORD, "BB2"), ANODE(WORD, "RR1"), 1.9f),
		ALINK2C(AND, ANODE(WORD, "CC3"), ANODE(WORD, "RR1"), 1.9f)
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_costly_and_distrib_left_w()
{
	And* and_right = new And(
		ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")),
		ANODEC(WORD, "RR1", 3.14f));
	Atom* computed = and_right->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK2(AND, ANODE(WORD, "BB2"), ANODEC(WORD, "RR1", 3.14f)),
		ALINK2(AND, ANODE(WORD, "CC3"), ANODEC(WORD, "RR1", 3.14f))
	);

	CHECK(__FUNCTION__, expected, computed);
}

// -----------------------------------------------
bool test_costly_and_distrib_right()
{
	And* and_right = new And(ANODE(WORD, "AA1"),
		ALINK2C(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"), 0.35));
	Atom* computed = and_right->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK2C(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2"), 0.35),
		ALINK2C(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3"), 0.35)
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_costly_and_distrib_right_sum()
{
	And* and_right = new And(ANODE(WORD, "AA1"),
		ALINK2C(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"), 0.35f), 0.5f);
	Atom* computed = and_right->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK2C(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2"), 0.85f),
		ALINK2C(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3"), 0.85f)
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_costly_and_distrib_middle()
{
	And* and_mid = new And(ANODE(WORD, "AA1"),
		ALINK2C(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"), 2.1f),
		ANODE(WORD, "DD4"), 0.6f);
	Atom* computed = and_mid->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK3C(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2"), ANODE(WORD, "DD4"), 2.7f),
		ALINK3C(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3"), ANODE(WORD, "DD4"), 2.7f)
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_costly_and_distrib_quad()
{
	And* and_mid = new And(
		ALINK2C(OR, ANODE(WORD, "AA1"), ANODE(WORD, "BB2"), 1.1f),
		ALINK2C(OR, ANODE(WORD, "CC3"), ANODE(WORD, "DD4"), 2.2f), 0.4f);
	Atom* computed = and_mid->disjoin();

	Lynk* expected =
	ALINK4(OR,
		ALINK2C(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3"), 3.7f),
		ALINK2C(AND, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"), 3.7f),
		ALINK2C(AND, ANODE(WORD, "AA1"), ANODE(WORD, "DD4"), 3.7f),
		ALINK2C(AND, ANODE(WORD, "BB2"), ANODE(WORD, "DD4"), 3.7f)
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_costly_and_distrib_quad_right()
{
	And* and_mid = new And(
		ALINK2C(OR, ANODE(WORD, "AA1"), ANODE(WORD, "BB2"), 0.25f),
		ALINK2C(OR, ANODE(WORD, "CC3"), ANODE(WORD, "DD4"), 0.35f),
		ANODE(WORD, "EE5"), 0.5f
	);
	Atom* computed = and_mid->disjoin();

	Lynk* expected =
	ALINK4(OR,
		ALINK3C(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3"), ANODE(WORD, "EE5"), 1.1f),
		ALINK3C(AND, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"), ANODE(WORD, "EE5"), 1.1f),
		ALINK3C(AND, ANODE(WORD, "AA1"), ANODE(WORD, "DD4"), ANODE(WORD, "EE5"), 1.1f),
		ALINK3C(AND, ANODE(WORD, "BB2"), ANODE(WORD, "DD4"), ANODE(WORD, "EE5"), 1.1f)
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_costly_and_distrib_quad_left()
{
	And* and_mid = new And(
		ANODE(WORD, "EE5"),
		ALINK2C(OR, ANODE(WORD, "AA1"), ANODE(WORD, "BB2"), 0.11f),
		ALINK2C(OR, ANODE(WORD, "CC3"), ANODE(WORD, "DD4"), 0.22f), 0.1f);
	Atom* computed = and_mid->disjoin();

	Lynk* expected =
	ALINK4(OR,
		ALINK3C(AND, ANODE(WORD, "EE5"), ANODE(WORD, "AA1"), ANODE(WORD, "CC3"), 0.43f),
		ALINK3C(AND, ANODE(WORD, "EE5"), ANODE(WORD, "BB2"), ANODE(WORD, "CC3"), 0.43f),
		ALINK3C(AND, ANODE(WORD, "EE5"), ANODE(WORD, "AA1"), ANODE(WORD, "DD4"), 0.43f),
		ALINK3C(AND, ANODE(WORD, "EE5"), ANODE(WORD, "BB2"), ANODE(WORD, "DD4"), 0.43f)
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_costly_or_dnf_single()
{
	Or* or_singleton = new Or(ANODE(WORD, "AA1"), 0.75f);
	Atom* computed = or_singleton->disjoin();

	Atom* expected = ANODEC(WORD, "AA1", 0.75f);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_costly_or_dnf_double()
{
	Or* or_two = new Or(ANODE(WORD, "AA1"), ANODE(WORD, "BB2"), 0.65f);
	Atom* computed = or_two->disjoin();

	Lynk* expected =
	ALINK2C(OR, ANODE(WORD, "AA1"), ANODE(WORD, "BB2"), 0.65f);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_costly_or_distrib_left()
{
	Or* or_right = new Or(
		ALINK2C(AND,
			ALINK2C(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"), 0.1f),
			ANODE(WORD, "RR1"), 0.22f),
	0.333f);
	Atom* computed = or_right->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK2C(AND, ANODE(WORD, "BB2"), ANODE(WORD, "RR1"), 0.653f),
		ALINK2C(AND, ANODE(WORD, "CC3"), ANODE(WORD, "RR1"), 0.653f)
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_costly_or_distrib_right()
{
	Or* or_right = new Or(
		ALINK2C(AND,
			ANODE(WORD, "AA1"),
			ALINK2C(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"), 0.111f), 0.222f),
	0.5f);
	Atom* computed = or_right->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK2C(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2"), 0.833f),
		ALINK2C(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3"), 0.833f)
	);

	CHECK(__FUNCTION__, expected, computed);
}

// -----------------------------------------------
bool test_costly_or_distrib_nest()
{
	Or* or_right = new Or(
		ALINK1C(OR,
			ALINK2C(AND,
				ANODE(WORD, "AA1"),
	  		 	ALINK2C(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"), 0.1f),
			0.02f),
		0.003f),
	0.0004f);
	Atom* computed = or_right->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK2C(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2"), 0.1234f),
		ALINK2C(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3"), 0.1234f)
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_costly_or_distrib_nest2()
{
	Or* or_right = new Or(
		ALINK1C(OR,
			ALINK2C(AND,
				ANODE(WORD, "AA1"),
	  		 	ANODEC(WORD, "BB2", 0.1f),
			0.02f),
		0.003f),
	0.0004f);
	Atom* computed = or_right->disjoin();

	Lynk* expected =
	ALINK2C(AND, ANODE(WORD, "AA1"), ANODEC(WORD, "BB2", 0.1f), 0.0234f);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_costly_or_distrib_nest3()
{
	Or* or_right = new Or(
		ALINK1C(OR,
	  	 	ANODEC(WORD, "BB2", 0.1f),
		0.003f),
	0.0004f);
	Atom* computed = or_right->disjoin();

	Atom* expected = ANODEC(WORD, "BB2", 0.1034f);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_costly_or_distrib_nest4()
{
	Or* or_right = new Or(
		ALINK3C(OR,
			ANODE(WORD, "DD4"),
			ALINK2C(AND,
				ANODE(WORD, "AA1"),
	  		 	ALINK2C(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"), 0.1f), 0.02f),
			ANODE(WORD, "EE5"), 0.003f),
	0.0004f);
	Atom* computed = or_right->disjoin();

	Lynk* expected =
	ALINK4C(OR,
		ANODEC(WORD, "DD4", 0.0f),
		ALINK2C(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2"), 0.12f),
		ALINK2C(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3"), 0.12f),
		ANODEC(WORD, "EE5", 0.0f),
	0.0034f);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_costly_or_distrib_nest5()
{
	Or* or_right = new Or(
		ALINK3C(AND,
			ANODE(WORD, "DD4"),
			ALINK2C(OR,
				ANODE(WORD, "AA1"),
	  		 	ALINK2C(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"), 0.1f), 0.02f),
			ANODE(WORD, "EE5"), 0.003f),
	0.0004f);
	Atom* computed = or_right->disjoin();

	Lynk* expected =
	ALINK3C(OR,
		ALINK3C(AND, ANODE(WORD, "DD4"), ANODE(WORD, "AA1"), ANODE(WORD, "EE5"), 0.0234f),
		ALINK3C(AND, ANODE(WORD, "DD4"), ANODEC(WORD, "BB2", 0.1f), ANODE(WORD, "EE5"), 0.0234f),
		ALINK3C(AND, ANODE(WORD, "DD4"), ANODEC(WORD, "CC3", 0.1f), ANODE(WORD, "EE5"), 0.0234f),
	0.0f);

	CHECK(__FUNCTION__, expected, computed);
}

int ntest_costly_disjoin()
{
	size_t num_failures = 0;
	if (!test_costly_and_dnf_single()) num_failures++;
	if (!test_costly_and_dnf_single_ne()) num_failures++;
	if (!test_costly_and_dnf_single_sum()) num_failures++;

	if (!test_costly_and_dnf_double()) num_failures++;
	if (!test_costly_and_dnf_double_w()) num_failures++;

	if (!test_costly_and_distrib_left()) num_failures++;
	if (!test_costly_and_distrib_left_sum()) num_failures++;
	if (!test_costly_and_distrib_left_w()) num_failures++;

	if (!test_costly_and_distrib_right()) num_failures++;
	if (!test_costly_and_distrib_right_sum()) num_failures++;

	if (!test_costly_and_distrib_middle()) num_failures++;
	if (!test_costly_and_distrib_quad()) num_failures++;
	if (!test_costly_and_distrib_quad_right()) num_failures++;
	if (!test_costly_and_distrib_quad_left()) num_failures++;

	if (!test_costly_or_dnf_single()) num_failures++;
	if (!test_costly_or_dnf_double()) num_failures++;
	if (!test_costly_or_distrib_left()) num_failures++;
	if (!test_costly_or_distrib_right()) num_failures++;

	if (!test_costly_or_distrib_nest()) num_failures++;
	if (!test_costly_or_distrib_nest2()) num_failures++;
	if (!test_costly_or_distrib_nest3()) num_failures++;
	if (!test_costly_or_distrib_nest4()) num_failures++;
	if (!test_costly_or_distrib_nest5()) num_failures++;
	return num_failures;
}


// ==================================================================
// A simple hello test; several different dictionaries
// should give exactly the same output.  The input sentence
// is just one word, it should connect to the left-wall in
// just one way. The result should be just one alternative:
// that alternatives has an empty state, and output with
// just one link.
bool test_hello(const char *id, const char *dict_str,
                bool empty_state, bool must_connect)
{
	total_tests++;

	Dictionary dict = dictionary_create_from_utf8(dict_str);

	// print_dictionary_data(dict);

	Parser parser(dict);
	parser.streamin("Hello");

	// This is the expected output, no matter what the
	// dictionary may be.  Its just one word, connected to the wall.
	Lynk* one_word =
	ALINK3(LING,
		ANODE(LING_TYPE, "Wd"),
		ALINK2(WORD_DISJ,
			ANODE(WORD, "LEFT-WALL"),
			ANODE(CONNECTOR, "Wd+")
		),
		ALINK2(WORD_DISJ,
			ANODE(WORD, "Hello"),
			ANODE(CONNECTOR, "Wd-")
		)
	);

	if (empty_state)
	{
		// This is the expected set of alternatives: just one alternative,
		// a single, empty state, and the output, above.
		Lynk* ans =
		ALINK1(SET,
			ALINK2(STATE_PAIR,
				ALINK0(SEQ),
				ALINK1(SEQ, one_word)
			)
		);

		Lynk* alts = parser.get_alternatives();
		if (not (ans->operator==(alts)))
		{
			cout << "Error: test failure on test \"" << id << "\"" << endl;
			cout << "=== Expecting:\n" << ans << endl;
			cout << "=== Got:\n" << alts << endl;
			return false;
		}
	}
	else
	{
		// This test will have lots of alternatives. One should have
		// an empty state.
		Lynk* ans =
			ALINK2(STATE_PAIR,
				ALINK0(SEQ),
				ALINK1(SEQ, one_word)
			);

		Lynk* out = new Seq(one_word);

		bool pass_test = false;
		Lynk* alts = parser.get_alternatives();
		for (size_t i=0; i<alts->get_arity(); i++)
		{
			Atom* alt = alts->get_outgoing_atom(i);
			StatePair* sp = dynamic_cast<StatePair*>(alt);

			// At least one alternative should have an empty state.
			if (ans->operator==(alt))
				pass_test = true;

			// In all cases, the output should be just the one word,
			// no matter what the state.
			if (must_connect and not sp->get_output()->operator==(out))
				pass_test = false;
		}
		if (pass_test)
		{
			cout<<"PASS: test_hello(" << id << ") " << endl;
			return true;
		}
		cout << "Error: test failure on test \"" << id << "\"" << endl;
		cout << "=== Expecting:\n" << ans << endl;
		cout << "=== Got:\n" << alts << endl;
		return false;
	}

	cout<<"PASS: test_hello(" << id << ") " << endl;
	return true;
}

bool test_simplest()
{
	return test_hello ("test_simplest",
		"LEFT-WALL: Wd+;"
		"Hello: Wd-;",
		true, true
	);
}

bool test_simple_left_disj()
{
	return test_hello ("simple left disj",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"Hello: Wd-;",
		true, true
	);
}

bool test_simple_optional_left_cset()
{
	return test_hello ("optional left cset",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {CP+} & {Xx+} & {RW+ or Xp+};"
		"Hello: Wd-;",
		false, true
	);
}

bool test_simple_right_disj()
{
	return test_hello ("simple right disj",
		"LEFT-WALL: Wd+;"
		"Hello: Wd- or Wi-;",
		true, true
	);
}

bool test_simple_right_required_cset()
{
	return test_hello ("required right cset",
		"LEFT-WALL: Wd+;"
		"Hello: Wd- or Xi- or (Xj- & (A+ or B+));",
		true, true
	);
}

bool test_simple_optional()
{
	return test_hello ("optionals in both csets",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {CP+} & {Xx+} & {RW+ or Xp+};"
		"Hello: Wd- or Xi- or (Xj- & {A+ or B+});",
		false, true
	);
}

bool test_simple_onereq()
{
	return test_hello ("one required link and opt righties (simple)",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"Hello: Wd- & {A+} & {B+} & {C+};",
		false, true
	);
}

bool test_simple_zeroreq()
{
	return test_hello ("zero required links and opt righties (simple)",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"Hello: {Wd-} & {A+} & {B+} & {C+};",
		false, false
	);
}

bool test_simple_onereq_and_left()
{
	return test_hello ("one required link and opt lefties (simple)",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"Hello: Wd- & {A-} & {B-} & {C+};",
		false, true
	);
}

int ntest_simple()
{
	size_t num_failures = 0;

	if (!test_simplest()) num_failures++;
	if (!test_simple_left_disj()) num_failures++;
	if (!test_simple_optional_left_cset()) num_failures++;
	if (!test_simple_right_disj()) num_failures++;
	if (!test_simple_right_required_cset()) num_failures++;
	if (!test_simple_optional()) num_failures++;
	if (!test_simple_onereq()) num_failures++;
	if (!test_simple_zeroreq()) num_failures++;
	if (!test_simple_onereq_and_left()) num_failures++;
	return num_failures;
}

// ==================================================================
// A test of two alternative parses of a sentence with single word in it.
// Expect to get back a set with two alternative parses, each parse is
// assigned a probability of 1/2.

bool test_alternative(const char *id, const char *dict_str, bool empty_state)
{
	total_tests++;

	Dictionary dict = dictionary_create_from_utf8(dict_str);

	// print_dictionary_data(dict);

	Parser parser(dict);
	parser.streamin("Hello");

	Lynk* alt_out_one =
	ALINK3(LING,
		ANODE(LING_TYPE, "Wd"),
		ALINK2(WORD_DISJ,
			ANODE(WORD, "LEFT-WALL"),
			ANODE(CONNECTOR, "Wd+")
		),
		ALINK2(WORD_DISJ,
			ANODE(WORD, "Hello"),
			ANODE(CONNECTOR, "Wd-")
		)
	);

	Lynk* alt_out_two =
	ALINK3(LING,
		ANODE(LING_TYPE, "Wi"),
		ALINK2(WORD_DISJ,
			ANODE(WORD, "LEFT-WALL"),
			ANODE(CONNECTOR, "Wi+")
		),
		ALINK2(WORD_DISJ,
			ANODE(WORD, "Hello"),
			ANODE(CONNECTOR, "Wi-")
		)
	);

	Lynk* alt_pair_one =
	ALINK2(STATE_PAIR,
		ALINK0(SEQ),
		ALINK1(SEQ, alt_out_one)
	);

	Lynk* alt_pair_two =
	ALINK2(STATE_PAIR,
		ALINK0(SEQ),
		ALINK1(SEQ, alt_out_two)
	);

	if (empty_state)
	{
		// This is the expected set of alternatives: two alternatives,
		// each with an empty state, and one of the two outputs, above.
		Lynk* ans = ALINK2(SET, alt_pair_one, alt_pair_two);

		Lynk* output = parser.get_alternatives();
		if (not (ans->operator==(output)))
		{
			cout << "Error: test failure on test \"" << id <<"\"" << endl;
			cout << "=== Expecting:\n" << ans << endl;
			cout << "=== Got:\n" << output << endl;
			return false;
		}
	}
	else
	{
		// The final state here might not be empty.  However, both
		// of the alternatives should show up somwhere in the output.

		bool found_one = false;
		bool found_two = false;
		Lynk* alts = parser.get_alternatives();
		for (size_t i=0; i<alts->get_arity(); i++)
		{
			Atom* alt = alts->get_outgoing_atom(i);
			StatePair* sp = dynamic_cast<StatePair*>(alt);

			// At least one alternative should have an empty state.
			if (alt_pair_one->operator==(alt))
				found_one = true;

			if (alt_pair_two->operator==(alt))
				found_two = true;
		}

		// Both should have been found, somewhere.
		if (not alt_pair_one or not alt_pair_two)
		{
			cout << "Error: test failure on test \"" << id << "\"" << endl;
			cout << "=== Expecting this alt:\n" << alt_pair_one << endl;
			cout << "=== Expecting this alt:\n" << alt_pair_two << endl;
			cout << "=== Got:\n" << alts << endl;
			return false;
		}
	}

	cout<<"PASS: test_alternative(" << id << ") " << endl;
	return true;
}

bool test_two_alts()
{
	return test_alternative("two alternatives",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"Hello: Wd- or Wi-;",
		true
	);
}

bool test_two_opts()
{
	return test_alternative("two alts plus opts",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {A+};"
		"Hello: Wd- or Wi- or (Xj- & {A+ or B+});",
		false
	);
}

bool test_two_one_opts()
{
	return test_alternative("two alt, or one opt",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {A+};"
		"Hello: Wd- or {Wi-} or (Xj- & {A+ or B+});",
		false
	);
}

bool test_two_all_opts()
{
	return test_alternative("two alts, or all opt",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {A+};"
		"Hello: {Wd-} or {Wi-} or (Xj- & {A+ or B+});",
		false
	);
}

bool test_two_and_opts()
{
	return test_alternative("two alts, and an opt",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {A+};"
		"Hello: Wd- or (Wi- & {Xj- & {A+ or B+}} & {C+});",
		false
	);
}

bool test_two_and_no_opts()
{
	return test_alternative("two alt, and all opt",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {A+};"
		"Hello: Wd- or ({Wi-} & {Xj- & {A+ or B+}} & {C+});",
		false
	);
}

bool test_two_and_excess()
{
	return test_alternative("two alt, and excess reqs",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {A+};"
		"Hello: Wd- or (Wi- & Xj- & {A+ or B+} & {C+}) or Wi-;",
		false
	);
}

int ntest_two()
{
	size_t num_failures = 0;

	if (!test_two_alts()) num_failures++;
	if (!test_two_opts()) num_failures++;
	if (!test_two_one_opts()) num_failures++;
	if (!test_two_all_opts()) num_failures++;
	if (!test_two_and_opts()) num_failures++;
	if (!test_two_and_no_opts()) num_failures++;
	if (!test_two_and_excess()) num_failures++;

	return num_failures;
}

// ==================================================================

bool test_simple_state(const char *id, const char *dict_str)
{
	total_tests++;

	Dictionary dict = dictionary_create_from_utf8(dict_str);

	// print_dictionary_data(dict);

	Parser parser(dict);
	// Expecting more words to follow, so a non-trivial state.
	parser.streamin("this");

	Lynk* ans_state =
	ALINK2(SEQ,
		ALINK2(WORD_CSET,
			ANODE(WORD, "this"),
			ANODE(CONNECTOR, "Ss*b+")
		),
		ALINK2(WORD_CSET,
			ANODE(WORD, "LEFT-WALL"),
			ALINK3(OR,
				ANODE(CONNECTOR, "Wd+"),
				ANODE(CONNECTOR, "Wi+"),
				ANODE(CONNECTOR, "Wq+")
			)
		)
	);

	Lynk* ans =
	ALINK1(SET,
		ALINK2(STATE_PAIR,
			ans_state,
			ALINK0(SEQ)
		)
	);

	Lynk* state = parser.get_alternatives();
	if (not (ans->operator==(state)))
	{
		cout << "Error: test failure on test " << id << endl;
		cout << "=== Expecting state:\n" << ans << endl;
		cout << "=== Got state:\n" << state << endl;
		return false;
	}

	cout<<"PASS: test_simple_state(" << id << ") " << endl;
	return true;
}

bool test_first_state()
{
	return test_simple_state("first state",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+;"
	);
}

bool test_first_opt_lefty()
{
	return test_simple_state("first state, left-going optional",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+ and {Xi-};"
	);
}

bool test_first_or_lefty()
{
	return test_simple_state("first state, OR left-going",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+ or Xi-;"
	);
}

bool test_first_or_multi_lefty()
{
	return test_simple_state("first state, multi-OR left-going",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+ or Xi- or Y- or Z-;"
	);
}

bool test_first_opt_cpx()
{
	return test_simple_state("first state, complex left-going optional",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+ and {Xi- or P- or {Q- & Z+}};"
	);
}

bool test_first_infer_opt()
{
	return test_simple_state("first state, complex infer optional",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+ and (Xi- or P- or {Q- & Z+});"
	);
}

int ntest_first()
{
	size_t num_failures = 0;

	if (!test_first_state()) num_failures++;
	if (!test_first_opt_lefty()) num_failures++;
	if (!test_first_or_lefty()) num_failures++;
	if (!test_first_or_multi_lefty()) num_failures++;
	if (!test_first_opt_cpx()) num_failures++;
	if (!test_first_infer_opt()) num_failures++;

	return num_failures;
}

// ==================================================================

bool test_short_sent(const char *id, const char *dict_str, bool empty_state)
{
	total_tests++;

	Dictionary dict = dictionary_create_from_utf8(dict_str);
	// print_dictionary_data(dict);

	Parser parser(dict);

	// Expecting more words to follow, so a non-trivial state.
	// In particular, the dictionary will link the left-wall to
	// "is", so "this" has to be pushed on stack until the "is"
	// shows up.  The test_seq_sent() below will link the other
	// way around.
	parser.streamin("this is");

	Lynk* alts = parser.get_alternatives();

	// At least one result should be this state pair.
	Lynk* sp =
		ALINK2(STATE_PAIR,
			ALINK0(SEQ),  // empty state
			ALINK2(SEQ,
				ALINK3(LING,
					ANODE(LING_TYPE, "Ss*b"),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "this"),
						ANODE(CONNECTOR, "Ss*b+")),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "is.v"),
						ANODE(CONNECTOR, "Ss-")))
				,
				ALINK3(LING,
					ANODE(LING_TYPE, "Wi"),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "LEFT-WALL"),
						ANODE(CONNECTOR, "Wi+")),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "is.v"),
						ANODE(CONNECTOR, "Wi-")))
			));

	if (empty_state)
	{
		Lynk* ans = ALINK1(SET, sp);
		if (not (ans->operator==(alts)))
		{
			cout << "Error: test failure on test \"" << id <<"\"" << endl;
			cout << "=== Expecting:\n" << ans << endl;
			cout << "=== Got:\n" << alts << endl;
			return false;
		}
	}
	else
	{
		// At least one alternative should be the desired state pair.
		bool found = false;
		size_t sz = alts->get_arity();
		for (size_t i=0; i<sz; i++)
		{
			Atom* a = alts->get_outgoing_atom(i);
			if (sp->operator==(a))
				found = true;
		}
		if (not found)
		{
			cout << "Error: test failure on test \"" << id <<"\"" << endl;
			cout << "=== Expecting one of them to be:\n" << sp << endl;
			cout << "=== Got:\n" << alts << endl;
			return false;
		}
	}

	cout<<"PASS: test_short_sent(" << id << ") " << endl;
	return true;
}

bool test_short_this()
{
	return test_short_sent("short sent",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+;"
		"is.v: Ss- and Wi-;",
		true
	);
}

bool test_short_this_opt()
{
	return test_short_sent("short sent opt",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+;"
		"is.v: Ss- and Wi- and {O+};",
		false
	);
}

bool test_short_this_obj_opt()
{
	return test_short_sent("short sent with obj",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+ or Os-;"
		"is.v: Ss- and Wi- and {O+};",
		false
	);
}

bool test_short_this_costly()
{
	return test_short_sent("short sent with costly null",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+ or [[[()]]];"
		"is.v: Ss- and Wi-;",
		true
	);
}

bool test_short_this_complex()
{
	return test_short_sent("short sent complex",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		""
		"<CLAUSE>: {({@COd-} & (C-)) or ({@CO-} & (Wd- & {CC+})) or [Rn-]};"
		"<noun-main-h>:"
		"  (Jd- & Dmu- & Os-)"
		"  or (Jd- & Dmu- & {Wd-} & Ss*b+)"
		"  or (Ss*b+ & <CLAUSE>) or SIs*b- or [[Js-]] or [Os-];"
		""
		"this:"
		"  <noun-main-h>;"
		""
		"is.v: Ss- and Wi- and {O+};",
		false
	);
}

bool test_short_this_noun_dict()
{
	return test_short_sent("short sent realistic dict entry for noun",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"<costly-null>: [[[()]]];"
		""
		"<post-nominal-x>:"
		"  ({[B*j+]} & Xd- & (Xc+ or <costly-null>) & MX-);"
		""
		"<clause-conjoin>: RJrc- or RJlc+;"
		""
		"<CLAUSE>: {({@COd-} & (C- or <clause-conjoin>)) or ({@CO-} & (Wd- & {CC+})) or [Rn-]};"
		""
		"<noun-main-h>:"
		"  (Jd- & Dmu- & Os-)"
		"  or (Jd- & Dmu- & {Wd-} & Ss*b+)"
		"  or (Ss*b+ & <CLAUSE>) or SIs*b- or [[Js-]] or [Os-]"
		"  or <post-nominal-x>"
		"  or <costly-null>;"
		""
		"this:"
		"  <noun-main-h>;"
		""
		"is.v: Ss- and Wi- and {O+};",
		false
	);
}

int ntest_short()
{
	size_t num_failures = 0;

	if (!test_short_this()) num_failures++;
	if (!test_short_this_opt()) num_failures++;
	if (!test_short_this_obj_opt()) num_failures++;
	if (!test_short_this_costly()) num_failures++;
	if (!test_short_this_complex()) num_failures++;
	if (!test_short_this_noun_dict()) num_failures++;

	return num_failures;
}

// ==================================================================

bool test_seq_sent(const char *id, const char *dict_str, bool empty_state)
{
	total_tests++;

	Dictionary dict = dictionary_create_from_utf8(dict_str);

	// print_dictionary_data(dict);

	Parser parser(dict);
	// Expecting more words to follow, so a non-trivial state.
	// Unlike test_short_sent() above, here, we link "this" to
	// the left wall, followed by "is" to for a sequence.
	parser.streamin("this is");

	Lynk* alts = parser.get_alternatives();

	// At least one result should be this state pair.
	Lynk* sp =
		ALINK2(STATE_PAIR,
			ALINK0(SEQ),  // empty state
			ALINK2(SEQ,
				ALINK3(LING,
					ANODE(LING_TYPE, "Wd"),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "LEFT-WALL"),
						ANODE(CONNECTOR, "Wd+")),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "this"),
						ANODE(CONNECTOR, "Wd-"))),
				ALINK3(LING,
					ANODE(LING_TYPE, "Ss*b"),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "this"),
						ANODE(CONNECTOR, "Ss*b+")),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "is.v"),
						ANODE(CONNECTOR, "Ss-")))));

	if (empty_state)
	{
		Lynk* ans = ALINK1(SET, sp);
		if (not (ans->operator==(alts)))
		{
			cout << "Error: test failure on test \"" << id <<"\"" << endl;
			cout << "=== Expecting:\n" << ans << endl;
			cout << "=== Got:\n" << alts << endl;
			return false;
		}
	}
	else
	{
		// At least one alternative should be the desired state pair.
		bool found = false;
		size_t sz = alts->get_arity();
		for (size_t i=0; i<sz; i++)
		{
			Atom* a = alts->get_outgoing_atom(i);
			if (sp->operator==(a))
				found = true;
		}
		if (not found)
		{
			cout << "Error: test failure on test \"" << id <<"\"" << endl;
			cout << "=== Expecting one of them to be:\n" << sp << endl;
			cout << "=== Got:\n" << alts << endl;
			return false;
		}
	}

	cout<<"PASS: test_short_sent(" << id << ") " << endl;
	return true;
}

bool test_seq_this()
{
	return test_seq_sent("short seq sent",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Wd- and Ss*b+;"
		"is.v: Ss-;",
		true
	);
}

bool test_seq_this_opt()
{
	return test_seq_sent("short seq sent opt",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Wd- and Ss*b+;"
		"is.v: Ss- and {O+};",
		false
	);
}

bool test_seq_this_obj_opt()
{
	return test_seq_sent("short seq sent with obj",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Wd- and (Ss*b+ or Os-);"
		"is.v: Ss- and {O+};",
		false
	);
}

bool test_seq_this_costly()
{
	return test_seq_sent("short seq sent with costly null",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Wd- and (Ss*b+ or [[[()]]]);"
		"is.v: Ss-;",
		true
	);
}

bool test_seq_this_complex()
{
	return test_seq_sent("short seq sent complex",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		""
		"<CLAUSE>: {({@COd-} & (C-)) or ({@CO-} & (Wd- & {CC+})) or [Rn-]};"
		"<noun-main-h>:"
		"  (Jd- & Dmu- & Os-)"
		"  or (Jd- & Dmu- & {Wd-} & Ss*b+)"
		"  or (Ss*b+ & <CLAUSE>) or SIs*b- or [[Js-]] or [Os-];"
		""
		"this:"
		"  <noun-main-h>;"
		""
		"is.v: Ss- and {O+};",
		false
	);
}

bool test_seq_this_noun_dict()
{
	return test_seq_sent("short seq sent realistic dict entry for noun",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"<costly-null>: [[[()]]];"
		""
		"<post-nominal-x>:"
		"  ({[B*j+]} & Xd- & (Xc+ or <costly-null>) & MX-);"
		""
		"<clause-conjoin>: RJrc- or RJlc+;"
		""
		"<CLAUSE>: {({@COd-} & (C- or <clause-conjoin>)) or ({@CO-} & (Wd- & {CC+})) or [Rn-]};"
		""
		"<noun-main-h>:"
		"  (Jd- & Dmu- & Os-)"
		"  or (Jd- & Dmu- & {Wd-} & Ss*b+)"
		"  or (Ss*b+ & <CLAUSE>) or SIs*b- or [[Js-]] or [Os-]"
		"  or <post-nominal-x>"
		"  or <costly-null>;"
		""
		"this:"
		"  <noun-main-h>;"
		""
		"is.v: Ss- and {O+};",
		false
	);
}


bool test_seq_this_verb_dict()
{
	return test_seq_sent("short seq sent realistic dict entry for verb",
		"LEFT-WALL:"
		"  (Wd+ or Wq+ or Ws+ or Wj+ or Wc+ or Wi+ or We+ or Qd+)"
		"    & {CP+} & {Xx+} & {RW+ or Xp+};"
		""
		"<costly-null>: [[[()]]];"
		""
		"<post-nominal-x>:"
		"  ({[B*j+]} & Xd- & (Xc+ or <costly-null>) & MX-);"
		""
		"<clause-conjoin>: RJrc- or RJlc+;"
		""
		"<CLAUSE>: {({@COd-} & (C- or <clause-conjoin>)) or ({@CO-} & (Wd- & {CC+})) or [Rn-]};"
		""
		"<noun-main-h>:"
		"  (Jd- & Dmu- & Os-)"
		"  or (Jd- & Dmu- & {Wd-} & Ss*b+)"
		"  or (Ss*b+ & <CLAUSE>) or SIs*b- or [[Js-]] or [Os-]"
		"  or <post-nominal-x>"
		"  or <costly-null>;"
		""
		"this:"
		"  <noun-main-h>;"
		""
		"<verb-and-s->: {@E-} & VJrs-;"
		"<verb-and-s+>: {@E-} & VJls+;"
		""
		"<verb-x-s,u>: {@E-} & (Ss- or SFs- or SFu- or (RS- & Bs-));"
		""
		"<vc-be-obj>:"
		"  {@EBm+} & O*t+ & {@MV+};"
		""
		"<vc-be-no-obj>:"
		"  ({@EBm+} & ((([B**t-] or [K+] or BI+ or OF+ or PF- or"
		"      (Osi+ & R+ & Bs+) or"
		"      (Opi+ & R+ & Bp+) or"
		"      [[()]]) & {@MV+}) or"
		"    (Pp+ & {THi+ or @MV+}) or"
		"    THb+ or"
		"    TO+ or"
		"    Pa+)) or"
		"  ({N+} & (AF- or Pv+ or I*v+)) or"
		"  (({N+} or {Pp+}) & Pg*b+);"
		""
		"<vc-be>: <vc-be-no-obj> or <vc-be-obj>;"
		""
		"is.v:"
		"  (<verb-x-s,u> & <vc-be>) or"
		"  (<verb-and-s-> & <vc-be>) or (<vc-be> & <verb-and-s+>) or"
		"  (((Rw- or ({Ic-} & Q-) or [()]) & (SIs+ or SFIs+)) & <vc-be>);"
		"",
		false
	);
}

int ntest_short_seq()
{
	size_t num_failures = 0;

	if (!test_seq_this()) num_failures++;
	if (!test_seq_this_opt()) num_failures++;
	if (!test_seq_this_obj_opt()) num_failures++;
	if (!test_seq_this_costly()) num_failures++;
	if (!test_seq_this_complex()) num_failures++;
	if (!test_seq_this_noun_dict()) num_failures++;
	if (!test_seq_this_verb_dict()) num_failures++;

	return num_failures;
}

// ==================================================================

bool test_state_sent(const char *id, const char *dict_str)
{
	total_tests++;

	Dictionary dict = dictionary_create_from_utf8(dict_str);

	// print_dictionary_data(dict);

	Parser parser(dict);
	// Expecting more words to follow, so a non-trivial state.
	parser.streamin("this is a test");

	Lynk* alts = parser.get_alternatives();

	// We expect no output, and a crazy state:
	// The provided dictionary will not allow a linkage to happen;
	// this is really just testing the push of stack state.
	Lynk* sp =
		ALINK2(STATE_PAIR,
			ALINK5(SEQ,
				ALINK2(WORD_CSET,
					ANODE(WORD, "test.n"),
					ALINK2(OR,
						ANODE(CONNECTOR, "XXXGIVEN+"),
						ANODE(CONNECTOR, "AN+")))
				,
				ALINK2(WORD_CSET,
					ANODE(WORD, "a"),
					ANODE(CONNECTOR, "Ds+"))
				,
				ALINK2(WORD_CSET,
					ANODE(WORD, "is.v"),
					ANODE(CONNECTOR, "SIs+"))
				,
				ALINK2(WORD_CSET,
					ANODE(WORD, "this.J2"),
					ANODE(CONNECTOR, "JDBKQ+"))
				,
				ALINK2(WORD_CSET,
					ANODE(WORD, "LEFT-WALL"),
					ANODE(CONNECTOR, "Wq+"))
			),
			ALINK0(SEQ));  // empty output

	Lynk* ans = ALINK1(SET, sp);
	if (not (ans->operator==(alts)))
	{
		cout << "Error: test failure on test \"" << id <<"\"" << endl;
		cout << "=== Expecting:\n" << ans << endl;
		cout << "=== Got:\n" << alts << endl;
		return false;
	}
	cout<<"PASS: test_state_sent(" << id << ") " << endl;
	return true;
}

bool test_state_order()
{
	return test_state_sent("short state sent",
		"LEFT-WALL: Wq+;"
		"this.J2: JDBKQ+;"
		"is.v: SIs+;"
		"a: Ds+;"
		"test.n: XXXGIVEN+ or AN+;"
	);
}

bool test_state_order_left()
{
	return test_state_sent("short state sent leftwards",
		"LEFT-WALL: Wq+;"
		"this.J2: JDBKQ+ or JAAA-;"
		"is.v: SIs+ or KBB-;"
		"a: Ds+ & {Junk-} ;"
		"test.n: XXXGIVEN+ or BOGUS- or (AN+ & {GLOP-});"
	);
}

int ntest_short_state()
{
	size_t num_failures = 0;

	if (!test_state_order()) num_failures++;
	if (!test_state_order_left()) num_failures++;
	return num_failures;
}

// ==================================================================

bool test_right_wall(const char *id, const char *dict_str, bool empty_state)
{
	total_tests++;

	Dictionary dict = dictionary_create_from_utf8(dict_str);

	// print_dictionary_data(dict);
	Parser parser(dict);
	// Expecting more words to follow, so a non-trivial state.
	parser.streamin("this is .");

	Lynk* alts = parser.get_alternatives();

	// We expect empty final state.
	Lynk* sp =
		ALINK2(STATE_PAIR,
			ALINK0(SEQ),  // empty state
			ALINK3(SEQ,
				ALINK3(LING,
					ANODE(LING_TYPE, "Wd"),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "LEFT-WALL"),
						ANODE(CONNECTOR, "Wd+")),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "this"),
						ANODE(CONNECTOR, "Wd-")))
				,
				ALINK3(LING,
					ANODE(LING_TYPE, "Ss*b"),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "this"),
						ANODE(CONNECTOR, "Ss*b+")),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "is.v"),
						ANODE(CONNECTOR, "Ss-")))
				,
				ALINK3(LING,
					ANODE(LING_TYPE, "Xp"),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "LEFT-WALL"),
						ANODE(CONNECTOR, "Xp+")),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "."),
						ANODE(CONNECTOR, "Xp-")))
			));


	Lynk* ans = ALINK1(SET, sp);
	if (not (ans->operator==(alts)))
	{
		cout << "Error: test failure on test \"" << id <<"\"" << endl;
		cout << "=== Expecting:\n" << ans << endl;
		cout << "=== Got:\n" << alts << endl;
		return false;
	}
	cout<<"PASS: test_right_wall(" << id << ") " << endl;
	return true;
}

bool test_period()
{
	return test_right_wall("period",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {Xp+};"
		"this: Wd- and Ss*b+;"
		"is.v: Ss-;"
		"\".\": Xp-;",
		false
	);
}

int ntest_right_wall()
{
	size_t num_failures = 0;

	if (!test_period()) num_failures++;
	return num_failures;
}

// ==================================================================

// XXX currently a copy f test_short_sent ...
bool test_cost(const char *id, const char *dict_str, bool empty_state)
{
	total_tests++;

	Dictionary dict = dictionary_create_from_utf8(dict_str);
	// print_dictionary_data(dict);

cout<<"xxxxxxxxxxxxxxxxxxxxxxxx last test xxxxxxxxxxxxxxxx" <<endl;
	Parser parser(dict);

	// Expecting more words to follow, so a non-trivial state.
	// In particular, the dictionary will link the left-wall to
	// "is", so "this" has to be pushed on stack until the "is"
	// shows up.  The test_seq_sent() below will link the other
	// way around.
	parser.streamin("this is");

	Lynk* alts = parser.get_alternatives();

	// At least one result should be this state pair.
	Lynk* sp =
		ALINK2(STATE_PAIR,
			ALINK0(SEQ),  // empty state
			ALINK2(SEQ,
				ALINK3(LING,
					ANODE(LING_TYPE, "Ss*b"),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "this"),
						ANODE(CONNECTOR, "Ss*b+")),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "is.v"),
						ANODE(CONNECTOR, "Ss-")))
				,
				ALINK3(LING,
					ANODE(LING_TYPE, "Wi"),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "LEFT-WALL"),
						ANODE(CONNECTOR, "Wi+")),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "is.v"),
						ANODE(CONNECTOR, "Wi-")))
			));

	if (empty_state)
	{
		Lynk* ans = ALINK1(SET, sp);
		if (not (ans->operator==(alts)))
		{
			cout << "Error: test failure on test \"" << id <<"\"" << endl;
			cout << "=== Expecting:\n" << ans << endl;
			cout << "=== Got:\n" << alts << endl;
			return false;
		}
	}
	else
	{
		// At least one alternative should be the desired state pair.
		bool found = false;
		size_t sz = alts->get_arity();
		for (size_t i=0; i<sz; i++)
		{
			Atom* a = alts->get_outgoing_atom(i);
			if (sp->operator==(a))
				found = true;
		}
		if (not found)
		{
			cout << "Error: test failure on test \"" << id <<"\"" << endl;
			cout << "=== Expecting one of them to be:\n" << sp << endl;
			cout << "=== Got:\n" << alts << endl;
			return false;
		}
	}

	cout<<"PASS: test_short_sent(" << id << ") " << endl;
	return true;
}

bool test_cost_this()
{
	return test_cost("short cost sent",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+;"
		"is.v: Ss- and Wi-;"
		"is.w: [[Ss- and Wd-]];",
		true
	);
}

int ntest_cost()
{
	size_t num_failures = 0;

	if (!test_cost_this()) num_failures++;
	return num_failures;
}

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

int
main(int argc, char *argv[])
{
	size_t num_failures = 0;
	bool exit_on_fail = true;

	num_failures += ntest_core();
	report(num_failures, exit_on_fail);

	num_failures += ntest_flatten();
	report(num_failures, exit_on_fail);

	num_failures += ntest_disjoin();
	report(num_failures, exit_on_fail);

	num_failures += ntest_costly_disjoin();
	report(num_failures, exit_on_fail);

	num_failures += ntest_simple();
	report(num_failures, exit_on_fail);

	num_failures += ntest_two();
	report(num_failures, exit_on_fail);

	num_failures += ntest_first();
	report(num_failures, exit_on_fail);

	num_failures += ntest_short();
	report(num_failures, exit_on_fail);

	num_failures += ntest_short_seq();
	report(num_failures, exit_on_fail);

	num_failures += ntest_short_state();
	report(num_failures, exit_on_fail);

	num_failures += ntest_right_wall();
	report(num_failures, exit_on_fail);

	num_failures += ntest_cost();
	report(num_failures, exit_on_fail);

	exit (0);
}

