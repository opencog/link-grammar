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

/* stuff for transforming a dictionary entry into a disjunct list */

#include <math.h>
#include "api.h"
#include "disjunct-utils.h"
#include "utilities.h" /* For Win32 compatibility features */

/* Temporary connectors used while converting expressions into disjunct lists */
typedef struct Tconnector_struct Tconnector;
struct Tconnector_struct
{
	char multi;   /* TRUE if this is a multi-connector */
	char dir;	 /* '-' for left and '+' for right */
	Tconnector * next;
	const char * string;
};

typedef struct clause_struct Clause;
struct clause_struct
{
	Clause * next;
	float cost;
	float maxcost;
	Tconnector * c;
};

static void free_Tconnectors(Tconnector *e)
{
	Tconnector * n;
	for(;e != NULL; e=n)
	{
		n = e->next;
		xfree((char *)e, sizeof(Tconnector));
	}
}

static void free_clause_list(Clause *c)
{
	Clause *c1;
	while (c != NULL)
	{
		c1 = c->next;
		free_Tconnectors(c->c);
		xfree((char *)c, sizeof(Clause));
		c = c1;
	}
}

#ifdef UNUSED_FUNCTION
/**
 * This builds a new copy of the connector list pointed to by c.
 * Strings, as usual, are not copied.
 */
static Tconnector * copy_Tconnectors(Tconnector * c)
{
	Tconnector *c1;
	if (c == NULL) return NULL;
	c1 = (Tconnector *) xalloc(sizeof(Tconnector));
	*c1 = *c;
	c1->next = copy_Tconnectors(c->next);
	return c1;
}

/**
 * This builds a new copy of the clause pointed to by d (except for the
 * next field which is set to NULL).  Strings, as usual, are not copied.
 */
static Clause * copy_clause(Clause * d)
{
	Clause * d1;
	if (d == NULL) return NULL;
	d1 = (Clause *) xalloc(sizeof(Clause));
	*d1 = *d;
	d1->next = NULL;
	d1->c = copy_Tconnectors(d->c);
	return d1;
}
#endif /* UNUSED_FUNCTION */

/**
 * reverse the order of the list e.  destructive
 */
static Tconnector * Treverse(Tconnector *e)
{
	Tconnector * head, *x;
	head = NULL;
	while (e != NULL) {
		x = e->next;
		e->next = head;
		head = e;
		e = x;
	}
	return head;
}

/**
 * reverse the order of the list e.  destructive
 */
static Connector * reverse(Connector *e)
{
	Connector * head, *x;
	head = NULL;
	while (e != NULL) {
		x = e->next;
		e->next = head;
		head = e;
		e = x;
	}
	return head;
}

/**
 * Builds a new list of connectors that is the catenation of e1 with e2.
 * does not effect lists e1 or e2.   Order is maintained.
 */
static Tconnector * catenate(Tconnector * e1, Tconnector * e2)
{
	Tconnector * e, * head;
	head = NULL;
	for (;e1 != NULL; e1 = e1->next) {
		e = (Tconnector *) xalloc(sizeof(Tconnector));
		*e = *e1;
		e->next = head;
		head = e;
	}
	for (;e2 != NULL; e2 = e2->next) {
		e = (Tconnector *) xalloc(sizeof(Tconnector));
		*e = *e2;
		e->next = head;
		head = e;
	}
	return Treverse(head);
}

/**
 * build the connector for the terminal node n
 */
static Tconnector * build_terminal(Exp * e)
{
	Tconnector * c;
	c = (Tconnector *) xalloc(sizeof(Tconnector));
	c->string = e->u.string;
	c->multi = e->multi;
	c->dir = e->dir;
	c->next = NULL;
	return c;
}

#ifdef UNUSED_FUNCTION
static xxxfloat-int maxcost_of_expression(Exp *e)
{
	E_list * e_list;
	int m, m1;

	m = 0;

	if ((e->type == AND_type) || (e->type == OR_type)) {
		for (e_list = e->u.l; e_list != NULL; e_list = e_list->next) {
			m1 = maxcost_of_expression(e_list->e);
			m = MAX(m, m1);
		}
	}
	return (m + e->cost);
}

/**
 * This returns the maximum maxcost of any disjunct in the sentence
 * Assumes the sentence expressions have been constructed
 */
static xxx-float-int maxcost_of_sentence(Sentence sent)
{
	X_node * x;
	int w, m, m1;
	m = 0;

	for (w = 0; w < sent->length; w++)
	{
		for (x = sent->word[w].x; x != NULL; x = x->next)
		{
			m1 = maxcost_of_expression(x->exp),
			m = MAX(m, m1);
		}
	}
	return m;
}
#endif /* UNUSED_FUNCTION */


/**
 * Build the clause for the expression e.  Does not change e
 */
static Clause * build_clause(Exp *e)
{
	Clause *c = NULL, *c1, *c2, *c3, *c4, *c_head;
	E_list * e_list;

	assert(e != NULL, "build_clause called with null parameter");
	if (e->type == AND_type)
	{
		c1 = (Clause *) xalloc(sizeof (Clause));
		c1->c = NULL;
		c1->next = NULL;
		c1->cost = 0.0f;
		c1->maxcost = 0.0f ;
		for (e_list = e->u.l; e_list != NULL; e_list = e_list->next)
		{
			c2 = build_clause(e_list->e);
			c_head = NULL;
			for (c3 = c1; c3 != NULL; c3 = c3->next)
			{
				for (c4 = c2; c4 != NULL; c4 = c4->next)
				{
					c = (Clause *) xalloc(sizeof (Clause));
					c->cost = c3->cost + c4->cost;
					c->maxcost = fmaxf(c3->maxcost,c4->maxcost);
					c->c = catenate(c3->c, c4->c);
					c->next = c_head;
					c_head = c;
				}
			}
			free_clause_list(c1);
			free_clause_list(c2);
			c1 = c_head;
		}
		c = c1;
	}
	else if (e->type == OR_type)
	{
		/* we'll catenate the lists of clauses */
		c = NULL;
		for (e_list = e->u.l; e_list != NULL; e_list = e_list->next)
		{
			c1 = build_clause(e_list->e);
			while(c1 != NULL) {
				c3 = c1->next;
				c1->next = c;
				c = c1;
				c1 = c3;
			}
		}
	}
	else if (e->type == CONNECTOR_type)
	{
		c = (Clause *) xalloc(sizeof(Clause));
		c->c = build_terminal(e);
		c->cost = 0.0f;
		c->maxcost = 0.0f;
		c->next = NULL;
	}
	else
	{
		assert(FALSE, "an expression node with no type");
	}

	/* c now points to the list of clauses */
	for (c1 = c; c1 != NULL; c1 = c1->next)
	{
		c1->cost += e->cost;
		/*	c1->maxcost = MAX(c1->maxcost,e->cost);  */
		/* Above is how Dennis had it. Someone changed it to below.
		 * However, this can sometimes lead to a maxcost that is less
		 * than the cost ! -- which seems wrong to me ... seems Dennis
		 * had it right!?
		 */
		c1->maxcost += e->cost;
	}
	return c;
}

/* #define DEBUG */
#ifdef DEBUG
/* Misc printing functions, useful for debugging */

static void print_Tconnector_list(Tconnector * e)
{
	for (;e != NULL; e=e->next) {
		if (e->multi) printf("@");
		printf("%s",e->string);
		printf("%c", e->dir);
		if (e->next != NULL) printf(" ");
	}
}

static void print_clause_list(Clause * c)
{
	for (;c != NULL; c=c->next) {
		printf("  Clause: ");
		printf("(%4.2f, %4.2f) ", c->cost, c->maxcost);
		print_Tconnector_list(c->c);
		printf("\n");
	}
}

static void print_connector_list(Connector * e)
{
	for (;e != NULL; e=e->next)
	{
		printf("%s",e->string);
		if (e->label != NORMAL_LABEL) {
			printf("%3d", e->label);
		} else {
			printf("   ");
		}
		if (e->next != NULL) printf(" ");
	}
}

static void print_disjunct_list(Disjunct * c)
{
	for (;c != NULL; c=c->next) {
		printf("%10s: ", c->string);
		printf("(%f) ", c->cost);
		print_connector_list(c->left);
		printf(" <--> ");
		print_connector_list(c->right);
		printf("\n");
	}
}
#endif /* DEBUG */

/**
 * Build a new list of connectors starting from the Tconnectors
 * in the list pointed to by e.  Keep only those whose strings whose
 * direction has the value c.
 */
static Connector * extract_connectors(Tconnector *e, int c)
{
	Connector *e1;
	if (e == NULL) return NULL;
	if (e->dir == c)
	{
		e1 = connector_new();
		e1->next = extract_connectors(e->next,c);
		e1->multi = e->multi;
		e1->string = e->string;
		e1->word = 0;
		return e1;
	}
	else
	{
		return extract_connectors(e->next,c);
	}
}

/**
 * Build a disjunct list out of the clause list c.
 * string is the print name of word that generated this disjunct.
 */
static Disjunct *
build_disjunct(Clause * cl, const char * string, float cost_cutoff)
{
	Disjunct *dis, *ndis;
	dis = NULL;
	for (; cl != NULL; cl = cl->next)
	{
		if (cl->maxcost <= cost_cutoff)
		{
			ndis = (Disjunct *) xalloc(sizeof(Disjunct));
			ndis->left = reverse(extract_connectors(cl->c, '-'));
			ndis->right = reverse(extract_connectors(cl->c, '+'));
			ndis->string = string;
			ndis->cost = cl->cost;
			ndis->next = dis;
			dis = ndis;
		}
	}
	return dis;
}

Disjunct * build_disjuncts_for_X_node(X_node * x, float cost_cutoff)
{
	Clause *c ;
	Disjunct * dis;
	/* print_expression(x->exp);  printf("\n"); */
	c = build_clause(x->exp);
	/* print_clause_list(c); */
	dis = build_disjunct(c, x->string, cost_cutoff);
	/* print_disjunct_list(dis); */
	free_clause_list(c);
	return dis;
}

#if DEBUG
/* There is a much better print_expression elsewhere 
 * This one is for low-level debug. */
void prt_exp(Exp *e, int i)
{
	if (e == NULL) return;

	for(int j =0; j<i; j++) printf(" ");
	printf ("type=%d dir=%c multi=%d cost=%f\n", e->type, e->dir, e->multi, e->cost);
	if (e->type != CONNECTOR_type)
	{
		E_list *l = e->u.l;
		while(l)
		{
			prt_exp(l->e, i+2);
			l = l->next;
		}
	}
	else
	{
		for(int j =0; j<i; j++) printf(" ");
		printf("con=%s\n", e->u.string);
	}
}
#endif

/**
 * Build a list of disjuncts.
 *
 * This is mostly used only for counting the number of disjuncts
 * (but is otherwise "almost" obsolete ??)
 */
Disjunct * build_disjuncts_for_dict_node(Dict_node *dn)
{
	Disjunct *dj;
	X_node x;
	x.exp = dn->exp;
	x.string = dn->string;
	dj = build_disjuncts_for_X_node(&x, MAX_CONNECTOR_COST);
	/* print_disjunct_list(dj); */
	return dj;
}

/**
 * build_word_expressions() -- build list of expressions for a word
 *
 * Looks up the word s in the dictionary.  Returns NULL if it's not there.
 * If there, it builds the list of expressions for the word, and returns
 * a pointer to it.
 */
X_node * build_word_expressions(Dictionary dict, const char * s)
{
	Dict_node * dn, *dn_head;
	X_node * x, * y;

	dn_head = dictionary_lookup_list(dict, s);
	dn = dn_head;

	x = NULL;
	while (dn != NULL)
	{
		y = (X_node *) xalloc(sizeof(X_node));
		y->next = x;
		x = y;
		x->exp = copy_Exp(dn->exp);
		x->string = dn->string;
		dn = dn->right;
	}
	free_lookup_list (dn_head);
	return x;
}

/**
 * Turn sentence expressions into disjuncts.
 * Sentence expressions must have been built, before calling this routine.
 */
void build_sentence_disjuncts(Sentence sent, float cost_cutoff)
{
	Disjunct * d;
	X_node * x;
	int w;
	for (w = 0; w < sent->length; w++)
	{
		d = NULL;
		for (x = sent->word[w].x; x != NULL; x = x->next)
		{
			Disjunct *dx = build_disjuncts_for_X_node(x, cost_cutoff);
			d = catenate_disjuncts(dx, d);
		}
		sent->word[w].d = d;
	}
}
