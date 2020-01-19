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

#include <math.h>                   // fabs

#include "api-structures.h"         // Parse_Options_s  (seems hacky to me)
#include "dict-common.h"
#include "dict-defines.h"
#include "dict-file/word-file.h"
#include "dict-file/read-dict.h"
#include "dict-utils.h"             // copy_Exp
#include "disjunct-utils.h"
#include "print/print.h"
#include "print/print-util.h"
#include "regex-morph.h"

/* ======================================================================== */

bool cost_eq(double cost1, double cost2)
{
	return (fabs(cost1 - cost2) < cost_epsilon);
}

/**
 * Convert cost to a string with at most cost_max_dec_places decimal places.
 */
const char *cost_stringify(double cost)
{
	static TLS char buf[16];

	int l = snprintf(buf, sizeof(buf), "%.*f", cost_max_dec_places, cost);
	if ((l < 0) || (l >= (int)sizeof(buf))) return "ERR_COST";

	return buf;
}

static void print_expression_tag(Dictionary dict, dyn_str *e, const Exp *n)
{
	if ((NULL == dict) || (Exptag_none == n->tag_type)) return;

	dyn_strcat(e, "]");
	dyn_strcat(e, dict->dialect_tag.name[n->tag_id]);
}

static void get_expression_cost(const Exp *e, unsigned int *icost, double *dcost)
{
	if (e->cost < -cost_epsilon)
	{
		*icost = 1;
		*dcost = e->cost;
	}
	else if (cost_eq(e->cost, 0.0))
	{
		/* avoid [X+]-0.00 */
		*icost = 0;
		*dcost = 0;
	}
	else
	{
		*icost = (int) (e->cost);
		*dcost = e->cost - *icost;
		if (*dcost > cost_epsilon)
		{
			*dcost = e->cost;
			*icost = 1;
		}
		else
		{
			if (*icost > 4)
			{
				/* don't print too many [] levels */
				*dcost = *icost;
				*icost = 1;
			}
			else
			{
				*dcost = 0;
			}
		}
	}
}

static bool is_expression_optional(const Exp *e)
{
	Exp *o = e->operand_first;

	return (e->type == OR_type) && (o != NULL) && (o->type == AND_type) &&
	    (NULL == o->operand_first) && (o->cost == 0) &&
	    (o->tag_type = Exptag_none);
}

static void print_expression_parens(Dictionary dict, dyn_str *e,
                                        const Exp *n, bool need_parens)
{
	unsigned int icost;
	double dcost;
	get_expression_cost(n, &icost, &dcost);
	for (unsigned int i = 0; i < icost; i++) dyn_strcat(e, "[");
	if (Exptag_none != n->tag_type) dyn_strcat(e, "[");

	const char *opr = NULL;
	Exp *opd = n->operand_first;

	if (n->type == CONNECTOR_type)
	{
		if (n->multi) dyn_strcat(e, "@");
		dyn_strcat(e, n->condesc ? n->condesc->string : "error-null-connector");
		dyn_strcat(e, (const char []){ n->dir, '\0' });
	}
	else if (is_expression_optional(n))
	{
		dyn_strcat(e, "{");
		if (NULL == opd->operand_next)
			dyn_strcat(e, "error-no-next"); /* unary OR */
		else
			print_expression_parens(dict, e, opd->operand_next, false);
		dyn_strcat(e, "}");
	}
	else
	{
		if (n->type == AND_type)
			opr = " & ";
		else if (n->type == OR_type)
			opr = " or ";
		else
			append_string(e, "error-exp-type-%d", (int)n->type);

		if (opr != NULL)
		{
			/* (opd == NULL) means this is a null expression. */
			if (((icost == 0) && need_parens) || (opd == NULL)) dyn_strcat(e, "(");

			if ((opd == NULL) && (n->type == OR_type))
				dyn_strcat(e, "error-zeroary-or");

			for (Exp *l = opd; l != NULL; l = l->operand_next)
			{
				print_expression_parens(dict, e, l, true);

				if (l->operand_next != NULL)
					dyn_strcat(e, opr);
				else if ((n->type == OR_type) && (l == n->operand_first))
					dyn_strcat(e, " or error-no-next"); /* unary OR */
			}

			if (((icost == 0) && need_parens) || (opd == NULL)) dyn_strcat(e, ")");
		}
	}

	for (unsigned int i = 0; i < icost; i++) dyn_strcat(e, "]");
	if (dcost != 0) dyn_strcat(e, cost_stringify(dcost));
	print_expression_tag(dict, e, n);
}


static const char *lg_exp_stringify_with_tags(Dictionary dict, const Exp *n)
{
	static TLS char *e_str;

	if (e_str != NULL) free(e_str);
	if (n == NULL)
	{
		e_str = NULL;
		return "(null)";
	}

	dyn_str *e = dyn_str_new();
	print_expression_parens(dict, e, n, false);
	e_str = dyn_str_take(e);
	return e_str;
}

/**
 * Stringify the given expression, ignoring tags.
 */
const char *lg_exp_stringify(const Exp *n)
{
	return lg_exp_stringify_with_tags(NULL, n);
}

/* ================ Display word expressions / disjuncts ================= */

const char do_display_expr; /* a sentinel to request an expression display */

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
               const char * word, Parse_Options opts,
               char * (*display)(Dictionary, const char *, const void **),
               const char **arg)
{
	Sentence sent;

	if ('\0' == *word) return NULL; /* avoid trying null strings */

	/* SUBSCRIPT_DOT in a sentence word is not interpreted as SUBSCRIPT_MARK,
	 * and hence a subscripted word that is found in the dict will not
	 * get found in the dict if it can split. E.g: 's.v (the info for s.v
	 * will not be shown). Fix it by replacing it to SUBSCRIPT_MARK. */
	char *pword = strdupa(word);
	patch_subscript(pword);

	dyn_str *s = dyn_str_new();

	int spell_option = parse_options_get_spell_guess(opts);
	parse_options_set_spell_guess(opts, 0);
	sent = sentence_create(pword, dict);
	if (0 == sentence_split(sent, opts))
	{
		/* List the splits */
		print_sentence_word_alternatives(s, sent, false, NULL, NULL, NULL);
		/* List the expression / disjunct information */

		/* Initialize the callback arguments */
		const void *carg[3] = { /*regex*/NULL, /*flags*/NULL, opts };

		Regex_node *rn = NULL;
		if (arg != NULL)
		{
			if (arg[0] == &do_display_expr)
			{
				carg[0] = &do_display_expr;
			}
			else if (arg[0] != NULL)
			{
				/* A regex is specified, which means displaying disjuncts. */
				carg[1] = arg[1]; /* flags */

				if (arg[0][0] != '\0')
				{
					rn = malloc(sizeof(Regex_node));
					rn->name = strdup("Disjunct regex");
					rn->pattern = strdup(arg[0]);
					rn->re = NULL;
					rn->neg = false;
					rn->next = NULL;

					if (compile_regexs(rn, NULL) != 0)
					{
						prt_error("Error: Failed to compile regex \"%s\".\n", arg[0]);
						return strdup(""); /* not NULL (NULL means no dict entry) */
					}

					carg[0] = rn;
				}
			}
		}
		print_sentence_word_alternatives(s, sent, false, display, carg, NULL);
		if (rn != NULL) free_regexs(rn);
	}
	sentence_delete(sent);
	parse_options_set_spell_guess(opts, spell_option);

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
		assert(false, "Unknown expression type %d", (int)e->type);
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

	dyn_strcat(s, "matches:\n");
	for (; dn != NULL; dn = dn->right)
	{
		append_string(s, "    %-*s %8u  disjuncts",
		              display_width(DJ_COL_WIDTH, dn->string), dn->string,
		              count_disjunct_for_dict_node(dn));

		if (dn->file != NULL)
		{
			append_string(s, " <%s>", dn->file->file);
		}
		dyn_strcat(s, "\n\n");
	}
	return dyn_str_take(s);
}

/**
 * Display the expressions associated with this dict node.
 */
static char *display_expr(Dictionary dict, const char *word, Dict_node *dn,
								  const void **arg)
{
	const Parse_Options opts = (Parse_Options)arg[2];

	/* copy_Exp() needs an Exp memory pool. */
	Pool_desc *Exp_pool = pool_new(__func__, "Exp", /*num_elements*/256,
	                               sizeof(Exp), /*zero_out*/false,
	                               /*align*/false, /*exact*/false);

	dyn_str *s = dyn_str_new();
	dyn_strcat(s, "expressions:\n");
	for (; dn != NULL; dn = dn->right)
	{
		Exp *e = copy_Exp(dn->exp, Exp_pool, opts); /* assign dialect costs */
		pool_reuse(Exp_pool);

		const char *expstr = lg_exp_stringify_with_tags(dict, e);

		append_string(s, "    %-*s %s",
		              display_width(DJ_COL_WIDTH, dn->string), dn->string,
		              expstr);
		dyn_strcat(s, "\n\n");
	}

	if (Exp_pool != NULL) pool_delete(Exp_pool);
	return dyn_str_take(s);
}

/**
 * A callback function to display \p word number of disjuncts and file name.
 *
 * @arg Callback args (unused).
 * @return String to display. Must be freed by the caller.
 */
static char *display_word_info(Dictionary dict, const char *word,
                               const void **arg)
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
		return display_word_info(dict, regex_name, arg);
	}

	return NULL;
}

/**
 * A callback function to display \p word expressions or disjuncts.
 * @param arg Callback data as follows:
 *    arg[0]: &do_display_expr or disjunct selection regex.
 *    arg[1]: flags
 *    argv[2]: Parse_Options
 * @return String to display. Must be freed by the caller.
 */
static char *display_word_expr(Dictionary dict, const char *word,
                               const void **arg)
{
	const char * regex_name;
	Dict_node *dn_head;
	char *out = NULL;

	dn_head = dictionary_lookup_wild(dict, word);
	if (dn_head)
	{
		if (arg[0] == &do_display_expr)
		{
			out = display_expr(dict, word, dn_head, arg);
		}
		else
		{
			out = display_disjuncts(dict, dn_head, arg);
		}
		free_lookup_list(dict, dn_head);
		return out;
	}

	/* Recurse, if it's a regex match */
	regex_name = match_regex(dict->regex_root, word);
	if (regex_name)
	{
		return display_word_expr(dict, regex_name, arg);
	}

	return NULL;
}

/**
 * Break word/re/flags into components.
 * /regex/ and flags are optional.
 * \p re and \p flags can be both NULL;
 * @param re[out] the regex component, unless \c NULL.
 * @param flags[out] the flags component, unless \c NULL.
 * @return The word component.
 */
static const char *display_word_extract(char *word, const char **re,
                                        const char **flags)
{
	if (re != NULL) *re = NULL;
	if (flags != NULL) *flags = NULL;

	char *r = strchr(word, '/');
	if (r == NULL) return word;
	*r = '\0';

	if (re != NULL)
	{
		*re = r + 1;
		char *f = strchr(*re, '/');
		if (f != NULL)
		{
			*f = '\0';
			*flags = f + 1;
		}
	}
	return word;
}

/**
 *  dict_display_word_info() - display the information about the given word.
 */
char *dict_display_word_info(Dictionary dict, const char *word,
		Parse_Options opts)
{
	char *wordbuf = strdupa(word);
	word = display_word_extract(wordbuf, NULL, NULL);

	return display_word_split(dict, word, opts, display_word_info, NULL);
}

/**
 *  dict_display_word_expr() - display the connector info for a given word.
 */
char *dict_display_word_expr(Dictionary dict, const char * word, Parse_Options opts)
{
	const char *arg[2];
	char *wordbuf = strdupa(word);
	word = display_word_extract(wordbuf, &arg[0], &arg[1]);

	/* If no regex component, then it's a request to display expressions. */
	if (arg[0] == NULL) arg[0] = &do_display_expr;

	return display_word_split(dict, word, opts, display_word_expr, arg);
}
