/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <limits.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include "api.h"
#include "disjunct-utils.h"
#include "error.h"

const char * linkgrammar_get_version(void)
{
	const char *s = "link-grammar-" LINK_VERSION_STRING;
	return s;
}

const char * linkgrammar_get_dict_version(Dictionary dict)
{
	static char * ver = NULL;
	char * p;
	Dict_node *dn;
	Exp *e;

	if (ver) return ver;

	/* The newer dictionaries should contain a macro of the form:
	 * <dictionary-version-number>: V4v6v6+;
	 * which would indicate dictionary verison 4.6.6
	 * Older dictionaries contain no version info.
	 */
	dn = dictionary_lookup_list(dict, "<dictionary-version-number>");
	if (NULL == dn) return "[unknown]";

	e = dn->exp;
	ver = strdup(&e->u.string[1]);
	p = strchr(ver, 'v');
	while (p)
	{
		*p = '.';
		p = strchr(p+1, 'v');
	}

	free_lookup_list(dn);
	return ver;
}


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
  Expressions may be written in prefix or infix form. In prefix-form,
  the expressions are lisp-like, with the operators &, | preceeding
  the operands. In infix-form, the operators are in the middle. The
  current dictionaries are in infix form.  If the C preprocessor
  constant INFIX_NOTATION is defined, then the dictionary is assumed
  to be in infix form.

  The connector begins with an optinal @, which is followed by an upper
  case sequence of letters. Each subsequent *, lower case letter or
  number is a subscript. At the end is a + or - sign.  The "@" allows
  this connector to attach to one or more other connectors.

  Here is a sample dictionary entry (in infix form):

      gone:         T- & {@EV+};

  (See our paper for more about how to interpret the meaning of the
  dictionary expressions.)

  A previously defined word (such as "gone" above) may be used instead
  of a connector to specify the expression it was defined to be.  Of
  course, in this case, it must uniquely specify a word in the
  dictionary, and have been previously defined.

  If a word is of the form "/foo", then the file current-dir/foo
  is a so-called word file, and is read in as a list of words.
  A word file is just a list of words separted by blanks or newlines.

  A word that contains the character "_" defines an idiomatic use of
  the words separated by the "_".  For example "kind of" is an idiomatic
  expression, so a word "kind_of" is defined in the dictionary.
  Idomatic expressions of any number of words can be defined in this way.
  When the word "kind" is encountered, all the idiomatic uses of the word
  are considered.

  An expresion enclosed in "[..]" is give a cost of 1.  This means
  that if any of the connectors inside the square braces are used,
  a cost of 1 is incurred.  (This cost is the first element of the cost
  vector printed when a sentence is parsed.)  Of course if something is
  inside of 10 levels of "[..]" then using it incurs a cost of 10.
  These costs are called "disjunct costs".  The linkages are printed out
  in order of non-increasing disjunct cost.

  The expression "(A+ or ())" means that you can choose either "A+" or
  the empty expression "()", that is, that the connector "A+" is
  optional.  This is more compactly expressed as "{A+}".  In other words,
  curly braces indicate an optional expression.

  The expression "(A+ or [])" is the same as that above, but there is a
  cost of 1 incurred for choosing not to use "A+".  The expression
  "(EXP1 & [EXP2])" is exactly the same as "[EXP1 & EXP2]".  The difference
  between "({[A+]} & B+)" and "([{A+}] & B+)" is that the latter always
  incurrs a cost of 1, while the former only gets a cost of 1 if "A+" is
  used.

  The dictionary writer is not allowed to use connectors that begin in
  "ID".  This is reserved for the connectors automatically
  generated for idioms.

  Dictionary words may be followed by a dot (period, "."), and a "subscript"
  identifying the word type. The subscript may be one or more letters or 
  numbers, but must begin with a letter. Currently, the dictionary contains
  (mostly?) subscripts consisting of a single letter, and these serve mostly
  to identify the part-of-speech. In general, subscripts can also be used
  to distinguish different word senses.
*/

static int link_advance(Dictionary dict);

static void dict_error2(Dictionary dict, const char * s, const char *s2)
{
	int i;
	char tokens[1024], t[128];

	if (dict->recursive_error) return;
	dict->recursive_error = TRUE;

	tokens[0] = '\0';
	for (i=0; i<5 && dict->token[0] != '\0' ; i++)
	{
		sprintf(t, "\"%s\" ", dict->token);
		strcat(tokens, t);
		link_advance(dict);
	}
	if (s2)
	{
		err_ctxt ec;
		ec.sent = NULL;
		err_msg(&ec, Error, "Error parsing dictionary %s.\n"
		          "%s %s\n\t line %d, tokens = %s\n",
		        dict->name,
		        s, s2, dict->line_number, tokens);
	}
	else
	{
		err_ctxt ec;
		ec.sent = NULL;
		err_msg(&ec, Error, "Error parsing dictionary %s.\n"
		          "%s\n\t line %d, tokens = %s\n",
		        dict->name,
		        s, dict->line_number, tokens);
	}
	dict->recursive_error = FALSE;
}

static void dict_error(Dictionary dict, const char * s)
{
	dict_error2(dict, s, NULL);
}

static void warning(Dictionary dict, const char * s)
{
	err_ctxt ec;
	ec.sent = NULL;
	err_msg(&ec, Warn, "Warning: %s\n"
	        "\tline %d, current token = \"%s\"\n",
	        s, dict->line_number, dict->token);
}

/**
 * This gets the next character from the input, eliminating comments.
 * If we're in quote mode, it does not consider the % character for
 * comments.
 */
static wchar_t get_character(Dictionary dict, int quote_mode)
{
	wchar_t c;

	c = *(dict->pin++);

	if ((c == '%') && (!quote_mode))
	{
		while ((c != 0x0) && (c != '\n')) c = *(dict->pin++);
	}
	if (c == '\n') dict->line_number++;
	return c;
}


/*
 * This set of 10 characters are the ones defining the syntax of the
 * dictionary.
 */
#define SPECIAL "(){};[]&|:"

/**
 * Return true if the input wide-character is one of the special
 * characters used to define the syntax of the dictionary.
 */
static int is_special(wchar_t wc, mbstate_t *ps)
{
	char buff[MB_LEN_MAX];
	int nr = wcrtomb(buff, wc, ps);
	if (1 != nr) return FALSE;
	return (NULL != strchr(SPECIAL, buff[0]));
}

/**
 * This reads the next token from the input into token.
 * Return 1 if a character was read, else return 0 (and print a warning).
 */
static int link_advance(Dictionary dict)
{
	wchar_t c;
	int nr, i;
	int quote_mode;

	dict->is_special = FALSE;

	if (dict->already_got_it != '\0')
	{
		dict->is_special = is_special(dict->already_got_it, &dict->mbss);
		if (dict->already_got_it == WEOF) {
			dict->token[0] = '\0';
		} else {
			dict->token[0] = dict->already_got_it; /* specials are one byte */
			dict->token[1] = '\0';
		}
		dict->already_got_it = '\0';
		return 1;
	}

	do { c = get_character(dict, FALSE); } while (iswspace(c));

	quote_mode = FALSE;

	i = 0;
	for (;;)
	{
		if (i > MAX_TOKEN_LENGTH-3) {  /* 3 for multi-byte tokens */
			dict_error(dict, "Token too long");
			return 0;
		}
		if (quote_mode) {
			if (c == '\"') {
				quote_mode = FALSE;
				dict->token[i] = '\0';
				return 1;
			}
			if (iswspace(c)) {
				dict_error(dict, "White space inside of token");
				return 0;
			}

			/* Although we read wide chars, we store UTF8 internally, always. */
			nr = wcrtomb(&dict->token[i], c, &dict->mbss);
			if (nr < 0) {
#ifndef _WIN32
				dict_error2(dict, "Unable to read UTF8 string in current locale",
				         nl_langinfo(CODESET));
				fprintf (stderr, "\tTry setting the locale with \"export LANG=en_US.UTF-8\"\n");
#else
				dict_error(dict, "Unable to read UTF8 string in current locale");
#endif
				return 0;
			}
			i += nr;
		} else {
			if (is_special(c, &dict->mbss))
			{
				if (i == 0)
				{
					dict->token[0] = c;  /* special toks are one char always */
					dict->token[1] = '\0';
					dict->is_special = TRUE;
					return 1;
				}
				dict->token[i] = '\0';
				dict->already_got_it = c;
				return 1;
			}
			if (c == 0x0) {
				if (i == 0) {
					dict->token[0] = '\0';
					return 1;
				}
				dict->token[i] = '\0';
				dict->already_got_it = c;
				return 1;
			}
			if (iswspace(c)) {
				dict->token[i] = '\0';
				return 1;
			}
			if (c == '\"') {
				quote_mode = TRUE;
			} else {
				/* store UTF8 internally, always. */
				nr = wctomb_check(&dict->token[i], c, &dict->mbss);
				if (nr < 0) {
#ifndef _WIN32
					dict_error2(dict, "Unable to read UTF8 string in current locale",
					         nl_langinfo(CODESET));
					fprintf (stderr, "\tTry setting the locale with \"export LANG=en_US.UTF-8\"\n");
#else
					dict_error(dict, "Unable to read UTF8 string in current locale");
#endif
					return 0;
				}
				i += nr;
			}
		}
		c = get_character(dict, quote_mode);
	}
	return 1;
}

/**
 * Returns TRUE if this token is a special token and it is equal to c
 */
static int is_equal(Dictionary dict, wint_t c)
{
	return (dict->is_special &&
	        wctob(c) == dict->token[0] &&
	        dict->token[1] == '\0');
}

/** 
 * Make sure the string s is a valid connector.
 * Return 1 if the connector is valid, else return 0,
 * and print an appropriate warning message.
 */
static int check_connector(Dictionary dict, const char * s)
{
	int i;
	i = strlen(s);
	if (i < 1) {
		dict_error(dict, "Expecting a connector.");
		return 0;
	}
	i = s[i-1];  /* the last character of the token */
	if ((i!='+') && (i!='-')) {
		dict_error(dict, "A connector must end in a \"+\" or \"-\".");
		return 0;
	}
	if (*s == '@') s++;
	if (!isupper((int)*s)) {
		dict_error(dict, "The first letter of a connector must be in [A--Z].");
		return 0;
	}
	if ((*s == 'I') && (*(s+1) == 'D')) {
		dict_error(dict, "Connectors beginning with \"ID\" are forbidden");
		return 0;
	}
	while (*(s+1)) {
		if ((!isalnum((int)*s)) && (*s != '*') && (*s != '^')) {
			dict_error(dict, "All letters of a connector must be ASCII alpha-numeric.");
			return 0;
		}
		s++;
	}
	return 1;
}

/* ======================================================================== */
/**
 * Dictionary entry comparison and ordering functions.
 *
 * The data structure storing the dictionary is simply a binary tree.
 * The entries in the binary tree are sorted by alphabetical order.
 * There is one catch, however: words may have suffixes (a dot, followed
 * by the suffix), and these suffixes are to be handled appripriately
 * during sorting and comparison.
 *
 * The use of suffixes means that the ordering of the words is not
 * exactly the order given by strcmp.  The order must be such that, for
 * example, "make" < "make.n" < "make-up" -- suffixed words come after
 * the bare words, but before any other other words with non-ascii-alpha
 * characters (such as the hyphen in "make-up", or possibly UTF8
 * characters). Thus, stright "strcmp" can't be used to determine 
 * dictionary order. 
 *
 * Thus, a set of specialized string comparison and ordering functions
 * are provided. These "do the right thing" when matching string with
 * and without suffixes.
 */
/** 
 * dict_order - order two dictionary words in proper sort order.
 * Return zero if the strings match, else return standard 
 * (locale-dependent) UTF8 sort order.
 * XXX but its not UTF-8 ordered ...! Does this matter ???
 */
/* verbose version */
/*
int dict_order(char *s, char *t)
{
	int ss, tt;
	while (*s != '\0' && *s == *t) {
		s++;
		t++;
	}
	if (*s == '.') {
		ss = 1;
	} else {
		ss = (*s)<<1;
	}
	if (*t == '.') {
		tt = 1;
	} else {
		tt = (*t)<<1;
	}
	return (ss - tt);
}
*/

/* terse version */
static inline int dict_order(const char *s, const char *t)
{
	while (*s != '\0' && *s == *t) {s++; t++;}
	return ((*s == '.')?(1):(*s))  -  ((*t == '.')?(1):(*t));
}

/**
 * dict_order_wild() -- order dictionary strings, with wildcard.
 * Assuming that s is a pointer to a dictionary string, and that
 * t is a pointer to a search string, this returns 0 if they
 * match, >0 if s>t, and <0 if s<t.
 *
 * The matching is done as follows.  Walk down the strings until
 * you come to the end of one of them, or until you find unequal
 * characters.  A "*" matches anything.  Otherwise, replace "."
 * by "\0", and take the difference.  This behavior matches that
 * of the function dict_order().
 *
 * Note: wild-cards are not used in the English dictionaries;
 * nor are they used in the Persian or Arabic dictionaries...
 * perhaps they are used to handle stemming in some other 
 * languages ?????? XXX FIXME, these wildcards might just be
 * old dead code ... 
 */
static inline int dict_order_wild(const char * s, const char * t)
{
	const char *so = s;
	while((*s != '\0') && (*s == *t)) {s++; t++;}

	if ((*s == '*') || (*t == '*'))
	{
		/* Do not allow wild-card as the very first char.
		 * Basically, allow *.v and *.eq in the Enlgish dict.
		 */
		if (s != so) return 0;
	}

	return (((*s == '.')?(0):(*s)) - ((*t == '.')?(0):(*t)));
}

/**
 * dict_match --  return true if strings match, else false.
 * A "bare" string (one without a suffix) will match any corresponding 
 * string with a suffix; so, for example, "make" and "make.n" are 
 * a match.  If both strings have suffixes, then the suffixes must match.
 *
 * A subscript is the part that followes the last "." in the word, and 
 * that does not begin with a digit.
 */
static int dict_match(const char * s, const char * t)
{
	char *ds, *dt;
	ds = strrchr(s, '.');
	dt = strrchr(t, '.');

	/* a dot at the end or a dot followed by a number is NOT
	 * considered a subscript */
	if ((dt != NULL) && ((*(dt+1) == '\0') ||
	    (isdigit((int)*(dt+1))))) dt = NULL;
	if ((ds != NULL) && ((*(ds+1) == '\0') ||
	    (isdigit((int)*(ds+1))))) ds = NULL;

	/* dt is NULL when there's no prefix ... */
	if (dt == NULL && ds != NULL) {
		if (((int)strlen(t)) > (ds-s)) return FALSE;   /* we need to do this to ensure that */
		return (strncmp(s, t, ds-s) == 0);	           /* "i.e." does not match "i.e" */
	} else if (dt != NULL && ds == NULL) {
		if (((int)strlen(s)) > (dt-t)) return FALSE;
		return (strncmp(s, t, dt-t) == 0);
	} else {
		return (strcmp(s, t) == 0);
	}
}

/* ======================================================================== */

static inline Dict_node * dict_node_new(void)
{
	return (Dict_node*) xalloc(sizeof(Dict_node));
}

static inline void free_dict_node(Dict_node *dn)
{
	xfree((char *)dn, sizeof(Dict_node));
}

/**
 * prune_lookup_list -- discard all list entries that don't match string
 * Walk the lookup list (of right links), discarding all nodes that do
 * not match the dictionary string s. The matching is dictionary matching:
 * suffixed entries will match "bare" entries.
 */
static Dict_node * prune_lookup_list(Dict_node *llist, const char * s)
{
	Dict_node *dn, *dnx, *list_new;

	list_new = NULL;
	for (dn = llist; dn != NULL; dn = dnx)
	{
		dnx = dn->right;
		/* now put dn onto the answer list, or free it */
		if (dict_match(dn->string, s))
		{
			dn->right = list_new;
			list_new = dn;
		}
		else
		{
			free_dict_node(dn);
		}
	}

	/* now reverse the list back */
	llist = NULL;
	for (dn = list_new; dn != NULL; dn = dnx)
	{
		dnx = dn->right;
		dn->right = llist;
		llist = dn;
	}
	return llist;
}

void free_lookup_list(Dict_node *llist)
{
	Dict_node * n;
	while(llist != NULL)
	{
		n = llist->right;
		free_dict_node(llist);
		llist = n;
	}
}

static void free_dict_node_recursive(Dict_node * dn)
{
	if (dn == NULL) return;
	free_dict_node_recursive(dn->left);
	free_dict_node_recursive(dn->right);
	free_dict_node(dn);
}

/* ======================================================================== */
/**
 * rdictionary_lookup() -- recursive dictionary lookup
 * Walk binary tree, given by 'dn', looking for the string 's'.
 * For every node in the tree where 's' matches (including wildcards)
 * make a copy of that node, and append it to llist.
 */
static Dict_node * rdictionary_lookup(Dict_node *llist,
                                      Dict_node * dn, const char * s, int match_idiom)
{
	/* see comment in dictionary_lookup below */
	int m;
	Dict_node * dn_new;
	if (dn == NULL) return llist;
	m = dict_order_wild(s, dn->string);
	if (m >= 0)
	{
		llist = rdictionary_lookup(llist, dn->right, s, match_idiom);
	}
	if ((m == 0) && (match_idiom || !is_idiom_word(dn->string)))
	{
		dn_new = dict_node_new();
		*dn_new = *dn;
		dn_new->right = llist;
		llist = dn_new;
	}
	if (m <= 0)
	{
		llist = rdictionary_lookup(llist, dn->left, s, match_idiom);
	}
	return llist;
}

/**
 * dictionary_lookup_list() - return lookup list of words in the dictionary
 *
 * Returns a pointer to a lookup list of the words in the dictionary.
 * Matches include word that appear in idioms.  Use
 * abridged_lookup_list() to obtain matches, excluding idioms.
 *
 * This list is made up of Dict_nodes, linked by their right pointers.
 * The node, file and string fields are copied from the dictionary.
 *
 * The returned list must be freed with free_lookup_list().
 */
Dict_node * dictionary_lookup_list(Dictionary dict, const char *s)
{
	Dict_node * llist = rdictionary_lookup(NULL, dict->root, s, TRUE);
	llist = prune_lookup_list(llist, s);
	return llist;
}

/**
 * abridged_lookup_list() - return lookup list of words in the dictionary
 *
 * Returns a pointer to a lookup list of the words in the dictionary.
 * Excludes any idioms that contain the word; use
 * dictionary_lookup_list() to obtain the complete list.
 *
 * This list is made up of Dict_nodes, linked by their right pointers.
 * The node, file and string fields are copied from the dictionary.
 *
 * The returned list must be freed with free_lookup_list().
 */
Dict_node * abridged_lookup_list(Dictionary dict, const char *s)
{
	Dict_node *llist;
	llist = rdictionary_lookup(NULL, dict->root, s, FALSE);
	llist = prune_lookup_list(llist, s);
	return llist;
}

int boolean_dictionary_lookup(Dictionary dict, const char *s)
{
	Dict_node *llist = dictionary_lookup_list(dict, s);
	int boool = (llist != NULL);
	free_lookup_list(llist);
	return boool;
}

/* ======================================================================== */
/**
 * Allocate a new Exp node and link it into the exp_list for freeing later.
 */
Exp * Exp_create(Dictionary dict)
{
	Exp * e;
	e = (Exp *) xalloc(sizeof(Exp));
	e->next = dict->exp_list;
	dict->exp_list = e;
	return e;
}

static inline void exp_free(Exp * e)
{
	xfree((char *)e, sizeof(Exp));
}

/* ======================================================================== */
/**
 * This creates a node with one child (namely e).  Initializes
 * the cost to zero.
 */
static Exp * make_unary_node(Dictionary dict, Exp * e)
{
	Exp * n;
	n = Exp_create(dict);
	n->type = AND_type;  /* these must be AND types */
	n->cost = 0.0f;
	n->u.l = (E_list *) xalloc(sizeof(E_list));
	n->u.l->next = NULL;
	n->u.l->e = e;
	return n;
}

/**
 * connector() -- make a node for a connector or dictionary word.
 *
 * Assumes the current token is a connector or dictionary word.
 */
static Exp * connector(Dictionary dict)
{
	Exp * n;
	Dict_node *dn, *dn_head;
	int i;

	i = strlen(dict->token) - 1;  /* this must be + or - if a connector */
	if ((dict->token[i] != '+') && (dict->token[i] != '-'))
	{
		/* If we are here, token is a word */
		dn_head = abridged_lookup_list(dict, dict->token);
		dn = dn_head;
		while ((dn != NULL) && (strcmp(dn->string, dict->token) != 0))
		{
			dn = dn->right;
		}
		if (dn == NULL)
		{
			free_lookup_list(dn_head);
			dict_error(dict, "\nPerhaps missing + or - in a connector.\n"
			                 "Or perhaps you forgot the suffix on a word.\n"
			                 "Or perhaps a word is used before it is defined.\n");
			return NULL;
		}
		n = make_unary_node(dict, dn->exp);
		free_lookup_list(dn_head);
	} 
	else
	{
		/* If we are here, token is a connector */
		if (!check_connector(dict, dict->token))
		{
			return NULL;
		}
		n = Exp_create(dict);
		n->dir = dict->token[i];
		dict->token[i] = '\0';				   /* get rid of the + or - */
		if (dict->token[0] == '@')
		{
			n->u.string = string_set_add(dict->token+1, dict->string_set);
			n->multi = TRUE;
		}
		else
		{
			n->u.string = string_set_add(dict->token, dict->string_set);
			n->multi = FALSE;
		}
		n->type = CONNECTOR_type;
		n->cost = 0.0f;
	}

	if (!link_advance(dict))
	{
		exp_free(n);
		return NULL;
	}
	return n;
}

/**
 * This creates a node with zero children.  Initializes
 * the cost to zero.
 */
static Exp * make_zeroary_node(Dictionary dict)
{
	Exp * n;
	n = Exp_create(dict);
	n->type = AND_type;  /* these must be AND types */
	n->cost = 0.0f;
	n->u.l = NULL;
	return n;
}

/**
 * This creates an OR node with two children, one the given node,
 * and the other as zeroary node.  This has the effect of creating
 * what used to be called an optional node.
 */
static Exp * make_optional_node(Dictionary dict, Exp * e)
{
	Exp * n;
	E_list *el, *elx;
	n = Exp_create(dict);
	n->type = OR_type;
	n->cost = 0.0f;
	n->u.l = el = (E_list *) xalloc(sizeof(E_list));
	el->e = make_zeroary_node(dict);
	el->next = elx = (E_list *) xalloc(sizeof(E_list));
	elx->next = NULL;
	elx->e = e;
	return n;
}

/* ======================================================================== */

#if ! defined INFIX_NOTATION

Exp * expression(Dictionary dict);
/**
 * We're looking at the first of the stuff after an "and" or "or".
 * Build a Exp node for this expression.  Set the cost and optional
 * fields to the default values.  Set the type field according to type
 */
Exp * operator_exp(Dictionary dict, int type)
{
	Exp * n;
	E_list first;
	E_list * elist;
	n = Exp_create(dict);
	n->type = type;
	n->cost = 0.0f;
	elist = &first;
	while((!is_equal(dict, ')')) && (!is_equal(dict, ']')) && (!is_equal(dict, '}'))) {
		elist->next = (E_list *) xalloc(sizeof(E_list));
		elist = elist->next;
		elist->next = NULL;
		elist->e = expression(dict);
		if (elist->e == NULL) {
			return NULL;
		}
	}
	if (elist == &first) {
		dict_error(dict, "An \"or\" or \"and\" of nothing");
		return NULL;
	}
	n->u.l = first.next;
	return n;
}

/**
 * Looks for the stuff that is allowed to be inside of parentheses
 * either & or | followed by a list, or a terminal symbol.
 */
Exp * in_parens(Dictionary dict)
{
	Exp * e;

	if (is_equal(dict, '&') || (strcmp(token, "and")==0)) {
		if (!link_advance(dict)) {
			return NULL;
		}
		return operator_exp(dict, AND_type);
	} else if (is_equal(dict, '|') || (strcmp(dict->token, "or")==0)) {
		if (!link_advance(dict)) {
			return NULL;
		}
		return operator_exp(dict, OR_type);
	} else {
		return expression(dict);
	}
}

/**
 * Build (and return the root of) the tree for the expression beginning
 * with the current token.  At the end, the token is the first one not
 * part of this expression.
 */
Exp * expression(Dictionary dict)
{
	Exp * n;
	if (is_equal(dict, '(')) {
		if (!link_advance(dict)) {
			return NULL;
		}
		n = in_parens(dict);
		if (!is_equal(dict, ')')) {
			dict_error(dict, "Expecting a \")\".");
			return NULL;
		}
		if (!link_advance(dict)) {
			return NULL;
		}
	} else if (is_equal(dict, '{')) {
		if (!link_advance(dict)) {
			return NULL;
		}
		n = in_parens(dict);
		if (!is_equal(dict, '}')) {
			dict_error(dict, "Expecting a \"}\".");
			return NULL;
		}
		if (!link_advance(dict)) {
			return NULL;
		}
		n = make_optional_node(dict, n);
	} else if (is_equal(dict, '[')) {
		if (!link_advance(dict)) {
			return NULL;
		}
		n = in_parens(dict);
		if (!is_equal(dict, ']')) {
			dict_error(dict, "Expecting a \"]\".");
			return NULL;
		}
		if (!link_advance(dict)) {
			return NULL;
		}
		n->cost += 1.0f;
	} else if (!dict->is_special) {
		n = connector(dict);
		if (n == NULL) {
			return NULL;
		}
	} else if (is_equal(dict, ')') || is_equal(dict, ']')) {
		/* allows "()" or "[]" */
		n = make_zeroary_node(dict);
	} else {
			dict_error(dict, "Connector, \"(\", \"[\", or \"{\" expected.");
		return NULL;
	}
	return n;
}

/* ======================================================================== */
#else /* This is for infix notation */

static Exp * restricted_expression(Dictionary dict, int and_ok, int or_ok);

/**
 * Build (and return the root of) the tree for the expression beginning
 * with the current token.  At the end, the token is the first one not
 * part of this expression.
 */
static Exp * expression(Dictionary dict)
{
	return restricted_expression(dict, TRUE, TRUE);
}

static Exp * restricted_expression(Dictionary dict, int and_ok, int or_ok)
{
	Exp *nl = NULL, *nr;
	E_list *ell, *elr;

	if (is_equal(dict, '('))
	{
		if (!link_advance(dict)) {
			return NULL;
		}
		nl = expression(dict);
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
		nl = expression(dict);
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
		nl = make_optional_node(dict, nl);
	}
	else if (is_equal(dict, '['))
	{
		if (!link_advance(dict)) {
			return NULL;
		}
		nl = expression(dict);
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
		nl->cost += 1.0f;
	}
	else if (!dict->is_special)
	{
		nl = connector(dict);
		if (nl == NULL) {
			return NULL;
		}
	}
	else if (is_equal(dict, ')') || is_equal(dict, ']'))
	{
		/* allows "()" or "[]" */
		nl = make_zeroary_node(dict);
	}
	else
	{
		dict_error(dict, "Connector, \"(\", \"[\", or \"{\" expected.");
		return NULL;
	}

	if (is_equal(dict, '&') || (strcmp(dict->token, "and") == 0))
	{
		Exp *n;

		if (!and_ok) {
			warning(dict, "\"and\" and \"or\" at the same level in an expression");
		}
		if (!link_advance(dict)) {
			return NULL;
		}
		nr = restricted_expression(dict, TRUE, FALSE);
		if (nr == NULL) {
			return NULL;
		}
		n = Exp_create(dict);
		n->u.l = ell = (E_list *) xalloc(sizeof(E_list));
		ell->next = elr = (E_list *) xalloc(sizeof(E_list));
		elr->next = NULL;

		ell->e = nl;
		elr->e = nr;
		n->type = AND_type;
		n->cost = 0.0f;
		return n;
	}
	else if (is_equal(dict, '|') || (strcmp(dict->token, "or") == 0))
	{
		Exp *n;

		if (!or_ok) {
			warning(dict, "\"and\" and \"or\" at the same level in an expression");
		}
		if (!link_advance(dict)) {
			return NULL;
		}
		nr = restricted_expression(dict, FALSE,TRUE);
		if (nr == NULL) {
			return NULL;
		}
		n = Exp_create(dict);
		n->u.l = ell = (E_list *) xalloc(sizeof(E_list));
		ell->next = elr = (E_list *) xalloc(sizeof(E_list));
		elr->next = NULL;

		ell->e = nl;
		elr->e = nr;
		n->type = OR_type;
		n->cost = 0.0f;
		return n;
	} 

	return nl;
}

#endif

/* ======================================================================== */
/* Tree balancing utilities, used to implement an AVL tree.
 * Unfortunately, AVL tree insertion is very slowww, unusably
 * slow for creating the dictionary. The code is thus ifdef'ed out
 * but is left here for debugging and other sundry purposes.
 * A better way to rebalance the tree is the DSW algo, implemented
 * further below.
 */

static Dict_node *rotate_right(Dict_node *root)
{
	Dict_node *pivot = root->left;
	root->left = pivot->right;
	pivot->right = root;
	return pivot;
}

#ifdef USE_AVL_TREE_FOR_INSERTION

static Dict_node *rotate_left(Dict_node *root)
{
	Dict_node *pivot = root->right;
	root->right = pivot->left;
	pivot->left = root;
	return pivot;
}

/* Return tree height. XXX this is not tail-recursive! */
static int tree_depth (Dict_node *n)
{
	int l, r;
	if (NULL == n) return 0;
	if (NULL == n->left) return 1+tree_depth(n->right);
	if (NULL == n->right) return 1+tree_depth(n->left);
	l = tree_depth(n->left);
	r = tree_depth(n->right);
	if (l < r) return r+1;
	return l+1;
}

static int tree_balance(Dict_node *n)
{
	int l = tree_depth(n->left);
	int r = tree_depth(n->right);
	return r-l;
}

/**
 * Rebalance the dictionary tree.
 * This recomputes the tree depth wayy too often, but so what.. this
 * only wastes cpu time during the initial dictinary read.
 */
static Dict_node *rebalance(Dict_node *root)
{
	int bal = tree_balance(root);
	if (2 == bal)
	{
		bal = tree_balance(root->right);
		if (-1 == bal)
		{
			root->right = rotate_right (root->right);
		}
		return rotate_left(root);
	}
	else if (-2 == bal)
	{
		bal = tree_balance(root->left);
		if (1 == bal)
		{
			root->left = rotate_left (root->left);
		}
		return rotate_right(root);
	}
	return root;
}

#endif /* USE_AVL_TREE_FOR_INSERTION */

/* ======================================================================== */
/* Implementation of the DSW algo for rebalancing a binary tree.
 * The point is -- after building the dictionary tree, we rebalance it 
 * once at the end. This is a **LOT LOT** quicker than maintaing an 
 * AVL tree along the way (less than quarter-of-a-second vs. about
 * a minute or more!) FWIW, the DSW tree is even more balanced than
 * the AVL tree is (its less deep, more full).
 *
 * The DSW algo, with C++ code, is described in 
 *
 * Timothy J. Rolfe, "One-Time Binary Search Tree Balancing:
 * The Day/Stout/Warren (DSW) Algorithm", inroads, Vol. 34, No. 4
 * (December 2002), pp. 85-88
 * http://penguin.ewu.edu/~trolfe/DSWpaper/
 */

static Dict_node * dsw_tree_to_vine (Dict_node *root)
{
	Dict_node *vine_tail, *vine_head, *rest;
	Dict_node vh;

	vine_head = &vh;
	vine_head->left = NULL;
	vine_head->right = root;
	vine_tail = vine_head;
	rest = root;

	while (NULL != rest)
	{
		/* If no left, we are done, do the right */
		if (NULL == rest->left)
		{
			vine_tail = rest;
			rest = rest->right;
		}
	 	/* eliminate the left subtree */
		else
		{
			rest = rotate_right(rest);
			vine_tail->right = rest;
		}
	}

	return vh.right;
}

static void dsw_compression (Dict_node *root, unsigned int count)
{
	unsigned int j;
	for (j = 0; j < count; j++)
	{
		/* Compound left rotation */
		Dict_node * pivot = root->right;
		root->right = pivot->right;
		root = pivot->right;
		pivot->right = root->left;
		root->left = pivot;
	}
}

/* Return size of the full portion of the tree
 * Gets the next pow(2,k)-1
 */
static inline unsigned int full_tree_size (unsigned int size)
{
	unsigned int pk = 1;
	while (pk < size) pk = 2*pk + 1;
	return pk/2;
}

static Dict_node * dsw_vine_to_tree (Dict_node *root, int size)
{
	Dict_node vine_head;
	unsigned int full_count = full_tree_size(size +1);

	vine_head.left = NULL;
	vine_head.right = root;

	dsw_compression(&vine_head, size - full_count);
	for (size = full_count ; size > 1 ; size /= 2)
	{
		dsw_compression(&vine_head, size / 2);
	}
	return vine_head.right;
}

/* ======================================================================== */
/**
 * Insert the new node into the dictionary below node n.
 * Give error message if the new element's string is already there.
 * Assumes that the "n" field of new is already set, and the left
 * and right fields of it are NULL.
 *
 * The resulting tree is highly unbalanced. It needs to be rebalanced
 * before used.
 */
Dict_node * insert_dict(Dictionary dict, Dict_node * n, Dict_node * newnode)
{
	int comp;

	if (NULL == n) return newnode;

	comp = dict_order(newnode->string, n->string);
	if (comp < 0)
	{
		if (NULL == n->left)
		{
			n->left = newnode;
			return n;
		} 
		n->left = insert_dict(dict, n->left, newnode);
		return n;
		/* return rebalance(n); Uncomment to get an AVL tree */
	}
	else if (comp > 0)
	{
		if (NULL == n->right)
		{
			n->right = newnode;
			return n;
		}
		n->right = insert_dict(dict, n->right, newnode);
		return n;
		/* return rebalance(n); Uncomment to get an AVL tree */
	}
	else
	{
		char t[256];
		snprintf(t, 256, "The word \"%s\" has been multiply defined\n", newnode->string);
		dict_error(dict, t);
		return NULL;
	}
}

/**
 * insert_list() -
 * p points to a list of dict_nodes connected by their left pointers.
 * l is the length of this list (the last ptr may not be NULL).
 * It inserts the list into the dictionary.
 * It does the middle one first, then the left half, then the right.
 *
 * Note: I think this insert middle, then left, then right, has
 * its origins as a lame attempt to hack around the fact that the 
 * resulting binary tree is rather badly unbalanced. This has been 
 * fixed by using the DSW rebalancing algo. Now, that would seem
 * to render this crazy bisected-insertion algo obsoloete, but ..
 * oddly enough, it seems to make the DSW balancing go really fast!
 * Faster than a simple insertion. Go figure. I think this has
 * something to do with the fact that the dictionaries are in
 * alphabetical order! This subdivision helps randomize a bit.
 */
static void insert_list(Dictionary dict, Dict_node * p, int l)
{
	Dict_node * dn, *dn_head, *dn_second_half;
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

	if (contains_underbar(dn->string))
	{
		insert_idiom(dict, dn);
	}
	else if (is_idiom_word(dn->string))
	{
		err_ctxt ec;
		ec.sent = NULL;
		err_msg(&ec, Warn, "Warning: Word \"%s\" found near line %d.\n"
		        "\tWords ending \".Ix\" (x a number) are reserved for idioms.\n"
		        "\tThis word will be ignored.\n",
		        dn->string, dict->line_number);
		free_dict_node(dn);
	}
	else if ((dn_head = abridged_lookup_list(dict, dn->string)) != NULL)
	{
		Dict_node *dnx;
		err_ctxt ec;
		ec.sent = NULL;
		err_msg(&ec, Warn, "Warning: The word \"%s\" "
		          "found near line %d of %s matches the following words:\n",
	             dn->string, dict->line_number, dict->name);
		for (dnx = dn_head; dnx != NULL; dnx = dnx->right) {
			fprintf(stderr, "\t%s", dnx->string);
		}
		fprintf(stderr, "\n\tThis word will be ignored.\n");
		free_lookup_list(dn_head);
		free_dict_node(dn);
	}
	else
	{
		dict->root = insert_dict(dict, dict->root, dn);
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
static int read_entry(Dictionary dict)
{
	Exp *n;
	int i;

	Dict_node *dn_new, *dnx, *dn = NULL;

	/* Reset multi-byte shift state every line. */
	memset(&dict->mbss, 0, sizeof(dict->mbss));

	while (!is_equal(dict, ':'))
	{
		if (dict->is_special)
		{
			dict_error(dict, "I expected a word but didn\'t get it.");
			return 0;
		}

		/* If it's a word-file name */
		/* However, be careful to reject "/.v" which is the division symbol
		 * used in equations (.v means verb-like) */
		if ((dict->token[0] == '/') && (dict->token[1] != '.'))
		{
			dn = read_word_file(dict, dn, dict->token);
			if (dn == NULL)
			{
				err_ctxt ec;
				ec.sent = NULL;
				err_msg(&ec, Error, "Error opening word file %s\n", dict->token);
				return 0;
			}
		}
		else
		{
			dn_new = dict_node_new();
			dn_new->left = dn;
			dn = dn_new;
			dn->file = NULL;
			dn->string = string_set_add(dict->token, dict->string_set);
		}

		/* Advance to next entry, unless error */
		if (0 == link_advance(dict)) goto syntax_error;
	}

	/* pass the : */
	if (!link_advance(dict))
	{
		goto syntax_error;
	}

	n = expression(dict);
	if (n == NULL)
	{
		goto syntax_error;
	}

	if (!is_equal(dict, ';'))
	{
		dict_error(dict, "Expecting \";\" at the end of an entry.");
		goto syntax_error;
	}

	/* pass the ; */
	if (!link_advance(dict))
	{
		goto syntax_error;
	}

	/* At this point, dn points to a list of Dict_nodes connected by
	 * their left pointers. These are to be inserted into the dictionary */
	i = 0;
	for (dnx = dn; dnx != NULL; dnx = dnx->left)
	{
		dnx->exp = n;
		i++;
	}
	insert_list(dict, dn, i);
	return 1;

syntax_error:
	free_lookup_list(dn);
	return 0;
}

#if ! defined INFIX_NOTATION
/**
 * print the expression, in prefix-style
 */
void print_expression(Exp * n)
{
	E_list * el;
	int i, icost;

	if (n == NULL)
	{
		printf("NULL expression");
		return;
	}

	icost = (int) (n->cost);
	if (n->type == CONNECTOR_type)
	{
		for (i=0; i<icost; i++) printf("[");
		if (n->multi) printf("@");
		printf("%s%c",n->u.string, n->dir);
		for (i=0; i<icost; i++) printf("]");
		if (icost > 0) printf(" ");
	}
	else
	{
		for (i=0; i<icost; i++) printf("[");
		if (icost == 0) printf("(");
		if (n->type == AND_type) printf("& ");
		if (n->type == OR_type) printf("or ");
		for (el = n->u.l; el != NULL; el = el->next)
		{
			print_expression(el->e);
		}
		for (i=0; i<icost; i++) printf("]");
		if (icost > 0) printf(" ");
		if (icost == 0) printf(") ");
	}
}

#else /* INFIX_NOTATION */

/**
 * print the expression, in infix-style
 */
static void print_expression_parens(Exp * n, int need_parens)
{
	E_list * el;
	int i, icost;

	if (n == NULL)
	{
		printf("NULL expression");
		return;
	}

	icost = (int) (n->cost);
	/* print the connector only */
	if (n->type == CONNECTOR_type)
	{
		for (i=0; i<icost; i++) printf("[");
		if (n->multi) printf("@");
		printf("%s%c",n->u.string, n->dir);
		for (i=0; i<icost; i++) printf("]");
		return;
	}

	/* Look for optional, and print only that */
	el = n->u.l; 
	if (el == NULL)
	{
		for (i=0; i<icost; i++) printf("[");
		printf ("()");
		for (i=0; i<icost; i++) printf("]");
		return;
	}

	for (i=0; i<icost; i++) printf("[");
	if ((n->type == OR_type) &&
	    el && el->e && (NULL == el->e->u.l))
	{
		printf ("{");
		print_expression_parens(el->next->e, FALSE);
		printf ("}");
		return;
	}

	if ((icost == 0) && need_parens) printf("(");

	/* print left side of binary expr */
	print_expression_parens(el->e, TRUE);

	/* get a funny "and optional" when its a named expression thing. */
	if ((n->type == AND_type) && (el->next == NULL))
	{
		for (i=0; i<icost; i++) printf("]");
		if ((icost == 0) && need_parens) printf(")");
		return;
	}

	if (n->type == AND_type) printf(" & ");
	if (n->type == OR_type) printf(" or ");

	/* print right side of binary expr */
	el = el->next;
	if (el == NULL)
	{
		printf ("()");
	}
	else
	{
		if (el->e->type == n->type)
		{
			print_expression_parens(el->e, FALSE);
		}
		else
		{
			print_expression_parens(el->e, TRUE);
		}
		if (el->next != NULL)
			printf ("\nERROR! Unexpected list!\n");
	}

	for (i=0; i<icost; i++) printf("]");
	if ((icost == 0) && need_parens) printf(")");
}

void print_expression(Exp * n)
{
	print_expression_parens(n, FALSE);
	printf("\n");
}
#endif /* INFIX_NOTATION */

static void rprint_dictionary_data(Dict_node * n)
{
	if (n == NULL) return;
	rprint_dictionary_data(n->left);
	printf("%s: ", n->string);
	print_expression(n->exp);
	printf("\n");
	rprint_dictionary_data(n->right);
}

/**
 * Dump the entire contents of the dictionary
 * XXX This is not currently called by anything, but is a "good thing
 * to keep around".
 */
void print_dictionary_data(Dictionary dict)
{
	rprint_dictionary_data(dict->root);
}

int read_dictionary(Dictionary dict)
{
	if (!link_advance(dict))
	{
		return 0;
	}
	while (dict->token[0] != '\0')
	{
		if (!read_entry(dict))
		{
			return 0;
		}
	}
	dict->root = dsw_tree_to_vine(dict->root);
	dict->root = dsw_vine_to_tree(dict->root, dict->num_entries);
	return 1;
}

/* ======================================================================= */
/* the following functions are for handling deletion */
/**
 * Returns true if it finds a non-idiom dict_node in a file that matches
 * the string s.
 *
 ** note: this now DOES include non-file words in its search.
 *
 * Also sets parent and to_be_deleted appropriately.
 */
static int find_one_non_idiom_node(Dict_node * p, Dict_node * dn,
                                   const char * s,
                                   Dict_node **parent, Dict_node **to_be_deleted)
{
	int m;
	if (dn == NULL) return FALSE;
	m = dict_order_wild(s, dn->string);
	if (m <= 0) {
		if (find_one_non_idiom_node(dn,dn->left, s, parent, to_be_deleted)) return TRUE;
	}
/*	if ((m == 0) && (!is_idiom_word(dn->string)) && (dn->file != NULL)) { */
	if ((m == 0) && (!is_idiom_word(dn->string))) {
		*to_be_deleted = dn;
		*parent = p;
		return TRUE;
	}
	if (m >= 0) {
		if (find_one_non_idiom_node(dn,dn->right, s, parent, to_be_deleted)) return TRUE;
	}
	return FALSE;
}

static void set_parent_of_node(Dictionary dict,
                               Dict_node *p,
                               Dict_node * del,
                               Dict_node * newnode)
{
	if (p == NULL) {
		dict->root = newnode;
	} else {
		if (p->left == del) {
			p->left = newnode;
		} else if (p->right == del) {
			p->right = newnode;
		} else {
			assert(FALSE, "Dictionary broken?");
		}
	}
}

/**
 * This deletes all the non-idiom words of the dictionary that match
 * the given string.  Returns TRUE if some deleted, FALSE otherwise.
 *
 * XXX Note: this function is not currently used anywhere in the code,
 * but it could be useful for general dictionary editing.
 */
int delete_dictionary_words(Dictionary dict, const char * s)
{
	Dict_node *pred, *pred_parent;
	Dict_node *parent, *to_be_deleted;

	if (!find_one_non_idiom_node(NULL, dict->root, s, &parent, &to_be_deleted)) return FALSE;
	for(;;) {
		/* now parent and to_be_deleted are set */
		if (to_be_deleted->file != NULL) {
			to_be_deleted->file->changed = TRUE;
		}
		if (to_be_deleted->left == NULL) {
			set_parent_of_node(dict, parent, to_be_deleted, to_be_deleted->right);
			free_dict_node(to_be_deleted);
		} else {
			pred_parent = to_be_deleted;
			pred = to_be_deleted->left;
			while(pred->right != NULL) {
				pred_parent = pred;
				pred = pred->right;
			}
			to_be_deleted->string = pred->string;
			to_be_deleted->file = pred->file;
			to_be_deleted->exp = pred->exp;
			set_parent_of_node(dict, pred_parent, pred, pred->left);
			free_dict_node(pred);
		}
		if (!find_one_non_idiom_node(NULL, dict->root, s, &parent, &to_be_deleted)) return TRUE;
	}
}

static void free_Word_file(Word_file * wf)
{
	Word_file *wf1;

	for (;wf != NULL; wf = wf1) {
		wf1 = wf->next;
		xfree((char *) wf, sizeof(Word_file));
	}
}

/**
 * The following two functions free the Exp s and the
 * E_lists of the dictionary.  Not to be confused with
 * free_E_list in utilities.c
 */
static void free_Elist(E_list * l)
{
	E_list * l1;

	for (; l != NULL; l = l1) {
		l1 = l->next;
		xfree(l, sizeof(E_list));
	}
}

static void free_Exp_list(Exp * e)
{
	Exp * e1;
	for (; e != NULL; e = e1)
	{
		e1 = e->next;
		if (e->type != CONNECTOR_type)
		{
		   free_Elist(e->u.l);
		}
		exp_free(e);
	}
}

void free_dictionary(Dictionary dict)
{
	free_dict_node_recursive(dict->root);
	free_Word_file(dict->word_file_header);
	free_Exp_list(dict->exp_list);
}

/**
 *  dict_display_word_info() - display the information about the given word.
 */
void dict_display_word_info(Dictionary dict, const char * s)
{
	Dict_node *dn, *dn_head;
	Disjunct * d1, * d2;
	int len;
	dn_head = dictionary_lookup_list(dict, s);
	if (dn_head == NULL)
	{
		printf("	\"%s\" matches nothing in the dictionary.\n", s);
		return;
	}
	printf("Matches:\n");
	for (dn = dn_head; dn != NULL; dn = dn->right)
	{
		len = 0;
		d1 = build_disjuncts_for_dict_node(dn);
		for(d2 = d1 ; d2 != NULL; d2 = d2->next)
		{
			len++;
		}
		free_disjuncts(d1);
		printf("    ");
		left_print_string(stdout, dn->string,
			"                         ");
		printf(" %5d  disjuncts ", len);
		if (dn->file != NULL)
		{
			printf("<%s>", dn->file->file);
		}
		printf("\n");
	}
	free_lookup_list(dn_head);
	return;
}

/**
 *  dict_display_word_expr() - display the connector info for a given word.
 */
void dict_display_word_expr(Dictionary dict, const char * s)
{
	Dict_node *dn, *dn_head;

	dn_head = dictionary_lookup_list(dict, s);
	if (dn_head == NULL)
	{
		printf("	\"%s\" matches nothing in the dictionary.\n", s);
		return;
	}
	printf("\nExpressions:\n");
	for (dn = dn_head; dn != NULL; dn = dn->right)
	{
		printf("    ");
		left_print_string(stdout, dn->string,
			"                         ");
		print_expression(dn->exp);
		printf("\n\n");
	}
	free_lookup_list(dn_head);
	return;
}
