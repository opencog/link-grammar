/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "api-structures.h"
#include "api-types.h"
#include "dict-api.h"
#include "dict-common.h"
#include "dict-ram/dict-ram.h"
#include "error.h"
#include "idiom.h"
#include "string-set.h"

/**
 * Find if a string signifies an idiom.
 * Returns true if the string contains an underbar character.
 * The check of s[0] prevents inclusion of '_'. In that case no check for
 * length=1 is done because it is not going to be a valid idiom anyway.
 *
 * If the underbar character is preceded by a backslash, it is not
 * considered. The subscript, if exists, is not checked.
 *
 * FIXME: Words with '\' escaped underbars that contain also unescaped
 * ones are not supported.
 */
bool contains_underbar(const char * s)
{
	if ((s[0] == '_') || (s[0] == '\0')) return false;
	while (*++s != '\0')
	{
		if (*s == SUBSCRIPT_MARK) return false;
		if ((*s == '_') && (s[-1] != '\\')) return true;
	}
	return false;
}

/**
 * Returns false if it is not a correctly formed idiom string.
 * Such a string is correct if it consists of non-empty strings
 * separated by '_'.
 */
static bool is_idiom_string(const char * s)
{
	size_t len;
	const char * t;

	len = strlen(s);
	if ((s[0] == '_') || (s[len-1] == '_'))
	{
		return false;
	}

	for (t = s; *t != '\0'; t++)
	{
		if (*s == SUBSCRIPT_MARK) return true;
		if ((*t == '_') && (*(t+1) == '_')) return false;
	}
	return true;
}

/**
 * build_idiom_word_name() -- return idiomized name of given string.
 *
 * Allocates string space and returns a pointer to it.
 * In this string is placed the idiomized name of the given string s.
 * This is the same as s, but with a postfix of ".I". */
static const char * build_idiom_word_name(Dictionary dict, const char * s)
{
	size_t n = strlen(s);
	char *buff = alloca(n+5);

	strcpy(buff, s);
	buff[n] = SUBSCRIPT_MARK;
	buff[n + 1] = '_';
	buff[n + 2] = 'I';
	buff[n + 3] = '\0';

	return string_set_add(buff, dict->string_set);
}

/**
 * Tear the idiom string apart.
 * Put the parts into a list of Dict_nodes (connected by their right pointers)
 * Sets the string fields of these Dict_nodes pointing to the
 * fragments of the string s.  Later these will be replaced by
 * correct names (with .Ix suffixes).
 * The list is reversed from the way they occur in the string.
 * A pointer to this list is returned.
 * This function is called after is_idiom_string() ensures the validity
 * of the given string.
 */
static Dict_node * make_idiom_Dict_nodes(Dictionary dict, const char * string)
{
	Dict_node * dn = NULL;
	char * s = strdupa(string);
	const char * t;
	const char *sm = get_word_subscript(s);

	for (t = s; NULL != s; t = s)
	{
		s = strchr(s, '_');
		if ((NULL != sm) && (s > sm)) s = NULL;
		if (NULL != s) *s++ = '\0';
		Dict_node *dn_new = (Dict_node *) malloc(sizeof (Dict_node));
		dn_new->right = dn;
		dn = dn_new;
		dn->string = string_set_add(t, dict->string_set);
		dn->file = NULL;
	}

	return dn;
}

static void increment_current_name(Dictionary dict)
{
	short i = IDIOM_LINK_SZ-2;

	do
	{
		dict->current_idiom[i]++;
		if (dict->current_idiom[i] <= 'Z') return;
		dict->current_idiom[i] = 'A';
	} while (i-- > 0);
	assert(0, "increment_current_name: Overflow");
}

/**
 * Generate a new connector name obtained from the current_name.
 * allocate string space for it.
 * @return a pointer to connector name.
 */
static const char * generate_id_connector(Dictionary dict)
{
	char buff[IDIOM_LINK_SZ+4];
	short i;
	char * t;

	for (i=0; dict->current_idiom[i] == 'A'; i++)
	  ;
	/* i is now the number of characters of current_name to skip */
	t = buff;

	/* All idiom connector names start with the two characters "_D" */
	*t++ = '_';
	*t++ = 'I';
	for (; i < IDIOM_LINK_SZ; i++)
	{
		*t++ = dict->current_idiom[i];
	}
	*t++ = '\0';
	return string_set_add(buff, dict->string_set);
}

/**
 * Takes as input a pointer to a Dict_node.
 * The string of this Dict_node is an idiom string.
 * This string is torn apart, and its components are inserted into the
 * dictionary as special idiom words (ending in .I).
 * The expression of this Dict_node (its node field) has already been
 * read and constructed.  This will be used to construct the special idiom
 * expressions.
 */
void insert_idiom(Dictionary dict, Dict_node * dn)
{
	Exp * nc, * no, * n1;
	const char * s;
	Dict_node * dn_list, * xdn, * start_dn_list;

	no = dn->exp;
	s = dn->string;

	if (!is_idiom_string(s))
	{
		prt_error("Warning: Word \"%s\" on line %d "
		          "is not a correctly formed idiom string.\n"
		          "\tThis word will be ignored\n",
		          s, dict->line_number);

		return;
	}

	dn_list = start_dn_list = make_idiom_Dict_nodes(dict, s);

	assert(dn_list->right != NULL, "Idiom string with only one connector");

	/* first make the nodes for the base word of the idiom (last word) */
	/* note that the last word of the idiom is first in our list */

	/* ----- this code just sets up the node fields of the dn_list ----*/
	nc = Exp_create(dict->Exp_pool);
	nc->condesc = condesc_add(&dict->contable, generate_id_connector(dict));
	nc->dir = '-';
	nc->multi = false;
	nc->type = CONNECTOR_type;
	nc->operand_next = no;
	nc->cost = 0;

	n1 = Exp_create(dict->Exp_pool);
	n1->operand_first = nc;
	n1->type = AND_type;
	n1->operand_next = NULL;
	n1->cost = 0;

	dn_list->exp = n1;

	dn_list = dn_list->right;

	while(dn_list->right != NULL)
	{
		/* generate the expression for a middle idiom word */

		n1 = Exp_create(dict->Exp_pool);
		n1->type = AND_type;
		n1->operand_next = NULL;
		n1->cost = 0;

		nc = Exp_create(dict->Exp_pool);
		nc->condesc = condesc_add(&dict->contable, generate_id_connector(dict));
		nc->dir = '+';
		nc->multi = false;
		nc->type = CONNECTOR_type;
		nc->operand_next = NULL;
		nc->cost = 0;
		n1->operand_first = nc;

		increment_current_name(dict);

		no = Exp_create(dict->Exp_pool);
		no->condesc = condesc_add(&dict->contable, generate_id_connector(dict));
		no->dir = '-';
		no->multi = false;
		no->type = CONNECTOR_type;
		no->operand_next = NULL;
		no->cost = 0;

		nc->operand_next = no;

		dn_list->exp = n1;

		dn_list = dn_list->right;
	}
	/* now generate the last one */

	nc = Exp_create(dict->Exp_pool);
	nc->condesc = condesc_add(&dict->contable, generate_id_connector(dict));
	nc->dir = '+';
	nc->multi = false;
	nc->type = CONNECTOR_type;
	nc->operand_next = NULL;
	nc->cost = 0;

	dn_list->exp = nc;

	increment_current_name(dict);

	/* ---- end of the code alluded to above ---- */

	/* now its time to insert them into the dictionary */

	dn_list = start_dn_list;

	while (dn_list != NULL)
	{
		xdn = dn_list->right;
		const char *word_name = build_idiom_word_name(dict, dn_list->string);

		Dict_node *t = dictionary_lookup_list(dict, word_name);
		if (NULL == t)
		{
			/* The word doesn't participate yet in any idiom. Just insert it. */
			dn_list->left = dn_list->right = NULL;
			dn_list->string = word_name;
			dict->root = dict_node_insert(dict, dict->root, dn_list);
			dict->num_entries++;
		}
		else
		{
			/* OR the word's expression to the dict expression t->exp. */

			if (t->exp->type != OR_type)
			{
				/* Prepend an OR element. */
				Exp *or = Exp_create(dict->Exp_pool);
				or->type = OR_type;
				or->cost = 0.0;
				or->operand_next = NULL;
				or->operand_first = t->exp;
				t->exp = or;
			}

			/* Prepend the word's expression to dict expression OR'ed elements. */
			dn_list->exp = Exp_create_dup(dict->Exp_pool, dn_list->exp);
			dn_list->exp->operand_next = t->exp->operand_first;
			t->exp->operand_first = dn_list->exp;
			t->left->exp = t->exp; /* Patch the dictionary */
			free_lookup_list(dict, t);
			free(dn_list);
		}
		dn_list = xdn;
	}
}

/**
 * returns true if this is a word ending in ".Ix", where x is a number.
 */
bool is_idiom_word(const char * s)
{
	const char *sm = get_word_subscript(s);

	if (NULL == sm) return false;
	if ((sm[1] == '_') && (sm[2] == 'I') && (sm[3] == '\0')) return true;
	return false;
}
