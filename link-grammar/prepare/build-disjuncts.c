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
#include "dict-common/dict-structures.h"  // Exp_struct, lg_exp_stringify
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

	assert(e != NULL, "build_clause called with null parameter");
	if (e->type == AND_type)
	{
		c1 = pool_alloc(ct->Clause_pool);
		c1->c = NULL;
		c1->next = NULL;
		c1->cost = 0.0;
		c1->maxcost = 0.0;
		for (Exp *opd = e->operand_first; opd != NULL; opd = opd->operand_next)
		{
			c2 = build_clause(opd, ct);
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
		c = build_clause(e->operand_first, ct);
		/* we'll catenate the lists of clauses */
		for (Exp *opd = e->operand_first->operand_next; opd != NULL; opd = opd->operand_next)
		{
			c1 = build_clause(opd, ct);
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
build_disjunct(Sentence sent, Clause * cl, const char * string,
               double cost_cutoff, Parse_Options opts)
{
	Disjunct *dis, *ndis;
	Pool_desc *connector_pool = NULL;

	dis = NULL;
	for (; cl != NULL; cl = cl->next)
	{
		if (cl->maxcost <= cost_cutoff)
		{
			if (NULL == sent) /* For the SAT-parser, until fixed. */
			{
				ndis = xalloc(sizeof(Disjunct));
			}
			else
			{
				ndis = pool_alloc(sent->Disjunct_pool);
				connector_pool = sent->Connector_pool;
			}
			ndis->left = ndis->right = NULL;

			/* Build a list of connectors from the Tconnectors. */
			for (Tconnector *t = cl->c; t != NULL; t = t->next)
			{
				Connector *n = connector_new(connector_pool, t->e->condesc, opts);
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

Disjunct * build_disjuncts_for_exp(Sentence sent, Exp* exp, const char *word,
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

	// printf("%s\n", lg_exp_stringify(exp));
	c = build_clause(exp, &ct);
	// print_clause_list(c);
	dis = build_disjunct(sent, c, word, cost_cutoff, opts);
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
		printf("%s", t->e->condesc->string);
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

/* There is a much better lg_exp_stringify() elsewhere
 * This one is for low-level debug. */
GNUC_UNUSED void prt_exp(Exp *e, int i)
{
	if (e == NULL) return;

	for(int j =0; j<i; j++) printf(" ");
	printf ("type=%d dir=%c multi=%d cost=%f\n",
	        e->type, e->dir, e->multi, e->cost);
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
		snprintf(unknown_type, sizeof(unknown_type)-1, "unknown_type-%d", type);
		type_str = unknown_type;
	}

	return type_str;
}

static bool is_ASAN_uninitialized(uintptr_t a)
{
	static const uintptr_t asan_uninitialized = 0xbebebebebebebebe;

	return (a == asan_uninitialized);
}

GNUC_UNUSED void prt_exp_mem(Exp *e, int i)
{
	if (is_ASAN_uninitialized((uintptr_t)e))
	{
		printf ("e=UNINITIALIZED\n");
		return;
	}
	if (e == NULL) return;

	for(int j =0; j<i; j++) printf(" ");
	printf ("e=%p: %s", e, stringify_Exp_type(e->type));

	if (is_ASAN_uninitialized((uintptr_t)e->operand_first))
		printf(" (UNINITIALIZED operand_first)");
	if (is_ASAN_uninitialized((uintptr_t)e->operand_next))
		printf(" (UNINITIALIZED operand_next)");

	if (e->type != CONNECTOR_type)
	{
		int operand_count = 0;
		for (Exp *opd = e->operand_first; NULL != opd; opd = opd->operand_next)
		{
			operand_count++;
			if (is_ASAN_uninitialized((uintptr_t)opd->operand_next))
			{
				printf(" (operand %d: UNINITIALIZED operand_next)\n", operand_count);
				return;
			}
		}
		printf(" (%d operand%s) cost=%f\n", operand_count,
		       operand_count == 1 ? "" : "s", e->cost);

		for (Exp *opd = e->operand_first; NULL != opd; opd = opd->operand_next)
		{
			prt_exp_mem(opd, i+2);
		}
	}
	else
	{
		printf(" %s%s%c cost=%f\n",
		       e->multi ? "@" : "",
		       e->condesc ? e->condesc->string : "(condesc=(null))",
		       e->dir, e->cost);
	}
}
#endif /* DEBUG */
