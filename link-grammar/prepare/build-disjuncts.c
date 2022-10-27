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
#include "dict-common/dict-structures.h"  // Exp_struct, exp_stringify
#include "dict-common/dict-common.h"      // Dictionary
#include "disjunct-utils.h"
#include "utilities.h"

/* Temporary connectors used while converting expressions into disjunct lists */
typedef struct Tconnector_struct Tconnector;
struct Tconnector_struct
{
	Tconnector * next;
	Exp *e; /* a CONNECTOR_type element from which to get the connector  */
	Connector *tracon; /* the created tracon, set through memory sharing */
};

typedef struct clause_struct Clause;
struct clause_struct
{
	Clause * next;
	float cost;
	float maxcost;
	Tconnector * c;
};

typedef struct
{
	Pool_desc *Tconnector_pool;
	Pool_desc *Clause_pool;
	float cost_cutoff;
	int exp_pos;
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

/**
 * Builds a new list of connectors that is the catenation of e1 with e2.
 * does not effect lists e1 or e2.   Order is maintained.
 */
static Tconnector * catenate(Tconnector * e1, Tconnector * e2, Pool_desc *tp)
{
	Tconnector head;
	Tconnector *preve = &head;

	for (;e1 != NULL; e1 = e1->next)
	{
		Tconnector *newe = pool_alloc(tp);
		*newe = *e1;

		preve->next = newe;
		preve = newe;
	}

	preve->next = e2; /* tracon memory sharing */
	return head.next;
}

/**
 * build the connector for the terminal node n
 */
static Tconnector * build_terminal(Exp *e, clause_context *ct)
{
	Tconnector *c = pool_alloc(ct->Tconnector_pool);
	c->e = e;
	c->next = NULL;
	c->e->pos = ct->exp_pos++;
	c->tracon = NULL;
	return c;
}

static void debug_last(Clause *c, Clause **c_last, const char *type)
{
#ifdef DEBUG
	if ((c == NULL) || (c_last == NULL)) return;
	do
	{
		if (c->next == NULL)
		{
			assert(c == *c_last, "%s", type);
			return;
		}
	} while ((c = c->next));
#endif
}

/**
 * Build the clause for the expression e.  Does not change e
 */
static Clause * build_clause(Exp *e, clause_context *ct, Clause **c_last)
{
	Clause *c = NULL;

	assert(e != NULL, "build_clause called with null parameter");
	if (e->type == AND_type)
	{
		Clause *c1 = pool_alloc(ct->Clause_pool);
		c1->c = NULL;
		c1->next = NULL;
		c1->cost = 0.0;
		c1->maxcost = 0.0;
		for (Exp *opd = e->operand_first; opd != NULL; opd = opd->operand_next)
		{
			Clause *c2 = build_clause(opd, ct, NULL);
			Clause *c_head = NULL;
			for (Clause *c3 = c1; c3 != NULL; c3 = c3->next)
			{
				for (Clause *c4 = c2; c4 != NULL; c4 = c4->next)
				{
					float maxcost = MAX(c3->maxcost,c4->maxcost);
					/* Cannot use this shortcut due to negative costs. */
					//if (maxcost + e->cost > ct->cost_cutoff) continue;

					Clause *c5 = pool_alloc(ct->Clause_pool);
					if ((c_head == NULL) && (c_last != NULL)) *c_last = c5;
					c5->cost = c3->cost + c4->cost;
					c5->maxcost = maxcost;
					c5->c = catenate(c4->c, c3->c, ct->Tconnector_pool);
					c5->next = c_head;
					c_head = c5;
				}
			}
#if BUILD_DISJUNCTS_FREE_INETERMEDIATE_MEMOEY /* Undefined - CPU overhead. */
			free_clause_list(c1, ct);
			free_clause_list(c2, ct);
#endif
			c1 = c_head;
		}
		c = c1;
		if ((c != NULL) && (c->next == NULL) && (c_last != NULL)) *c_last = c;
		debug_last(c, c_last, "AND_type");
	}
	else if (e->type == OR_type)
	{
		Clause *or_last = NULL;

		c = build_clause(e->operand_first, ct, &or_last);
		/* we'll catenate the lists of clauses */
		for (Exp *opd = e->operand_first->operand_next; opd != NULL; opd = opd->operand_next)
		{
			Clause *last;
			or_last->next = build_clause(opd, ct, &last);
			or_last = last;
		}

		if (c_last != NULL) *c_last = or_last;
		debug_last(c, c_last, "OR_type");
	}
	else if (e->type == CONNECTOR_type)
	{
		c = pool_alloc(ct->Clause_pool);
		c->c = build_terminal(e, ct);
		c->cost = 0.0;
		c->maxcost = 0.0;
		c->next = NULL;
		if (c_last != NULL) *c_last = c;
	}
	else
	{
		assert(false, "Unknown expression type %d", (int)e->type);
	}

	/* c now points to the list of clauses */
	for (Clause *c1 = c; c1 != NULL; c1 = c1->next)
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
		 * the inner loop of AND_type. If it is changed here, it needs to be
		 * changed there too. */
	}
	return c;
}

/**
 * Build a disjunct list out of the clause list c.
 * string is the print name of word that generated this disjunct.
 */
static Disjunct *
build_disjunct(Sentence sent, Clause * cl, const char * string,
               const gword_set *gs, float cost_cutoff, Parse_Options opts)
{
	Disjunct *dis, *ndis;
	Pool_desc *connector_pool = NULL;
	bool sat_solver = false;

#if USE_SAT_SOLVER
		sat_solver = (opts != NULL) && opts->use_sat_solver;
#endif /* USE_SAT_SOLVER */

	dis = NULL;
	for (; cl != NULL; cl = cl->next)
	{
		if (unlikely(NULL == cl->c)) continue; /* no connectors */
		if (cl->maxcost > cost_cutoff) continue;

#if USE_SAT_SOLVER
		if (sat_solver) /* For the SAT-parser, until fixed. */
		{
			ndis = xalloc(sizeof(Disjunct));
		}
		else
#endif
		{
			ndis = pool_alloc(sent->Disjunct_pool);
			connector_pool = sent->Connector_pool;
		}
		ndis->left = ndis->right = NULL;

		/* Build a list of connectors from the Tconnectors. */
		Connector **jet[2] = { &ndis->left, &ndis->right };
		bool is_tracon[2] = { false, false };
		for (Tconnector *t = cl->c; t != NULL; t = t->next)
		{
			int idir = ('+' == t->e->dir);

			if (is_tracon[idir]) continue; /* this direction is complete */
			if (NULL != t->tracon)
			{
				/* Use the cashed tracon and mark this direction as complete. */
				*(jet[idir]) = t->tracon;
				is_tracon[idir] = true;
				continue;
			}

			Connector *n = connector_new(connector_pool, t->e->condesc, opts);
			t->tracon = n; /* cache this tracon */

			n->exp_pos = t->e->pos;
			n->multi = t->e->multi;
			n->farthest_word = t->e->farthest_word;

			*(jet[idir]) = n;
			jet[idir] = &n->next;
		}

		/* XXX add_category() starts category strings by ' '.
		 * FIXME Replace it by a better indication. */
		if (sat_solver || (!IS_GENERATION(sent->dict) || (' ' != string[0])))
		{
			ndis->word_string = string;
			ndis->cost = cl->cost;
			ndis->is_category = 0;
		}
		else
		{
			ndis->num_categories_alloced = 4;
			ndis->category =
				malloc(sizeof(*ndis->category) * ndis->num_categories_alloced);
			ndis->num_categories = 1;
			ndis->category[0].num = strtol(string, NULL, 16);
			ndis->category[1].num = 0; /* API array terminator */
			assert(sat_solver || ((ndis->category[0].num > 0) &&
			       (ndis->category[0].num < 64*1024)),
			       "Insane category %u", ndis->category[0].num);
			ndis->category[0].cost = cl->cost;
		}

		ndis->originating_gword = (gword_set*)gs; /* XXX remove constness */
		ndis->next = dis;
		dis = ndis;
	}
	return dis;
}

Disjunct *build_disjuncts_for_exp(Sentence sent, Exp* exp, const char *word,
                                  const gword_set *gs, float cost_cutoff,
                                  Parse_Options opts)
{
	Clause *c;
	Disjunct * dis;
	clause_context ct = { 0 };

	ct.cost_cutoff = cost_cutoff;
	if (unlikely(sent->Clause_pool == NULL))
	{
		ct.Clause_pool = pool_new(__func__, "Clause",
		                          /*num_elements*/4096, sizeof(Clause),
		                          /*zero_out*/false, /*align*/false, /*exact*/false);
		ct.Tconnector_pool = pool_new(__func__, "Tconnector",
		                              /*num_elements*/32768, sizeof(Tconnector),
		                              /*zero_out*/false, /*align*/false, /*exact*/false);
		/* Keep for freeing at the caller. */
		sent->Clause_pool = ct.Clause_pool;
		sent->Tconnector_pool = ct.Tconnector_pool;
	}
	else
	{
		ct.Clause_pool = sent->Clause_pool;
		ct.Tconnector_pool = sent->Tconnector_pool;
	}

	// printf("%s\n", lg_exp_stringify(exp));
	c = build_clause(exp, &ct, NULL);
	// print_clause_list(c);
	dis = build_disjunct(sent, c, word, gs, cost_cutoff, opts);
	// print_disjunct_list(dis);
	pool_reuse(ct.Clause_pool);
	pool_reuse(ct.Tconnector_pool);
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
#endif /* DEBUG */
