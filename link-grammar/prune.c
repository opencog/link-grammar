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

#include "api-structures.h"
#include "count.h"
#include "dict-api.h"  /* for print_expression when debugging */
#include "disjunct-utils.h"
#include "externs.h"
#include "post-process.h"
#include "print.h"
#include "prune.h"
#include "resources.h"
#include "word-utils.h"

#define CONTABSZ 8192
typedef Connector * connector_table;

typedef struct disjunct_dup_table_s disjunct_dup_table;
struct disjunct_dup_table_s
{
	unsigned int dup_table_size;
	Disjunct ** dup_table;
};

/* Indicator that this connector cannot be used -- that its "obsolete".  */
#define BAD_WORD (MAX_SENTENCE+1)

typedef struct c_list_s C_list;
struct c_list_s
{
	C_list * next;
	Connector * c;
	bool shallow;
};

typedef struct power_table_s power_table;
struct power_table_s
{
	unsigned int power_table_size;
	unsigned int *l_table_size;  /* the sizes of the hash tables */
	unsigned int *r_table_size;
	C_list *** l_table;
	C_list *** r_table;
};

typedef struct cms_struct Cms;
struct cms_struct
{
	Cms * next;
	const char * name;
	int count;	  /* the number of times this is in the multiset */
};

#define CMS_SIZE (2<<10)
typedef struct multiset_table_s multiset_table;
struct multiset_table_s
{
	Cms * cms_table[CMS_SIZE];
};

typedef struct prune_context_s prune_context;
struct prune_context_s
{
	bool null_links;
	int power_cost;
	int N_changed;   /* counts the number of changes
						   of c->word fields in a pass */
	power_table *pt;
	Sentence sent;
};

/*

  The algorithms in this file prune disjuncts from the disjunct list
  of the sentence that can be elimininated by a simple checks.  The first
  check works as follows:

  A series of passes are made through the sentence, alternating
  left-to-right and right-to-left.  Consier the left-to-right pass (the
  other is symmetric).  A set S of connectors is maintained (initialized
  to be empty).  Now the disjuncts of the current word are processed.
  If a given disjunct's left pointing connectors have the property that
  at least one of them has no connector in S to which it can be matched,
  then that disjunct is deleted. Now the set S is augmented by the right
  connectors of the remaining disjuncts of that word.  This completes
  one word.  The process continues through the words from left to right.
  Alternate passes are made until no disjunct is deleted.

  It worries me a little that if there are some really huge disjuncts lists,
  then this process will probably do nothing.  (This fear turns out to be
  unfounded.)

  Notes:  Power pruning will not work if applied before generating the
  "and" disjuncts.  This is because certain of it's tricks don't work.
  Think about this, and finish this note later....
  Also, currently I use the standard connector match procedure instead
  of the pruning one, since I know power pruning will not be used before
  and generation.  Replace this to allow power pruning to work before
  generating and disjuncts.

  Currently it seems that normal pruning, power pruning, and generation,
  pruning, and power pruning (after "and" generation) and parsing take
  about the same amount of time.  This is why doing power pruning before
  "and" generation might be a very good idea.

  New idea:  Suppose all the disjuncts of a word have a connector of type
  c pointing to the right.  And further, suppose that there is exactly one
  word to its right containing that type of connector pointing to the left.
  Then all the other disjuncts on the latter word can be deleted.
  (This situation is created by the processing of "either...or", and by
  the extra disjuncts added to a "," neighboring a conjunction.)

*/

/**
 * This hash function only looks at the leading upper case letters of
 * the connector string, and the label fields.  This ensures that if two
 * strings match (formally), then they must hash to the same place.
 */
static inline unsigned int hash_S(Connector * c)
{
	unsigned int h = connector_hash(c);
	return (h & (CONTABSZ-1));
}

/**
 * This is almost identical to do_match().  Its reason for existance
 * is the rather subtle fact that with "and" can transform a "Ss"
 * connector into "Sp".  This means that in order for pruning to
 * work, we must allow a "Ss" connector on word match an "Sp" connector
 * on a word to its right.  This is what this version of match allows.
 * We assume that a is on a word to the left of b.
 */
static inline bool prune_match(Connector *a, Connector *b)
{
	return easy_match(a->string, b->string);
}

static void zero_connector_table(connector_table *ct)
{
	memset(ct, 0, sizeof(Connector *) * CONTABSZ);
}

/**
 * This function puts connector c into the connector table
 * if one like it isn't already there.
 */
static void insert_connector(connector_table *ct, Connector * c)
{
	unsigned int h;
	Connector * e;

	h = hash_S(c);

	for (e = ct[h]; e != NULL; e = e->tableNext)
	{
		if (strcmp(c->string, e->string) == 0)
			return;
	}
	c->tableNext = ct[h];
	ct[h] = c;
}

/*
   The second algorithm eliminates disjuncts that are dominated by
   another.  It works by hashing them all, and checking for domination.
*/

#if 0
/* ============================================================x */

/*
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
#endif

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
 * performance(?)
 */

static Exp* purge_Exp(Exp *);

/**
 * Get rid of the elements with null expressions
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
		if (e->u.string == NULL)
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
 * Returns TRUE if c can match anything in the set S (err. the connector table ct).
 */
static inline bool matches_S(connector_table *ct, Connector * c, char dir)
{
	Connector * e;
	unsigned int h = hash_S(c);

	if (dir == '-')
	{
		for (e = ct[h]; e != NULL; e = e->tableNext)
		{
			if (prune_match(e, c)) return true;
		}
		return false;
	}
	else
	{
		for (e = ct[h]; e != NULL; e = e->tableNext)
		{
			if (prune_match(c, e)) return true;
		}
		return false;
	}
}

/**
 * Mark as dead all of the dir-pointing connectors
 * in e that are not matched by anything in the current set.
 * Returns the number of connectors so marked.
 */
static int mark_dead_connectors(connector_table *ct, Exp * e, char dir)
{
	int count;
	count = 0;
	if (e->type == CONNECTOR_type)
	{
		if (e->dir == dir)
		{
			Connector dummy;
			init_connector(&dummy);
			dummy.string = e->u.string;
			if (!matches_S(ct, &dummy, dir))
			{
				e->u.string = NULL;
				count++;
			}
		}
	}
	else
	{
		E_list *l;
		for (l = e->u.l; l != NULL; l = l->next)
		{
			count += mark_dead_connectors(ct, l->e, dir);
		}
	}
	return count;
}

/**
 * Put into the set S all of the dir-pointing connectors still in e.
 * Return a list of allocated dummy connectors; these will need to be
 * freed.
 */
static Connector * insert_connectors(connector_table *ct, Exp * e,
                                     Connector *alloc_list, int dir)
{
	if (e->type == CONNECTOR_type)
	{
		if (e->dir == dir)
		{
			Connector *dummy = connector_new();
			dummy->string = e->u.string;
			insert_connector(ct, dummy);
			dummy->next = alloc_list;
			alloc_list = dummy;
		}
	}
	else
	{
		E_list *l;
		for (l=e->u.l; l!=NULL; l=l->next)
		{
			alloc_list = insert_connectors(ct, l->e, alloc_list, dir);
		}
	}
	return alloc_list;
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

/* #define DBG(X) X */
#define DBG(X)

void expression_prune(Sentence sent)
{
	int N_deleted;
	X_node * x;
	size_t w;
	Connector *ct[CONTABSZ];
	Connector *dummy_list = NULL;

	zero_connector_table(ct);

	N_deleted = 1;  /* a lie to make it always do at least 2 passes */

	while (1)
	{
		/* Left-to-right pass */
		/* For every word */
		for (w = 0; w < sent->length; w++)
		{
			/* For every expression in word */
			for (x = sent->word[w].x; x != NULL; x = x->next)
			{
DBG(printf("before marking: "); print_expression(x->exp); printf("\n"););
				N_deleted += mark_dead_connectors(ct, x->exp, '-');
DBG(printf(" after marking: "); print_expression(x->exp); printf("\n"););
			}
			for (x = sent->word[w].x; x != NULL; x = x->next)
			{
DBG(printf("before purging: "); print_expression(x->exp); printf("\n"););
				x->exp = purge_Exp(x->exp);
DBG(printf("after purging: "); print_expression(x->exp); printf("\n"););
			}

			/* gets rid of X_nodes with NULL exp */
			clean_up_expressions(sent, w);
			for (x = sent->word[w].x; x != NULL; x = x->next)
			{
				dummy_list = insert_connectors(ct, x->exp, dummy_list, '+');
			}
		}

		if (verbosity > 2)
		{
			printf("l->r pass removed %d\n", N_deleted);
			print_expression_sizes(sent);
		}

		/* Free the allocated dummy connectors */
		free_connectors(dummy_list);
		dummy_list = NULL;
		zero_connector_table(ct);

		if (N_deleted == 0) break;

		/* Right-to-left pass */
		N_deleted = 0;
		for (w = sent->length-1; w != (size_t) -1; w--)
		{
			for (x = sent->word[w].x; x != NULL; x = x->next)
			{
/*	 printf("before marking: "); print_expression(x->exp); printf("\n"); */
				N_deleted += mark_dead_connectors(ct, x->exp, '+');
/*	 printf("after marking: "); print_expression(x->exp); printf("\n"); */
			}
			for (x = sent->word[w].x; x != NULL; x = x->next)
			{
/*	 printf("before perging: "); print_expression(x->exp); printf("\n"); */
				x->exp = purge_Exp(x->exp);
/*	 printf("after perging: "); print_expression(x->exp); printf("\n"); */
			}
			clean_up_expressions(sent, w);  /* gets rid of X_nodes with NULL exp */
			for (x = sent->word[w].x; x != NULL; x = x->next)
			{
				dummy_list = insert_connectors(ct, x->exp, dummy_list, '-');
			}
		}

		if (verbosity > 2)
		{
			printf("r->l pass removed %d\n", N_deleted);
			print_expression_sizes(sent);
		}

		/* Free the allocated dummy connectors */
		free_connectors(dummy_list);
		dummy_list = NULL;
		zero_connector_table(ct);
		if (N_deleted == 0) break;
		N_deleted = 0;
	}
}



/*
  Here is what you've been waiting for: POWER-PRUNE

  The kinds of constraints it checks for are the following:

	1) successive connectors on the same disjunct have to go to
	   nearer and nearer words.

	2) two deep connectors cannot attach to each other
	   (A connectors is deep if it is not the first in its list; it
	   is shallow if it is the first in its list; it is deepest if it
	   is the last on its list.)

	3) on two adjacent words, a pair of connectors can be used
	   only if they're the deepest ones on their disjuncts

	4) on two non-adjacent words, a pair of connectors can be used only
	   if not [both of them are the deepest].

   The data structure consists of a pair of hash tables on every word.
   Each bucket of a hash table has a list of pointers to connectors.
   These nodes also store if the chosen connector is shallow.
*/
/*
   As with normal pruning, we make alternate left->right and right->left
   passes.  In the R->L pass, when we're on a word w, we make use of
   all the left-pointing hash tables on the words to the right of w.
   After the pruning on this word, we build the left-pointing hash table
   this word.  This guarantees idempotence of the pass -- after doing an
   L->R, doing another would change nothing.

   Each connector has an integer c_word field.  This refers to the closest
   word that it could be connected to.  These are initially determined by
   how deep the connector is.  For example, a deepest connector can connect
   to the neighboring word, so its c_word field is w+1 (w-1 if this is a left
   pointing connector).  It's neighboring shallow connector has a c_word
   value of w+2, etc.

   The pruning process adjusts these c_word values as it goes along,
   accumulating information about any way of linking this sentence.
   The pruning process stops only after no disjunct is deleted and no
   c_word values change.

   The difference between RUTHLESS and GENTLE power pruning is simply
   that GENTLE uses the deletable region array, and RUTHLESS does not.
   So we can get the effect of these two different methods simply by
   always unsuring that deletable[][] has been defined.  With nothing
   deletable, this is equivalent to RUTHLESS.   --DS, 7/97
*/

/**
 * returns the number of connectors in the left lists of the disjuncts.
 */
static int left_connector_count(Disjunct * d)
{
	Connector *c;
	int i=0;
	for (;d!=NULL; d=d->next) {
		for (c = d->left; c!=NULL; c = c->next) i++;
	}
	return i;
}

static int right_connector_count(Disjunct * d)
{
	Connector *c;
	int i=0;
	for (;d!=NULL; d=d->next) {
	  for (c = d->right; c!=NULL; c = c->next) i++;
	}
	return i;
}

static void free_C_list(C_list * t)
{
	C_list *xt;
	for (; t!=NULL; t=xt) {
		xt = t->next;
		xfree((char *)t, sizeof(C_list));
	}
}

/**
 * free all of the hash tables and C_lists
 */
static void power_table_delete(power_table *pt)
{
	unsigned int w;
	unsigned int i;

	for (w = 0; w < pt->power_table_size; w++)
	{
		for (i = 0; i < pt->l_table_size[w]; i++)
		{
			free_C_list(pt->l_table[w][i]);
		}
		xfree((char *)pt->l_table[w], pt->l_table_size[w] * sizeof (C_list *));

		for (i = 0; i < pt->r_table_size[w]; i++)
		{
			free_C_list(pt->r_table[w][i]);
		}
		xfree((char *)pt->r_table[w], pt->r_table_size[w] * sizeof (C_list *));
	}
	xfree(pt->l_table_size, 2 * pt->power_table_size * sizeof(unsigned int));
	xfree(pt->l_table, 2 * pt->power_table_size * sizeof(C_list **));
	xfree(pt, sizeof(power_table));
}

/**
 * The disjunct d (whose left or right pointer points to c) is put
 * into the appropriate hash table
 */
static void put_into_power_table(unsigned int size, C_list ** t, Connector * c, bool shal)
{
	unsigned int h;
	C_list * m;
	h = connector_hash(c) & (size-1);
	m = (C_list *) xalloc (sizeof(C_list));
	m->next = t[h];
	t[h] = m;
	m->c = c;
	m->shallow = shal;
}

static int set_dist_fields(Connector * c, size_t w, int delta)
{
	int i;
	if (c == NULL) return (int) w;
	i = set_dist_fields(c->next, w, delta) + delta;
	c->word = i;
	return i;
}

/**
 * Allocates and builds the initial power hash tables
 */
static power_table * power_table_new(Sentence sent)
{
	power_table *pt;
	size_t w, len;
	unsigned int i, size;
	C_list ** t;
	Disjunct * d, * xd, * head;
	Connector * c;

	pt = (power_table *) xalloc (sizeof(power_table));
	pt->power_table_size = sent->length;
	pt->l_table_size = xalloc (2 * sent->length * sizeof(unsigned int));
	pt->r_table_size = pt->l_table_size + sent->length;
	pt->l_table = xalloc (2 * sent->length * sizeof(C_list **));
	pt->r_table = pt->l_table + sent->length;

   /* First, we initialize the word fields of the connectors, and
	  eliminate those disjuncts with illegal connectors */
	for (w=0; w<sent->length; w++)
	{
	  head = NULL;
	  for (d=sent->word[w].d; d!=NULL; d=xd) {
		  xd = d->next;
		  if ((set_dist_fields(d->left, w, -1) < 0) ||
			  (set_dist_fields(d->right, w, 1) >= (int) sent->length)) {
			  d->next = NULL;
			  free_disjuncts(d);
		  } else {
			  d->next = head;
			  head = d;
		  }
	  }
	  sent->word[w].d = head;
	}

	for (w=0; w<sent->length; w++)
	{
		/* The below uses variable-sized hash tables. This seems to
		 * provide performance that is equal or better than the best
		 * fixed-size prformance.
		 * The best fixed-size performance seems to come at about
		 * a 1K table size, for both English and Russian. (Both have
		 * about 100 fixed link-types, and many thousands of auto-genned
		 * link types (IDxxx idioms for both, LLxxx suffix links for
		 * Russian).  Plusses and minuses:
		 * + small fixed tables are faster to initialize.
		 * - small fixed tables have more collisions
		 * - variable-size tables require counting connectors.
		 *   (and the more complex code to go with)
		 * CPU cache-size effects ...
		 * Strong depenence on the hashing algo!
		 */
		len = left_connector_count(sent->word[w].d);
		size = next_power_of_two_up(len);
#define TOPSZ 32768
		if (TOPSZ < size) size = TOPSZ;
		pt->l_table_size[w] = size;
		t = pt->l_table[w] = (C_list **) xalloc(size * sizeof(C_list *));
		for (i=0; i<size; i++) t[i] = NULL;

		for (d=sent->word[w].d; d!=NULL; d=d->next) {
			c = d->left;
			if (c != NULL) {
				put_into_power_table(size, t, c, true);
				for (c=c->next; c!=NULL; c=c->next){
					put_into_power_table(size, t, c, false);
				}
			}
		}

		len = right_connector_count(sent->word[w].d);
		size = next_power_of_two_up(len);
		if (TOPSZ < size) size = TOPSZ;
		pt->r_table_size[w] = size;
		t = pt->r_table[w] = (C_list **) xalloc(size * sizeof(C_list *));
		for (i=0; i<size; i++) t[i] = NULL;

		for (d=sent->word[w].d; d!=NULL; d=d->next) {
			c = d->right;
			if (c != NULL) {
				put_into_power_table(size, t, c, true);
				for (c=c->next; c!=NULL; c=c->next){
					put_into_power_table(size, t, c, false);
				}
			}
		}
	}

	return pt;
}

/**
 * This runs through all the connectors in this table, and eliminates those
 * who are obsolete.  The word fields of an obsolete one has been set to
 * BAD_WORD.
 */
static void clean_table(unsigned int size, C_list ** t)
{
	unsigned int i;
	C_list * m, * xm, * head;
	for (i = 0; i < size; i++) {
		head = NULL;
		for (m = t[i]; m != NULL; m = xm) {
			xm = m->next;
			if (m->c->word != BAD_WORD) {
				m->next = head;
				head = m;
			} else {
				xfree((char *) m, sizeof(C_list));
			}
		}
		t[i] = head;
	}
}

/**
 * This takes two connectors (and whether these are shallow or not)
 * (and the two words that these came from) and returns TRUE if it is
 * possible for these two to match based on local considerations.
 */
static bool possible_connection(prune_context *pc,
                               Connector *lc, Connector *rc,
                               bool lshallow, bool rshallow,
                               int lword, int rword)
{
	int dist;
	if ((!lshallow) && (!rshallow)) return false;

	/* Two deep connectors can't work */
	if ((lc->word > rword) || (rc->word < lword)) return false;

	assert(lword < rword, "Bad word order in possible connection.");

	/* Word range constraints */
	if (lword == rword-1) {
		if (!((lc->next == NULL) && (rc->next == NULL))) return false;
	}
	else
	/* If the words are NOT next to each other, then there must be
	 * at least one intervening connector (i.e. cannot have both
	 * lc->next amnd rc->next being null).  But we only enforce this
	 * when we think its still possible to have a complete parse,
	 * i.e. before well allow null-linked words.
	 */
	if ((!pc->null_links) &&
	    (lc->next == NULL) &&
	    (rc->next == NULL) &&
	    (!lc->multi) && (!rc->multi))
	{
		return false;
	}

	dist = rword - lword;
	if (dist > lc->length_limit || dist > rc->length_limit) return false;

	return easy_match(lc->string, rc->string);
}

/**
 * This returns TRUE if the right table of word w contains
 * a connector that can match to c.  shallow tells if c is shallow.
 */
static bool
right_table_search(prune_context *pc, int w, Connector *c,
                   bool shallow, int word_c)
{
	unsigned int size, h;
	C_list *cl;
	power_table *pt;

	pt = pc->pt;
	size = pt->r_table_size[w];
	h = connector_hash(c) & (size-1);
	for (cl = pt->r_table[w][h]; cl != NULL; cl = cl->next)
	{
		if (possible_connection(pc, cl->c, c, cl->shallow, shallow, w, word_c))
		{
			return true;
		}
	}
	return false;
}

/**
 * This returns TRUE if the right table of word w contains
 * a connector that can match to c.  shallows tells if c is shallow
 */
static bool
left_table_search(prune_context *pc, int w, Connector *c,
                  bool shallow, int word_c)
{
	unsigned int size, h;
	C_list *cl;
	power_table *pt;

	pt = pc->pt;
	size = pt->l_table_size[w];
	h = connector_hash(c) & (size-1);
	for (cl = pt->l_table[w][h]; cl != NULL; cl = cl->next)
	{
		if (possible_connection(pc, c, cl->c, shallow, cl->shallow, word_c, w))
			return true;
	}
	return false;
}

/**
 * Take this connector list, and try to match it with the words
 * w-1, w-2, w-3...  Returns the word to which the first connector of
 * the list could possibly be matched.  If c is NULL, returns w.  If
 * there is no way to match this list, it returns a negative number.
 * If it does find a way to match it, it updates the c->word fields
 * correctly.
 */
static int
left_connector_list_update(prune_context *pc, Connector *c,
                           int word_c, int w, bool shallow)
{
	int n;
	bool foundmatch;

	if (c == NULL) return w;
	n = left_connector_list_update(pc, c->next, word_c, w, false) - 1;
	if (((int) c->word) < n) n = c->word;

	/* n is now the rightmost word we need to check */
	foundmatch = false;
	for (; n >= 0 ; n--)
	{
		pc->power_cost++;
		if (right_table_search(pc, n, c, shallow, word_c))
		{
			foundmatch = true;
			break;
		}
	}
	if (n < ((int) c->word))
	{
		c->word = n;
		pc->N_changed++;
	}
	return (foundmatch ? n : -1);
}

/**
 * Take this connector list, and try to match it with the words
 * w+1, w+2, w+3...  Returns the word to which the first connector of
 * the list could possibly be matched.  If c is NULL, returns w.  If
 * there is no way to match this list, it returns a number greater than
 * N_words - 1.   If it does find a way to match it, it updates the
 * c->word fields correctly.
 */
static size_t
right_connector_list_update(prune_context *pc, Sentence sent, Connector *c,
                            size_t word_c, size_t w, bool shallow)
{
	size_t n;
	bool foundmatch;

	if (c == NULL) return w;
	n = right_connector_list_update(pc, sent, c->next, word_c, w, false) + 1;
	if (c->word > n) n = c->word;

	/* n is now the leftmost word we need to check */
	foundmatch = false;
	for (; n < sent->length ; n++)
	{
		pc->power_cost++;
		if (left_table_search(pc, n, c, shallow, word_c))
		{
			foundmatch = true;
			break;
		}
	}
	if (n > c->word) {
		c->word = n;
		pc->N_changed++;
	}
	return (foundmatch ? n : sent->length);
}

/** The return value is the number of disjuncts deleted */
int power_prune(Sentence sent, Parse_Options opts)
{
	power_table *pt;
	prune_context *pc;
	Disjunct *d, *free_later, *dx, *nd;
	Connector *c;
	size_t N_deleted, total_deleted;

	pc = (prune_context *) xalloc (sizeof(prune_context));
	pc->power_cost = 0;
	pc->null_links = (opts->min_null_count > 0);
	pc->N_changed = 1;  /* forces it always to make at least two passes */

	pc->sent = sent;

	pt = power_table_new(sent);
	pc->pt = pt;

	free_later = NULL;
	N_deleted = 0;

	total_deleted = 0;

	while (1)
	{
		size_t w;

		/* left-to-right pass */
		for (w = 0; w < sent->length; w++) {
			if ((2 == w%7) && parse_options_resources_exhausted(opts)) break;
			for (d = sent->word[w].d; d != NULL; d = d->next) {
				if (d->left == NULL) continue;
				if (left_connector_list_update(pc, d->left, w, w, true) < 0) {
					for (c=d->left;  c != NULL; c = c->next) c->word = BAD_WORD;
					for (c=d->right; c != NULL; c = c->next) c->word = BAD_WORD;
					N_deleted++;
					total_deleted++;
				}
			}

			clean_table(pt->r_table_size[w], pt->r_table[w]);
			nd = NULL;
			for (d = sent->word[w].d; d != NULL; d = dx) {
				dx = d->next;
				if ((d->left != NULL) && (d->left->word == BAD_WORD)) {
					d->next = free_later;
					free_later = d;
				} else {
					d->next = nd;
					nd = d;
				}
			}
			sent->word[w].d = nd;
		}
		if (verbosity > 2)
		{
			printf("l->r pass changed %d and deleted %zu\n", pc->N_changed, N_deleted);
		}

		if (pc->N_changed == 0) break;

		pc->N_changed = N_deleted = 0;
		/* right-to-left pass */

		for (w = sent->length-1; w != (size_t) -1; w--) {
			if ((2 == w%7) && parse_options_resources_exhausted(opts)) break;
			for (d = sent->word[w].d; d != NULL; d = d->next) {
				if (d->right == NULL) continue;
				if (right_connector_list_update(pc, sent, d->right, w, w, true) >= sent->length) {
					for (c=d->right; c != NULL; c = c->next) c->word = BAD_WORD;
					for (c=d->left;  c != NULL; c = c->next) c->word = BAD_WORD;
					N_deleted++;
					total_deleted++;
				}
			}
			clean_table(pt->l_table_size[w], pt->l_table[w]);
			nd = NULL;
			for (d = sent->word[w].d; d != NULL; d = dx) {
				dx = d->next;
				if ((d->right != NULL) && (d->right->word == BAD_WORD)) {
					d->next = free_later;
					free_later = d;
				} else {
					d->next = nd;
					nd = d;
				}
			}
			sent->word[w].d = nd;
		}

		if (verbosity > 2)
		{
			printf("r->l pass changed %d and deleted %zu\n",
				pc->N_changed, N_deleted);
		}

		if (pc->N_changed == 0) break;
		pc->N_changed = N_deleted = 0;
	}
	free_disjuncts(free_later);
	power_table_delete(pt);
	pt = NULL;
	pc->pt = NULL;

	if (verbosity > 2) printf("power prune cost: %d\n", pc->power_cost);

	print_time(opts, "power pruned");
	if (verbosity > 2)
	{
		printf("\nAfter power_pruning:\n");
		print_disjunct_counts(sent);
	}

	xfree(pc, sizeof(prune_context));
	return total_deleted;
}

/* ===================================================================
   PP Pruning

   The "contains one" post-processing constraints give us a new way to
   prune.  Suppose there's a rule that says "a group that contains foo
   must contain a bar or a baz."  Here foo, bar, and baz are connector
   types.  foo is the trigger link, bar and baz are called the criterion
   links.  If, after considering the disjuncts we find that there is is
   a foo, but neither a bar, nor a baz, then we can eliminte the disjuct
   containing bar.

   Things are actually a bit more complex, because of the matching rules
   and subscripts.  The problem is that post-processing deals with link
   names, while at this point all we have to work with is connector
   names.  Consider the foo part.  Consider a connector C.  When does
   foo match C for our purposes?  It matches it if every possible link
   name L (that can result from C being at one end of that link) results
   in post_process_match(foo,L) being true.  Suppose foo contains a "*".
   Then there is no C that has this property.  This is because the *s in
   C may be replaced by some other subscripts in the construction of L.
   And the non-* chars in L will not post_process_match the * in foo.

   So let's assume that foo has no *.  Now the result we want is simply
   given by post_process_match(foo, C).  Proof: L is the same as C but
   with some *s replaced by some other letters.  Since foo contains no *
   the replacement in C of some * by some other letter could change
   post_process_match from FALSE to TRUE, but not vice versa.  Therefore
   it's conservative to use this test.

   For the criterion parts, we need to determine if there is a
   collection of connectors C1, C2,... such that by combining them you
   can get a link name L that post_process_matches bar or baz.  Here's a
   way to do this.  Say bar="Xabc".  Then we see if there are connector
   names that post_process_match "Xa##", "X#b#", and "X##c".  They must
   all be there in order for it to be possible to create a link name
   "Xabc".  A "*" in the criterion part is a little different.  In this
   case we can simply skip the * (treat it like an upper case letter)
   for this purpose.  So if bar="X*ab" then we look for "X*#b" and
   "X*a#".  (The * in this case could be treated the same as another
   subscript without breaking it.)  Note also that it's only necessary
   to find a way to match one of the many criterion links that may be in
   the rule.  If none can match, then we can delete the disjunct
   containing C.

   Here's how we're going to implement this.  We'll maintain a multiset
   of connector names.  We'll represent them in a hash table, where the
   hash function uses only the upper case letters of the connector name.
   We'll insert all the connectors into the multiset.  The multiset will
   support the operation of deletion (probably simplest to just
   decrement the count).  Here's the algorithm.

   Insert all the connectors into M.

   While the previous pass caused a count to go to 0 do:
	  For each connector C do
		 For each rule R do
			if C is a trigger for R and the criterion links
			of the rule cannot be satisfied by the connectors in
			M, Then:
			   We delete C's disjunct.  But before we do,
			   we remove all the connectors of this disjunct
			   from the multiset.  Keep tabs on whether or not
			   any of the counts went to 0.



	Efficiency hacks to be added later:
		Note for a given rule can become less and less satisfiable.
		That is, rule_satisfiable(r) for a given rule r can change from
		TRUE to FALSE, but not vice versa.  So once it's FALSE, we can just
		remember that.

		Consider the effect of a pass p on the set of rules that are
		satisfiable.  Suppose this set does not change.  Then pass p+1
		will do nothing.  This is true even if pass p caused some
		disjuncts to be deleted.  (This observation will only obviate
		the need for the last pass.)

  */

static multiset_table * cms_table_new(void)
{
	multiset_table *mt;
	int i;

	mt = (multiset_table *) xalloc(sizeof(multiset_table));

	for (i=0; i<CMS_SIZE; i++) {
		mt->cms_table[i] = NULL;
	}
	return mt;
}

static void cms_table_delete(multiset_table *mt)
{
	Cms * cms, *xcms;
	int i;
	for (i=0; i<CMS_SIZE; i++)
	{
		for (cms = mt->cms_table[i]; cms != NULL; cms = xcms)
		{
			xcms = cms->next;
			xfree(cms, sizeof(Cms));
		}
	}
	xfree(mt, sizeof(multiset_table));
}

static unsigned int cms_hash(const char * s)
{
	unsigned int i = 5381;
	if (islower((int) *s)) s++; /* skip head-dependent indicator */
	while (isupper((int) *s))
	{
		i = ((i << 5) + i) + *s;
		s++;
	}
	return (i & (CMS_SIZE-1));
}

/**
 * This returns TRUE if there is a connector name C in the table
 * such that post_process_match(pp_match_name, C) is TRUE
 */
static bool match_in_cms_table(multiset_table *cmt, const char * pp_match_name)
{
	Cms * cms;
	for (cms = cmt->cms_table[cms_hash(pp_match_name)]; cms != NULL; cms = cms->next)
	{
		if (post_process_match(pp_match_name, cms->name)) return true;
	}
	return false;
}

static Cms * lookup_in_cms_table(multiset_table *cmt, const char * str)
{
	Cms * cms;
	for (cms = cmt->cms_table[cms_hash(str)]; cms != NULL; cms = cms->next)
	{
		if(strcmp(str, cms->name) == 0) return cms;
	}
	return NULL;
}

static void insert_in_cms_table(multiset_table *cmt, const char * str)
{
	Cms * cms;
	unsigned int h;
	cms = lookup_in_cms_table(cmt, str);
	if (cms != NULL) {
		cms->count++;
	} else {
		cms = (Cms *) xalloc(sizeof(Cms));
		cms->name = str;  /* don't copy the string...just keep a pointer to it.
							 we won't free these later */
		cms->count = 1;
		h = cms_hash(str);
		cms->next = cmt->cms_table[h];
		cmt->cms_table[h] = cms;
	}
}

/**
 * Delete the given string from the table.  Return TRUE if
 * this caused a count to go to 0, return FALSE otherwise.
 */
static bool delete_from_cms_table(multiset_table *cmt, const char * str)
{
	Cms * cms = lookup_in_cms_table(cmt, str);
	if (cms != NULL && cms->count > 0)
	{
		cms->count--;
		return (cms->count == 0);
	}
	return false;
}

static bool rule_satisfiable(multiset_table *cmt, pp_linkset *ls)
{
	unsigned int hashval;
	const char * t;
	char name[20], *s;
	pp_linkset_node *p;
	int bad, n_subscripts;

	for (hashval = 0; hashval < ls->hash_table_size; hashval++)
	{
		for (p = ls->hash_table[hashval]; p!=NULL; p=p->next)
		{
			/* ok, we've got our hands on one of the criterion links */
			strncpy(name, p->str, sizeof(name)-1);
			/* could actually use the string in place because we change it back */
			name[sizeof(name)-1] = '\0';
			/* now we want to see if we can satisfy this criterion link */
			/* with a collection of the links in the cms table */

			s = name;
			if (islower((int)*s)) s++; /* skip head-dependent indicator */
			for (; isupper((int)*s); s++) {}
			for (;*s != '\0'; s++) if (*s != '*') *s = '#';

			s = name;
			t = p->str;
			if (islower((int)*s)) s++; /* skip head-dependent indicator */
			if (islower((int)*t)) t++; /* skip head-dependent indicator */
			for (; isupper((int) *s); s++, t++) {}

			/* s and t remain in lockstep */
			bad = 0;
			n_subscripts = 0;
			for (;*s != '\0' && bad==0; s++, t++) {
				if (*s == '*') continue;
				n_subscripts++;
				/* after the upper case part, and is not a * so must be a regular subscript */
				*s = *t;
				if (!match_in_cms_table(cmt, name)) bad++;
				*s = '#';
			}

			if (n_subscripts == 0) {
				/* now we handle the special case which occurs if there
				   were 0 subscripts */
				if (!match_in_cms_table(cmt, name)) bad++;
			}

			/* now if bad==0 this criterion link does the job
			   to satisfy the needs of the trigger link */

			if (bad == 0) return true;
		}
	}
	return false;
}

static int pp_prune(Sentence sent, Parse_Options opts)
{
	pp_knowledge * knowledge;
	size_t i, w;
	int total_deleted, N_deleted;
	bool change, deleteme;
	multiset_table *cmt;

	if (sent->postprocessor == NULL) return 0;

	knowledge = sent->postprocessor->knowledge;

	cmt = cms_table_new();

	for (w = 0; w < sent->length; w++)
	{
		Disjunct *d;
		for (d = sent->word[w].d; d != NULL; d = d->next)
		{
			char dir;
			d->marked = true;
			for (dir=0; dir < 2; dir++)
			{
				Connector *c;
				for (c = ((dir) ? (d->left) : (d->right)); c != NULL; c = c->next)
				{
					insert_in_cms_table(cmt, c->string);
				}
			}
		}
	}

	total_deleted = 0;
	change = true;
	while (change)
	{
		change = false;
		N_deleted = 0;
		for (w = 0; w < sent->length; w++)
		{
			Disjunct *d;
			for (d = sent->word[w].d; d != NULL; d = d->next)
			{
				char dir;
				if (!d->marked) continue;
				deleteme = false;
				for (dir = 0; dir < 2; dir++)
				{
					Connector *c;
					for (c = ((dir) ? (d->left) : (d->right)); c != NULL; c = c->next)
					{
						for (i = 0; i < knowledge->n_contains_one_rules; i++)
						{
							pp_rule* rule = &knowledge->contains_one_rules[i]; /* the ith rule */
							const char * selector = rule->selector;  /* selector string for this rule */
							pp_linkset * link_set = rule->link_set;  /* the set of criterion links */

							if (strchr(selector, '*') != NULL) continue;  /* If it has a * forget it */

							if (!post_process_match(selector, c->string)) continue;

							/*
							printf("pp_prune: trigger ok.  selector = %s  c->string = %s\n", selector, c->string);
							*/

							/* We know c matches the trigger link of the rule. */
							/* Now check the criterion links */

							if (!rule_satisfiable(cmt, link_set))
							{
								deleteme = true;
								rule->use_count++;
							}
							if (deleteme) break;
						}
						if (deleteme) break;
					}
					if (deleteme) break;
				}

				if (deleteme)         /* now we delete this disjunct */
				{
					char dir;
					N_deleted++;
					total_deleted++;
					d->marked = false; /* mark for deletion later */
					for (dir=0; dir < 2; dir++)
					{
						Connector *c;
						for (c = ((dir) ? (d->left) : (d->right)); c != NULL; c = c->next)
						{
							change |= delete_from_cms_table(cmt, c->string);
						}
					}
				}
			}
		}

		if (verbosity > 2)
		{
			printf("pp_prune pass deleted %d\n", N_deleted);
		}
	}
	delete_unmarked_disjuncts(sent);
	cms_table_delete(cmt);

	if (verbosity > 2)
	{
		printf("\nAfter pp_pruning:\n");
		print_disjunct_counts(sent);
	}

	print_time(opts, "pp pruning");

	return total_deleted;
}


/**
 * Do the following pruning steps until nothing happens:
 * power pp power pp power pp....
 * Make sure you do them both at least once.
 */
void pp_and_power_prune(Sentence sent, Parse_Options opts)
{
	power_prune(sent, opts);

	for (;;) {
		if (parse_options_resources_exhausted(opts)) break;
		if (pp_prune(sent, opts) == 0) break;
		if (parse_options_resources_exhausted(opts)) break;
		if (power_prune(sent, opts) == 0) break;
	}
}
