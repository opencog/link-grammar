/********************************************************************************/
/* Copyright (c) 2004                                                           */
/* Daniel Sleator, David Temperley, and John Lafferty                           */
/* All rights reserved                                                          */
/*                                                                              */
/* Use of the link grammar parsing system is subject to the terms of the        */
/* license set forth in the LICENSE file included with this software,           */
/* and also available at http://www.link.cs.cmu.edu/link/license.html           */
/* This license allows free redistribution and use in source and binary         */
/* forms, with or without modification, subject to certain conditions.          */
/*                                                                              */
/********************************************************************************/

#include <link-grammar/api.h>

/*
  The dictionary format:

  In what follows:
    Every "%" symbol and everything after it is ignored on every line.
    Every newline or tab is replaced by a space.

  The dictionary file is a sequence of ENTRIES.  Each ENTRY is one or
  more WORDS (a sequence of upper or lower case letters) separated by
  spaces, followed by a ":", followed by an EXPRESSION followed by a
  ";".  An EXPRESSION is a lisp expression where the functions are "&"
  or "and" or "|" or "or", and there are three types of parentheses:
  "()", "{}", and "[]".  The terminal symbols of this grammar are the
  connectors, which are strings of letters or numbers or *s.  (This
  description applies to the prefix form of the dictionary.  the current
  dictionary is written in infix form.  If the defined constant
  INFIX_NOTATION is defined, then infix is used otherwise prefix is used.)

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

*/

static int link_advance(Dictionary dict);

static void dict_error(Dictionary dict, char * s) {
	int i;
	char tokens[1024], t[128];

	tokens[0] = '\0';
	for (i=0; i<5 && dict->token[0] != '\0' ; i++) {
		sprintf(t, "\"%s\" ", dict->token);
		strcat(tokens, t);
		(void) link_advance(dict);
	}
	lperror(DICTPARSE, ". %s\n\t line %d, tokens = %s\n",
			s, dict->line_number, tokens);
}

void warning(Dictionary dict, char * s) {
	printf("\nWarning: %s\n",s);
	printf("line %d, current token = \"%s\"\n", dict->line_number, dict->token);
}

Exp * Exp_create(Dictionary dict) {
	/* allocate a new Exp node and link it into the
	   exp_list for freeing later */
	Exp * e;
	e = (Exp *) xalloc(sizeof(Exp));
	e->next = dict->exp_list;
	dict->exp_list = e;
	return e;
}

int get_character(Dictionary dict, int quote_mode) {
/* This gets the next character from the input, eliminating comments.
   If we're in quote mode, it does not consider the % character for
   comments */

	int c;

	c = fgetc(dict->fp);
	if ((c == '%') && (!quote_mode)) {
		while((c != EOF) && (c != '\n')) c = fgetc(dict->fp);
	}
	if (c == '\n') dict->line_number++;
	return c;
}


/*
   This set of 10 characters are the ones defining the syntax of the dictionary.
*/
#define SPECIAL "(){};[]&|:"

static int link_advance(Dictionary dict) {
   /* this reads the next token from the input into token */
	int c, i;
	int quote_mode;

	dict->is_special = FALSE;

	if (dict->already_got_it != '\0') {
		dict->is_special = (strchr(SPECIAL, dict->already_got_it) != NULL);
		if (dict->already_got_it == EOF) {
			dict->token[0] = '\0';
		} else {
			dict->token[0] = dict->already_got_it;
			dict->token[1] = '\0';
		}
		dict->already_got_it = '\0';
		return 1;
	}

	do c=get_character(dict, FALSE); while (isspace(c));

	quote_mode = FALSE;

	i = 0;
	for (;;) {
		if (i > MAX_TOKEN_LENGTH-1) {
			dict_error(dict, "Token too long");
			return 0;
		}
		if (quote_mode) {
			if (c == '\"') {
				quote_mode = FALSE;
				dict->token[i] = '\0';
				return 1;
			}
			if (isspace(c)) {
				dict_error(dict, "White space inside of token");
				return 0;
			}
			dict->token[i] = c;
			i++;
		} else {
			if (strchr(SPECIAL, c) != NULL) {
				if (i==0) {
					dict->token[0] = c;
					dict->token[1] = '\0';
					dict->is_special = TRUE;
					return 1;
				}
				dict->token[i] = '\0';
				dict->already_got_it = c;
				return 1;
			}
			if (c==EOF) {
				if (i==0) {
					dict->token[0] = '\0';
					return 1;
				}
				dict->token[i] = '\0';
				dict->already_got_it = c;
				return 1;
			}
			if (isspace(c)) {
				dict->token[i] = '\0';
				return 1;
			}
			if (c == '\"') {
				quote_mode = TRUE;
			} else {
				dict->token[i] = c;
				i++;
			}
		}
		c = get_character(dict, quote_mode);
	}
	return 1;
}

int is_equal(Dictionary dict, int c) {
/* returns TRUE if this token is a special token and it is equal to c */
	return (dict->is_special && c==dict->token[0] && dict->token[1] == '\0');
}

int check_connector(Dictionary dict, char * s) {
	/* makes sure the string s is a valid connector */
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
	if ((*s=='I') && (*(s+1)=='D')) {
		dict_error(dict, "Connectors beginning with \"ID\" are forbidden");
		return 0;
	}
	while (*(s+1)) {
		if ((!isalnum((int)*s)) && (*s != '*') && (*s != '^')) {
			dict_error(dict, "All letters of a connector must be alpha-numeric.");
			return 0;
		}
		s++;
	}
	return 1;
}

Exp * make_unary_node(Dictionary dict, Exp * e);

Exp * connector(Dictionary dict) {
/* the current token is a connector (or a dictionary word)           */
/* make a node for it                                                */

	Exp * n;
	Dict_node * dn;
	int i;

	i = strlen(dict->token)-1;  /* this must be + or - if a connector */
	if ((dict->token[i] != '+') && (dict->token[i] != '-')) {
		dn = abridged_lookup(dict, dict->token);
		while((dn != NULL) && (strcmp(dn->string, dict->token) != 0)) {
			dn = dn->right;
		}
		if (dn == NULL) {

			dict_error(dict, "\nPerhaps missing + or - in a connector.\n"
			                 "Or perhaps you forgot the suffix on a word.\n"
			                 "Or perhaps a word is used before it is defined.\n");
			return NULL;
		}
		n = make_unary_node(dict, dn->exp);
	} else {
		if (!check_connector(dict, dict->token)) {
			return NULL;
		}
		n = Exp_create(dict);
		n->dir = dict->token[i];
		dict->token[i] = '\0';				   /* get rid of the + or - */
		if (dict->token[0] == '@') {
			n->u.string = string_set_add(dict->token+1, dict->string_set);
			n->multi = TRUE;
		} else {
			n->u.string = string_set_add(dict->token, dict->string_set);
			n->multi = FALSE;
		}
		n->type = CONNECTOR_type;
		n->cost = 0;
	}
	if (!link_advance(dict)) {
		return NULL;
	}
	return n;
}

/** 
 * This creates a node with one child (namely e).  Initializes
 * the cost to zero.
 */
Exp * make_unary_node(Dictionary dict, Exp * e) {
	Exp * n;
	n = Exp_create(dict);
	n->type = AND_type;  /* these must be AND types */
	n->cost = 0;
	n->u.l = (E_list *) xalloc(sizeof(E_list));
	n->u.l->next = NULL;
	n->u.l->e = e;
	return n;
}

/** 
 * This creates a node with zero children.  Initializes
 * the cost to zero.
 */
Exp * make_zeroary_node(Dictionary dict) {
	Exp * n;
	n = Exp_create(dict);
	n->type = AND_type;  /* these must be AND types */
	n->cost = 0;
	n->u.l = NULL;
	return n;
}

Exp * make_optional_node(Dictionary dict, Exp * e) {
/* This creates an OR node with two children, one the given node,
   and the other as zeroary node.  This has the effect of creating
   what used to be called an optional node.
*/
	Exp * n;
	E_list *el, *elx;
	n = Exp_create(dict);
	n->type = OR_type;
	n->cost = 0;
	n->u.l = el = (E_list *) xalloc(sizeof(E_list));
	el->e = make_zeroary_node(dict);
	el->next = elx = (E_list *) xalloc(sizeof(E_list));
	elx->next = NULL;
	elx->e = e;
	return n;
}

Exp * expression(Dictionary dict);

#if ! defined INFIX_NOTATION

Exp * operator_exp(Dictionary dict, int type) {
/* We're looking at the first of the stuff after an "and" or "or".	 */
/* Build a Exp node for this expression.  Set the cost and optional	*/
/* fields to the default values.  Set the type field according to type */
	Exp * n;
	E_list first;
	E_list * elist;
	n = Exp_create(dict);
	n->type = type;
	n->cost = 0;
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
Exp * in_parens(Dictionary dict) {
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

Exp * expression(Dictionary dict) {
/* Build (and return the root of) the tree for the expression beginning	 */
/* with the current token.  At the end, the token is the first one not part */
/* of this expression.													  */

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
		n->cost += 1;
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

#else

/* This is for infix notation */

Exp * restricted_expression(Dictionary dict, int and_ok, int or_ok);

Exp * expression(Dictionary dict) {
/* Build (and return the root of) the tree for the expression beginning  */
/* with the current token.  At the end, the token is the first one not   */
/* part of this expression.											  */
	return restricted_expression(dict, TRUE,TRUE);
}

Exp * restricted_expression(Dictionary dict, int and_ok, int or_ok) {
	Exp * nl=NULL, * nr, * n;
	E_list *ell, *elr;
	if (is_equal(dict, '(')) {
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
	} else if (is_equal(dict, '{')) {
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
	} else if (is_equal(dict, '[')) {
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
		nl->cost += 1;
	} else if (!dict->is_special) {
		nl = connector(dict);
		if (nl == NULL) {
			return NULL;
		}
	} else if (is_equal(dict, ')') || is_equal(dict, ']')) {
		/* allows "()" or "[]" */
		nl = make_zeroary_node(dict);
	} else {
			dict_error(dict, "Connector, \"(\", \"[\", or \"{\" expected.");
		return NULL;
	}

	if (is_equal(dict, '&') || (strcmp(dict->token, "and")==0)) {
		if (!and_ok) {
			warning(dict, "\"and\" and \"or\" at the same level in an expression");
		}
		if (!link_advance(dict)) {
			return NULL;
		}
		nr = restricted_expression(dict, TRUE,FALSE);
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
		n->cost = 0;
	} else if (is_equal(dict, '|') || (strcmp(dict->token, "or")==0)) {
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
		n->cost = 0;
	} else return nl;
	return n;
}

#endif

/* The data structure storing the dictionary is simply a binary tree.  */
/* There is one catch however.  The ordering of the words is not       */
/* exactly the order given by strcmp.  It was necessary to             */
/* modify the order to make it so that "make" < "make.n" < "make-up"   */
/* The problem is that if some other string could  lie between '\0'    */
/* and '.' (such as '-' which strcmp would give) then it makes it much */
/* harder to return all the strings that match a given word.           */
/* For example, if "make-up" was inserted, then "make" was inserted    */
/* the a search was done for "make.n", the obvious algorithm would     */
/* not find the match.                                                 */

/* verbose version */
/*
int dict_compare(char *s, char *t) {
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
int dict_compare(char *s, char *t) {
	while (*s != '\0' && *s == *t) {s++; t++;}
	return (((*s == '.')?(1):((*s)<<1))  -  ((*t == '.')?(1):((*t)<<1)));
}

Dict_node * insert_dict(Dictionary dict, Dict_node * n, Dict_node * new) {
/* Insert the new node into the dictionary below node n                  */
/* give error message if the new element's string is already there       */
/* assumes that the "n" field of new is already set, and the left        */
/* and right fields of it are NULL                                       */
	int comp;
	char t[128];

	if (n == NULL) return new;
	comp = dict_compare(new->string, n->string);
	if (comp < 0) {
		n->left = insert_dict(dict, n->left, new);
	} else if (comp > 0) {
		n->right = insert_dict(dict, n->right, new);
	} else {
		sprintf(t, "The word \"%s\" has been multiply defined\n", new->string);
		dict_error(dict, t);
		return NULL;
	}
	return n;
}

void insert_list(Dictionary dict, Dict_node * p, int l) {
/* p points to a list of dict_nodes connected by their left pointers   */
/* l is the length of this list (the last ptr may not be NULL)         */
/* It inserts the list into the dictionary.                            */
/* It does the middle one first, then the left half, then the right.   */

	Dict_node * dn, *dnx, *dn_second_half;
	int k, i; /* length of first half */

	if (l == 0) return;

	k = (l-1)/2;
	dn = p;
	for (i=0; i<k; i++) {
		dn = dn->left;
	}
	/* dn now points to the middle element */
	dn_second_half = dn->left;
	dn->left = dn->right = NULL;
	if (contains_underbar(dn->string)) {
		insert_idiom(dict, dn);
	} else if (is_idiom_word(dn->string)) {
		printf("*** Word \"%s\" found near line %d.\n", dn->string, dict->line_number);
		printf("	Words ending \".Ix\" (x a number) are reserved for idioms.\n");
		printf("	This word will be ignored.\n");
		xfree((char *)dn, sizeof(Dict_node));
	} else if ((dnx = abridged_lookup(dict, dn->string))!= NULL) {
		printf("*** The word \"%s\"", dn->string);
		printf(" found near line %d matches the following words:\n",
			   dict->line_number);
		for (;dnx != NULL; dnx = dnx->right) {
			printf(" %s", dnx->string);
		}
		printf("\n	This word will be ignored.\n");
		xfree((char *)dn, sizeof(Dict_node));
	} else {
		dict->root = insert_dict(dict, dict->root, dn);
		dict->num_entries++;
	}

	insert_list(dict, p, k);
	insert_list(dict, dn_second_half, l-k-1);
}

int read_entry(Dictionary dict) {
/* Starting with the current token parse one dictionary entry.         */
/* Add these words to the dictionary                                   */
    Exp *n;
    int i;

	Dict_node  *dn_new, *dnx, *dn = NULL;

	for (; !is_equal(dict, ':') ; link_advance(dict)) {
		  if (dict->is_special) {
			  dict_error(dict, "I expected a word but didn\'t get it.");
			  return 0;
		  }
		  if (dict->token[0] == '/') {
			  /* it's a word-file name */
			  dn = read_word_file(dict, dn, dict->token);
			  if (dn == NULL) {
				  lperror(WORDFILE, " %s\n", dict->token);
				  return 0;
			  }
		  } else {
			  dn_new = (Dict_node *) xalloc(sizeof(Dict_node));
			  dn_new->left = dn;
			  dn = dn_new;
			  dn->file = NULL;
			  dn->string = string_set_add(dict->token, dict->string_set);
		  }
	}
	if (!link_advance(dict)) {  /* pass the : */
		return 0;
	}
	n = expression(dict);
	if (n == NULL) {
		return 0;
	}
	if (!is_equal(dict, ';')) {
		dict_error(dict, "Expecting \";\" at the end of an entry.");
		return 0;
	}
	if (!link_advance(dict)) {  /* pass the ; */
		return 0;
	}

	/* at this point, dn points to a list of Dict_nodes connected by	  */
	/* their left pointers.  These are to be inserted into the dictionary */
	i = 0;
	for (dnx = dn; dnx != NULL; dnx = dnx->left) {
		dnx->exp = n;
		i++;
	}
	insert_list(dict, dn, i);
	return 1;
}

void print_expression(Exp * n) {
	E_list * el; int i;
	if (n == NULL) {
		printf("NULL expression");
		return;
	}
	if (n->type == CONNECTOR_type) {
		for (i=0; i<n->cost; i++) printf("[");
		printf("%s%c",n->u.string, n->dir);
		for (i=0; i<n->cost; i++) printf("] ");
	} else {
		for (i=0; i<n->cost; i++) printf("[");
		if (n->cost == 0) printf("(");
		if (n->type == AND_type) printf("& ");
		if (n->type == OR_type) printf("or ");
		for (el = n->u.l; el != NULL; el = el->next) {
			print_expression(el->e);
		}
		for (i=0; i<n->cost; i++) printf("] ");
		if (n->cost == 0) printf(") ");
	}
}

void rprint_dictionary_data(Dict_node * n) {
		if (n==NULL) return;
		rprint_dictionary_data(n->left);
		printf("%s: ", n->string);
		print_expression(n->exp);
		printf("\n");
		rprint_dictionary_data(n->right);
}

void print_dictionary_data(Dictionary dict) {
	rprint_dictionary_data(dict->root);
}


int read_dictionary(Dictionary dict) {
	lperrno = 0;
	if (!link_advance(dict)) {
		fclose(dict->fp);
		return 0;
	}
	while(dict->token[0] != '\0') {
			if (!read_entry(dict)) {
			fclose(dict->fp);
			return 0;
		}
	}
	fclose(dict->fp);
	return 1;
}

static int dict_match(const char * s, const char * t)
{
/* assuming that s is a pointer to a dictionary string, and that
   t is a pointer to a search string, this returns 0 if they
   match, >0 if s>t, and <0 if s<t.

   The matching is done as follows.  Walk down the strings until
   you come to the end of one of them, or until you find unequal
   characters.  A "*" matches anything.  Otherwise, replace "."
   by "\0", and take the difference.  This behavior matches that
   of the function dict_compare().
*/
	while((*s != '\0') && (*s == *t)) {s++; t++;}
	if ((*s == '*') || (*t == '*')) return 0;
	return (((*s == '.')?('\0'):(*s))  -  ((*t == '.')?('\0'):(*t)));
}

/* We need to prune out the lists thus generated.               */
/* A sub string will only be considered a subscript if it       */
/* followes the last "." in the word, and it does not begin     */
/* with a digit.                                                */

static int true_dict_match(char * s, const char * t)
{
	char *ds, *dt;
	ds = strrchr(s, '.');
	dt = strrchr(t, '.');

	/* a dot at the end or a dot followed by a number is NOT considered a subscript */
	if ((dt != NULL) && ((*(dt+1) == '\0') || (isdigit((int)*(dt+1))))) dt = NULL;
	if ((ds != NULL) && ((*(ds+1) == '\0') || (isdigit((int)*(ds+1))))) ds = NULL;

	if (dt == NULL && ds != NULL) {
		if (((int)strlen(t)) > (ds-s)) return FALSE;   /* we need to do this to ensure that */
		return (strncmp(s, t, ds-s) == 0);	  /*"i.e." does not match "i.e" */
	} else if (dt != NULL && ds == NULL) {
		if (((int)strlen(s)) > (dt-t)) return FALSE;
		return (strncmp(s, t, dt-t) == 0);
	} else {
		return (strcmp(s, t) == 0);
	}
}

static Dict_node * lookup_list = NULL;
		  /* a pointer to the temporary lookup list */

void prune_lookup_list(const char * s)
{
	Dict_node *dn, *dnx, *dn_new;
	dn_new = NULL;
	for (dn = lookup_list; dn!=NULL; dn = dnx) {
		dnx = dn->right;
		/* now put dn onto the answer list, or free it */
		if (true_dict_match(dn->string, s)) {
			dn->right = dn_new;
			dn_new = dn;
		} else {
			xfree((char *)dn, sizeof(Dict_node));
		}
	}
	/* now reverse the list back */
	lookup_list = NULL;
	for (dn = dn_new; dn!=NULL; dn = dnx) {
		dnx = dn->right;
		dn->right = lookup_list;
		lookup_list = dn;
	}
}

void free_lookup_list(void) {
	Dict_node * n;
	while(lookup_list != NULL) {
		n = lookup_list->right;
		xfree((char *)lookup_list, sizeof(Dict_node));
		lookup_list = n;
	}
}

void rdictionary_lookup(Dict_node * dn, const char * s)
{
		/* see comment in dictionary_lookup below */
	int m;
	Dict_node * dn_new;
	if (dn == NULL) return;
	m = dict_match(s, dn->string);
	if (m >= 0) {
		rdictionary_lookup(dn->right, s);
	}
	if (m == 0) {
		dn_new = (Dict_node*) xalloc(sizeof(Dict_node));
		*dn_new = *dn;
		dn_new->right = lookup_list;
		lookup_list = dn_new;
	}
	if (m <= 0) {
		rdictionary_lookup(dn->left, s);
	}
}

Dict_node * dictionary_lookup(Dictionary dict, const char *s)
{
		/* Returns a pointer to a lookup list of the words in the dictionary.
   This list is made up of Dict_nodes, linked by their right pointers.
   The node, file and string fields are copied from the dictionary.

   Freeing this list elsewhere is unnecessary, as long as the rest of
   the program merely examines the list (doesn't change it) */

   free_lookup_list();
   rdictionary_lookup(dict->root, s);
   prune_lookup_list(s);
   return lookup_list;
}

int boolean_dictionary_lookup(Dictionary dict, char *s) {
	return (dictionary_lookup(dict, s) != NULL);
}


/**
 * The following routines are exactly the same as those above,
 * only they do not consider the idiom words
 */

void rabridged_lookup(Dict_node * dn, char * s) {
	int m;
	Dict_node * dn_new;
	if (dn == NULL) return;
	m = dict_match(s, dn->string);
	if (m >= 0) {
		rabridged_lookup(dn->right, s);
	}
	if ((m == 0) && (!is_idiom_word(dn->string))) {
		dn_new = (Dict_node*) xalloc(sizeof(Dict_node));
		*dn_new = *dn;
		dn_new->right = lookup_list;
		lookup_list = dn_new;
	}
	if (m <= 0) {
		rabridged_lookup(dn->left, s);
	}
}

Dict_node * abridged_lookup(Dictionary dict, char *s) {
   free_lookup_list();
   rabridged_lookup(dict->root, s);
   prune_lookup_list(s);
   return lookup_list;
}

int boolean_abridged_lookup(Dictionary dict, char *s) {
/* returns true if in the dictionary, false otherwise */
	return (abridged_lookup(dict, s) != NULL);
}

/* the following functions are for handling deletion */

static Dict_node * parent;
static Dict_node * to_be_deleted;


int find_one_non_idiom_node(Dict_node * p, Dict_node * dn, char * s) {
/* Returns true if it finds a non-idiom dict_node in a file that matches
   the string s.

   ** note: this now DOES include non-file words in its search.

	Also sets parent and to_be_deleted appropriately.
*/
	int m;
	if (dn == NULL) return FALSE;
	m = dict_match(s, dn->string);
	if (m <= 0) {
		if (find_one_non_idiom_node(dn,dn->left, s)) return TRUE;
	}
/*	if ((m == 0) && (!is_idiom_word(dn->string)) && (dn->file != NULL)) { */
	if ((m == 0) && (!is_idiom_word(dn->string))) {
		to_be_deleted = dn;
		parent = p;
		return TRUE;
	}
	if (m >= 0) {
		if (find_one_non_idiom_node(dn,dn->right, s)) return TRUE;
	}
	return FALSE;
}

void set_parent_of_node(Dictionary dict,
						Dict_node *p, Dict_node * del, Dict_node * new) {
	if (p == NULL) {
		dict->root = new;
	} else {
		if (p->left == del) {
			p->left = new;
		} else if (p->right == del) {
			p->right = new;
		} else {
			assert(FALSE, "Dictionary broken?");
		}
	}
}

int delete_dictionary_words(Dictionary dict, char * s) {
/* This deletes all the non-idiom words of the dictionary that match
   the given string.  Returns TRUE if some deleted, FALSE otherwise.
*/
	Dict_node *pred, *pred_parent;
	if (!find_one_non_idiom_node(NULL, dict->root, s)) return FALSE;
	for(;;) {
		/* now parent and to_be_deleted are set */
		if (to_be_deleted->file != NULL) {
			to_be_deleted->file->changed = TRUE;
		}
		if (to_be_deleted->left == NULL) {
			set_parent_of_node(dict, parent, to_be_deleted, to_be_deleted->right);
			xfree((char *)to_be_deleted, sizeof(Dict_node));
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
			xfree((char *)pred, sizeof(Dict_node));
		}
		if (!find_one_non_idiom_node(NULL, dict->root, s)) return TRUE;
	}
}

int open_dictionary(char * dict_path_name, Dictionary dict) {
	if ((dict->fp = dictopen(dict_path_name, dict->name, "r")) == NULL) {
		return 0;
	}
	return 1;
}


void free_Word_file(Word_file * wf) {
	Word_file *wf1;

	for (;wf != NULL; wf = wf1) {
		wf1 = wf->next;
		xfree((char *) wf, sizeof(Word_file));
	}
}

/* The following two functions free the Exp s and the
   E_lists of the dictionary.  Not to be confused with
   free_E_list in utilities.c
 */
static void free_Elist(E_list * l) {
	E_list * l1;

	for (; l != NULL; l = l1) {
		l1 = l->next;
		xfree(l, sizeof(E_list));
	}
}

static void free_Exp_list(Exp * e) {
	Exp * e1;
	for (; e != NULL; e = e1) {
		e1 = e->next;
		if (e->type != CONNECTOR_type) {
		   free_Elist(e->u.l);
		}
		xfree((char *)e, sizeof(Exp));
	}
}

void free_Dict_node(Dict_node * dn) {
	if (dn == NULL) return;
	free_Dict_node(dn->left);
	free_Dict_node(dn->right);
	xfree(dn, sizeof(Dict_node));
}

void free_dictionary(Dictionary dict) {
	free_Dict_node(dict->root);
	free_Word_file(dict->word_file_header);
	free_Exp_list(dict->exp_list);
}

void dict_display_word_info(Dictionary dict, char * s) {
	/* display the information about the given word */
	Dict_node * dn;
	Disjunct * d1, * d2;
	int len;
	if ((dn=dictionary_lookup(dict, s)) == NULL) {
		printf("	\"%s\" matches nothing in the dictionary.\n", s);
		return;
	}
	printf("Matches:\n");
	for (;dn != NULL; dn = dn->right) {
		len=0;
		d1 = build_disjuncts_for_dict_node(dn);
		for(d2 = d1 ; d2!=NULL; d2 = d2->next){
			len++;
		}
		free_disjuncts(d1);
		printf("		  ");
		left_print_string(stdout, dn->string, "				  ");
		printf(" %5d  ", len);
		if (dn->file != NULL) {
			printf("<%s>", dn->file->file);
		}
		printf("\n");
	}
	free_lookup_list();
	return;
}
