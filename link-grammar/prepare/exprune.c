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
#include "dict-common/dict-api.h"        // expression_stringify
#include "dict-common/dict-utils.h"      // size_of_expression
#include "print/print-util.h"            // dyn_str functions
#include "string-set.h"
#include "tokenize/word-structures.h"    // for Word_struct
#include "exprune.h"

#define D_EXPRUNE 9

#ifdef DEBUG
#define DBG(p, w, X) \
	if (verbosity_level(+D_EXPRUNE))\
	{\
		char *e = expression_stringify(x->exp);\
		err_msg(lg_Trace, "pass%d w%zu: ", p, w);\
		err_msg(lg_Trace, X ": %s\n", e);\
		free(e);\
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

/* ================================================================= */
/**
 * Here is expression pruning.  This is done even before the expressions
 * are turned into lists of disjuncts.
 *
 * This uses many of the same data structures and functions that are used
 * by prune.
 *
 * The purge operations remove all irrelevant stuff from the expression,
 * and free the purged stuff.  A connector is deemed irrelevant if its
 * string pointer has been set to NULL.  The passes through the sentence
 * have the job of doing this.
 *
 * If an OR or AND type expression node has one child, we can replace it
 * by its child.  This, of course, is not really necessary, except for
 * performance(?).
 */

static Exp* purge_Exp(Exp *);

/**
 * Get rid of the current_elements with null expressions
 */
static E_list * or_purge_E_list(E_list * l)
{
	E_list * el;
	if (l == NULL) return NULL;
	if ((l->e = purge_Exp(l->e)) == NULL)
	{
		el = or_purge_E_list(l->next);
		xfree((char *)l, sizeof(E_list));
		return el;
	}
	l->next = or_purge_E_list(l->next);
	return l;
}

/**
 * Returns 0 iff the length of the disjunct list is 0.
 * If this is the case, it frees the structure rooted at l.
 */
static int and_purge_E_list(E_list * l)
{
	if (l == NULL) return 1;
	if ((l->e = purge_Exp(l->e)) == NULL)
	{
		free_E_list(l->next);
		xfree((char *)l, sizeof(E_list));
		return 0;
	}
	if (and_purge_E_list(l->next) == 0)
	{
		free_Exp(l->e);
		xfree((char *)l, sizeof(E_list));
		return 0;
	}
	return 1;
}

/**
 * Must be called with a non-null expression.
 * Return NULL iff the expression has no disjuncts.
 */
static Exp* purge_Exp(Exp *e)
{
	if (e->type == CONNECTOR_type)
	{
		if (e->u.c.desc == NULL)
		{
			xfree((char *)e, sizeof(Exp));
			return NULL;
		}
		else
		{
			return e;
		}
	}
	if (e->type == AND_type)
	{
		if (and_purge_E_list(e->u.l) == 0)
		{
			xfree((char *)e, sizeof(Exp));
			return NULL;
		}
	}
	else /* if we are here, its OR_type */
	{
		e->u.l = or_purge_E_list(e->u.l);
		if (e->u.l == NULL)
		{
			xfree((char *)e, sizeof(Exp));
			return NULL;
		}
	}

/* This code makes it kill off nodes that have just one child
   (1) It's going to give an insignificant speed-up
   (2) Costs have not been handled correctly here.
   The code is excised for these reasons.
*/
/*
	if ((e->u.l != NULL) && (e->u.l->next == NULL))
	{
		ne = e->u.l->e;
		xfree((char *) e->u.l, sizeof(E_list));
		xfree((char *) e, sizeof(Exp));
		return ne;
	}
*/
	return e;
}

/**
 * This hash function only looks at the leading upper case letters of
 * the connector string, and the label fields.  This ensures that if two
 * strings match (formally), then they must hash to the same place.
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
		if (e->farthest_word <= 0)
		{
			if (w < -e->farthest_word) continue;
		}
		else
		{
			if (w > e->farthest_word) continue;
		}
		if (easy_match_desc(e->condesc, c)) return true;
	}
	return false;
}

static void zero_connector_table(exprune_context *ctxt)
{
	memset(ctxt->ct, 0, sizeof(*ctxt->ct) * ctxt->ct_size);
	ctxt->current_element = ctxt->connector_table_element;
	ctxt->end_current_block = &ctxt->connector_table_element[CT_BLKSIZE-1];
}

/**
 * Mark as dead all of the dir-pointing connectors
 * in e that are not matched by anything in the current set.
 * Returns the number of connectors so marked.
 */
static int mark_dead_connectors(connector_table **ct, int w, Exp * e, char dir)
{
	int count;
	count = 0;
	if (e->type == CONNECTOR_type)
	{
		if (e->u.c.dir == dir)
		{
			if (!matches_S(ct, w, e->u.c.desc))
			{
				e->u.c.desc = NULL;
				count++;
			}
		}
	}
	else
	{
		E_list *l;
		for (l = e->u.l; l != NULL; l = l->next)
		{
			count += mark_dead_connectors(ct, w, l->e, dir);
		}
	}
	return count;
}

/**
 * This function puts connector c into the connector table
 * if one like it isn't already there.
 */
static void insert_connector(exprune_context *ctxt, int farthest_word, condesc_t * c)
{
	unsigned int h;
	connector_table *e;

	h = hash_S(c);

	for (e = ctxt->ct[h]; e != NULL; e = e->next)
	{
		if (c == e->condesc)
		{
			{
				if (e->farthest_word < farthest_word) e->farthest_word = farthest_word;
			}
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
		if (e->u.c.dir == dir)
		{
			assert(NULL != e->u.c.desc, "NULL connector");
			Connector c;
			c.conn.desc = e->u.c.desc;

			set_connector_length_limit(&c, ctxt->opts);
			int farthest_word = (dir == '-') ? -MAX(0, w-c.length_limit) :
				                              w+c.length_limit;
			insert_connector(ctxt, farthest_word, e->u.c.desc);
		}
	}
	else
	{
		E_list *l;
		for (l = e->u.l; l != NULL; l = l->next)
		{
			insert_connectors(ctxt, w, l->e, dir);
		}
	}
}

/**
 * This removes the expressions that are empty from the list corresponding
 * to word w of the sentence.
 */
static void clean_up_expressions(Sentence sent, int w)
{
	X_node head_node, *d, *d1;
	d = &head_node;
	d->next = sent->word[w].x;
	while (d->next != NULL)
	{
		if (d->next->exp == NULL)
		{
			d1 = d->next;
			d->next = d1->next;
			xfree((char *)d1, sizeof(X_node));
		}
		else
		{
			d = d->next;
		}
	}
	sent->word[w].x = head_node.next;
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
	int N_deleted;
	X_node * x;
	size_t w;
	exprune_context ctxt;

	ctxt.opts = opts;
	ctxt.ct_size = sent->dict->contable.num_uc;
	ctxt.ct = malloc(ctxt.ct_size * sizeof(*ctxt.ct));
	zero_connector_table(&ctxt);
	ctxt.end_current_block->next = NULL;

	N_deleted = 1;  /* a lie to make it always do at least 2 passes */

	DBG_EXPSIZES("Initial expression sizes\n%s", e);

	int pass = -1;
	while (pass++)
	{
		/* Left-to-right pass */
		/* For every word */
		for (w = 0; w < sent->length; w++)
		{
			/* For every expression in word */
			for (x = sent->word[w].x; x != NULL; x = x->next)
			{
				DBG(pass, w, "l->r pass before marking");
				N_deleted += mark_dead_connectors(ctxt.ct, w, x->exp, '-');
				DBG(pass, w, "l->r pass after marking");
			}
			for (x = sent->word[w].x; x != NULL; x = x->next)
			{
				DBG(pass, w, "l->r pass before purging");
				x->exp = purge_Exp(x->exp);
				DBG(pass, w, "l->r pass after purging");
			}

			/* gets rid of X_nodes with NULL exp */
			clean_up_expressions(sent, w);
			for (x = sent->word[w].x; x != NULL; x = x->next)
			{
				insert_connectors(&ctxt, w, x->exp, '+');
			}
		}

		DBG_EXPSIZES("l->r pass removed %d\n%s", N_deleted, e);

		zero_connector_table(&ctxt);
		if (N_deleted == 0) break;

		/* Right-to-left pass */
		N_deleted = 0;
		for (w = sent->length-1; w != (size_t) -1; w--)
		{
			for (x = sent->word[w].x; x != NULL; x = x->next)
			{
				DBG(pass, w, "r->l pass before marking");
				N_deleted += mark_dead_connectors(ctxt.ct, w, x->exp, '+');
				DBG(pass, w, "r->l pass after marking");
			}
			for (x = sent->word[w].x; x != NULL; x = x->next)
			{
				DBG(pass, w, "r->l pass before purging");
				x->exp = purge_Exp(x->exp);
				DBG(pass, w, "r->l pass after purging");
			}
			clean_up_expressions(sent, w);  /* gets rid of X_nodes with NULL exp */
			for (x = sent->word[w].x; x != NULL; x = x->next)
			{
				insert_connectors(&ctxt, w, x->exp, '-');
			}
		}

		DBG_EXPSIZES("r->l pass removed %d\n%s", N_deleted, e);

		zero_connector_table(&ctxt);
		if (N_deleted == 0) break;
		N_deleted = 0;
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
	for (e = d->left ; e != NULL; e = e->next)
	{
		i = pconnector_hash(dt, e, i);
	}
	for (e = d->right ; e != NULL; e = e->next)
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
