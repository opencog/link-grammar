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

#include "build-disjuncts.h"
#include "dict-common.h"
#include "print/print.h"
#include "regex-morph.h"
#include "dict-file/word-file.h"
#include "dict-file/read-dict.h"


/* ======================================================================== */

/* INFIX_NOTATION is always defined; we simply never use the format below. */
/* #if ! defined INFIX_NOTATION */
#if 0
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
		printf("%s%c", n->u.string, n->dir);
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

#define COST_FMT "%.3f"
/**
 * print the expression, in infix-style
 */
static void print_expression_parens(const Exp * n, int need_parens)
{
	E_list * el;
	int i, icost;
	double dcost;

	if (n == NULL)
	{
		err_msg(lg_Debug, "NULL expression");
		return;
	}

	icost = (int) (n->cost);
	dcost = n->cost - icost;
	if (dcost > 10E-4)
	{
		dcost = n->cost;
		icost = 1;
	}
	else
	{
		dcost = 0;
	}

	/* print the connector only */
	if (n->type == CONNECTOR_type)
	{
		for (i=0; i<icost; i++) err_msg(lg_Debug, "[");
		if (n->multi) err_msg(lg_Debug, "@");
		err_msg(lg_Debug, "%s%c", n->u.string, n->dir);
		for (i=0; i<icost; i++) err_msg(lg_Debug, "]");
		if (0 != dcost) err_msg(lg_Debug, COST_FMT, dcost);
		return;
	}

	/* Look for optional, and print only that */
	el = n->u.l;
	if (el == NULL)
	{
		for (i=0; i<icost; i++) err_msg(lg_Debug, "[");
		prt_error ("()");
		for (i=0; i<icost; i++) err_msg(lg_Debug, "]");
		if (0 != dcost) err_msg(lg_Debug, COST_FMT, dcost);
		return;
	}

	for (i=0; i<icost; i++) err_msg(lg_Debug, "[");
	if ((n->type == OR_type) &&
	    el && el->e && (NULL == el->e->u.l))
	{
		prt_error ("{");
		if (NULL == el->next) err_msg(lg_Debug, "error-no-next");
		else print_expression_parens(el->next->e, false);
		prt_error ("}");
		return;
	}

	if ((icost == 0) && need_parens) err_msg(lg_Debug, "(");

	/* print left side of binary expr */
	print_expression_parens(el->e, true);

	/* get a funny "and optional" when its a named expression thing. */
	if ((n->type == AND_type) && (el->next == NULL))
	{
		for (i=0; i<icost; i++) err_msg(lg_Debug, "]");
		if (0 != dcost) err_msg(lg_Debug, COST_FMT, dcost);
		if ((icost == 0) && need_parens) err_msg(lg_Debug, ")");
		return;
	}

	if (n->type == AND_type) err_msg(lg_Debug, " & ");
	if (n->type == OR_type) err_msg(lg_Debug, " or ");

	/* print right side of binary expr */
	el = el->next;
	if (el == NULL)
	{
		prt_error ("()");
	}
	else
	{
		if (el->e->type == n->type)
		{
			print_expression_parens(el->e, false);
		}
		else
		{
			print_expression_parens(el->e, true);
		}
		if (el->next != NULL)
		{
			// prt_error ("\nERROR! Unexpected list!\n");
			/* The SAT parser just naively joins all X_node expressions
			 * using "or", and this check used to give an error due to that,
			 * preventing a convenient debugging.
			 * Just accept it (but mark it with '!'). */
			if (n->type == AND_type) err_msg(lg_Debug, " &! ");
			if (n->type == OR_type) err_msg(lg_Debug, " or! ");
			print_expression_parens(el->next->e, true);
		}
	}

	for (i=0; i<icost; i++) err_msg(lg_Debug, "]");
	if (0 != dcost) err_msg(lg_Debug, COST_FMT, dcost);
	if ((icost == 0) && need_parens) err_msg(lg_Debug, ")");
}

void print_expression(const Exp * n)
{
	print_expression_parens(n, false);
	err_msg(lg_Debug, "\n");
}
#endif /* INFIX_NOTATION */

/* ======================================================================= */

/**
 * Display the information about the given word.
 * If the word can split, display the information about each part.
 * Note that the splits may be invalid grammatically.
 *
 * Wild-card search is supported; the command-line user can type in !!word* or
 * !!word*.sub and get a list of all words that match up to the wild-card.
 * In this case no split is done.
 *
 * FIXME: Errors are printed twice, since display_word_split() is invoked twice
 * per word. One way to fix it is to change display_word_split() to return false
 * on failure. However, this is a big fix, because the failure is several
 * functions deep, all not returning a value or returning a value for another
 * purpose. An easy fix, which has advantages for other things, is to add (and
 * use here) a "char *last_error" field in the Dictionary structure, serving
 * like an "errno" of library calls.
 */

static void display_word_split(Dictionary dict,
                               const char * word,
                               Parse_Options opts,
                               void (*display)(Dictionary, const char *))
{
	Sentence sent;
	struct Parse_Options_s display_word_opts = *opts;

	if ('\0' == word) return; /* avoid trying null strings */

	parse_options_set_spell_guess(&display_word_opts, 0);
	sent = sentence_create(word, dict);
	if (0 == sentence_split(sent, &display_word_opts))
	{
		/* List the splits */
		print_sentence_word_alternatives(sent, false, NULL, NULL);
		/* List the disjuncts information. */
		print_sentence_word_alternatives(sent, false, display, NULL);
	}
	sentence_delete(sent);
}

/**
 * Prints string `s`, aligned to the left, in a field width `w`.
 * If the width of `s` is shorter than `w`, then the remainder of
 * field is padded with blanks (on the right).
 */
static void left_print_string(FILE * fp, const char * s, int w)
{
	int width = w + strlen(s) - utf8_strwidth(s);
	fprintf(fp, "%-*s", width, s);
}

#define DJ_COL_WIDTH sizeof("                         ")

/**
 * Display the number of disjuncts associated with this dict node
 */
static void display_counts(const char *word, Dict_node *dn)
{
	printf("matches:\n");

	for (; dn != NULL; dn = dn->right)
	{
		unsigned int len;
		char * s;
		char * t;

		len = count_disjunct_for_dict_node(dn);
		s = strdup(dn->string);
		t = strrchr(s, SUBSCRIPT_MARK);
		if (t) *t = SUBSCRIPT_DOT;
		printf("    ");
		left_print_string(stdout, s, DJ_COL_WIDTH);
		free(s);
		printf(" %8u  disjuncts ", len);
		if (dn->file != NULL)
		{
			printf("<%s>", dn->file->file);
		}
		printf("\n");
	}
}

/**
 * Display the number of disjuncts associated with this dict node
 */
static void display_expr(const char *word, Dict_node *dn)
{
	printf("expressions:\n");
	for (; dn != NULL; dn = dn->right)
	{
		char * s;
		char * t;

		s = strdup(dn->string);
		t = strrchr(s, SUBSCRIPT_MARK);
		if (t) *t = SUBSCRIPT_DOT;
		printf("    ");
		left_print_string(stdout, s, DJ_COL_WIDTH);
		free(s);
		print_expression(dn->exp);
		if (NULL != dn->right) /* avoid extra newlines at the end */
			printf("\n\n");
	}
}

static void display_word_info(Dictionary dict, const char * word)
{
	const char * regex_name;
	Dict_node *dn_head;

	dn_head = dictionary_lookup_wild(dict, word);
	if (dn_head)
	{
		display_counts(word, dn_head);
		free_lookup(dn_head);
		return;
	}

	/* Recurse, if it's a regex match */
	regex_name = match_regex(dict->regex_root, word);
	if (regex_name)
	{
		display_word_info(dict, regex_name);
		return;
	}
	printf("matches nothing in the dictionary.");
}

static void display_word_expr(Dictionary dict, const char * word)
{
	const char * regex_name;
	Dict_node *dn_head;

	dn_head = dictionary_lookup_wild(dict, word);
	if (dn_head)
	{
		display_expr(word, dn_head);
		free_lookup(dn_head);
		return;
	}

	/* Recurse, if it's a regex match */
	regex_name = match_regex(dict->regex_root, word);
	if (regex_name)
	{
		display_word_expr(dict, regex_name);
		return;
	}
	printf("matches nothing in the dictionary.");
}

/**
 *  dict_display_word_info() - display the information about the given word.
 */
void dict_display_word_info(Dictionary dict, const char * word,
		Parse_Options opts)
{
	display_word_split(dict, word, opts, display_word_info);
}

/**
 *  dict_display_word_expr() - display the connector info for a given word.
 */
void dict_display_word_expr(Dictionary dict, const char * word, Parse_Options opts)
{
	display_word_split(dict, word, opts, display_word_expr);
}
