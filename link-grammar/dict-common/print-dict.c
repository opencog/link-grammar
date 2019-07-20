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

#include "api-structures.h" // for Parse_Options_s  (seems hacky to me)
#include "dict-common.h"
#include "dict-defines.h"
#include "print/print.h"
#include "print/print-util.h"
#include "regex-morph.h"
#include "dict-file/word-file.h"
#include "dict-file/read-dict.h"


/* ======================================================================== */

#define COST_FMT "%.3f"
/**
 * print the expression, in infix-style
 */
static dyn_str *print_expression_parens(dyn_str *e,
                                        const Exp * n, int need_parens)
{
	Exp *operand;
	int i, icost;
	double dcost;

	if (n == NULL)
	{
		dyn_strcat(e, "NULL expression");
		return e;
	}

	if (n->cost < -10E-4)
	{
		icost = 1;
		dcost = n->cost;
	}
	else if ((n->cost > -10E-4) && (n->cost < 0))
	{
		/* avoid [X+]-0.00 */
		icost = 0;
		dcost = 0;
	}
	else
	{
		icost = (int) (n->cost);
		dcost = n->cost - icost;
		if (dcost > 10E-4)
		{
			dcost = n->cost;
			icost = 1;
		}
		else
		{
			if (icost > 4)
			{
				/* don't print too many [] levels */
				dcost = icost;
				icost = 1;
			}
			else
			{
				dcost = 0;
			}
		}
	}

	/* print the connector only */
	if (n->type == CONNECTOR_type)
	{
		for (i=0; i<icost; i++) dyn_strcat(e, "[");
		if (n->multi) dyn_strcat(e, "@");
		append_string(e, "%s%c", n->condesc?n->condesc->string:"(null)", n->dir);
		for (i=0; i<icost; i++) dyn_strcat(e, "]");
		if (0 != dcost) append_string(e, COST_FMT, dcost);
		return e;
	}

	operand = n->operand_first;
	if (operand == NULL)
	{
		for (i=0; i<icost; i++) dyn_strcat(e, "[");
		dyn_strcat(e, "()");
		for (i=0; i<icost; i++) dyn_strcat(e, "]");
		if (0 != dcost) append_string(e, COST_FMT, dcost);
		return e;
	}

	for (i=0; i<icost; i++) dyn_strcat(e, "[");

	/* look for optional, and print only that */
	if ((n->type == OR_type) && operand && (operand->type == AND_type) &&
	    operand->cost == 0 && (NULL == operand->operand_first))
	{
		dyn_strcat(e, "{");
		if (NULL == operand->operand_next) dyn_strcat(e, "error-no-next");
		 else print_expression_parens(e, operand->operand_next, false);
		dyn_strcat(e, "}");
		for (i=0; i<icost; i++) dyn_strcat(e, "]");
		if (0 != dcost) append_string(e, COST_FMT, dcost);
		return e;
	}

	if ((icost == 0) && need_parens) dyn_strcat(e, "(");

	/* print left side of binary expr */
	print_expression_parens(e, operand, true);

	/* get a funny "and optional" when it's a named expression thing. */
	if ((n->type == AND_type) && (operand->operand_next == NULL))
	{
		for (i=0; i<icost; i++) dyn_strcat(e, "]");
		if (0 != dcost) append_string(e, COST_FMT, dcost);
		if ((icost == 0) && need_parens) dyn_strcat(e, ")");
		return e;
	}

	if (n->type == AND_type) dyn_strcat(e, " & ");
	if (n->type == OR_type) dyn_strcat(e, " or ");

	/* print right side of binary expr */
	operand = operand->operand_next;
	if (operand == NULL)
	{
		if (n->type == OR_type)
			dyn_strcat(e, "error-no-next");
		else
			dyn_strcat(e, "()");
	}
	else
	{
		do
		{
			if (operand->type == n->type)
				{
					print_expression_parens(e, operand, false);
			}
			else
			{
				print_expression_parens(e, operand, true);
			}

			operand = operand->operand_next;
			if (operand != NULL)
			{
				if (n->type == AND_type) dyn_strcat(e, " & ");
				if (n->type == OR_type) dyn_strcat(e, " or ");
			}
		} while (operand != NULL);
	}

	for (i=0; i<icost; i++) dyn_strcat(e, "]");
	if (0 != dcost) append_string(e, COST_FMT, dcost);
	if ((icost == 0) && need_parens) dyn_strcat(e, ")");

	return e;
}

void print_expression(const Exp * n)
{
	dyn_str *e = dyn_str_new();

	char *s = dyn_str_take(print_expression_parens(e, n, false));
	err_msg(lg_Debug, "%s\n", s);
	free(s);
}

const char *expression_stringify(const Exp *n)
{
	dyn_str *e = dyn_str_new();

	return dyn_str_take(print_expression_parens(e, n, false));
}

/* ======================================================================= */

/**
 * Display the information about the given word.
 * If the word can split, display the information about each part.
 * Note that the splits may be invalid grammatically.
 *
 * Wild-card search is supported; the command-line user can type in !!word* or
 * !!word*.sub and get a list of all words that match up to the wild-card.
 * In this case no split is done.
 */
static char *display_word_split(Dictionary dict,
                               const char * word,
                               Parse_Options opts,
                               char * (*display)(Dictionary, const char *))
{
	Sentence sent;
	struct Parse_Options_s display_word_opts = *opts;

	if ('\0' == *word) return NULL; /* avoid trying null strings */

	/* SUBSCRIPT_DOT in a sentence word is not interpreted as SUBSCRIPT_MARK,
	 * and hence a subscripted word that is found in the dict will not
	 * get found in the dict if it can split. E.g: 's.v (the info for s.v
	 * will not be shown). Fix it by replacing it to SUBSCRIPT_MARK. */
	char *pword = strdupa(word);
	patch_subscript(pword);

	dyn_str *s = dyn_str_new();

	parse_options_set_spell_guess(&display_word_opts, 0);
	sent = sentence_create(pword, dict);
	if (0 == sentence_split(sent, &display_word_opts))
	{
		/* List the splits */
		print_sentence_word_alternatives(s, sent, false, NULL, NULL);
		/* List the disjuncts information. */
		print_sentence_word_alternatives(s, sent, false, display, NULL);
	}
	sentence_delete(sent);

	char *out = dyn_str_take(s);
	if ('\0' != out[0]) return out;
	free(out);
	return NULL; /* no dict entry */
}

/**
 * Count the number of clauses (disjuncts) for the expression e.
 * Should return the number of disjuncts that would be returned
 * by build_disjunct().  This in turn should be equal to the number
 * of clauses built by build_clause().
 *
 * Only one minor cheat here: we are ignoring the cost_cutoff, so
 * this potentially over-counts if the cost_cutoff is set low.
 */
static unsigned int count_clause(Exp *e)
{
	unsigned int cnt = 0;

	assert(e != NULL, "count_clause called with null parameter");
	if (e->type == AND_type)
	{
		/* multiplicative combinatorial explosion */
		cnt = 1;
		for (Exp *opd = e->operand_first; opd != NULL; opd = opd->operand_next)
			cnt *= count_clause(opd);
	}
	else if (e->type == OR_type)
	{
		/* Just additive */
		for (Exp *opd = e->operand_first; opd != NULL; opd = opd->operand_next)
			cnt += count_clause(opd);
	}
	else if (e->type == CONNECTOR_type)
	{
		return 1;
	}
	else
	{
		assert(false, "an expression node with no type");
	}

	return cnt;
}

/**
 * Count number of disjuncts given the dict node dn.
 */
static unsigned int count_disjunct_for_dict_node(Dict_node *dn)
{
	return (NULL == dn) ? 0 : count_clause(dn->exp);
}

#define DJ_COL_WIDTH sizeof("                         ")

/**
 * Display the number of disjuncts associated with this dict node
 */
static char *display_counts(const char *word, Dict_node *dn)
{
	dyn_str *s = dyn_str_new();

	append_string(s, "matches:\n");
	for (; dn != NULL; dn = dn->right)
	{
		append_string(s, "    %-*s %8u  disjuncts",
		              display_width(DJ_COL_WIDTH, dn->string), dn->string,
		              count_disjunct_for_dict_node(dn));

		if (dn->file != NULL)
		{
			append_string(s, " <%s>", dn->file->file);
		}
		append_string(s, "\n\n");
	}
	return dyn_str_take(s);
}

/**
 * Display the number of disjuncts associated with this dict node
 */
static char *display_expr(const char *word, Dict_node *dn)
{
	dyn_str *s = dyn_str_new();

	append_string(s, "expressions:\n");
	for (; dn != NULL; dn = dn->right)
	{
		char *expstr = expression_stringify(dn->exp);

		append_string(s, "    %-*s %s",
		              display_width(DJ_COL_WIDTH, dn->string), dn->string,
		              expstr);
		free(expstr);
		append_string(s, "\n\n");
	}
	return dyn_str_take(s);
}

static char *display_word_info(Dictionary dict, const char * word)
{
	const char * regex_name;
	Dict_node *dn_head;

	dn_head = dictionary_lookup_wild(dict, word);
	if (dn_head)
	{
		char *out = display_counts(word, dn_head);
		free_lookup_list(dict, dn_head);
		return out;
	}

	/* Recurse, if it's a regex match */
	regex_name = match_regex(dict->regex_root, word);
	if (regex_name)
	{
		return display_word_info(dict, regex_name);
	}

	return NULL;
}

static char *display_word_expr(Dictionary dict, const char * word)
{
	const char * regex_name;
	Dict_node *dn_head;

	dn_head = dictionary_lookup_wild(dict, word);
	if (dn_head)
	{
		char *out = display_expr(word, dn_head);
		free_lookup_list(dict, dn_head);
		return out;
	}

	/* Recurse, if it's a regex match */
	regex_name = match_regex(dict->regex_root, word);
	if (regex_name)
	{
		return display_word_expr(dict, regex_name);
	}

	return NULL;
}

/**
 *  dict_display_word_info() - display the information about the given word.
 */
char *dict_display_word_info(Dictionary dict, const char * word,
		Parse_Options opts)
{
	return display_word_split(dict, word, opts, display_word_info);
}

/**
 *  dict_display_word_expr() - display the connector info for a given word.
 */
char *dict_display_word_expr(Dictionary dict, const char * word, Parse_Options opts)
{
	return display_word_split(dict, word, opts, display_word_expr);
}
