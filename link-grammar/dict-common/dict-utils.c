/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2013 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/
/*
 * Miscellaneous utilities for dealing with word types.
 */

#include "api-structures.h"             // Parse_Options
#include "connectors.h"
#include "dict-api.h"
#include "string-set.h"
#include "dict-utils.h"

/* ======================================================== */
/* Public API ... */

const char * lg_exp_get_string(const Exp* exp)
{
	return exp->condesc->string;
}

/* ======================================================== */

#if 0
void free_Exp(Exp *e)
{
	if (NULL == e) return; /* Exp might be null if the user has a bad dict. */
	Exp *operand_next;

	if (e->type != CONNECTOR_type)
	{
		for (Exp *opd = e->operand_first; opd != NULL; opd = operand_next)
		{
			operand_next = opd->operand_next;
			free_Exp(opd);
		}
	}
	free(e);
}
#endif

/* Exp utilities ... */

/* Returns the number of connectors in the expression e */
int size_of_expression(Exp * e)
{
	int size = 0;

	if (e->type == CONNECTOR_type) return 1;
	for (Exp *opd = e->operand_first; opd != NULL; opd = opd->operand_next)
		size += size_of_expression(opd);

	return size;
}

Exp *copy_Exp(Exp *e, Pool_desc *Exp_pool, Parse_Options opts)
{
	if (e == NULL) return NULL;
	Exp *new_e = pool_alloc(Exp_pool);

	*new_e = *e;
	if (NULL != e->tag)
		new_e->cost += opts->dialect.cost_table[new_e->tag->index];

#if 0 /* Not used - left here for documentation. */
	new_e->operand_next = copy_Exp(dict, e->operand_next, Exp_pool);
	if (CONNECTOR_type == e->type) return new_e;
	new_e->operand_first = copy_Exp(dict, e->operand_first, Exp_pool);
#else
	if (CONNECTOR_type == e->type) return new_e;

	/* Iterate operands to avoid a deep recursion due to a lot of operands. */
	Exp **tmp_e_a = &new_e->operand_first;
	for(Exp *opd = e->operand_first; opd != NULL; opd = opd->operand_next)
	{
		*tmp_e_a = copy_Exp(dict, opd, Exp_pool, opts);
		tmp_e_a = &(*tmp_e_a)->operand_next;
	}
	*tmp_e_a = NULL;
#endif

	return new_e;
}

/**
 * Compare two expressions, return true for equal, false for unequal.
 */
static bool exp_compare(Exp *e1, Exp *e2)
{
	if ((e1 == NULL) && (e2 == NULL))
	  return true;
	if ((e1 == NULL) || (e2 == NULL))
	  return false;
	if (e1->type != e2->type)
		return false;
	if (!cost_eq(e1->cost, e2->cost))
		return false;

	if (e1->type == CONNECTOR_type)
	{
		if (e1->condesc != e2->condesc)
			return false;
		if (e1->dir != e2->dir)
			return false;
	}
	else
	{
		/* Iterate operands to avoid a deep recursion due to a lot of operands. */
		for (e1 = e1->operand_first, e2 = e2->operand_first;
		     (e1 != NULL) && (e2 != NULL);
		     e1 = e1->operand_next, e2 = e2->operand_next)
		{
			if (!exp_compare(e1, e2))
				return false;
		}
		return ((e1 == NULL) && (e2 == NULL));
	}
	return true;
}

/**
 * Sub-expression matcher -- return 1 if sub is non-NULL and
 * contained in super, 0 otherwise.
 */
static int exp_contains(Exp * super, Exp * sub)
{
#if 0 /* DEBUG */
	if (super) printf("SUP: %s\n", lg_exp_stringify(super));
#endif

	if (sub==NULL || super==NULL)
		return 0;
	if (exp_compare(sub,super))
		return 1;
	if (super->type==CONNECTOR_type)
	  return 0; /* super is a leaf */

	/* proceed through supers children and return 1 if sub
	   is contained in any of them */
	for(Exp *opd = super->operand_first; opd != NULL; opd = opd->operand_next)
	{
		if (exp_contains(opd, sub)==1)
			return 1;
	}
	return 0;
}

/* ======================================================== */
/* X_node utilities ... */

/**
 * Destructively catenates the two disjunct lists d1 followed by d2.
 * Doesn't change the contents of the disjuncts.
 * Traverses the first list, but not the second.
 */
X_node * catenate_X_nodes(X_node *d1, X_node *d2)
{
	X_node * dis = d1;

	if (d1 == NULL) return d2;
	if (d2 == NULL) return d1;
	while (dis->next != NULL) dis = dis->next;
	dis->next = d2;
	return d1;
}

#ifdef DEBUG
GNUC_UNUSED static void print_x_node(X_node *x)
{
	if (x == NULL) printf("NULL X_node\n");
	for (; x != NULL; x = x->next)
	{
		printf("%p: exp=%p next=%p\n", x, x->exp, x->next);
	}
}
#endif

/* ======================================================== */
/* More connector utilities ... */

/**
 * word_has_connector() -- return TRUE if dictionary expression has connector
 * This function takes a dict_node (corresponding to an entry in a
 * given dictionary), a string (representing a connector), and a
 * direction (+ = right-pointing, '-' = left-pointing); it returns true
 * if the dictionary expression for the word includes the connector,
 * false otherwise.  This can be used to see if a word is in a certain
 * category (checking for a category connector in a table), or to see
 * if a word has a connector in a normal dictionary. The connector
 * check uses a "smart-match", the same kind used by the parser.
 */
#if CRAZY_OBESE_CHECKING_AGLO
bool word_has_connector(Dict_node * dn, const char * cs, char direction)
{
	Connector * c2 = NULL;
	Disjunct *d, *d0;
	if (dn == NULL) return false;
	d0 = d = build_disjuncts_for_dict_node(dn);
	if (d == NULL) return false;
	for (; d != NULL; d = d->next) {
		if (direction == '+') c2 = d->right;
		if (direction == '-') c2 = d->left;
		for (; c2 != NULL; c2 = c2->next) {
			if (easy_match(c2->string, cs)) {
				free_disjuncts(d0);
				return true;
			}
		}
	}
	free_disjuncts(d0);
	return false;
}
#else /* CRAZY_OBESE_CHECKING_AGLO */

/**
 * Return true if the given expression has the given connector.
 * The connector cs argument must originally be in the dictionary string set.
 */
static bool exp_has_connector(const Exp * e, int depth, const char * cs,
                              char direction, bool smart_match)
{
	if (e->type == CONNECTOR_type)
	{
		if (direction != e->dir) return false;
		return smart_match ? easy_match(e->condesc->string, cs)
		                   : string_set_cmp(e->condesc->string, cs);
	}

	if (depth == 0) return false;
	if (depth > 0) depth--;

	for (Exp *opd = e->operand_first; opd != NULL; opd = opd->operand_next)
	{
		if (exp_has_connector(opd, depth, cs, direction, smart_match))
			return true;
	}
	return false;
}

bool word_has_connector(Dict_node * dn, const char * cs, char direction)
{
	return exp_has_connector(dn->exp, -1, cs, direction, /*smart_match*/true);
}
#endif /* CRAZY_OBESE_CHECKING_AGLO */

/**
 * Find if an expression has a connector ZZZ- (that an empty-word has).
 * This is a costly way to find it. To reduce the overhead, the
 * exp_has_connector() "depth" argument limits the expression depth check,
 * supposing the ZZZ- connectors are not deep in the word expression.
 * FIXME? A cheaper way is to have a dictionary entry which lists such
 * words, or to mark such words at dictionary read time.
 **/
bool is_exp_like_empty_word(Dictionary dict, Exp *exp)
{
	const char *cs = string_set_lookup(EMPTY_CONNECTOR, dict->string_set);
	if (NULL == cs) return false;
	return exp_has_connector(exp, 2, cs, '-', /*smart_match*/false);
}

/**
 * If word has a connector, return it.
 * If word has more than one connector, return NULL.
 */
const char * word_only_connector(Dict_node * dn)
{
	Exp * e = dn->exp;
	if (CONNECTOR_type == e->type)
		return e->condesc->string;
	return NULL;
}

/* ======================================================== */
/* Dictionary utilities ... */

static bool dn_word_contains(Dictionary dict,
                             Dict_node * w_dn, const char * macro)
{
	Exp * m_exp;
	Dict_node *m_dn;

	if (w_dn == NULL) return false;

	m_dn = dictionary_lookup_list(dict, macro);
	if (m_dn == NULL) return false;

	m_exp = m_dn->exp;

#if 0 /* DEBUG */
	printf("\nWORD: %s\n", lg_exp_stringify(w_dn->exp));
	printf("\nMACR: %s\n", lg_exp_stringify(m_exp));
#endif

	for (;w_dn != NULL; w_dn = w_dn->right)
	{
		if (1 == exp_contains(w_dn->exp, m_exp))
		{
			free_lookup_list(dict, m_dn);
			return true;
		}
	}
	free_lookup_list(dict, m_dn);
	return false;
}

/**
 * word_contains: return true if the word may involve application of
 * a rule.
 *
 * @return: true if word's expression contains macro's expression,
 * false otherwise.
 */
bool word_contains(Dictionary dict, const char * word, const char * macro)
{
	Dict_node *w_dn = dictionary_lookup_list(dict, word);
	bool ret = dn_word_contains(dict, w_dn, macro);
	free_lookup_list(dict, w_dn);
	return ret;
}

/* ========================= END OF FILE ============================== */
