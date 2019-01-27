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

/* stuff for transforming a dictionary entry into a disjunct list */

#include "build-disjuncts.h"
#include "connectors.h"
//#include "dict-common/print-dict.h"      // print_expression()
#include "dict-common/dict-structures.h"   // Exp_struct()
#include "disjunct-utils.h"
#include "utilities.h"

/* Temporary connectors used while converting expressions into disjunct lists */
typedef struct Tconnector_struct Tconnector;
struct Tconnector_struct
{
	Tconnector * next;
	const Exp *e; /* a CONNECTOR_type element from which to get the connector  */
};

typedef struct clause_struct Clause;
struct clause_struct
{
	Clause * next;
	double cost;
	double maxcost;
	Tconnector * c;
};

typedef struct
{
	double cost_cutoff;
	Pool_desc *Tconnector_pool;
	Pool_desc *Clause_pool;
} clause_context;

#ifdef DEBUG
static void print_Tconnector_list(Tconnector * e);
static void print_clause_list(Clause * c);
#endif

#if BUILD_DISJUNCTS_FREE_INETERMEDIATE_MEMOEY /* Undefined - CPU overhead. */
static void free_Tconnectors(Tconnector *e, Pool_desc *mp)
{
	Tconnector * n;
	for(;e != NULL; e=n)
	{
		n = e->next;
		pool_free(mp, e);
	}
}

static void free_clause_list(Clause *c, clause_context *ct)
{
	Clause *c1;
	while (c != NULL)
	{
		c1 = c->next;
		free_Tconnectors(c->c, ct->Tconnector_pool);
		pool_free(ct->Clause_pool, c);
		c = c1;
	}
}
#endif

#if 0 /* old stuff */
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
#endif

/**
 * Builds a new list of connectors that is the catenation of e1 with e2.
 * does not effect lists e1 or e2.   Order is maintained.
 */
static Tconnector * catenate(Tconnector * e1, Tconnector * e2, Pool_desc *tp)
{
	Tconnector head;
	Tconnector *preve = &head;
	Tconnector *newe = &head;

	for (;e1 != NULL; e1 = e1->next)
	{
		newe = pool_alloc(tp);
		*newe = *e1;

		preve->next = newe;
		preve = newe;
	}
	for (;e2 != NULL; e2 = e2->next)
	{
		newe = pool_alloc(tp);
		*newe = *e2;

		preve->next = newe;
		preve = newe;
	}

	newe->next = NULL;
	return head.next;
}

/**
 * build the connector for the terminal node n
 */
static Tconnector * build_terminal(Exp *e, Pool_desc *tp)
{
	Tconnector *c = pool_alloc(tp);
	c->e = e;
	c->next = NULL;
	return c;
}

/**
 * Build the clause for the expression e.  Does not change e
 */
static Clause * build_clause(Exp *e, clause_context *ct)
{
	Clause *c = NULL, *c1, *c2, *c3, *c4, *c_head;
	E_list * e_list;

	assert(e != NULL, "build_clause called with null parameter");
	if (e->type == AND_type)
	{
		c1 = pool_alloc(ct->Clause_pool);
		c1->c = NULL;
		c1->next = NULL;
		c1->cost = 0.0;
		c1->maxcost = 0.0;
		for (e_list = e->u.l; e_list != NULL; e_list = e_list->next)
		{
			c2 = build_clause(e_list->e, ct);
			c_head = NULL;
			for (c3 = c1; c3 != NULL; c3 = c3->next)
			{
				for (c4 = c2; c4 != NULL; c4 = c4->next)
				{
					double maxcost = MAX(c3->maxcost,c4->maxcost);
					if (maxcost + e->cost > ct->cost_cutoff) continue;

					c = pool_alloc(ct->Clause_pool);
					c->cost = c3->cost + c4->cost;
					c->maxcost = maxcost;
					c->c = catenate(c3->c, c4->c, ct->Tconnector_pool);
					c->next = c_head;
					c_head = c;
				}
			}
#if BUILD_DISJUNCTS_FREE_INETERMEDIATE_MEMOEY /* Undefined - CPU overhead. */
			free_clause_list(c1, ct);
			free_clause_list(c2, ct);
#endif
			c1 = c_head;
		}
		c = c1;
	}
	else if (e->type == OR_type)
	{
		c = build_clause(e->u.l->e, ct);
		/* we'll catenate the lists of clauses */
		for (e_list = e->u.l->next; e_list != NULL; e_list = e_list->next)
		{
			c1 = build_clause(e_list->e, ct);
			if (c1 == NULL) continue;
			if (c == NULL)
			{
				c = c1;
			}
			else
			{
				for (c2 = c; ; c2 = c2->next)
				{
					if (NULL == c2->next)
					{
						c2->next = c1;
						break;
					}
				}
			}
		}
	}
	else if (e->type == CONNECTOR_type)
	{
		c = pool_alloc(ct->Clause_pool);
		c->c = build_terminal(e, ct->Tconnector_pool);
		c->cost = 0.0;
		c->maxcost = 0.0;
		c->next = NULL;
	}
	else
	{
		assert(false, "an expression node with no type");
	}

	/* c now points to the list of clauses */
	for (c1 = c; c1 != NULL; c1 = c1->next)
	{
		c1->cost += e->cost;
		/* c1->maxcost = MAX(c1->maxcost,e->cost);  */
		/* Above is how Dennis had it. Someone changed it to below.
		 * However, this can sometimes lead to a maxcost that is less
		 * than the cost ! -- which seems wrong to me ... seems Dennis
		 * had it right!?
		 */
		c1->maxcost += e->cost;
		/* Note: The above computation is used as a saving shortcut in
		 * build_clause(). If it is changed here, it needs to be changed
		 * there too. */
	}
	return c;
}

/**
 * Build a disjunct list out of the clause list c.
 * string is the print name of word that generated this disjunct.
 */
static Disjunct *
build_disjunct(Clause * cl, const char * string, double cost_cutoff,
               Parse_Options opts)
{
	Disjunct *dis, *ndis;
	dis = NULL;
	for (; cl != NULL; cl = cl->next)
	{
		if (cl->maxcost <= cost_cutoff)
		{
			ndis = (Disjunct *) xalloc(sizeof(Disjunct));
			ndis->left = ndis->right = NULL;

			/* Build a list of connectors from the Tconnectors. */
			for (Tconnector *t = cl->c; t != NULL; t = t->next)
			{
				Connector *n = connector_new(t->e->u.condesc, opts);
				Connector **loc = ('-' == t->e->dir) ? &ndis->left : &ndis->right;

				n->multi = t->e->multi;
				n->next = *loc;   /* prepend the connector to the current list */
				*loc = n;         /* update the connector list */
			}

			ndis->word_string = string;
			ndis->cost = cl->cost;
			ndis->next = dis;
			dis = ndis;
		}
	}
	return dis;
}

Disjunct * build_disjuncts_for_exp(Exp* exp, const char *word,
                                   double cost_cutoff, Parse_Options opts)
{
	Clause *c ;
	Disjunct * dis;
	clause_context ct = { 0 };

	ct.cost_cutoff = cost_cutoff;
	ct.Clause_pool = pool_new(__func__, "Clause",
	                   /*num_elements*/4096, sizeof(Clause),
	                   /*zero_out*/false, /*align*/false, /*exact*/false);
	ct.Tconnector_pool = pool_new(__func__, "Tconnector",
	                   /*num_elements*/32768, sizeof(Tconnector),
	                   /*zero_out*/false, /*align*/false, /*exact*/false);

	// print_expression(exp);  printf("\n");
	c = build_clause(exp, &ct);
	// print_clause_list(c);
	dis = build_disjunct(c, word, cost_cutoff, opts);
	// print_disjunct_list(dis);
	pool_delete(ct.Tconnector_pool);
	pool_delete(ct.Clause_pool);
	return dis;
}

#ifdef DEBUG
/* Misc printing functions, useful for debugging */

static void print_Tconnector_list(Tconnector *t)
{
	for (; t != NULL; t = t->next)
	{
		if (t->e->multi) printf("@");
		printf("%s", t->e->u.condesc->string);
		printf("%c", t->e->dir);
		if (t->next != NULL) printf(" ");
	}
}

GNUC_UNUSED static void print_clause_list(Clause * c)
{
	for (;c != NULL; c=c->next) {
		printf("  Clause: ");
		printf("(%4.2f, %4.2f) ", c->cost, c->maxcost);
		print_Tconnector_list(c->c);
		printf("\n");
	}
}

/* There is a much better print_expression elsewhere
 * This one is for low-level debug. */
GNUC_UNUSED void prt_exp(Exp *e, int i)
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
		printf("con=%s\n", e->u.condesc->string);
	}
}

GNUC_UNUSED void prt_exp_mem(Exp *e, int i)
{
	char unknown_type[32] = "";
	const char *type;

	if (e == NULL) return;

	if (e->type > 0 && e->type <= 3)
	{
		type = ((const char *[]) {"OR_type", "AND_type", "CONNECTOR_type"}) [e->type-1];
	}
	else
	{
		snprintf(unknown_type, sizeof(unknown_type)-1, "unknown-%d", e->type);
		type = unknown_type;
	}

	for(int j =0; j<i; j++) printf(" ");
	printf ("e=%p: %s cost=%f\n", e, type, e->cost);
	if (e->type != CONNECTOR_type)
	{
		E_list *l;
		for(int j =0; j<i+2; j++) printf(" ");
		printf("E_list=%p (", e->u.l);
		for (l = e->u.l; NULL != l; l = l->next)
		{
			printf("%p", l->e);
			if (NULL != l->next) printf(" ");
		}
		printf(")\n");

		for (l = e->u.l; NULL != l; l = l->next)
		{
			prt_exp_mem(l->e, i+2);
		}
	}
	else
	{
		for(int j =0; j<i; j++) printf(" ");
		printf("con=%s dir=%c multi=%d\n",
		       e->u.condesc ? e->u.condesc->string : "(condesc=(null))",
		       e->dir, e->multi);
	}
}
#endif /* DEBUG */
