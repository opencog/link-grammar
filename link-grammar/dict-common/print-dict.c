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

#include <ctype.h>
#include <math.h>                       // fabs

#include "api-structures.h"             // Parse_Options_s  (seems hacky to me)
#include "dict-common.h"
#include "dict-defines.h"
#include "dict-file/word-file.h"
#include "dict-file/read-dict.h"
#include "dict-utils.h"                 // copy_Exp
#include "disjunct-utils.h"
#include "prepare/build-disjuncts.h"    // build_disjuncts_for_exp
#include "print/print.h"
#include "print/print-util.h"
#include "regex-morph.h"
#include "tokenize/tokenize.h"          // word_add
#include "tokenize/word-structures.h"   // Word_struct
#include "utilities.h"                  // GNU_UNUSED
/* ======================================================================== */

bool cost_eq(float cost1, float cost2)
{
	return (fabs(cost1 - cost2) < cost_epsilon);
}

/**
 * Convert cost to a string with at most cost_max_dec_places decimal places.
 */
const char *cost_stringify(float cost)
{
	static TLS char buf[16];

	int l = snprintf(buf, sizeof(buf), "%.*f", cost_max_dec_places, cost);
	if ((l < 0) || (l >= (int)sizeof(buf))) return "ERR_COST";

	return buf;
}

/* Check for an existing newline before issuing "\n" in order to prevent
 * empty lines when printing connector macros. This allows to use the
 * same print_expression_tag_*() function for printing expressions
 * with macros and also disjunct connectors with macros. */
static void dyn_ensure_empty_line(dyn_str *e)
{
	if (dyn_strlen(e) > 0)
	{
		dyn_trimback(e);
		if ((dyn_str_value(e)[dyn_strlen(e)-1]) != '\n')
			dyn_strcat(e, "\n");
	}
}

#define MACRO_INDENTATION 4

static void print_expression_tag_start(Dictionary dict, dyn_str *e, const Exp *n,
                                 int *indent)
{
	if (n->type == CONNECTOR_type) return;
	if (NULL == dict) return;

	switch (n->tag_type)
	{
		case Exptag_none:
			break;
		case Exptag_dialect:
			dyn_strcat(e, "[");
			break;
		case Exptag_macro:
			if (*indent < 0) break;
			dyn_ensure_empty_line(e);
			for(int i = 0; i < *indent; i++) dyn_strcat(e, " ");
			dyn_strcat(e, dict->macro_tag->name[n->tag_id]);
			dyn_strcat(e, ": ");
			*(indent) += MACRO_INDENTATION;
			break;
		default:
			for(int i = 0; i < *indent; i++) dyn_strcat(e, " ");
			append_string(e, "Unknown tag type %d: ", (int)n->tag_type);
			*(indent) += MACRO_INDENTATION;
	}
}

static void print_expression_tag_end(Dictionary dict, dyn_str *e, const Exp *n,
                                 int *indent)
{
	if (n->type == CONNECTOR_type) return;
	if (NULL == dict) return;

	switch (n->tag_type)
	{
		case Exptag_none:
			break;
		case Exptag_dialect:
			dyn_strcat(e, "]");
			dyn_strcat(e, dict->dialect_tag.name[n->tag_id]);
			break;
		case Exptag_macro:
			if (*indent < 0) break;
			dyn_ensure_empty_line(e);
			for(int i = 0; i < *indent - MACRO_INDENTATION/2; i++)
				dyn_strcat(e, " ");
			(*indent) -= MACRO_INDENTATION;
			break;
		default:
			/* Handled in print_expression_tag_start(). */
			;
	}
}

static void get_expression_cost(const Exp *e, unsigned int *icost, float *dcost)
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
	    (o->tag_type == Exptag_none);
}

static void print_expression_parens(Dictionary dict, dyn_str *e, const Exp *n,
                                    bool need_parens, int *indent)
{
	unsigned int icost;
	float dcost;
	get_expression_cost(n, &icost, &dcost);
	for (unsigned int i = 0; i < icost; i++) dyn_strcat(e, "[");
	print_expression_tag_start(dict, e, n, indent);

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
			print_expression_parens(dict, e, opd->operand_next, false, indent);
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
				print_expression_parens(dict, e, l, true, indent);

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
	print_expression_tag_end(dict, e, n, indent);
}

/**
 * Find if the given connector is in the given expression.
 * @param e Expression.
 * @param find_pos Connector position to search in \p e.
 * @param pos Temporary connector position while the search advances.
 * Return \c true iff the connector is found.
 */
static bool exp_contains_connector(const Exp *e, int *pos, int find_pos)
{
	if (NULL == e) return false;

	if (CONNECTOR_type == e->type)
	{
#if 0
		printf("exp_contains_connector: pos=%d C=%s%s%c %s\n",
		       *pos,e->multi?"@":"",e->condesc->string,e->dir,
		       (find_pos == *pos) ? "FOUND" : "");
#endif
		return (find_pos == (*pos)++);
	}

	for(Exp *opd = e->operand_first; opd != NULL; opd = opd->operand_next)
	{
		if (exp_contains_connector(opd, pos, find_pos))
			return true;
	}

	return false;
}

typedef struct
{
	Dictionary dict;
	dyn_str *e;
	int indent;
	int pos;                 /* current connector position in expression */
	int *find_pos;           /* disjunct expression connectors positions */
	bool is_after_connector; /* an indicator for printing '&' */
} cmacro_context;

/**
 * Print nested macros for each desired connector.
 * The desired connector positions are in find_pos[] (ascending sorted,
 * -1 terminated). Its first element holds the next position of the next
 *  connector to print, and is chopped after the connector is printed.
 */
static void print_connector_macros(cmacro_context *cmc, const Exp *n)
{
	if ((cmc->find_pos)[0] == -1)
		return; /* fast termination when nothing more to do */

	bool macro_started = false;
	int current_pos = cmc->pos;
	if ((Exptag_macro == n->tag_type) &&
	    exp_contains_connector(n, &current_pos, (cmc->find_pos)[0]))
	{
		if (cmc->is_after_connector)
		{
			dyn_strcat(cmc->e, " & ");
			cmc->is_after_connector = false;
		}
		print_expression_tag_start(cmc->dict, cmc->e, n, &cmc->indent);
		macro_started = true;
	}

	Exp *opd = n->operand_first;

	if (n->type == CONNECTOR_type)
	{
		if ((cmc->find_pos)[0] == cmc->pos)
		{
			if (cmc->is_after_connector) dyn_strcat(cmc->e, " & ");
			cmc->is_after_connector = true;
			if (n->multi) dyn_strcat(cmc->e, "@");
			dyn_strcat(cmc->e,
			           n->condesc ? n->condesc->string : "error-null-connector");
			dyn_strcat(cmc->e, (const char []){ n->dir, '\0' });
			cmc->find_pos++; /* each expression position is used only once */
		}
		cmc->pos++;
	}
	else
	{
		for (Exp *l = opd; l != NULL; l = l->operand_next)
		{
			print_connector_macros(cmc, l);
		}
	}

	/* The -1 check is to suppress unneeded newlines at the end. */
	if (macro_started && ((cmc->find_pos)[0] != -1))
		print_expression_tag_end(cmc->dict, cmc->e, n, &cmc->indent);
}

/* Note: The returned value is to be freed by the caller. */
static char *lg_exp_stringify_with_tags(Dictionary dict, const Exp *n,
                                        bool show_macros)
{
	if (n == NULL) return strdup("(null)");

	int indent = show_macros ? 0 : -1;

	dyn_str *e = dyn_str_new();
	print_expression_parens(dict, e, n, false, &indent);
	return dyn_str_take(e);
}

/**
 * Stringify the given expression, ignoring tags.
 * Note: The returned value is to be freed by the caller.
 */
char *lg_exp_stringify(const Exp *n)
{
	return lg_exp_stringify_with_tags(NULL, n, false);
}

/* For usage convenience when assigning to a temporary variable + free() is
 * cumbersome. Care should be taken due to the static memory of the result. */
const char *exp_stringify(const Exp *n)
{
	static TLS char *s;

	free(s);
	s = NULL;
	if (n == NULL) return ("(null)");
	s = lg_exp_stringify_with_tags(NULL, n, false);
	return s;
}

#ifdef DEBUG
/* There is a much better lg_exp_stringify() elsewhere
 * This one is for low-level debug. */
GNUC_UNUSED void prt_exp(Exp *e, int i)
{
	if (e == NULL) return;

	for(int j =0; j<i; j++) printf(" ");
	printf ("type=%d dir=%c multi=%d cost=%s\n",
	        e->type, e->dir, e->multi, cost_stringify(e->cost));
	if (e->type != CONNECTOR_type)
	{
		for (e = e->operand_next; e != NULL; e = e->operand_next) prt_exp(e, i+2);
	}
	else
	{
		for(int j =0; j<i; j++) printf(" ");
		printf("con=%s\n", e->condesc->string);
	}
}
#endif

static const char *stringify_Exp_type(Exp_type type)
{
	static TLS char unknown_type[32] = "";
	const char *type_str;

	if (type > 0 && type <= 3)
	{
		type_str = ((const char *[]) {"OR", "AND", "CONNECTOR"}) [type-1];
	}
	else
	{
		snprintf(unknown_type, sizeof(unknown_type), "unknown_type-%d",
		         (int)(type));
		type_str = unknown_type;
	}

	return type_str;
}

static const char *stringify_Exp_tag(Exp *e, Dictionary dict)
{
	static TLS char tag_info[64];

	if (e->type == CONNECTOR_type) return "";

		switch (e->tag_type)
		{
			case Exptag_none:
				return "";
			case Exptag_dialect:
				if (dict == NULL)
				{
					snprintf(tag_info, sizeof(tag_info), " dialect_tag=%u",
					         (unsigned int)e->tag_id);
				}
				else
				{
					snprintf(tag_info, sizeof(tag_info), " dialect_tag=%s",
					       dict->dialect_tag.name[e->tag_id]);
				}
				break;
			case Exptag_macro:
				if (dict == NULL)
				{
					snprintf(tag_info, sizeof(tag_info), " macro_tag");
				}
				else
				{
					snprintf(tag_info, sizeof(tag_info), " macro_tag=%s",
					       dict->macro_tag->name[e->tag_id]);
				}
				break;
			default:
				snprintf(tag_info, sizeof(tag_info), " unknown_tag_type-%d",
				         (int)(e->tag_type));
				;
		}

	return tag_info;
}

static bool is_ASAN_uninitialized(uintptr_t a)
{
	static const uintptr_t asan_uninitialized = (uintptr_t)0xbebebebebebebebeULL;

	return (a == asan_uninitialized);
}

void prt_exp_all(dyn_str *s, Exp *e, int i, Dictionary dict)
{
	if (is_ASAN_uninitialized((uintptr_t)e))
	{
		dyn_strcat(s, "e=UNINITIALIZED\n");
		return;
	}
	if (e == NULL) return;

	for(int j =0; j<i; j++) dyn_strcat(s, " ");
	append_string(s, "e=%p: %s", e, stringify_Exp_type(e->type));

	if (is_ASAN_uninitialized((uintptr_t)e->operand_first))
		dyn_strcat(s, " (UNINITIALIZED operand_first)");
	if (is_ASAN_uninitialized((uintptr_t)e->operand_next))
		dyn_strcat(s, " (UNINITIALIZED operand_next)");

	if (e->type != CONNECTOR_type)
	{
		int operand_count = 0;
		for (Exp *opd = e->operand_first; NULL != opd; opd = opd->operand_next)
		{
			operand_count++;
			if (is_ASAN_uninitialized((uintptr_t)opd->operand_next))
			{
				append_string(s, " (operand %d: UNINITIALIZED operand_next)\n",
				              operand_count);
				return;
			}
		}
		append_string(s, " (%d operand%s) cost=%s%s\n", operand_count,
		       operand_count == 1 ? "" : "s", cost_stringify(e->cost),
		       stringify_Exp_tag(e, dict));
		for (Exp *opd = e->operand_first; NULL != opd; opd = opd->operand_next)
		{
			prt_exp_all(s, opd, i+2, dict);
		}
	}
	else
	{
		append_string(s, " %s%s%c cost=%s%s\n",
		              e->multi ? "@" : "",
		              e->condesc ? e->condesc->string : "(condesc=(null))",
		              e->dir, cost_stringify(e->cost),
		              stringify_Exp_tag(e, dict));
	}
}

#ifdef DEBUG
GNUC_UNUSED void prt_exp_mem(Exp *e)
{
	dyn_str *s = dyn_str_new();

	prt_exp_all(s, e, 0, NULL);
	char *e_str = dyn_str_take(s);
	printf("%s", e_str);
	free(e_str);
}
#endif

/* ================ Print disjuncts and connectors ============== */
static bool is_flag(uint32_t flags, char flag)
{
	return (flags>>(flag-'a')) & 1;
}

static uint32_t make_flags(const char *flags)
{
	if (flags == NULL) return 0;
	uint32_t r  = 0;

	for (const char *f = flags; *f != '\0'; f++)
		r |= (1<<(*f-'a'));

	return r;
}

/* Print one connector with all the details.
 * mCnameD<tracon_id>{refcount}(nearest_word, length_limit)x
 * Optional m: "@" for multi (else nothing).
 * Cname: Connector name.
 * Optional D: "-" / "+" (if dir != -1).
 * Optional <tracon_id>: (flag 't').
 * Optional [nearest_word, length_limit or farthest_word]: (flag 'l').
 * x: Shallow/deep indication as "s" / "d"
 */
static void dyn_print_one_connector(dyn_str *s, const Connector *e, int dir,
                                    uint32_t flags)
{
	if (e->multi)
		dyn_strcat(s, "@");
	dyn_strcat(s, (e->desc != NULL) ? connector_string(e) : "NULLDESC");
	if (-1 != dir) dyn_strcat(s, (dir == 0) ? "-" : "+");
	if (is_flag(flags, 't') && e->tracon_id)
		append_string(s, "<%d>", e->tracon_id);
	if (is_flag(flags, 'r') && e->refcount)
		append_string(s, "{%d}",e->refcount);
	if (is_flag(flags, 'l'))
		append_string(s, "(%d,%d)", e->nearest_word, e->farthest_word);
#if 0
	append_string(s, "<<%d>>", e->exp_pos);
#endif
	if (is_flag(flags, 's'))
		dyn_strcat(s, e->shallow ? "s" : "d");
}

void print_one_connector(const Connector *e, const char *flags)
{
	dyn_str *s = dyn_str_new();
	int dir = -1;

	if (flags == NULL) flags = "lt";
	if (*flags == '-') { flags++; dir = 0; }
	if (*flags == '+') { flags++; dir = 1; }

	uint32_t int_flags = make_flags(flags);
	dyn_print_one_connector(s, e, dir, int_flags);

	char *t = dyn_str_take(s);
	puts(t);
	free(t);
}

static void dyn_print_connector_list(dyn_str *s, const Connector *e, int dir,
                                     uint32_t flags)
{

	if (e == NULL) return;
	dyn_print_connector_list(s, e->next, dir, flags);
	if (e->next != NULL) dyn_strcat(s, " ");
	dyn_print_one_connector(s, e, dir, flags);
}

/* Special flags here: Initial "-" or "+" for direction sign. */

char *print_connector_list_str(const Connector *e, const char *flags)
{
	dyn_str *s = dyn_str_new();
	int dir = -1;

	if (flags == NULL) flags = "lt";
	if (*flags == '-') { flags++; dir = 0; }
	if (*flags == '+') { flags++; dir = 1; }

	dyn_print_connector_list(s, e, dir, make_flags(flags));

	return dyn_str_take(s);
}

char *print_one_connector_str(const Connector *e, const char *flags)
{
	dyn_str *s = dyn_str_new();
	int dir = -1;

	if (flags == NULL) flags = "lt";
	if (*flags == '-') { flags++; dir = 0; }
	if (*flags == '+') { flags++; dir = 1; }

	uint32_t int_flags = make_flags(flags);
	dyn_print_one_connector(s, e, dir, int_flags);

	return dyn_str_take(s);
}

/* Ascending sort of connector positions. */
static int ascending_int(const void *a, const void *b)
{
	const int a1 = *(const int *)a;
	const int b1 = *(const int *)b;

	if (a1 <  b1) return -1;
	if (a1 == b1) return 0;
	return 1;
}

typedef struct
{
	const Regex_node *regex;
	Exp *exp;
	Dictionary dict;
	unsigned int num_selected;
	unsigned int num_tunnels;
} select_data;

static void dyn_print_disjunct_list(dyn_str *s, const Disjunct *dj,
              uint32_t flags,
              bool (* select)(const char *dj_str, select_data *criterion),
              select_data *criterion)
{
	int djn = 0;
	char word[MAX_WORD + 32];
	int max_ccnt = 0;
	int *exp_pos = NULL;
	bool print_disjunct_address = test_enabled("disjunct-address");

	for (;dj != NULL; dj=dj->next)
	{
		if (dj->is_category == 0)
		{
			lg_strlcpy(word, dj->word_string, sizeof(word));
		}
		else
		{
			int n = snprintf(word, sizeof(word), "%x", dj->category[0].num);
			if (dj->num_categories > 1)
				snprintf(word + n, sizeof(word) - n, " (%u)", dj->num_categories);
		}
		patch_subscript_mark(word);
		dyn_str *l = dyn_str_new();

		append_string(l, "%16s", word);
		if (print_disjunct_address) append_string(s, "(%p)", dj);
		dyn_strcat(l, ": ");

		const char *cost_str = cost_stringify(dj->cost);
		append_string(l, "[%d]%s%s= ", djn++, &" "[(*cost_str == '-')], cost_str);
		dyn_print_connector_list(l, dj->left, /*dir*/0, flags);
		dyn_strcat(l, " <> ");
		dyn_print_connector_list(l, dj->right, /*dir*/1, flags);

		char *ls = dyn_str_take(l);

		if ((NULL == select) || select(ls, criterion))
		{
			dyn_strcat(s, ls);
			dyn_strcat(s, "\n");

			if ((criterion != NULL) && (criterion->exp != NULL))
			{
				int ccnt = 1; /* 1 for exp_pos -1 terminator. */
				for (Connector *c = dj->left; c != NULL; c = c->next)
					ccnt++;
				for (Connector *c = dj->right; c != NULL; c = c->next)
					ccnt++;

				if (ccnt > max_ccnt)
				{
					max_ccnt = (ccnt == 0) ? 32 : ccnt;
					exp_pos = alloca(max_ccnt * sizeof(int));
				}
				int *i = exp_pos;

				for (Connector *c = dj->left; c != NULL; c = c->next)
					*i++ = c->exp_pos;
				for (Connector *c = dj->right; c != NULL; c = c->next)
					*i++ = c->exp_pos;
				*i = -1;

				qsort(exp_pos, ccnt-1, sizeof(int), ascending_int);

				cmacro_context cmc = {
					.dict = criterion->dict,
					.e = s,
					.find_pos = exp_pos,
				};
				print_connector_macros(&cmc, criterion->exp);
				dyn_strcat(s, "\n\n");
			}
		}
		free(ls);
	}
}

void print_disjunct_list(const Disjunct *d, const char *flags)
{
	dyn_str *s = dyn_str_new();

	if (flags == NULL) flags = "lt";
	uint32_t int_flags = make_flags(flags);

	dyn_print_disjunct_list(s, d, int_flags, NULL, NULL);
	char *t = dyn_str_take(s);
	puts(t);
	free(t);
}

void print_all_disjuncts(Sentence sent)
{
	dyn_str *s = dyn_str_new();
	uint32_t int_flags = make_flags("lt");

	for (WordIdx w = 0; w < sent->length; w++)
	{
		append_string(s, "Word %zu:\n", w);
		dyn_print_disjunct_list(s, sent->word[w].d, int_flags, NULL, NULL);
	}

	char *t = dyn_str_take(s);
	puts(t);
	free(t);
}

/* ================ Display word expressions / disjuncts ================= */
#define DJ_COL_WIDTH sizeof("                         ")

static bool select_disjunct(const char *dj_str, select_data *criterion)
{
	/* Count number of disjuncts with tunnel connectors. */
	for (const char *p = dj_str; *p != '\0'; p++)
	{
		if ((p[0] == ' ') && (p[1] == 'x'))
		{
			criterion->num_tunnels++;
			break;
		}
	}

	/* Select desired disjuncts. If several connectors need to match in any
	 * order, the Regex_node list contains more than one component, and all
	 * of them must match. A horrible hack is done below to achieve that. */
	for (Regex_node *rn = (Regex_node *)criterion->regex; rn != NULL;
	     rn = rn->next)
	{
		Regex_node *savenext = rn->next;
		rn->next = NULL;
		if (match_regex(rn , dj_str) == NULL)
		{
			rn->next = savenext;
			return false;
		}
		rn->next = savenext;
	}

	criterion->num_selected++;
	return true;
}

/**
 * Display the disjuncts of expressions in \p dn.
 */
static char *display_disjuncts(Dictionary dict, const Dict_node *dn,
                               const void **arg)
{
	const Regex_node *rn = arg[0];
	const char *flags = arg[1];
	const Parse_Options opts = (Parse_Options)arg[2];
	float max_cost = opts->disjunct_cost;
	uint32_t int_flags = make_flags(flags);;

	/* build_disjuncts_for_exp() needs memory pools for efficiency. */
	Sentence dummy_sent = sentence_create("", dict); /* For memory pools. */
	dummy_sent->Disjunct_pool = pool_new(__func__, "Disjunct",
	                               /*num_elements*/8192, sizeof(Disjunct),
	                               /*zero_out*/false, /*align*/false, false);
	dummy_sent->Connector_pool = pool_new(__func__, "Connector",
	                              /*num_elements*/65536, sizeof(Connector),
	                              /*zero_out*/true, /*align*/false, false);

	select_data criterion = { .regex = rn };
	void *select = (rn == NULL) ? NULL : select_disjunct;

	dyn_str *s = dyn_str_new();
	dyn_strcat(s, "disjuncts:\n");
	for (; dn != NULL; dn = dn->right)
	{
		/* Use copy_Exp() to assign dialect cost. */
		Exp *e = copy_Exp(dn->exp, dummy_sent->Exp_pool, opts);
		Disjunct *d = build_disjuncts_for_exp(dummy_sent, e, dn->string, NULL,
		                                      max_cost, NULL);

		unsigned int dnum0 = count_disjuncts(d);
		d = eliminate_duplicate_disjuncts(d, false);
		unsigned int dnum1 = count_disjuncts(d);

		if ((flags != NULL) && (strchr(flags, 'm') != NULL))
		{
			criterion.exp = e;
			criterion.dict = dict;
		}
		criterion.num_selected = 0;
		dyn_str *dyn_pdl = dyn_str_new();
		dyn_print_disjunct_list(dyn_pdl, d, int_flags, select, &criterion);
		char *dliststr = dyn_str_take(dyn_pdl);

		pool_reuse(dummy_sent->Exp_pool);
		pool_reuse(dummy_sent->Disjunct_pool);
		pool_reuse(dummy_sent->Connector_pool);

		append_string(s, "    %-*s %8u/%u disjuncts",
		              display_width(DJ_COL_WIDTH, dn->string), dn->string,
		              dnum1, dnum0);
		if (criterion.num_tunnels != 0)
			append_string(s, " (%u tunnels)", criterion.num_tunnels);
		dyn_strcat(s, "\n\n");
		dyn_strcat(s, dliststr);
		dyn_strcat(s, "\n");
		free(dliststr);

		if (rn != NULL)
		{
			if (criterion.num_selected == dnum1)
				dyn_strcat(s, "(all the disjuncts matched)\n\n");
			else
				append_string(s, "(%u disjunct%s matched)\n\n",
				              criterion.num_selected,
				              criterion.num_selected == 1 ? "" : "s");
		}
	}
	sentence_delete(dummy_sent);

	return dyn_str_take(s);
}

static void notify_ignoring_flag(const char *flag)
{
	if (flag != NULL) prt_error("Warning: Ignoring flag \"%c\".\n", *flag);
}

/**
 * Copy bytes from \p src to \p dst, while quoting with \c '\\'
 * the bytes in \p q.
 * Note: This is not a general-purpose function, since:
 * - It assumes \p dst has enough space.
 * - \p src is not checked for NUL.
 * - No NUL termination is done.
 *
 * @param src Source buffer.
 * @param dst[out] Destination buffer.
 * @return Number of bytes added to \p dst.
 */
static size_t copy_quoted(const char *q, char *dst, const char *src, size_t len)
{
	const char *orig_dst = dst;
	for (; len-- > 0; src++, dst++)
	{
		if (strchr(q, *src) != NULL) *dst++ = '\\';
		*dst = *src;
	}

	return (size_t)(dst - orig_dst);
}

static Regex_node *new_disjunct_regex_node(Regex_node *current, char *regpat)
{
	Regex_node *rn = malloc(sizeof(Regex_node));

	rn->name = "Disjunct regex";
	rn->pattern = strdup(regpat);
	rn->re = NULL;
	rn->neg = false;
	rn->next = current;

	return rn;
}

static Regex_node *make_disjunct_pattern(const char *pattern, const char *flags)
{
	if (NULL == flags) flags = "";
	const char *is_full = strchr(flags, 'f');
	const char *is_regex = strchr(flags, 'r');
	const char *is_anyorder = strchr(flags, 'a');

	char *regpat; /* constructed regex pattern */
	size_t pat_len = strlen(pattern);
	Regex_node *rn = NULL;

	if ('\0' == pattern[strspn(pattern, "0123456789")])
	{
		notify_ignoring_flag(is_regex);
		notify_ignoring_flag(is_full);
		notify_ignoring_flag(is_anyorder);

		const size_t added_chars_len = sizeof("\\[]");
		regpat = alloca(pat_len + added_chars_len);
		strcpy(regpat, "\\[");
		strcat(regpat, pattern);
		strcat(regpat, "]");
	}
	else if (is_full != NULL)
	{
		notify_ignoring_flag(is_regex);
		notify_ignoring_flag(is_anyorder);
		/* Assume this is the complete specification of the disjunct, e.g.
		 * as copied from the !disjuncts output. Insert "<>" after the LHS
		 * connectors and build a search regex. Extra spaces are not
		 * supported (and are not checked). */
		for (size_t i = 0; i < pat_len; i++)
		{
			const char *c =  &pattern[i];
			if (!isalnum((unsigned char)*c) && (strchr("*+- ", *c) == NULL))
			{
				prt_error("Warning: Invalid character \"%.*s\" in full "
				          "disjunct specification.\n",
				          (utf8_charlen(c) < 0) ? 0 : utf8_charlen(c), c);
			}
		}

		size_t rhs_pos = strcspn(pattern, "+");

		if ('\0' != pattern[rhs_pos])
		{
			for (rhs_pos--; rhs_pos != 0; rhs_pos--)
				if (' ' == pattern[rhs_pos]) break;
			if (' ' == pattern[rhs_pos]) rhs_pos++;
		}

		/* Note: This section is sensitive to the disjunct print format. */
		const size_t added_chars_len = sizeof("= <> $");
		const size_t regpat_size = 2 * (pat_len + added_chars_len);
		regpat = alloca(regpat_size);

		size_t dst_pos = lg_strlcpy(regpat, "= ", regpat_size);
		if (rhs_pos == 0)
			regpat[dst_pos++] = ' '; /* when no LHS, we need "=  " */
		else
			dst_pos += copy_quoted("*+", regpat+dst_pos, pattern, rhs_pos);
		if (regpat[dst_pos-1] != ' ') regpat[dst_pos++] = ' ';
		dst_pos += lg_strlcpy(regpat + dst_pos, "<> ", regpat_size - dst_pos);
		dst_pos += copy_quoted("*+", regpat+dst_pos, pattern+rhs_pos,
		                       pat_len-rhs_pos);
		regpat[dst_pos++] = '$';
		regpat[dst_pos++] = '\0';
	}
	else
	{
		if ((is_regex != NULL) || pattern[strcspn(pattern, "({[.?$\\")] != '\0')
		{
			regpat = strdupa(pattern);
			is_regex = "r";
		}
		else
		{
			/* No regex flag and no regex characters (* and + are ignored). */
			regpat = alloca(2 * pat_len + 1);
			size_t ncopied = copy_quoted("*+", regpat, pattern, pat_len);
			regpat[ncopied] = '\0';
		}

		if (is_anyorder != NULL)
		{
			/* Assume this is an unordered list of blank-separated connectors. */
			char *constring;
			while ((constring = strtok_r(regpat, " ", &regpat)))
			{
				if (is_regex == NULL)
				{
					/* Arrange for matching only whole connectors. */
					const char added_chars[] = " ( |$)";
					char *word_boundary_constring =
						alloca(strlen(constring) + sizeof(added_chars));
					word_boundary_constring[0] = ' ';
					strcpy(word_boundary_constring+1, constring);
					strcat(word_boundary_constring+1, added_chars+1);
					constring = word_boundary_constring;

				}
				rn = new_disjunct_regex_node(rn, constring);
			}
		}
	}

	if (NULL == rn)
		rn = new_disjunct_regex_node(NULL, regpat);

	if (!compile_regexs(rn, NULL))
		return NULL; /* compile_regexs() issues the error message */

	return rn;
}

const char do_display_expr; /* a sentinel to request an expression display */

static bool validate_flags(const char *display_type, const char *flags)
{
	const char *known_flags;

	if (&do_display_expr == display_type)
		known_flags = "lm";
	else
		known_flags = "afmr";

	size_t unknown_flag_pos = strspn(flags, known_flags);
	if (flags[unknown_flag_pos] != '\0')
	{
		prt_error("Error: Token display: Unknown flag \"%c\".\n",
		          flags[unknown_flag_pos]);
		if (&do_display_expr == display_type)
		{
			prt_error("Valid flags for the \"!!word/\" command "
			          "(show expression):\n"
			          "l - low level expression details.\n"
			          "m - macro context.\n");
		}
		else
		{
			prt_error("Valid flags for the \"!!word//\" command "
			          "(show disjuncts):\n"
			          "a - any connector order.\n"
			          "f - full disjunct specification.\n"
			          "m - macro context for connectors.\n"
			          "r - regex pattern (automatically detected usually).\n");
		}

		return false;
	}

	return true;
}

/**
 * Display the information about the given word.
 * If the word can split, display the information about each part.
 * Note that the splits may be invalid grammatically.
 *
 * Wild-card search is supported; the command-line user can type in !!word* or
 * !!word*.sub and get a list of all words that match up to the wild-card.
 * In this case no split is done.
 *
 * @param arg arg[0] specifies the display type. If it is equal to
 * &do_display_expr than this is request to display expressions. Else it
 * is a request to display disjuncts. arg[2] specifies the request flags.
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

	if (pword[0] == '<' && (strchr(pword, '>') != NULL) &&
	    ((strchr(pword, '>')[1] == '\0') ||
	     (strchr(pword, '>')[1] == SUBSCRIPT_MARK)))
	{
		/* Dictionary macro - don't split. */
		if (!word0_set(sent, pword, opts)) goto display_word_split_error;
	}
	else
	{
		if (0 != sentence_split(sent, opts)) goto display_word_split_error;
	}

	/* List the splits */
	print_sentence_word_alternatives(s, sent, false, NULL, NULL, NULL);
	/* List the expression / disjunct information */

	/* Initialize the callback arguments */
	const void *carg[3] = { /*regex*/NULL, /*flags*/NULL, opts };

	Regex_node *rn = NULL;
	if (arg != NULL)
	{
		if (NULL != arg[1])
		{
			if (!validate_flags(arg[0], arg[1]))
			{
				dyn_strcat(s, " "); /* avoid no-expression error */
				goto display_word_split_error;
			}
		}

		carg[1] = arg[1]; /* flags */

		if (arg[0] == &do_display_expr)
		{
			carg[0] = &do_display_expr;
		}
		else if (arg[0] != NULL)
		{
			/* A pattern is specified, which means displaying disjuncts. */
			if (arg[0][0] != '\0')
			{
				rn = make_disjunct_pattern(arg[0], arg[1]);
				if (NULL == rn)
				{
					dyn_strcat(s, " "); /* avoid a no-match error */
					goto display_word_split_error;
				}
				carg[0] = rn;
			}
		}
	}
	print_sentence_word_alternatives(s, sent, false, display, carg, NULL);
	if (rn != NULL) free_regexs(rn);

display_word_split_error:
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
static uint64_t count_clause(const Exp *e)
{
	uint64_t cnt = 0;

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
static uint64_t count_disjunct_for_dict_node(Dict_node *dn)
{
	return (NULL == dn) ? 0 : count_clause(dn->exp);
}

/**
 * Display the number of disjuncts associated with this dict node
 */
static char *display_counts(const char *word, Dict_node *dn)
{
	dyn_str *s = dyn_str_new();

	dyn_strcat(s, "matches:\n");
	for (; dn != NULL; dn = dn->right)
	{
		append_string(s, "    %-*s %8zu  disjuncts",
		              display_width(DJ_COL_WIDTH, dn->string), dn->string,
		              count_disjunct_for_dict_node(dn));

		if (dn->file != NULL)
		{
			append_string(s, " <%s>", dn->file);
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
	const char *flags = arg[1];
	const Parse_Options opts = (Parse_Options)arg[2];
	bool show_macros = ((flags != NULL) && (strchr(flags, 'm') != NULL));
	bool low_level = ((flags != NULL) && (strchr(flags, 'l') != NULL));

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

		if (low_level)
		{
			append_string(s, "    %s\n", dn->string);
			prt_exp_all(s, e, 0, dict);
			dyn_strcat(s, "\n\n");
		}

		char *expstr = lg_exp_stringify_with_tags(dict, e, show_macros);

		append_string(s, "    %-*s %s",
		              display_width(DJ_COL_WIDTH, dn->string), dn->string,
		              expstr);
		dyn_strcat(s, "\n\n");
		free(expstr);
	}

	if (Exp_pool != NULL) pool_delete(Exp_pool);
	return dyn_str_take(s);
}

/**
 * A callback function to display \p word number of disjuncts and file name.
 *
 * @param arg Callback args (unused).
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
 *    arg[2]: Parse_Options
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

static char *find_unescaped_slash(char *word)
{
	size_t len = strlen(word);
	for (char *src = word, *dst = word; *src != '\0'; src++, dst++)
	{
		if (('\\' == *src) && (('\\' == src[1]) || ('/' == src[1])))
		{
			memmove(dst, src+1, len - (src - word));
		}
		else
		{
			if ('/' == *src) return dst;
		}
	}
	return NULL;
}

/**
 * Break "word", "word/flags" or "word/regex/flags" into components.
 * "regex" and "flags" are optional.  "word/" means an empty regex.
 * \p re and \p flags can be both NULL.
 * @param re[out] the regex component, unless \c NULL.
 * @param flags[out] the flags component, unless \c NULL.
 * @return The word component.
 */
static const char *display_word_extract(char *word, const char **re,
                                        const char **flags)
{
	if (re != NULL) *re = NULL;
	if (flags != NULL) *flags = NULL;

	char *r = find_unescaped_slash(word);

	if (r == NULL) return word;
	*r = '\0';

	if (re != NULL)
	{
		char *f = find_unescaped_slash(r + 1);
		if (f != NULL)
		{
			*re = r + 1;
			*f = '\0';
			*flags = f + 1;  /* disjunct display flags */
		}
		else
		{
			*flags = r + 1;  /* expression display flags */
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
	if ('\0' == *word)
	{
		prt_error("Error: Missing word argument.\n");
		return strdup(" ");
	}

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
	if ('\0' == *word) return strdup(" ");

	/* If no regex component, then it's a request to display expressions. */
	if (arg[0] == NULL) arg[0] = &do_display_expr;

	return display_word_split(dict, word, opts, display_word_expr, arg);
}
