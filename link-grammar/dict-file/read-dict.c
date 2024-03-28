/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright 2013, 2014 Linas Vepstas                                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <string.h>

#include "connectors.h"
#include "dict-common/dialect.h"
#include "dict-common/dict-affix.h"     // is_stem
#include "dict-common/dict-common.h"
#include "dict-common/dict-internals.h"
#include "dict-common/dict-utils.h"     // patch_subscript
#include "dict-common/file-utils.h"
#include "dict-common/idiom.h"
#include "dict-ram/dict-ram.h"
#include "error.h"
#include "externs.h"
#include "print/print.h"
#include "read-dict.h"
#include "string-set.h"
#include "tokenize/tok-structures.h"    // MT_WALL
#include "utilities.h"
#include "word-file.h"

/*
  The dictionary format:

  In what follows:
    Every "%" symbol and everything after it is ignored on every line.
    Every newline or tab is replaced by a space.

  The dictionary file is a sequence of ENTRIES.  Each ENTRY is one or
  more WORDS (a sequence of upper or lower case letters) separated by
  spaces, followed by a ":", followed by an EXPRESSION followed by a
  ";".  An EXPRESSION is an expression where the operators are "&"
  or "and" or "|" or "or", and there are three types of parentheses:
  "()", "{}", and "[]".  The terminal symbols of this grammar are the
  connectors, which are strings of letters or numbers or *s.
  Expressions are written in infix form.

  The connector begins with an optional @, which is followed by an upper
  case sequence of letters. Each subsequent *, lower case letter or
  number is a subscript. At the end is a + or - sign.  The "@" allows
  this connector to attach to one or more other connectors.

  Here is a sample dictionary entry:

      gone:         T- & {@EV+};

  (See our paper for more about how to interpret the meaning of the
  dictionary expressions.)

  A previously defined word (such as "gone" above) may be used instead
  of a connector to specify the expression it was defined to be.  Of
  course, in this case, it must uniquely specify a word in the
  dictionary, and have been previously defined.

  If a word is of the form "/foo", then the file current-dir/foo
  is a so-called word file, and is read in as a list of words.
  A word file is just a list of words separated by blanks or newlines.

  A word that contains the character "_" defines an idiomatic use of
  the words separated by the "_".  For example "kind of" is an idiomatic
  expression, so a word "kind_of" is defined in the dictionary.
  Idiomatic expressions of any number of words can be defined in this way.
  When the word "kind" is encountered, all the idiomatic uses of the word
  are considered.

  An expression enclosed in "[..]" is give a cost of 1.  This means
  that if any of the connectors inside the square braces are used,
  a cost of 1 is incurred.  (This cost is the first element of the cost
  vector printed when a sentence is parsed.)  Of course if something is
  inside of 10 levels of "[..]" then using it incurs a cost of 10.
  These costs are called "disjunct costs".  The linkages are printed out
  in order of non-increasing disjunct cost.

  A number following a square bracket over-rides the cost of that bracket.
  Thus, [...].5 has a cost of 0.5 while [...]2.0 has a cost of 2; that
  is it is the same as [[...]]. Only a sign, decimal digits and a point
  are allowed. The maximum recognized number is "99.9999". Digits which
  are more than 4 positions to the right of the decimal point are
  ignored.

  Instead of a numerical cost, a symbolic cost can be used, a.k.a. a
  "dialect component name".  The file "4.0.dialect" defines dialect names
  (in square brackets) and their component names and values.

  The expression "(A+ or ())" means that you can choose either "A+" or
  the empty expression "()", that is, that the connector "A+" is
  optional.  This is more compactly expressed as "{A+}".  In other words,
  curly braces indicate an optional expression.

  The expression "(A+ or [])" is the same as that above, but there is a
  cost of 1 incurred for choosing not to use "A+".  The expression
  "(EXP1 & [EXP2])" is exactly the same as "[EXP1 & EXP2]".  The difference
  between "({[A+]} & B+)" and "([{A+}] & B+)" is that the latter always
  incurs a cost of 1, while the former only gets a cost of 1 if "A+" is
  used.

  The dictionary writer is not allowed to use connectors that begin in
  "_".  This is reserved for connectors that are automatically
  generated (currently only for idioms).

  Dictionary words may be followed by a dot (period, "."), and a "subscript"
  identifying the word type. The subscript may be one or more letters or
  numbers, but must begin with a letter. Currently, the dictionary contains
  (mostly?) subscripts consisting of a single letter, and these serve mostly
  to identify the part-of-speech. In general, subscripts can also be used
  to distinguish different word senses.

  Subscripts that start with "_" are reserved for words that are
  automatically generated (currently only for idioms).
*/

struct FileCursor_s
{
	Dictionary      dict;
	const char    * input;
	const char    * pin;
	bool            recursive_error;
	bool            is_special;
	int             already_got_it; /* For char, but needs to hold EOF */
	char            token[MAX_TOKEN_LENGTH];
};
typedef struct FileCursor_s * FileCursor;

static bool link_advance(FileCursor);

static void dict_error2(FileCursor fcurs, const char * s, const char *s2)
{
	/* The link_advance used to print the error message can
	 * throw more errors while printing... */
	if (fcurs->recursive_error) return;
	fcurs->recursive_error = true;

	Dictionary dict = fcurs->dict;
	char token[MAX_TOKEN_LENGTH];
	strcpy(token, fcurs->token);
	bool save_is_special    = fcurs->is_special;
	const char * save_input = fcurs->input;
	const char * save_pin   = fcurs->pin;
	int save_already_got_it = fcurs->already_got_it;
	int save_line_number    = dict->line_number;

#define ERRBUFLEN 1024
	char tokens[ERRBUFLEN], t[ERRBUFLEN];
	int pos = 1;
	tokens[0] = '\0';
	for (int i=0; i<5 && fcurs->token[0] != '\0'; i++)
	{
		pos += snprintf(t, ERRBUFLEN, "\"%s\" ", fcurs->token);
		strncat(tokens, t, ERRBUFLEN-1-pos);
		if (!link_advance(fcurs)) break;
	}
	tokens[pos] = '\0';

	strcpy(fcurs->token, token);
	fcurs->is_special     = save_is_special;
	fcurs->input          = save_input;
	fcurs->pin            = save_pin;
	fcurs->already_got_it = save_already_got_it;
	dict->line_number     = save_line_number;

	if (s2)
	{
		prt_error("Error: While parsing dictionary \"%s\":\n"
		          "%s \"%s\"\n\t Line %d, next tokens: %s\n",
		          dict->name, s, s2, dict->line_number, tokens);
	}
	else
	{
		prt_error("Error: While parsing dictionary \"%s\":\n"
		          "%s\n\t Line %d, next tokens: %s\n",
		          dict->name, s, dict->line_number, tokens);
	}
	fcurs->recursive_error = false;
}

static void dict_error(FileCursor fcurs, const char * s)
{
	dict_error2(fcurs, s, NULL);
}

static void warning(FileCursor fcurs, const char * s)
{
	prt_error("Warning: %s\n"
	        "\tline %d, current token = \"%s\"\n",
	        s, fcurs->dict->line_number, fcurs->token);
}

/**
 * This gets the next UTF8 character from the input, eliminating comments.
 * If we're in quote mode, it does not consider the % character for
 * comments.   Note that the returned character is a wide character!
 */
#define MAXUTFLEN 7
typedef char utf8char[MAXUTFLEN];
static bool get_character(FileCursor fcurs, int quote_mode, utf8char uc)
{
	Dictionary dict = fcurs->dict;

	int i = 0;
	while (1)
	{
		char c = *(fcurs->pin++);

		/* Skip over all comments */
		if ((c == '%') && (!quote_mode))
		{
			while ((c != 0x0) && (c != '\n')) c = *(fcurs->pin++);
			if (c == 0x0) break;
			dict->line_number++;
			continue;
		}

		/* Newline counts as whitespace */
		if (c == '\n')
			dict->line_number++;

		/* If it's a 7-bit ascii, we are done */
		if ((0 == i) && ((c & 0x80) == 0x0))
		{
			uc[0] = c;
			uc[1] = 0x0;
			return true;
		}

		uc[0] = c;
		i = 1;
		while (i < MAXUTFLEN-1)
		{
			c = *(fcurs->pin++);
			/* If we're onto the next char, we're done. */
			if (((c & 0x80) == 0x0) || ((c & 0xc0) == 0xc0))
			{
				fcurs->pin--;
				uc[i] = 0x0;
				return true;
			}
			uc[i] = c;
			i++;
		}
		dict_error(fcurs, "UTF8 char is too long.");
		return false;
	}
	uc[0] = 0x0;
	return true;
}


/*
 * This set of 10 characters are the ones defining the syntax of the
 * dictionary.
 */
#define SPECIAL "(){};[]&^|:"

/* Commutative (symmetric) AND */
#define SYM_AND '^'

/* Bi-directional connector: + or - */
#define ANY_DIR '$'

/* Wild-card link type */
#define WILD_TYPE '*'

/**
 * Return true if the input character is one of the special
 * characters used to define the syntax of the dictionary.
 */
static bool char_is_special(char c)
{
	return (NULL != strchr(SPECIAL, c));
}

/**
 * This reads the next token from the input into 'token'.
 * Return 1 if a character was read, else return 0 (and print a warning).
 */
NO_SAN_DICT
static bool link_advance(FileCursor fcurs)
{
	bool quote_mode = false;
	fcurs->is_special = false;

	if (fcurs->already_got_it != '\0')
	{
		fcurs->is_special = char_is_special(fcurs->already_got_it);
		if (fcurs->already_got_it == EOF) {
			fcurs->token[0] = '\0';
		} else {
			fcurs->token[0] = (char)fcurs->already_got_it; /* specials are one byte */
			fcurs->token[1] = '\0';
		}
		fcurs->already_got_it = '\0';
		return true;
	}

	utf8char c;
	do
	{
		bool ok = get_character(fcurs, false, c);
		if (!ok) return false;
	}
	while (lg_isspace((unsigned char)c[0]));

	int i = 0;
	for (;;)
	{
		if (i > MAX_TOKEN_LENGTH-3) {
			dict_error(fcurs, "Token too long.");
			return false;
		}

		/* Everything quoted is part of the token, until a matching
		 * close-quote is found. A closing quote is a quote that is
		 * followed by whitespace, or by ':' or by ';'. Everything
		 * between the inital quote and the closing quote is copied
		 * into the token. This includes embedded white-space.
		 *
		 * This is slightly awkward, and is NOT conventional
		 * programming practice for quotes. It does allow quotes to be
		 * embedded into the middle of tokens. It misbehaves mildly
		 * when a quoted string is used with a #define statement.
		 */
		if (quote_mode) {
			if (c[0] == '"' &&
			    /* Check the next character too, to allow " in words */
			    (*fcurs->pin == ':' || *fcurs->pin == ';' ||
			    lg_isspace((unsigned char)*fcurs->pin))) {

				fcurs->token[i] = '\0';
				return true;
			}

			if (c[0] == '\0')
			{
				dict_error(fcurs, "EOF while reading quoted token.");
				return false;
			}

			/* Copy all of the UTF8 bytes. */
			int nr = 0;
			while (c[nr]) {fcurs->token[i] = c[nr]; i++; nr++; }
		} else {
			if ('\0' == c[1] && char_is_special(c[0]))
			{
				if (i == 0)
				{
					fcurs->token[0] = c[0];  /* special toks are one char always */
					fcurs->token[1] = '\0';
					fcurs->is_special = true;
					return true;
				}
				fcurs->token[i] = '\0';
				fcurs->already_got_it = c[0];
				return true;
			}
			if (c[0] == 0x0) {
				if (i != 0) fcurs->already_got_it = '\0';
				fcurs->token[0] = '\0';
				return true;
			}
			if (lg_isspace((unsigned char)c[0])) {
				fcurs->token[i] = '\0';
				return true;
			}
			if (c[0] == '\"') {
				quote_mode = true;
			} else {
				int nr = 0;
				while (c[nr]) {fcurs->token[i] = c[nr]; i++; nr++; }
			}
		}
		bool ok = get_character(fcurs, quote_mode, c);
		if (!ok) return false;
	}
	/* unreachable */
}

/**
 * Returns true if this token is a special token and it is equal to c
 */
static int is_equal(FileCursor fcurs, char c)
{
	return (fcurs->is_special &&
	        c == fcurs->token[0] &&
	        fcurs->token[1] == '\0');
}

/**
 * Make sure the string s is a valid connector.
 * Return true if the connector is valid, else return false,
 * and print an appropriate warning message.
 */
static bool check_connector(FileCursor fcurs, const char * s)
{
	int i;
	i = strlen(s);
	if (i < 1) {
		dict_error(fcurs, "Expecting a connector.");
		return false;
	}
	i = s[i-1];  /* the last character of the token */
	if ((i != '+') && (i != '-') && (i != ANY_DIR)) {
		dict_error(fcurs, "A connector must end in a \"+\", \"-\" or \"$\".");
		return false;
	}
	if (*s == '@') s++;
	if (('h' == *s) || ('d' == *s)) s++;
	if (!is_connector_name_char(*s)) {
		dict_error2(fcurs, "Invalid character in connector "
		            "(connectors must start with an uppercase letter "
		            "after an optional \"h\" or \"d\"):", (char[]){*s, '\0'});
		return false;
	}
	if (*s == '_')
	{
		dict_error(fcurs, "Invalid character in connector "
		           "(an initial \"_\" is reserved for internal use).");
		return false;
	}

	/* The first uppercase has been validated above. */
	do { s++; } while (is_connector_name_char(*s));
	while (s[1]) {
		if (!is_connector_subscript_char(*s) && (*s != WILD_TYPE)) {
			dict_error2(fcurs, "Invalid character in connector subscript "
			            "(only lowercase letters, digits, and \"*\" are allowed):",
			            (char[]){*s, '\0'});
			return false;
		}
		s++;
	}
	return true;
}

/* ======================================================================== */
/**
 * make_dir_connector() -- make a single node for a connector
 * that is a + or a - connector.
 *
 * Assumes the current token is the connector.
 */
static Exp * make_dir_connector(Dictionary dict, FileCursor fcurs, int i)
{
	char *constring;
	bool multi = false;

	char dir = fcurs->token[i];
	fcurs->token[i] = '\0';   /* get rid of the + or - */
	if (fcurs->token[0] == '@')
	{
		constring = fcurs->token+1;
		multi = true;
	}
	else
		constring = fcurs->token;

	return  make_connector_node(dict, dict->Exp_pool,
	                            constring, dir, multi);
}

/* ======================================================================== */
/**
 * Add an optional macro/word tag, for expression debugging.
 * Enabled by !test="macro-tag". This tag is used only in expression printing.
 */
static unsigned int exptag_macro_add(Dictionary dict, const char *tag)
{
	expression_tag *mt = dict->macro_tag;
	if (mt == NULL) return 0;

	if (mt->num == mt->size)
	{
		if (mt->num == 0)
			mt->size = 128;
		else
			mt->size *= 2;

		mt->name = realloc(mt->name, mt->size * sizeof(*mt->name));
	}
	mt->name[mt->num] = tag;

	return mt->num++;
}

/**
 * make_connector() -- make a node for a connector or dictionary word.
 *
 * Assumes the current token is a connector or dictionary word.
 */
static Exp * make_connector(FileCursor fcurs)
{
	Dictionary dict = fcurs->dict;
	Exp * n;

	int i = strlen(fcurs->token) - 1;  /* this must be +, - or $ if a connector */
	if ((fcurs->token[i] != '+') &&
	    (fcurs->token[i] != '-') &&
	    (fcurs->token[i] != ANY_DIR))
	{
		/* If we are here, token is a word */
		patch_subscript(fcurs->token);
		Dict_node * dn = strict_lookup_list(dict, fcurs->token);
		if (dn == NULL)
		{
			dict_error2(fcurs, "Perhaps missing + or - in a connector.\n"
			                 "Or perhaps you forgot the subscript on a word.\n"
			                 "Or perhaps the word is used before it is defined:",
			                 fcurs->token);
			return NULL;
		}
		if (dn->right != NULL)
		{
			dict_node_free_list(dn);
			dict_error2(fcurs, "Referencing a duplicate word:", fcurs->token);
			/* Note: A word which becomes duplicate latter evades this check. */
			return NULL;
		}

		/* Wrap it in a unary node as a placeholder for a macro tag and cost. */
		n = make_unary_node(dict->Exp_pool, dn->exp);
		n->tag_id = exptag_macro_add(dict, dn->string);
		if (n->tag_id != 0) n->tag_type = Exptag_macro;

		dict_node_free_list(dn);
	}
	else
	{
		/* If we are here, token is a connector */
		if (!check_connector(fcurs, fcurs->token))
		{
			return NULL;
		}
		if ((fcurs->token[i] == '+') || (fcurs->token[i] == '-'))
		{
			/* A simple, unidirectional connector. Just make that. */
			n = make_dir_connector(dict, fcurs, i);
			if (NULL == n) return NULL;
		}
		else if (fcurs->token[i] == ANY_DIR)
		{
			Exp *plu, *min;
			/* If we are here, then it's a bi-directional connector.
			 * Make both a + and a - version, and or them together.  */
			fcurs->token[i] = '+';
			plu = make_dir_connector(dict, fcurs, i);
			if (NULL == plu) return NULL;
			fcurs->token[i] = '-';
			min = make_dir_connector(dict, fcurs, i);
			if (NULL == min) return NULL;

			n = make_or_node(dict->Exp_pool, plu, min);
		}
		else
		{
			dict_error(fcurs, "Unknown connector direction type.");
			return NULL;
		}
	}

	if (!link_advance(fcurs))
	{
		free(n);
		return NULL;
	}
	return n;
}

/* ======================================================================== */

/**
 * Return true if the string is a (floating point) number.
 * Float points can be proceeded by a single plus or minus sign.
 */
static bool is_number(const char * str)
{
	if (str[0] == '\0') return false; /* End of file. */
	if ('+' == str[0] || '-' == str[0]) str++;
	size_t numlen = strspn(str, "0123456789.");

	return str[numlen] == '\0';
}

/* ======================================================================== */

/**
 * Build (and return the root of) the tree for the expression beginning
 * with the current token.  At the end, the token is the first one not
 * part of this expression.
 */
static Exp *make_expression(FileCursor fcurs)
{
	Dictionary dict = fcurs->dict;

	Exp *nl = NULL;
	Exp *e_head = NULL;
	Exp *e_tail = NULL; /* last part of the expression */
	bool is_sym_and = false;

	while (true)
	{
		if (is_equal(fcurs, '('))
		{
			if (!link_advance(fcurs)) {
				return NULL;
			}
			nl = make_expression(fcurs);
			if (nl == NULL) {
				return NULL;
			}
			if (!is_equal(fcurs, ')')) {
				dict_error(fcurs, "Expecting a \")\".");
				return NULL;
			}
			if (!link_advance(fcurs)) {
				return NULL;
			}
		}
		else if (is_equal(fcurs, '{'))
		{
			if (!link_advance(fcurs)) {
				return NULL;
			}
			nl = make_expression(fcurs);
			if (nl == NULL) {
				return NULL;
			}
			if (!is_equal(fcurs, '}')) {
				dict_error(fcurs, "Expecting a \"}\".");
				return NULL;
			}
			if (!link_advance(fcurs)) {
				return NULL;
			}
			nl = make_optional_node(dict->Exp_pool, nl);
		}
		else if (is_equal(fcurs, '['))
		{
			if (!link_advance(fcurs)) {
				return NULL;
			}
			nl = make_expression(fcurs);
			if (nl == NULL) {
				return NULL;
			}
			if (!is_equal(fcurs, ']')) {
				dict_error(fcurs, "Expecting a \"]\".");
				return NULL;
			}
			if (!link_advance(fcurs)) {
				return NULL;
			}

			/* A square bracket can have a number or a name after it.
			 * If a number is present, then that it is interpreted
			 * as the cost of the bracket. If a name is present, it
			 * is used as an expression tag. Else, the cost of a
			 * square bracket is 1.0.
			 */
			if (is_number(fcurs->token))
			{
				float cost;

				if (strtofC(fcurs->token, &cost))
				{
					nl->cost += cost;
				}
				else
				{
					warning(fcurs, "Invalid cost (using 1.0)\n");
					nl->cost += 1.0F;
				}
				if (!link_advance(fcurs)) {
					return NULL;
				}
			}
			else if ((strcmp(fcurs->token, "or") != 0) &&
			         (strcmp(fcurs->token, "and") != 0) &&
			         isalpha((unsigned char)fcurs->token[0]))
			{
				const char *bad = valid_dialect_name(fcurs->token);
				if (bad != NULL)
				{
					char badchar[] = { *bad, '\0' };
					dict_error2(fcurs, "Invalid character in dialect tag name:",
					           badchar);
					return NULL;
				}
				if ((nl->type == CONNECTOR_type) || (nl->tag_type != Exptag_none))
				{
					nl = make_unary_node(dict->Exp_pool, nl);
				}
				nl->tag_id = exptag_dialect_add(dict, fcurs->token);
				nl->tag_type = Exptag_dialect;
				if (!link_advance(fcurs)) {
					return NULL;
				}
			}
			else
			{
				nl->cost += 1.0F;
			}
		}
		else if (!fcurs->is_special)
		{
			nl = make_connector(fcurs);
			if (nl == NULL) {
				return NULL;
			}
		}
		else if (is_equal(fcurs, ')') || is_equal(fcurs, ']'))
		{
			/* allows "()" or "[]" */
			nl = make_zeroary_node(dict->Exp_pool);
		}
		else
		{
			dict_error(fcurs, "Connector, \"(\", \"[\", or \"{\" expected.");
			return NULL;
		}

		if (is_sym_and)
		{
			/* Part 2/2 of SYM_AND processing. */

			/* Expand A ^ B into the expr ((A & B) or (B & A)). */
			Exp *na = make_and_node(dict->Exp_pool,
			                   Exp_create_dup(dict->Exp_pool, e_tail),
			                   Exp_create_dup(dict->Exp_pool, nl));
			Exp *nb = make_and_node(dict->Exp_pool,
			                   Exp_create_dup(dict->Exp_pool, nl),
			                   Exp_create_dup(dict->Exp_pool, e_tail));
			Exp *or = make_or_node(dict->Exp_pool, na, nb);

			*e_tail = *or; /* SYM_AND result */
			is_sym_and = false;
		}
		else if (e_tail != NULL)
		{
			/* This is Non-commuting AND or Commuting OR.
			 * Append the just read expression (nl) to its operand chain. */
			e_tail->operand_next = nl;
			e_tail = nl;
		}

		/* Extract the operator. */

		Exp_type op;

		/* Non-commuting AND */
		if (is_equal(fcurs, '&') || (strcmp(fcurs->token, "and") == 0))
		{
			op = AND_type;
		}
		/* Commuting OR */
		else if (is_equal(fcurs, '|') || (strcmp(fcurs->token, "or") == 0))
		{
			op =  OR_type;
		}
		/* Commuting AND */
		else if (is_equal(fcurs, SYM_AND) || (strcmp(fcurs->token, "sym") == 0))
		{
			/* Part 1/2 of SYM_AND processing */
			op = AND_type; /* allow mixing with ordinary ands at the same level */
			is_sym_and = true; /* processing to be completed after next argument */
		}
		else
		{
			if (e_head != NULL) return e_head;
			return nl;
		}

		/* If this is the first operand, use nl for it.  Else just validate
		 * that the current operator is consistent with that of the current
		 * expression level. */
		if (e_head == NULL)
		{
			e_head = make_join_node(dict->Exp_pool, nl, NULL, op);
		}
		else
		{
			if (e_head->type != op)
			{
				dict_error(fcurs, "\"and\" and \"or\" at the same level in an expression.");
				return NULL;
			}
		}

		if (!link_advance(fcurs)) {
			return NULL;
		}

		if (e_tail == NULL)
			e_tail = e_head->operand_first;
	}
		/* unreachable */
}

/* ======================================================================== */

/**
 * Remember the length_limit definitions in a list according to their order.
 * The order is kept to allow later more specific definitions to override
 * already applied ones.
 */
static void add_condesc_length_limit(Dictionary dict, Dict_node *dn,
                                     int length_limit)
{
	length_limit_def_t *lld = malloc(sizeof(*lld));
	lld->next = NULL;
	lld->length_limit = length_limit;
	lld->defexp = dn->exp;
	lld->defword = dn->string;
	*dict->contable.length_limit_def_next = lld;
	dict->contable.length_limit_def_next = &lld->next;
}

static void insert_length_limit(Dictionary dict, Dict_node *dn)
{
	int length_limit;

	if (0 == strcmp(UNLIMITED_CONNECTORS_WORD, dn->string))
	{
		length_limit = UNLIMITED_LEN;
	}
	else if (0 == strncmp(LIMITED_CONNECTORS_WORD, dn->string,
	                 sizeof(LIMITED_CONNECTORS_WORD)-1))
	{
		char *endp;
		length_limit =
			(int)strtol(dn->string + sizeof(LIMITED_CONNECTORS_WORD)-1, &endp, 10);
		if ((length_limit < 0) || (length_limit > MAX_SENTENCE) ||
		  (('\0' != *endp) && (SUBSCRIPT_MARK != *endp)))
		{
			prt_error("Warning: Word \"%s\" found near line %d of \"%s\".\n"
					  "\tThis word should end with a number (1-%d).\n"
					  "\tThis word will be ignored.\n",
					  dn->string, dict->line_number, dict->name, MAX_SENTENCE);
			return;
		}
	}
	else return;

	/* We cannot set the connectors length_limit yet because the
	 * needed data structure is not defined yet. For now, just
	 * remember the definitions in their order. */
	add_condesc_length_limit(dict, dn, length_limit);
}

void free_insert_list(Dict_node *ilist)
{
	Dict_node * n;
	while (ilist != NULL)
	{
		n = ilist->left;
		free(ilist);
		ilist = n;
	}
}

/**
 * insert_list() -
 * p points to a list of dict_nodes connected by their left pointers.
 * l is the length of this list (the last ptr may not be NULL).
 * It inserts the list into the dictionary.
 * It does the middle one first, then the left half, then the right.
 *
 * Note: I think this "insert middle, then left, then right" algo has
 * its origins as a lame attempt to hack around the fact that the
 * resulting binary tree is rather badly unbalanced. This has been
 * fixed by using the DSW rebalancing algo. Now, that would seem
 * to render this crazy bisected-insertion algo obsolete, but ..
 * oddly enough, it seems to make the DSW balancing go really fast!
 * Faster than a simple insertion. Go figure. I think this has
 * something to do with the fact that the dictionaries are in
 * alphabetical order! This subdivision helps randomize a bit.
 */
void insert_list(Dictionary dict, Dict_node * p, int l)
{
	Dict_node * dn, *dn_second_half;
	int k, i; /* length of first half */

	if (l == 0) return;

	k = (l-1)/2;
	dn = p;
	for (i = 0; i < k; i++)
	{
		dn = dn->left;
	}

	/* dn now points to the middle element */
	dn_second_half = dn->left;
	dn->left = dn->right = NULL;

	const char *sm = get_word_subscript(dn->string);
	if ((NULL != sm) && ('_' == sm[1]))
	{
		prt_error("Warning: Word \"%s\" found near line %d of \"%s\".\n"
		        "\tWords ending \"._\" are reserved for internal use.\n"
		        "\tThis word will be ignored.\n",
		        dn->string, dict->line_number, dict->name);
		free(dn);
	}
	else
	{
		if (contains_underbar(dn->string))
		{
			insert_idiom(dict, dn);
		}

		dict->root = dict_node_insert(dict, dict->root, dn);
		insert_length_limit(dict, dn);
		dict->num_entries++;
	}

	insert_list(dict, p, k);
	insert_list(dict, dn_second_half, l-k-1);
}

/**
 * read_entry() -- read one dictionary entry
 * Starting with the current token, parse one dictionary entry.
 * A single dictionary entry must have one and only one colon in it,
 * and is terminated by a semi-colon.
 * Add these words to the dictionary.
 */
static bool read_entry(FileCursor fcurs)
{
	Dict_node *dnx, *dn = NULL;

	while (!is_equal(fcurs, ':'))
	{
		if (fcurs->is_special)
		{
			dict_error(fcurs, "I expected a word but didn\'t get it.");
			goto syntax_error;
		}

		/* If it's a word-file name. */
		/* However, be careful to reject "/.v" which is the division symbol
		 * used in equations (.v means verb-like). Also reject an affix regex
		 * specification (may appear only in the affix file). */
		if ((fcurs->token[0] == '/') &&
		    (fcurs->token[1] != '.') && (get_affix_regex_cg(fcurs->token) < 0))
		{
			Dict_node *new_dn = read_word_file(fcurs->dict, dn, fcurs->token);
			if (new_dn == NULL)
			{
				prt_error("Error: Cannot open word file \"%s\".\n", fcurs->token);

				goto syntax_error; /* not a syntax error, but need to free dn */
			}
			dn = new_dn;
		}
		else if (0 == strcmp(fcurs->token, "#include"))
		{
			if (!link_advance(fcurs)) goto syntax_error;

			/* OK, token contains the filename to read ... */
			char* dict_name = strdupa(fcurs->token);
			size_t skip_slash = ('/' == fcurs->token[0]) ? 1 : 0;
			char* instr = get_file_contents(dict_name + skip_slash);
			if (NULL == instr)
			{
				Dictionary dict = fcurs->dict;
				prt_error("Error: While parsing dictionary \"%s\":\n"
				          "\t Line %d: Could not open subdictionary \"%s\"\n",
				          dict->name, dict->line_number-1, dict_name);
				goto syntax_error;
			}

			/* The dict name and line-number are used for error reporting */
			Dictionary dict = fcurs->dict;
			const char * save_name = dict->name;
			int save_line_number = dict->line_number;
			dict->name = dict_name;

			/* Now read the thing in. */
			bool rc = read_dictionary(dict, instr);

			dict->name            = save_name;
			dict->line_number     = save_line_number;

			free_file_contents(instr);
			if (!rc) goto syntax_error;

			/* when we return, point to the next entry */
			if (!link_advance(fcurs)) goto syntax_error;

			/* If a semicolon follows the include, that's OK... ignore it. */
			if (';' == fcurs->token[0])
			{
				if (!link_advance(fcurs)) goto syntax_error;
			}

			return true;
		}
		else if (0 == strcmp(fcurs->token, "#define"))
		{
			if (!link_advance(fcurs)) goto syntax_error;
			const char *name = strdupa(fcurs->token);

			/* Get the value. */
			if (!link_advance(fcurs)) goto syntax_error;
			add_define(fcurs->dict, name, fcurs->token);

			if (!link_advance(fcurs)) goto syntax_error;
			if (!is_equal(fcurs, ';'))
			{
				dict_error(fcurs, "Expecting \";\" at the end of #define.");
				goto syntax_error;
			}
		}
		else
		{
			Dict_node * dn_new = dict_node_new();
			dn_new->left = dn;
			dn_new->right = NULL;
			dn_new->exp = NULL;
			dn = dn_new;
			dn->file = NULL;

			/* Note: The following patches a dot in regexes appearing in
			 * the affix file... It is corrected later. */
			patch_subscript(fcurs->token);
			dn->string = string_set_add(fcurs->token, fcurs->dict->string_set);
		}

		/* Advance to next entry, unless error */
		if (!link_advance(fcurs)) goto syntax_error;
	}

	/* pass the : */
	if (!link_advance(fcurs))
	{
		goto syntax_error;
	}

	Exp * n = make_expression(fcurs);
	if (n == NULL)
		goto syntax_error;

	if (!is_equal(fcurs, ';'))
	{
		dict_error(fcurs, "Expecting \";\" at the end of an entry.");
		goto syntax_error;
	}

	if (dn == NULL)
	{
		dict_error(fcurs, "Expecting a token before \":\".");
		goto syntax_error;
	}

	/* At this point, dn points to a list of Dict_nodes connected by
	 * their left pointers. These are to be inserted into the dictionary. */
	int i = 0;
	for (dnx = dn; dnx != NULL; dnx = dnx->left)
	{
		dnx->exp = n;
		i++;
	}

	Dictionary dict = fcurs->dict;
	if (IS_GENERATION(dict))
		add_category(dict, n, dn, i);

	dict->insert_entry(dict, dn, i);

	/* pass the ; */
	if (!link_advance(fcurs))
	{
		/* Avoid freeing dn, since it is already inserted into the dict. */
		return false;
	}

	return true;

syntax_error:
	free_insert_list(dn);
	return false;
}

static bool fread_dict(FileCursor fcurs)
{
	if (!link_advance(fcurs))
		return false;

	/* The last character of a dictionary is NUL.
	 * Note: At the end of reading a dictionary, dict->pin points to one
	 * character after the input. Referring its [-1] element is safe even if
	 * the dict file size is 0. */
	while ('\0' != fcurs->pin[-1])
	{
		if (!read_entry(fcurs))
			return false;
	}

	Dictionary dict = fcurs->dict;
	if (dict->category != NULL)
	{
		/* Create a category element which contains 0 words, to signify the
		 * end of the category array for the user API. The number of
		 * categories will not get changed because macros are considered an
		 * invalid word. */
		Exp dummy_exp;
		add_category(dict, &dummy_exp, NULL, 0);
		dict->category[dict->num_categories + 1].num_words = 0;
	}

	dict->root = dsw_tree_to_vine(dict->root);
	dict->root = dsw_vine_to_tree(dict->root, dict->num_entries);
	return true;
}

bool read_dictionary(Dictionary dict, const char * input)
{
	FileCursor fcurs = alloca(sizeof(struct FileCursor_s));

	dict->line_number = 1;
	fcurs->dict = dict;
	fcurs->input = input;
	fcurs->pin = fcurs->input;

	return fread_dict(fcurs);
}

/* ======================================================================= */
