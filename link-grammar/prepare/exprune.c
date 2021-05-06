/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2009, 2013, 2014 Linas Vepstas                          */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "api-structures.h"              // for Sentence_s
#include "connectors.h"
#include "dict-common/dict-structures.h" // expression_stringify
#include "dict-common/dict-utils.h"      // size_of_expression
#include "print/print-util.h"            // dyn_str functions
#include "string-set.h"
#include "tokenize/word-structures.h"    // for Word_struct
#include "exprune.h"

/**
 * Here is expression pruning.  This is done even before the expressions
 * are turned into lists of disjuncts. The idea is to make the work of the
 * next steps (building the disjuncts, removing duplicate disjuncts,
 * disjunct pruning) lighter by first removing irrelevant connectors.
 *
 * The algorithm prunes connectors that can be eliminated by simple checks.
 *
 * A series of passes are made through the sentence, alternating
 * left-to-right and right-to-left.  Consider the left-to-right pass (the
 * other is symmetric).  A set S of connectors is maintained (initialized
 * to be empty).  Now the expressions of the current word are processed.
 * If a given left pointing connector has no connector in S to which it
 * can be matched, then that connector is deleted. The expression is then
 * simplified by deleting AND sub-expression with a deleted component
 * (which may be a connector or a sub-expression), and removing deleted
 * components from OR sub-expressions (deleting the OR sub-expression upon
 * deleting of its last component).  Now the set S is augmented by the
 * right connectors of the remaining disjuncts of that word.  This
 * completes one word.  The process continues through the words from left
 * to right.
 * Alternate passes are made until no connector is deleted.
 *
 * FIXME: Mark shallow connectors on dictionary read and enhance the
 * pruning accordingly.
 */

#define D_EXPRUNE 9

#ifdef DEBUG
#define DBG(p, w, X) \
	if (verbosity_level(+D_EXPRUNE))\
	{\
		err_msg(lg_Trace, "pass%d w%zu: ", p, w);\
		err_msg(lg_Trace, X ": %s\n", exp_stringify(x->exp));\
	}
#else /* !DEBUG */
#define DBG(p, w, X)
#endif /* DEBUG */

#define DBG_EXPSIZES(...) \
	if (verbosity_level(+D_EXPRUNE))\
	{\
		char *e = print_expression_sizes(sent);\
		err_msg(lg_Trace, __VA_ARGS__);\
		free(e);\
	}

typedef struct connector_table_s connector_table;
struct connector_table_s
{
	condesc_t *condesc;
	connector_table *next;
	int farthest_word;
};

#define CT_BLKSIZE 512
/* The connector table elements are allocated in a kind of an unrolled
 * linked list with fixed blocks, when the first block is pre-allocated
 * (this simplifies the handling). Additional blocks are
 * dynamically allocated, but they are rarely needed. The existing
 * allocation is reused on each pass, and freed only at the end of the
 * expression pruning. */
// connector_table_element-> ... CT_BLKSIZE-1 elements
//                           ...
//                           ...
//                           block connecting element ->---+
//                                                         |
// (additional block)        CT_BLKSIZE-1 elements     <---+
//                           ...
// current_element        -> ...
//                           ...
// end_current_block      -> block connecting element ->---+
//                                                         |
// (additional block)        CT_BLKSIZE-1 elements     <---+
//                           ...
//                           ...
//                           ...
//                           block connecting element

typedef struct exprune_context_s exprune_context;
struct exprune_context_s
{
	connector_table **ct;
	size_t ct_size;
	Parse_Options opts;
	connector_table *current_element;
	connector_table *end_current_block;
	connector_table connector_table_element[CT_BLKSIZE];
	int N_deleted;
};

static connector_table *ct_element_new(exprune_context *ctxt)
{
	if (ctxt->current_element == ctxt->end_current_block)
	{
		if (ctxt->end_current_block->next == NULL)
		{
			connector_table *newblock =
				malloc(CT_BLKSIZE * sizeof(*ctxt->current_element));
			newblock[CT_BLKSIZE-1].next = NULL;
			ctxt->end_current_block->next = newblock;
		} /* else - reuse next block. */

		ctxt->current_element = ctxt->end_current_block->next;
		ctxt->end_current_block = &ctxt->current_element[CT_BLKSIZE-1];
	}

	return ctxt->current_element++;
}

static void free_connector_table(exprune_context *ctxt)
{
	connector_table *x;
	connector_table *t = ctxt->connector_table_element[CT_BLKSIZE-1].next;

	while (t != NULL)
	{
			 x = t[CT_BLKSIZE-1].next;
			 free(t);
			 t = x;
	}

	free(ctxt->ct);
	ctxt->ct = NULL;
	ctxt->ct_size = 0;
}

/**
 * Use the uniquely-assigned number per uppercase part of each
 * connector string as a hash table key.
 *
 * This ensures that connectors with identical uppercase parts
 * will hash to the same place.
 */
static inline unsigned int hash_S(condesc_t * c)
{
	return c->uc_num;
}

/**
 * Returns TRUE if c can match anything in the set S (err. the connector table ct).
 */
static inline bool matches_S(connector_table **ct, int w, condesc_t * c)
{
	connector_table *e;

	for (e = ct[hash_S(c)]; e != NULL; e = e->next)
	{
		if (w > e->farthest_word) continue;
		if (easy_match_desc(e->condesc, c)) return true;
	}
	return false;
}

/* ================================================================= */
/**
 * The purge operations remove all irrelevant stuff from the expression,
 * and free the purged stuff.  A connector is deemed irrelevant if it
 * doesn't match anything in the set S.
 *
 * If an OR or AND type expression node has one child, we can replace it
 * by it's child.  This, of course, is not really necessary, except for
 * performance.
 */

static Exp* purge_Exp(exprune_context *ctxt, int, Exp *, char);

/**
 * Get rid of the current_elements with null expressions
 */
static bool or_purge_operands(exprune_context *ctxt, int w, Exp *e, char dir)
{
#if NOTYET
	const float nullexp_nonexistence = -9999;
	float nullexp_mincost = nullexp_nonexistence;
	int nullexp_count = 0;
#endif

	for (Exp **opdp = &e->operand_first; *opdp != NULL; /* See: NEXT */)
	{
		Exp *opd = *opdp;

#if NOTYET
		if ((opd->type == AND_type) && (opd->operand_first == NULL))
		{
			if (opd->cost > nullexp_mincost) nullexp_mincost = opd->cost;
			nullexp_count++;
		}
		else
#endif
		if (purge_Exp(ctxt, w, opd, dir) == NULL)
		{
			*opdp = opd->operand_next; /* Discard element + NEXT */
			continue;
		}

		opdp = &opd->operand_next; /* NEXT */
	};

#if NOTYET
	if ((nullexp_count > 1) && (nullexp_mincost != nullexp_nonexistence))
	{
		bool nullexp_retained = false;

		for (Exp **opdp = &e->operand_first; *opdp != NULL; /* See: NEXT */)
		{
			Exp *opd = *opdp;

			if ((opd->type == AND_type) && (opd->operand_first == NULL))
			{
				if (!nullexp_retained && opd->cost == nullexp_mincost)
				{
					 nullexp_retained = true;
				}
				else
				{
					*opdp = opd->operand_next; /* Discard element + NEXT */
					continue;
				}
			}

			opdp = &opd->operand_next; /* NEXT */
		}
	}
#endif

	return (e->operand_first != NULL);
}

/**
 * Returns true iff the length of the disjunct list is 0.
 * If this is the case, it frees the structure rooted at l.
 */
static bool and_purge_operands(exprune_context *ctxt, int w, Exp *e, char dir)
{
	for (Exp **opdp = &e->operand_first; *opdp != NULL; /* See: NEXT */)
	{
		Exp *opd = *opdp;

#ifdef NOTYET
		if ((opd->type == AND_type) && (opd->operand_first == NULL))
		{
			e->cost += opd->cost;
			*opdp = opd->operand_next; /* Discard element + NEXT */
		}
#endif

		if (purge_Exp(ctxt, w, opd, dir) == NULL) return false;
		opdp = &opd->operand_next; /* NEXT */
	}
	return true;
}

/**
 * Must be called with a non-null expression.
 * Return NULL iff the expression has no disjuncts.
 * After returning, ctxt->N_deleted contains the number of deleted connectors.
 */
static Exp* purge_Exp(exprune_context *ctxt, int w, Exp *e, char dir)
{
	if (e->type == CONNECTOR_type)
	{
		if (e->dir == dir)
		{
			if (!matches_S(ctxt->ct, (dir == '-') ? w : -w, e->condesc))
			{
				ctxt->N_deleted++;
				return NULL;
			}
		}

		return e;
	}

	if (e->type == AND_type)
	{
		if (!and_purge_operands(ctxt, w, e, dir)) return NULL;
	}
	else /* if we are here, it's OR_type */
	{
		if (!or_purge_operands(ctxt, w, e, dir)) return NULL;
	}

	/* Unary node elimination (for a slight performance improvement). */
	if ((e->operand_first != NULL) && (e->operand_first->operand_next == NULL))
	{
		Exp *opd = e->operand_first;
		opd->cost += e->cost;
		opd->operand_next = e->operand_next;
		*e = *opd;
	}

	return e;
}

static void zero_connector_table(exprune_context *ctxt)
{
	memset(ctxt->ct, 0, sizeof(*ctxt->ct) * ctxt->ct_size);
	ctxt->current_element = ctxt->connector_table_element;
	ctxt->end_current_block = &ctxt->connector_table_element[CT_BLKSIZE-1];
}

/**
 * This function puts connector c into the connector table
 * if one like it isn't already there.
 */
static void insert_connector(exprune_context *ctxt, int farthest_word,
                             condesc_t *c)
{
	unsigned int h;
	connector_table *e;

	h = hash_S(c);

	for (e = ctxt->ct[h]; e != NULL; e = e->next)
	{
		if (c == e->condesc)
		{
			if (e->farthest_word < farthest_word) e->farthest_word = farthest_word;
			return;
		}
	}

	e = ct_element_new(ctxt);
	e->condesc = c;
	e->farthest_word = farthest_word;
	e->next = ctxt->ct[h];
	ctxt->ct[h] = e;
}
/**
 * Put into the set S all of the dir-pointing connectors still in e.
 * Return a list of allocated dummy connectors; these will need to be
 * freed.
 */
static void insert_connectors(exprune_context *ctxt, int w, Exp * e, int dir)
{
	if (e->type == CONNECTOR_type)
	{
		if (e->dir == dir)
		{
			assert(NULL != e->condesc, "NULL connector");
			int farthest_word = (dir == '-') ? -e->farthest_word : e->farthest_word;
			insert_connector(ctxt, farthest_word, e->condesc);
		}
	}
	else
	{
		for (Exp *opd = e->operand_first; opd != NULL; opd = opd->operand_next)
		{
			insert_connectors(ctxt, w, opd, dir);
		}
	}
}

static char *print_expression_sizes(Sentence sent)
{
	X_node * x;
	size_t w, size;
	dyn_str *e = dyn_str_new();

	for (w=0; w<sent->length; w++) {
		size = 0;
		for (x=sent->word[w].x; x!=NULL; x = x->next) {
			size += size_of_expression(x->exp);
		}
		/* XXX alternatives[0] is not really correct, here .. */
		append_string(e, "%s[%zu] ", sent->word[w].alternatives[0], size);
	}
	append_string(e, "\n\n");
	return dyn_str_take(e);
}

void expression_prune(Sentence sent, Parse_Options opts)
{
	size_t w;
	exprune_context ctxt;

	ctxt.opts = opts;
	ctxt.ct_size = sent->dict->contable.num_uc;
	ctxt.ct = malloc(ctxt.ct_size * sizeof(*ctxt.ct));
	zero_connector_table(&ctxt);
	ctxt.end_current_block->next = NULL;

	ctxt.N_deleted = 1;  /* a lie to make it always do at least 2 passes */

	DBG_EXPSIZES("Initial expression sizes\n%s", e);

	for (int pass = 0; ; pass++)
	{
		/* Left-to-right pass */
		/* For every word */
		for (w = 0; w < sent->length; w++)
		{
			/* For every expression in word */
			for (X_node **xp = &sent->word[w].x; *xp != NULL; /* See: NEXT */)
			{
				X_node *x = *xp;

				DBG(pass, w, "l->r pass before purging");
				//if (pass == 0 && w == 0) {printf("Exp: ");prt_exp_mem(x->exp, 0);}
				x->exp = purge_Exp(&ctxt, w, x->exp, '-');
				DBG(pass, w, "l->r pass after purging");

				/* Get rid of X_nodes with NULL exp */
				if (x->exp == NULL)
				{
					*xp = x->next; /* NEXT - set current X_node to the next one */
				}
				else
				{
					xp = &x->next; /* NEXT */
				}
			}

			for (X_node *x = sent->word[w].x; x != NULL; x = x->next)
			{
				insert_connectors(&ctxt, w, x->exp, '+');
			}
		}

		DBG_EXPSIZES("l->r pass removed %d\n%s", ctxt.N_deleted, e);

		if (ctxt.N_deleted == 0) break;
		zero_connector_table(&ctxt);

		/* Right-to-left pass */
		ctxt.N_deleted = 0;

		for (w = sent->length-1; w != (size_t) -1; w--)
		{
			/* For every expression in word */
			for (X_node **xp = &sent->word[w].x; *xp != NULL; /* See: NEXT */)
			{
				X_node *x = *xp;

				DBG(pass, w, "r->l pass before purging");
				x->exp = purge_Exp(&ctxt, w, x->exp, '+');
				DBG(pass, w, "r->l pass after purging");

				/* Get rid of X_nodes with NULL exp */
				if (x->exp == NULL)
				{
					*xp = x->next; /* NEXT - set current X_node to the next one */
				}
				else
				{
					xp = &x->next; /* NEXT */
				}
			}

			for (X_node *x = sent->word[w].x; x != NULL; x = x->next)
			{
				insert_connectors(&ctxt, w, x->exp, '-');
			}
		}

		DBG_EXPSIZES("r->l pass removed %d\n%s", ctxt.N_deleted, e);

		if (ctxt.N_deleted == 0) break;
		zero_connector_table(&ctxt);
		ctxt.N_deleted = 0;
	}

	free_connector_table(&ctxt);
}

#if 0 // VERY_DEAD_NO_GOOD_IDEA

/* ============================================================x */
/*
  The second algorithm eliminates disjuncts that are dominated by
  another.  It works by hashing them all, and checking for domination.

  Consider the idea of deleting a disjunct if it is dominated (in terms of
  what it can match) by some other disjunct on the same word.  This has
  been implemented below.  There are three problems with it:

  (1) It is almost never the case that any disjuncts are eliminated.

  (2) connector_matches_alam may not be exactly correct.

  (3) The linkage that is eliminated by this, might just be the one that
      passes post-processing, as the following example shows.
      This is pretty silly, and should probably be changed.

> telling John how our program works would be stupid
Accepted (2 linkages, 1 with no P.P. violations)
  Linkage 1, cost vector = (0, 0, 7)

   +------------------G-----------------+
   +-----R-----+----CL----+             |
   +---O---+   |   +---D--+---S---+     +--I-+-AI-+
   |       |   |   |      |       |     |    |    |
telling.g John how our program.n works would be stupid

               /////         CLg    <---CLg--->  CL      telling.g
 (g)           telling.g     G      <---G----->  G       would
 (g) (d)       telling.g     R      <---R----->  R       how
 (g) (d)       telling.g     O      <---O----->  O       John
 (g) (d)       how           CLe    <---CLe--->  CL      program.n
 (g) (d) (e)   our           D      <---Ds---->  Ds      program.n
 (g) (d) (e)   program.n     Ss     <---Ss---->  Ss      works
 (g)           would         I      <---Ix---->  Ix      be
 (g)           be            AI     <---AIi--->  AIi     stupid

(press return for another)
>
  Linkage 2 (bad), cost vector = (0, 0, 7)

   +------------------G-----------------+
   +-----R-----+----CL----+             |
   +---O---+   |   +---D--+---S---+     +--I-+-AI-+
   |       |   |   |      |       |     |    |    |
telling.g John how our program.n works would be stupid

               /////         CLg    <---CLg--->  CL      telling.g
 (g)           telling.g     G      <---G----->  G       would
 (g) (d)       telling.g     R      <---R----->  R       how
 (g) (d)       telling.g     O      <---O----->  O       John
 (g) (d)       how           CLe    <---CLe--->  CL      program.n
 (g) (d) (e)   our           D      <---Ds---->  Ds      program.n
 (g) (d) (e)   program.n     Ss     <---Ss---->  Ss      works
 (g)           would         I      <---Ix---->  Ix      be
 (g)           be            AI     <---AI---->  AI      stupid

P.P. violations:
      Special subject rule violated
*/

/**
 * hash function that takes a string and a seed value i
 */
static int string_hash(disjunct_dup_table *dt, const char * s, int i)
{
	for(;*s != '\0';s++) i = i + (i<<1) + randtable[(*s + i) & (RTSIZE-1)];
	return (i & (dt->dup_table_size-1));
}

/**
 * This returns true if the connector a matches everything that b
 *  matches, and possibly more.  (alam=at least as much)
 *
 * TRUE for equal connectors.
 * remains TRUE if multi-match added to the first.
 * remains TRUE if subsrcripts deleted from the first.
 */
static bool connector_matches_alam(Connector * a, Connector * b)
{
	char * s, * t, *u;
	if (((!a->multi) && b->multi) ||
		(a->label != b->label))  return false;
	s = a->string;
	t = b->string;

	/* isupper -- connectors cannot be UTF8 at this time */
	while (isupper(*s) || isupper(*t))
	{
		if (*s == *t) {
			s++;
			t++;
		} else return false;
	}
	while ((*s != '\0') && (*t != '\0')) {
		if ((*s == *t) || (*s == '*')) {
			s++;
			t++;
		} else return false;
	}
	while ((*s != '\0') && (*s == '*')) s++;
	return (*s == '\0');
}


/**
 * This hash function that takes a connector and a seed value i.
 * It only looks at the leading upper case letters of
 * the string, and the label.  This ensures that if two connectors
 * match, then they must hash to the same place.
 */
static int conn_hash(Connector * c, int i)
{
	int nb;
	const char * s;
	s = c->string;

	i = i + (i<<1) + randtable[(c->label + i) & (RTSIZE-1)];
	nb = is_utf8_upper(s);
	while (nb)
	{
		i = i + (i<<1) + randtable[(*s + i) & (RTSIZE-1)];
		s += nb;
		nb = is_utf8_upper(s);
	}
	return i;
}

static inline int pconnector_hash(disjunct_dup_table *dt, Connector * c, int i)
{
	i = conn_hash(c, i);
	return (i & (ct->dup_table_size-1));
}

/**
 * This is a hash function for disjuncts
 */
static int hash_disjunct(disjunct_dup_table *dt, Disjunct * d)
{
	int i;
	Connector *e;
	i = 0;
	for (e = d->left; e != NULL; e = e->next)
	{
		i = pconnector_hash(dt, e, i);
	}
	for (e = d->right; e != NULL; e = e->next)
	{
		i = pconnector_hash(dt, e, i);
	}
	return string_hash(dt, d->string, i);
}

/**
 * Returns TRUE if disjunct d1 can match anything that d2 can
 * if this happens, it constitutes a proof that there is absolutely
 * no use for d2.
 */
static bool disjunct_matches_alam(Disjunct * d1, Disjunct * d2)
{
	Connector *e1, *e2;
	if (d1->cost > d2->cost) return false;
	e1 = d1->left;
	e2 = d2->left;
	while ((e1!=NULL) && (e2!=NULL))
	{
		if (!connector_matches_alam(e1,e2)) break;
		e1 = e1->next;
		e2 = e2->next;
	}
	if ((e1!=NULL) || (e2!=NULL)) return false;
	e1 = d1->right;
	e2 = d2->right;
	while ((e1!=NULL) && (e2!=NULL))
	{
		if (!connector_matches_alam(e1,e2)) break;
		e1 = e1->next;
		e2 = e2->next;
	}
	if ((e1!=NULL) || (e2!=NULL)) return false;
	return (strcmp(d1->string, d2->string) == 0);
}

/**
 * Takes the list of disjuncts pointed to by d, eliminates all
 * duplicates, and returns a pointer to a new list.
 * It frees the disjuncts that are eliminated.
 */
Disjunct * eliminate_duplicate_disjuncts(Disjunct * d)
{
	int i, h, count;
	Disjunct *dn, *dx, *dxn, *front;
	count = 0;
	disjunct_dup_table *dt;

	dt = disjunct_dup_table_new(next_power_of_two_up(2 * count_disjuncts(d)));

	for (;d!=NULL; d = dn)
	{
		dn = d->next;
		h = hash_disjunct(d);

		front = NULL;
		for (dx = dt->dup_table[h]; dx != NULL; dx = dxn)
		{
			dxn = dx->next;
			if (disjunct_matches_alam(dx,d))
			{
				/* we know that d should be killed */
				d->next = NULL;
				free_disjuncts(d);
				count++;
				front = catenate_disjuncts(front, dx);
				break;
			} else if (disjunct_matches_alam(d,dx)) {
				/* we know that dx should be killed off */
				dx->next = NULL;
				free_disjuncts(dx);
				count++;
			} else {
				/* neither should be killed off */
				dx->next = front;
				front = dx;
			}
		}
		if (dx == NULL) {
			/* we put d in the table */
			d->next = front;
			front = d;
		}
		dt->dup_table[h] = front;
	}

	/* d is now NULL */
	for (i = 0; i < dt->dup_table_size; i++)
	{
		for (dx = dt->dup_table[i]; dx != NULL; dx = dxn)
		{
			dxn = dx->next;
			dx->next = d;
			d = dx;
		}
	}

	if ((verbosity > 2) && (count != 0)) printf("killed %d duplicates\n", count);

	disjunct_dup_table_delete(dt);
	return d;
}

/* ============================================================x */
#endif /* VERY_DEAD_NO_GOOD_IDEA */
