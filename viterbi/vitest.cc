/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
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

int total_tests = 0;

// ==================================================================
// A simple hello test; several different dictionaries
// should give exactly the same output.  The input sentence
// is just one word, it should connect to the left-wall in
// just one way. The result should be just one alternative:
// that alternatives has an empty state, and output with
// just one link.
bool test_hello(const char *id, const char *dict_str)
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
		cout << "Error: test failure on test " << id << endl;
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

bool test_simple_optional_left_cset()
{
	return test_hello ("optional left cset",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {CP+} & {Xx+} & {RW+ or Xp+};"
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

bool test_simple_optional()
{
	return test_hello ("optionals in both csets",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {CP+} & {Xx+} & {RW+ or Xp+};"
		"Hello: Wd- or Xi- or (Xj- & {A+ or B+});"
	);
}

bool test_simple_onereq()
{
	return test_hello ("one required link and opt righties (simple)",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"Hello: Wd- & {A+} & {B+} & {C+};"
	);
}

bool test_simple_zeroreq()
{
	return test_hello ("zero required links and opt righties (simple)",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"Hello: {Wd-} & {A+} & {B+} & {C+};"
	);
}

bool test_simple_onereq_and_left()
{
	return test_hello ("one required link and opt lefties (simple)",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"Hello: Wd- & {A-} & {B-} & {C+};"
	);
}

int ntest_simple()
{
	size_t num_failures = 0;

	if (!test_simplest()) num_failures++;
	if (!test_simple_left_disj()) num_failures++;
	if (!test_simple_optional_left_cset()) num_failures++;
	if (!test_simple_right_disj()) num_failures++;
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

bool test_alternative(const char *id, const char *dict_str)
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

	// This is the expected set of alternatives: two alternatives,
	// each with an empty state, and one of the two outputs, above.
	Lynk* ans =
	ALINK2(SET,
		ALINK2(STATE_PAIR,
			ALINK0(SEQ),
			ALINK1(SEQ, alt_out_one)
		),
		ALINK2(STATE_PAIR,
			ALINK0(SEQ),
			ALINK1(SEQ, alt_out_two)
		)
	);

	Lynk* output = parser.get_alternatives();
	if (not (ans->operator==(output)))
	{
		cout << "Error: test failure on test " << id << endl;
		cout << "=== Expecting:\n" << ans << endl;
		cout << "=== Got:\n" << output << endl;
		return false;
	}

	cout<<"PASS: test_alternative(" << id << ") " << endl;
	return true;
}

bool test_two_alts()
{
	return test_alternative("two alternatives",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"Hello: Wd- or Wi-;"
	);
}

bool test_two_opts()
{
	return test_alternative("two alts plus opts",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {A+};"
		"Hello: Wd- or Wi- or (Xj- & {A+ or B+});"
	);
}

bool test_two_one_opts()
{
	return test_alternative("two alt, or one opt",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {A+};"
		"Hello: Wd- or {Wi-} or (Xj- & {A+ or B+});"
	);
}

bool test_two_all_opts()
{
	return test_alternative("two alts, or all opt",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {A+};"
		"Hello: {Wd-} or {Wi-} or (Xj- & {A+ or B+});"
	);
}

bool test_two_and_opts()
{
	return test_alternative("two alts, and an opt",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {A+};"
		"Hello: Wd- or (Wi- & {Xj- & {A+ or B+}} & {C+});"
	);
}

bool test_two_and_no_opts()
{
	return test_alternative("two alt, and all opt",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {A+};"
		"Hello: Wd- or ({Wi-} & {Xj- & {A+ or B+}} & {C+});"
	);
}

bool test_two_and_excess()
{
	return test_alternative("two alt, and excess reqs",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {A+};"
		"Hello: Wd- or (Wi- & Xj- & {A+ or B+} & {C+}) or Wi-;"
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

bool test_short_sent(const char *id, const char *dict_str)
{
	total_tests++;

	Dictionary dict = dictionary_create_from_utf8(dict_str);

	// print_dictionary_data(dict);
cout<<"xxxxxxxxxxxxxxxxxxxxxxxx last test xxxxxxxxxxxxxxxx" <<endl;

	Parser parser(dict);
	// Expecting more words to follow, so a non-trivial state.
	parser.streamin("this is");

	Lynk* output = parser.get_alternatives();
cout<<"final alts >>>" << output<<endl;

return false;

	cout<<"PASS: test_short_sent(" << id << ") " << endl;
	return true;
}

bool test_short_this()
{
	return test_short_sent("short sent",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+;"
		"is.v: Ss- and Wi-;"
	);
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

	num_failures += ntest_simple();
	report(num_failures, exit_on_fail);

	num_failures += ntest_two();
	report(num_failures, exit_on_fail);

	num_failures += ntest_first();
	report(num_failures, exit_on_fail);

	if (!test_short_this()) num_failures++;
	report(num_failures, exit_on_fail);

	exit (0);
}

