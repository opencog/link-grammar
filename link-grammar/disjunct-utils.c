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

#include <stdio.h>
#include <string.h>
#include "disjunct-utils.h"
#include "externs.h"
#include "structures.h"
#include "utilities.h"
#include "word-utils.h"

/* Disjunct utilities ... */

/**
 * free_disjuncts() -- free the list of disjuncts pointed to by c
 * (does not free any strings)
 */
void free_disjuncts(Disjunct *c)
{
	Disjunct *c1;
	for (;c != NULL; c = c1) {
		c1 = c->next;
		free_connectors(c->left);
		free_connectors(c->right);
		xfree((char *)c, sizeof(Disjunct));
	}
}

/**
 * This builds a new copy of the disjunct pointed to by d (except for the
 * next field which is set to NULL).  Strings, as usual,
 * are not copied.
 */
Disjunct * copy_disjunct(Disjunct * d)
{
	Disjunct * d1;
	if (d == NULL) return NULL;
	d1 = (Disjunct *) xalloc(sizeof(Disjunct));
	*d1 = *d;
	d1->next = NULL;
	d1->left = copy_connectors(d->left);
	d1->right = copy_connectors(d->right);
	return d1;
}

/**
 * Destructively catenates the two disjunct lists d1 followed by d2.
 * Doesn't change the contents of the disjuncts.
 * Traverses the first list, but not the second.
 */
Disjunct * catenate_disjuncts(Disjunct *d1, Disjunct *d2)
{
	Disjunct * dis = d1;

	if (d1 == NULL) return d2;
	if (d2 == NULL) return d1;
	while (dis->next != NULL) dis = dis->next;
	dis->next = d2;
	return d1;
}

/** Returns the number of disjuncts in the list pointed to by d */
int count_disjuncts(Disjunct * d)
{
	int count = 0;
	for (; d != NULL; d = d->next)
	{
		count++;
	}
	return count;
}

/* ============================================================= */

typedef struct disjunct_dup_table_s disjunct_dup_table;
struct disjunct_dup_table_s
{
	int dup_table_size;
	Disjunct ** dup_table;
};

/**
 * This is a hash function for disjuncts
 *
 * This is the old version that doesn't check for domination, just
 * equality.
 */
static inline int old_hash_disjunct(disjunct_dup_table *dt, Disjunct * d)
{
	int i;
	Connector *e;
	i = 0;
	for (e = d->left ; e != NULL; e = e->next) {
		i += string_hash(e->string);
	}
	for (e = d->right ; e != NULL; e = e->next) {
		i += string_hash(e->string);
	}
	i += string_hash(d->string);
	i += (i>>10);
	return (i & (dt->dup_table_size-1));
}

/**
 * The connectors must be exactly equal.  A similar function
 * is connectors_equal_AND(), but that ignores priorities,
 * this does not.
 */
static int connectors_equal_prune(Connector *c1, Connector *c2)
{
	return (c1->label == c2->label) &&
		   (c1->multi == c2->multi) &&
		   (c1->priority == c2->priority) &&
		   (strcmp(c1->string, c2->string) == 0);
}

/** returns TRUE if the disjuncts are exactly the same */
static int disjuncts_equal(Disjunct * d1, Disjunct * d2)
{
	Connector *e1, *e2;
	e1 = d1->left;
	e2 = d2->left;
	while((e1!=NULL) && (e2!=NULL)) {
		if (!connectors_equal_prune(e1,e2)) break;
		e1 = e1->next;
		e2 = e2->next;
	}
	if ((e1!=NULL) || (e2!=NULL)) return FALSE;
	e1 = d1->right;
	e2 = d2->right;
	while((e1!=NULL) && (e2!=NULL)) {
		if (!connectors_equal_prune(e1,e2)) break;
		e1 = e1->next;
		e2 = e2->next;
	}
	if ((e1!=NULL) || (e2!=NULL)) return FALSE;
	return (strcmp(d1->string, d2->string) == 0);
}

static disjunct_dup_table * disjunct_dup_table_new(size_t sz)
{
	size_t i;
	disjunct_dup_table *dt;
	dt = (disjunct_dup_table *) malloc(sizeof(disjunct_dup_table));

	dt->dup_table_size = sz;
	dt->dup_table = (Disjunct **) xalloc(sz * sizeof(Disjunct *));

	for (i=0; i<sz; i++) dt->dup_table[i] = NULL;

	return dt;
}

static void disjunct_dup_table_delete(disjunct_dup_table *dt)
{
	xfree((char *) dt->dup_table, dt->dup_table_size * sizeof(Disjunct *));
	free(dt);
}

/**
 * Takes the list of disjuncts pointed to by d, eliminates all
 * duplicates, and returns a pointer to a new list.
 * It frees the disjuncts that are eliminated.
 */
Disjunct * eliminate_duplicate_disjuncts(Disjunct * d)
{
	int i, h, count;
	Disjunct *dn, *dx;
	disjunct_dup_table *dt;

	count = 0;
	dt = disjunct_dup_table_new(next_power_of_two_up(2 * count_disjuncts(d)));

	while (d != NULL)
	{
		dn = d->next;
		h = old_hash_disjunct(dt, d);

		for (dx = dt->dup_table[h]; dx!=NULL; dx=dx->next)
		{
			if (disjuncts_equal(dx, d)) break;
		}
		if (dx == NULL)
		{
			d->next = dt->dup_table[h];
			dt->dup_table[h] = d;
		}
		else
		{
			d->next = NULL;  /* to prevent it from freeing the whole list */
			if (d->cost < dx->cost) dx->cost = d->cost;
			free_disjuncts(d);
			count++;
		}
		d = dn;
	}

	/* d is already null */
	for (i=0; i<dt->dup_table_size; i++)
	{
		for (dn = dt->dup_table[i]; dn != NULL; dn = dx) {
			dx = dn->next;
			dn->next = d;
			d = dn;
		}
	}

	if ((verbosity > 2) && (count != 0)) printf("killed %d duplicates\n", count);

	disjunct_dup_table_delete(dt);
	return d;
}

/* ============================================================= */

/* Return the stringified disjunct.
 * Be sure to free the string upon return.
 */

static char * prt_con(Connector *c, char * p, char dir)
{
	size_t n;

	if (NULL == c) return p;
	p = prt_con (c->next, p, dir);

	if (c->multi)
	{
		n = sprintf(p, "@%s%c ", c->string, dir);
	}
	else
	{
		n = sprintf(p, "%s%c ", c->string, dir);
	}
	return p+n;
}

char * print_one_disjunct(Disjunct *dj)
{
	char buff[MAX_LINE];
	char * p = buff;

	p = prt_con(dj->left, p, '-');
	p = prt_con(dj->right, p, '+');
	
	return strdup(buff);
}

/* ========================= END OF FILE ============================== */
