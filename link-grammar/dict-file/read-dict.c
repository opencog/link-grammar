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

#include "api-structures.h"             // Sentence_s (add_empty_word)
#include "connectors.h"
#include "dict-common/dialect.h"
#include "dict-common/dict-affix.h"     // is_stem
#include "dict-common/dict-common.h"
#include "dict-common/dict-utils.h"     // patch_subscript
#include "dict-common/file-utils.h"
#include "dict-common/idiom.h"
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
  is it is the same as [[...]].  Any floating point number is allowed.

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

static bool link_advance(Dictionary dict);

static void dict_error2(Dictionary dict, const char * s, const char *s2)
{
#define ERRBUFLEN 1024
	char tokens[ERRBUFLEN], t[ERRBUFLEN];
	int pos = 1;
	int i;

	/* The link_advance used to print the error message can
	 * throw more errors while printing... */
	if (dict->recursive_error) return;
	dict->recursive_error = true;

	char token[MAX_TOKEN_LENGTH];
	strcpy(token, dict->token);
	bool save_is_special    = dict->is_special;
	const char * save_input = dict->input;
	const char * save_pin   = dict->pin;
	int save_already_got_it = dict->already_got_it;
	int save_line_number    = dict->line_number;

	tokens[0] = '\0';
	for (i=0; i<5 && dict->token[0] != '\0'; i++)
	{
		pos += snprintf(t, ERRBUFLEN, "\"%s\" ", dict->token);
		strncat(tokens, t, ERRBUFLEN-1-pos);
		if (!link_advance(dict)) break;
	}
	tokens[pos] = '\0';

	strcpy(dict->token, token);
	dict->is_special     = save_is_special;
	dict->input          = save_input;
	dict->pin            = save_pin;
	dict->already_got_it = save_already_got_it;
	dict->line_number    = save_line_number;

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
	dict->recursive_error = false;
}

static void dict_error(Dictionary dict, const char * s)
{
	dict_error2(dict, s, NULL);
}

static void warning(Dictionary dict, const char * s)
{
	prt_error("Warning: %s\n"
	        "\tline %d, current token = \"%s\"\n",
	        s, dict->line_number, dict->token);
}

/**
 * This gets the next UTF8 character from the input, eliminating comments.
 * If we're in quote mode, it does not consider the % character for
 * comments.   Note that the returned character is a wide character!
 */
#define MAXUTFLEN 7
typedef char utf8char[MAXUTFLEN];
static bool get_character(Dictionary dict, int quote_mode, utf8char uc)
{
	int i = 0;

	while (1)
	{
		char c = *(dict->pin++);

		/* Skip over all comments */
		if ((c == '%') && (!quote_mode))
		{
			while ((c != 0x0) && (c != '\n')) c = *(dict->pin++);
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
			c = *(dict->pin++);
			/* If we're onto the next char, we're done. */
			if (((c & 0x80) == 0x0) || ((c & 0xc0) == 0xc0))
			{
				dict->pin--;
				uc[i] = 0x0;
				return true;
			}
			uc[i] = c;
			i++;
		}
		dict_error(dict, "UTF8 char is too long.");
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
static bool link_advance(Dictionary dict)
{
	utf8char c;
	int nr, i;
	int quote_mode;

	dict->is_special = false;

	if (dict->already_got_it != '\0')
	{
		dict->is_special = char_is_special(dict->already_got_it);
		if (dict->already_got_it == EOF) {
			dict->token[0] = '\0';
		} else {
			dict->token[0] = (char)dict->already_got_it; /* specials are one byte */
			dict->token[1] = '\0';
		}
		dict->already_got_it = '\0';
		return true;
	}

	do
	{
		bool ok = get_character(dict, false, c);
		if (!ok) return false;
	}
	while (lg_isspace((unsigned char)c[0]));

	quote_mode = false;

	i = 0;
	for (;;)
	{
		if (i > MAX_TOKEN_LENGTH-3) {
			dict_error(dict, "Token too long.");
			return false;
		}
		if (quote_mode) {
			if (c[0] == '"' &&
			    /* Check the next character too, to allow " in words */
			    (*dict->pin == ':' || *dict->pin == ';' ||
			    lg_isspace((unsigned char)*dict->pin))) {

				dict->token[i] = '\0';
				return true;
			}
			if (lg_isspace((unsigned char)c[0])) {
				dict_error(dict, "White space inside of token.");
				return false;
			}
			if (c[0] == '\0')
			{
				dict_error(dict, "EOF while reading quoted token.");
				return false;
			}

			nr = 0;
			while (c[nr]) {dict->token[i] = c[nr]; i++; nr++; }
		} else {
			if ('\0' == c[1] && char_is_special(c[0]))
			{
				if (i == 0)
				{
					dict->token[0] = c[0];  /* special toks are one char always */
					dict->token[1] = '\0';
					dict->is_special = true;
					return true;
				}
				dict->token[i] = '\0';
				dict->already_got_it = c[0];
				return true;
			}
			if (c[0] == 0x0) {
				if (i != 0) dict->already_got_it = '\0';
				dict->token[0] = '\0';
				return true;
			}
			if (lg_isspace((unsigned char)c[0])) {
				dict->token[i] = '\0';
				return true;
			}
			if (c[0] == '\"') {
				quote_mode = true;
			} else {
				nr = 0;
				while (c[nr]) {dict->token[i] = c[nr]; i++; nr++; }
			}
		}
		bool ok = get_character(dict, quote_mode, c);
		if (!ok) return false;
	}
	/* unreachable */
}

/**
 * Returns true if this token is a special token and it is equal to c
 */
static int is_equal(Dictionary dict, char c)
{
	return (dict->is_special &&
	        c == dict->token[0] &&
	        dict->token[1] == '\0');
}

/**
 * Make sure the string s is a valid connector.
 * Return true if the connector is valid, else return false,
 * and print an appropriate warning message.
 */
static bool check_connector(Dictionary dict, const char * s)
{
	int i;
	i = strlen(s);
	if (i < 1) {
		dict_error(dict, "Expecting a connector.");
		return false;
	}
	i = s[i-1];  /* the last character of the token */
	if ((i != '+') && (i != '-') && (i != ANY_DIR)) {
		dict_error(dict, "A connector must end in a \"+\", \"-\" or \"$\".");
		return false;
	}
	if (*s == '@') s++;
	if (('h' == *s) || ('d' == *s)) s++;
	if (!is_connector_name_char(*s)) {
		dict_error2(dict, "Invalid character in connector "
		            "(connectors must start with an uppercase letter "
		            "after an optional \"h\" or \"d\"):", (char[]){*s, '\0'});
		return false;
	}
	if (*s == '_')
	{
		dict_error(dict, "Invalid character in connector "
		           "(an initial \"_\" is reserved for internal use).");
		return false;
	}

	/* The first uppercase has been validated above. */
	do { s++; } while (is_connector_name_char(*s));
	while (s[1]) {
		if (!is_connector_subscript_char(*s) && (*s != WILD_TYPE)) {
			dict_error2(dict, "Invalid character in connector subscript "
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
static Exp * make_connector(Dictionary dict)
{
	Exp * n;
	Dict_node *dn;
	int i;

	i = strlen(dict->token) - 1;  /* this must be +, - or $ if a connector */
	if ((dict->token[i] != '+') &&
	    (dict->token[i] != '-') &&
	    (dict->token[i] != ANY_DIR))
	{
		/* If we are here, token is a word */
		patch_subscript(dict->token);
		dn = strict_lookup_list(dict, dict->token);
		if (dn == NULL)
		{
			file_free_lookup(dn);
			dict_error2(dict, "Perhaps missing + or - in a connector.\n"
			                 "Or perhaps you forgot the subscript on a word.\n"
			                 "Or perhaps the word is used before it is defined:",
			                 dict->token);
			return NULL;
		}
		if (dn->right != NULL)
		{
			file_free_lookup(dn);
			dict_error2(dict, "Referencing a duplicate word:", dict->token);
			/* Note: A word which becomes duplicate latter evades this check. */
			return NULL;
		}

		/* Wrap it in a unary node as a placeholder for a macro tag and cost. */
		n = make_unary_node(dict->Exp_pool, dn->exp);
		n->tag_id = exptag_macro_add(dict, dn->string);
		if (n->tag_id != 0) n->tag_type = Exptag_macro;

		file_free_lookup(dn);
	}
	else
	{
		/* If we are here, token is a connector */
		if (!check_connector(dict, dict->token))
		{
			return NULL;
		}
		if ((dict->token[i] == '+') || (dict->token[i] == '-'))
		{
			/* A simple, unidirectional connector. Just make that. */
			n = make_dir_connector(dict, i);
			if (NULL == n) return NULL;
		}
		else if (dict->token[i] == ANY_DIR)
		{
			Exp *plu, *min;
			/* If we are here, then it's a bi-directional connector.
			 * Make both a + and a - version, and or them together.  */
			dict->token[i] = '+';
			plu = make_dir_connector(dict, i);
			if (NULL == plu) return NULL;
			dict->token[i] = '-';
			min = make_dir_connector(dict, i);
			if (NULL == min) return NULL;

			n = make_or_node(dict->Exp_pool, plu, min);
		}
		else
		{
			dict_error(dict, "Unknown connector direction type.");
			return NULL;
		}
	}

	if (!link_advance(dict))
	{
		free(n);
		return NULL;
	}
	return n;
}

/* ======================================================================== */
/* Empty-word handling. */

/** Insert ZZZ+ connectors.
 *  This function was mainly used to support using empty-words, a concept
 *  that has been eliminated. However, it is still used to support linking of
 *  quotes that don't get the QUc/QUd links.
 */
void add_empty_word(Sentence sent, X_node *x)
{
	Exp *zn, *an;
	const char *ZZZ = string_set_lookup(EMPTY_CONNECTOR, sent->dict->string_set);
	/* This function is called only if ZZZ is in the dictionary. */

	/* The left-wall already has ZZZ-. The right-wall will not arrive here. */
	if (MT_WALL == x->word->morpheme_type) return;

	/* Replace plain-word-exp by {ZZZ+} & (plain-word-exp) in each X_node.  */
	for(; NULL != x; x = x->next)
	{
		/* Ignore stems for now, decreases a little the overhead for
		 * stem-suffix languages. */
		if (is_stem(x->string)) continue; /* Avoid an unneeded overhead. */
		//lgdebug(+0, "Processing '%s'\n", x->string);

		/* zn points at {ZZZ+} */
		zn = Exp_create(sent->Exp_pool);
		zn->dir = '+';
		zn->condesc = condesc_add(&sent->dict->contable, ZZZ);
		zn->multi = false;
		zn->type = CONNECTOR_type;
		zn->operand_next = NULL; /* unused, but to be on the safe side */
		zn->cost = 0.0;
		zn = make_optional_node(sent->Exp_pool, zn);

		/* an will be {ZZZ+} & (plain-word-exp) */
		an = Exp_create(sent->Exp_pool);
		an->type = AND_type;
		an->operand_next = NULL;
		an->cost = 0.0;
		an->operand_first = zn;
		zn->operand_next = x->exp;

		x->exp = an;
	}
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
static Exp *make_expression(Dictionary dict)
{
	Exp *nl = NULL;
	Exp *e_head = NULL;
	Exp *e_tail = NULL; /* last part of the expression */
	bool is_sym_and = false;

	while (true)
	{
		if (is_equal(dict, '('))
		{
			if (!link_advance(dict)) {
				return NULL;
			}
			nl = make_expression(dict);
			if (nl == NULL) {
				return NULL;
			}
			if (!is_equal(dict, ')')) {
				dict_error(dict, "Expecting a \")\".");
				return NULL;
			}
			if (!link_advance(dict)) {
				return NULL;
			}
		}
		else if (is_equal(dict, '{'))
		{
			if (!link_advance(dict)) {
				return NULL;
			}
			nl = make_expression(dict);
			if (nl == NULL) {
				return NULL;
			}
			if (!is_equal(dict, '}')) {
				dict_error(dict, "Expecting a \"}\".");
				return NULL;
			}
			if (!link_advance(dict)) {
				return NULL;
			}
			nl = make_optional_node(dict->Exp_pool, nl);
		}
		else if (is_equal(dict, '['))
		{
			if (!link_advance(dict)) {
				return NULL;
			}
			nl = make_expression(dict);
			if (nl == NULL) {
				return NULL;
			}
			if (!is_equal(dict, ']')) {
				dict_error(dict, "Expecting a \"]\".");
				return NULL;
			}
			if (!link_advance(dict)) {
				return NULL;
			}

			/* A square bracket can have a number or a name after it.
			 * If a number is present, then that it is interpreted
			 * as the cost of the bracket. If a name is present, it
			 * is used as an expression tag. Else, the cost of a
			 * square bracket is 1.0.
			 */
			if (is_number(dict->token))
			{
				float cost;

				if (strtodC(dict->token, &cost))
				{
					nl->cost += cost;
				}
				else
				{
					warning(dict, "Invalid cost (using 1.0)\n");
					nl->cost += 1.0;
				}
				if (!link_advance(dict)) {
					return NULL;
				}
			}
			else if ((strcmp(dict->token, "or") != 0) &&
			         (strcmp(dict->token, "and") != 0) &&
			         isalpha((unsigned char)dict->token[0]))
			{
				const char *bad = valid_dialect_name(dict->token);
				if (bad != NULL)
				{
					char badchar[] = { *bad, '\0' };
					dict_error2(dict, "Invalid character in dialect tag name:",
					           badchar);
					return NULL;
				}
				if (nl->tag_type != Exptag_none)
				{
					nl = make_unary_node(dict->Exp_pool, nl);
				}
				nl->tag_id = exptag_dialect_add(dict, dict->token);
				nl->tag_type = Exptag_dialect;
				if (!link_advance(dict)) {
					return NULL;
				}
			}
			else
			{
				nl->cost += 1.0;
			}
		}
		else if (!dict->is_special)
		{
			nl = make_connector(dict);
			if (nl == NULL) {
				return NULL;
			}
		}
		else if (is_equal(dict, ')') || is_equal(dict, ']'))
		{
			/* allows "()" or "[]" */
			nl = make_zeroary_node(dict->Exp_pool);
		}
		else
		{
			dict_error(dict, "Connector, \"(\", \"[\", or \"{\" expected.");
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
		if (is_equal(dict, '&') || (strcmp(dict->token, "and") == 0))
		{
			op = AND_type;
		}
		/* Commuting OR */
		else if (is_equal(dict, '|') || (strcmp(dict->token, "or") == 0))
		{
			op =  OR_type;
		}
		/* Commuting AND */
		else if (is_equal(dict, SYM_AND) || (strcmp(dict->token, "sym") == 0))
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
			e_head = make_op_Exp(dict->Exp_pool, op);
			e_head->operand_first = nl;
		}
		else
		{
			if (e_head->type != op)
			{
				dict_error(dict, "\"and\" and \"or\" at the same level in an expression.");
				return NULL;
			}
		}

		if (!link_advance(dict)) {
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
	else
	if (0 == strncmp(LIMITED_CONNECTORS_WORD, dn->string,
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

/**
 * read_entry() -- read one dictionary entry
 * Starting with the current token, parse one dictionary entry.
 * A single dictionary entry must have one and only one colon in it,
 * and is terminated by a semi-colon.
 * Add these words to the dictionary.
 */
static bool read_entry(Dictionary dict)
{
	Exp *n;
	int i;

	Dict_node *dnx, *dn = NULL;

	while (!is_equal(dict, ':'))
	{
		if (dict->is_special)
		{
			dict_error(dict, "I expected a word but didn\'t get it.");
			goto syntax_error;
		}

		/* If it's a word-file name */
		/* However, be careful to reject "/.v" which is the division symbol
		 * used in equations (.v means verb-like) */
		if ((dict->token[0] == '/') && (dict->token[1] != '.'))
		{
			dn = read_word_file(dict, dn, dict->token);
			if (dn == NULL)
			{
				prt_error("Error: Cannot open word file \"%s\".\n", dict->token);
				return false;
			}
		}
		else if (0 == strcmp(dict->token, "#include"))
		{
			bool rc;
			char* instr;
			char* dict_name;
			const char * save_name;
			bool save_is_special;
			const char * save_input;
			const char * save_pin;
			int save_already_got_it;
			int save_line_number;
			size_t skip_slash;

			if (!link_advance(dict)) goto syntax_error;

			skip_slash          = ('/' == dict->token[0]) ? 1 : 0;
			dict_name           = strdupa(dict->token);
			save_name           = dict->name;
			save_is_special     = dict->is_special;
			save_input          = dict->input;
			save_pin            = dict->pin;
			save_already_got_it = dict->already_got_it;
			save_line_number    = dict->line_number;

			/* OK, token contains the filename to read ... */
			instr = get_file_contents(dict_name + skip_slash);
			if (NULL == instr)
			{
				prt_error("Error: While parsing dictionary \"%s\":\n"
				          "\t Line %d: Could not open subdictionary \"%s\"\n",
				          dict->name, dict->line_number-1, dict_name);
				goto syntax_error;
			}
			dict->input = instr;
			dict->pin = dict->input;

			/* The line number and dict name are used for error reporting */
			dict->line_number = 1;
			dict->name = dict_name;

			/* Now read the thing in. */
			rc = read_dictionary(dict);

			dict->name           = save_name;
			dict->is_special     = save_is_special;
			dict->input          = save_input;
			dict->pin            = save_pin;
			dict->already_got_it = save_already_got_it;
			dict->line_number    = save_line_number;

			free(instr);
			if (!rc) goto syntax_error;

			/* when we return, point to the next entry */
			if (!link_advance(dict)) goto syntax_error;

			/* If a semicolon follows the include, that's OK... ignore it. */
			if (';' == dict->token[0])
			{
				if (!link_advance(dict)) goto syntax_error;
			}

			return true;
		}
		else if (0 == strcmp(dict->token, "#define"))
		{
			if (!link_advance(dict)) goto syntax_error;
			const char *name = strdupa(dict->token);

			/* Get the value. */
			if (!link_advance(dict)) goto syntax_error;
			add_define(dict, name, dict->token);

			if (!link_advance(dict)) goto syntax_error;
			if (!is_equal(dict, ';'))
			{
				dict_error(dict, "Expecting \";\" at the end of #define.");
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
			patch_subscript(dict->token);
			dn->string = string_set_add(dict->token, dict->string_set);
		}

		/* Advance to next entry, unless error */
		if (!link_advance(dict)) goto syntax_error;
	}

	/* pass the : */
	if (!link_advance(dict))
	{
		goto syntax_error;
	}

	n = make_expression(dict);
	if (n == NULL)
	{
		goto syntax_error;
	}

	if (!is_equal(dict, ';'))
	{
		dict_error(dict, "Expecting \";\" at the end of an entry.");
		goto syntax_error;
	}

	if (dn == NULL)
	{
		dict_error(dict, "Expecting a token before \":\".");
		goto syntax_error;
	}

	/* At this point, dn points to a list of Dict_nodes connected by
	 * their left pointers. These are to be inserted into the dictionary. */
	i = 0;
	for (dnx = dn; dnx != NULL; dnx = dnx->left)
	{
		dnx->exp = n;
		i++;
	}
	if (IS_GENERATION(dict))
		add_category(dict, n, dn, i);

	dict->insert_entry(dict, dn, i);

	/* pass the ; */
	if (!link_advance(dict))
	{
		/* Avoid freeing dn, since it is already inserted into the dict. */
		return false;
	}

	return true;

syntax_error:
	free_insert_list(dn);
	return false;
}

bool read_dictionary(Dictionary dict)
{
	if (!link_advance(dict))
	{
		return false;
	}
	/* The last character of a dictionary is NUL.
	 * Note: At the end of reading a dictionary, dict->pin points to one
	 * character after the input. Referring its [-1] element is safe even if
	 * the dict file size is 0. */
	while ('\0' != dict->pin[-1])
	{
		if (!read_entry(dict))
		{
			return false;
		}
	}

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

/* ======================================================================= */
