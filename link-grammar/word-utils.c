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
/*
 * Miscellaneous utilities for dealing with word types.
 */

#include <math.h>
#include <stdio.h>

#include "api.h"
#include "disjunct-utils.h"
#include "word-utils.h"

/* ======================================================== */
/* Exp utilities ... */

void free_E_list(E_list *);
void free_Exp(Exp * e)
{
	if (e->type != CONNECTOR_type) {
		free_E_list(e->u.l);
	}
	xfree((char *)e, sizeof(Exp));
}

void free_E_list(E_list * l)
{
	if (l == NULL) return;
	free_E_list(l->next);
	free_Exp(l->e);
	xfree((char *)l, sizeof(E_list));
}

/* Returns the number of connectors in the expression e */
int size_of_expression(Exp * e)
{
	int size;
	E_list * l;
	if (e->type == CONNECTOR_type) return 1;
	size = 0;
	for (l=e->u.l; l!=NULL; l=l->next) {
		size += size_of_expression(l->e);
	}
	return size;
}

/** 
 * Build a copy of the given expression (don't copy strings, of course)
 */
static E_list * copy_E_list(E_list * l);
Exp * copy_Exp(Exp * e)
{
	Exp * n;
	if (e == NULL) return NULL;
	n = (Exp *) xalloc(sizeof(Exp));
	*n = *e;
	if (e->type != CONNECTOR_type) {
		n->u.l = copy_E_list(e->u.l);
	}
	return n;
}

static E_list * copy_E_list(E_list * l)
{
	E_list * nl;
	if (l == NULL) return NULL;
	nl = (E_list *) xalloc(sizeof(E_list));
	nl->next = copy_E_list(l->next);
	nl->e = copy_Exp(l->e);
	return nl;
}

/**
 * Compare two expressions, return 1 for equal, 0 for unequal
 */
static int exp_compare(Exp * e1, Exp * e2)
{
	E_list *el1, *el2;

	if ((e1 == NULL) && (e2 == NULL))
	  return 1; /* they are equal */
	if ((e1 == NULL) || (e2 == NULL))
	  return 0; /* they are not equal */
	if (e1->type != e2->type)
		return 0;
	if (fabs (e1->cost - e2->cost) > 0.001)
		return 0;
	if (e1->type == CONNECTOR_type)
	{
		if (e1->dir != e2->dir)
			return 0;
		/* printf("%s %s\n",e1->u.string,e2->u.string); */
		if (strcmp(e1->u.string,e2->u.string)!=0)
			return 0;
	}
	else
	{
		el1 = e1->u.l;
		el2 = e2->u.l;
		/* while at least 1 is non-null */
		for (;(el1!=NULL)||(el2!=NULL);) {
		  /*fail if 1 is null */
			if ((el1==NULL)||(el2==NULL))
				return 0;
			/* fail if they are not compared */
			if (exp_compare(el1->e, el2->e) == 0)
				return 0;
			if (el1!=NULL)
				el1 = el1->next;
			if (el2!=NULL)
				el2 = el2->next;
		}
	}
	return 1; /* if never returned 0, return 1 */
}

/**
 * Sub-expression matcher -- return 1 if sub is non-NULL and 
 * contained in super, 0 otherwise.
 */
static int exp_contains(Exp * super, Exp * sub)
{
	E_list * el;

#ifdef DEBUG
	printf("SUP: ");
	if (super) print_expression(super); 
	printf("\n");
#endif

	if (sub==NULL || super==NULL)
		return 0;
	if (exp_compare(sub,super)==1)
		return 1;
	if (super->type==CONNECTOR_type)
	  return 0; /* super is a leaf */

	/* proceed through supers children and return 1 if sub
	   is contained in any of them */
	for(el = super->u.l; el!=NULL; el=el->next) {
		if (exp_contains(el->e, sub)==1)
			return 1;
	}
	return 0;
}

/* ======================================================== */
/* X_node utilities ... */
/** 
 * frees the list of X_nodes pointed to by x, and all of the expressions
 */
void free_X_nodes(X_node * x)
{
	X_node * y;
	for (; x!= NULL; x = y) {
		y = x->next;
		free_Exp(x->exp);
		xfree((char *)x, sizeof(X_node));
	}
}

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

/* ======================================================== */
/* Connector utilities ... */

/**
 * free_connectors() -- free the list of connectors pointed to by e
 * (does not free any strings)
 */
void free_connectors(Connector *e)
{
	Connector * n;
	for (; e != NULL; e = n)
	{
		n = e->next;
		xfree((char *)e, sizeof(Connector));
	}
}

void exfree_connectors(Connector *e)
{
	Connector * n;
	for(;e != NULL; e = n) {
		n = e->next;
		exfree((void *) e->string, sizeof(char)*(strlen(e->string)+1));
		exfree(e, sizeof(Connector));
	}
}

Connector * connector_new(void)
{
	Connector *c = (Connector *) xalloc(sizeof(Connector));
	c->length_limit = UNLIMITED_LEN;
	c->string = "";
	c->label = NORMAL_LABEL;
	c->hash = -1;
	c->priority = THIN_priority;
	c->multi = FALSE;
	c->next = NULL;
	c->tableNext = NULL;
	return c;
}

Connector * init_connector(Connector *c)
{
	c->hash = -1;
	c->length_limit = UNLIMITED_LEN;
	return c;
}

/**
 * This builds a new copy of the connector list pointed to by c.
 * Strings, as usual, are not copied.
 */
Connector * copy_connectors(Connector * c)
{
	Connector *c1;
	if (c == NULL) return NULL;
	c1 = connector_new();
	*c1 = *c;
	c1->next = copy_connectors(c->next);
	return c1;
}

Connector * excopy_connectors(Connector * c)
{
	char * s;
	Connector *c1;

	if (c == NULL) return NULL;

	c1 = connector_new();
	*c1 = *c;
	s = (char *) exalloc(sizeof(char)*(strlen(c->string)+1));
	strcpy(s, c->string);
	c1->string = s;
	c1->next = excopy_connectors(c->next);

	return c1;
}

/* ======================================================== */
/* Link utilities ... */

Link * excopy_link(Link * l)
{
	char * s;
	Link * newl;

	if (l == NULL) return NULL;

	newl = (Link *) exalloc(sizeof(Link));
	s = (char *) exalloc(sizeof(char)*(strlen(l->name)+1));
	strcpy(s, l->name);
	newl->name = s;
	newl->l = l->l;
	newl->r = l->r;
	newl->lc = excopy_connectors(l->lc);
	newl->rc = excopy_connectors(l->rc);

	return newl;
}

void exfree_link(Link * l)
{
	exfree_connectors(l->rc);
	exfree_connectors(l->lc);
	exfree((void *)l->name, sizeof(char)*(strlen(l->name)+1));
	exfree(l, sizeof(Link));
}

/* ======================================================== */
/* Connector-set utilities ... */
/**
 * This hash function only looks at the leading upper case letters of
 * the string, and the direction, '+' or '-'.
 */
static int connector_set_hash(Connector_set *conset, const char * s, int d)
{
	unsigned int i;
	/* djb2 hash */
	i = 5381;
	i = ((i << 5) + i) + d;
	while (isupper((int) *s)) /* connector tables cannot contain UTF8, yet */
	{
		i = ((i << 5) + i) + *s;
		s++;
	}
	return (i & (conset->table_size-1));
}

static void build_connector_set_from_expression(Connector_set * conset, Exp * e)
{
	E_list * l;
	Connector * c;
	int h;
	if (e->type == CONNECTOR_type)
	{
		c = connector_new();
		c->string = e->u.string;
		c->word = e->dir;       /* just use the word field to give the dir */
		h = connector_set_hash(conset, c->string, c->word);
		c->next = conset->hash_table[h];
		conset->hash_table[h] = c;
	} else {
		for (l=e->u.l; l!=NULL; l=l->next) {
			build_connector_set_from_expression(conset, l->e);
		}
	}
}

Connector_set * connector_set_create(Exp *e)
{
	int i;
	Connector_set *conset;

	conset = (Connector_set *) xalloc(sizeof(Connector_set));
	conset->table_size = next_power_of_two_up(size_of_expression(e));
	conset->hash_table =
	  (Connector **) xalloc(conset->table_size * sizeof(Connector *));
	for (i=0; i<conset->table_size; i++) conset->hash_table[i] = NULL;
	build_connector_set_from_expression(conset, e);
	return conset;
}

void connector_set_delete(Connector_set * conset)
{
	int i;
	if (conset == NULL) return;
	for (i=0; i<conset->table_size; i++) free_connectors(conset->hash_table[i]);
	xfree(conset->hash_table, conset->table_size * sizeof(Connector *));
	xfree(conset, sizeof(Connector_set));
}

/**
 * Returns TRUE the given connector is in this conset.  FALSE otherwise.
 * d='+' means this connector is on the right side of the disjunct.
 * d='-' means this connector is on the left side of the disjunct.
 */
int match_in_connector_set(Sentence sent, Connector_set *conset, Connector * c, int d)
{
	int h;
	Connector * c1;
	if (conset == NULL) return FALSE;
	h = connector_set_hash(conset, c->string, d);
	for (c1 = conset->hash_table[h]; c1 != NULL; c1 = c1->next)
	{
		if (x_match(sent, c1, c) && (d == c1->word)) return TRUE;
	}
	return FALSE;
}

/* ======================================================== */
/* More connector utilities ... */

/**
 * This is like the basic "match" function in count.c - the basic
 * connector-matching function used in parsing - except it ignores
 * "priority" (used to handle fat links)
 */
static int easy_match(const char * s, const char * t)
{
	while(isupper((int)*s) || isupper((int)*t)) {
		if (*s != *t) return FALSE;
		s++;
		t++;
	}

	while ((*s!='\0') && (*t!='\0')) {
		if ((*s == '*') || (*t == '*') ||
		   ((*s == *t) && (*s != '^'))) {
		s++;
		t++;
		} else return FALSE;
	}
	return TRUE;
}

/**
 * word_has_connector() -- return TRUE if dictionary expression has connector
 * This function takes a dict_node (corresponding to an entry in a
 * given dictionary), a string (representing a connector), and a
 * direction (0 = right-pointing, 1 = left-pointing); it returns 1
 * if the dictionary expression for the word includes the connector,
 * 0 otherwise.  This can be used to see if a word is in a certain
 * category (checking for a category connector in a table), or to see
 * if a word has a connector in a normal dictionary. The connector
 * check uses a "smart-match", the same kind used by the parser.
 */
int word_has_connector(Dict_node * dn, const char * cs, int direction)
{
	Connector * c2=NULL;
	Disjunct * d, *d0;
	if(dn == NULL) return -1;
	d0 = d = build_disjuncts_for_dict_node(dn);
	if(d == NULL) return 0;
	for(; d!=NULL; d=d->next) {
		if(direction==0) c2 = d->right;
		if(direction==1) c2 = d->left;
		for(; c2!=NULL; c2=c2->next) {
			if(easy_match(c2->string, cs)==1) {
				free_disjuncts(d0);
				return 1;
			}
		}
	}
	free_disjuncts(d0);
	return 0;
}

/* ======================================================== */
/* Dictionary utilities ... */

static int dn_word_contains(Dictionary dict,
                            Dict_node * w_dn, const char * macro)
{
	Exp * m_exp;
	Dict_node *m_dn;

	if (w_dn == NULL) return 0;

	m_dn = dictionary_lookup_list(dict, macro);
	if (m_dn == NULL) return 0;

	m_exp = m_dn->exp;
	free_lookup_list(m_dn);

#ifdef DEBUG
	printf("\nWORD: ");
	print_expression(w_dn->exp);
	printf("\nMACR: ");
	print_expression(m_exp); 
	printf("\n");
#endif

	for (;w_dn != NULL; w_dn = w_dn->right)
	{
		if (1 == exp_contains(w_dn->exp, m_exp))
			return 1;
	}
	return 0;
}

/**
 * word_contains: return true if the word may involve application of
 * a rule.
 *
 * @return: true if word's expression contains macro's expression,
 * false otherwise.
 */
int word_contains(Dictionary dict, const char * word, const char * macro)
{
	Dict_node *w_dn;
	int ret;
	w_dn = abridged_lookup_list(dict, word);
	ret = dn_word_contains(dict, w_dn, macro);
	free_lookup_list(w_dn);
	return ret;
}

Dict_node * list_whole_dictionary(Dict_node *root, Dict_node *dn)
{
	Dict_node *c, *d;
	if (root == NULL) return dn;
	c = (Dict_node *) xalloc(sizeof(Dict_node));
	*c = *root;
	d = list_whole_dictionary(root->left, dn);
	c->right = list_whole_dictionary(root->right, d);
	return c;
}

#define PAST_TENSE_FORM_MARKER "<marker-past>"
#define ENTITY_MARKER          "<marker-entity>"
#define COMMON_ENTITY_MARKER   "<marker-common-entity>"

/* This is exported to public API (for Java) 
 * @deprecated -- past-tense verbs are tagged with .v-d or .w-d or .q-d
 * subscripts. use those instead to figure out if a verb is past tense.
 */
int dictionary_is_past_tense_form(Dictionary dict, const char * str)
{
	if (word_contains(dict, str, PAST_TENSE_FORM_MARKER) == 1)
		return 1;
	return 0;
}

/**
 * dictionary_is_entity - Return true if word is entity.
 * Entities are proper names (geographical names,
 * names of people), street addresses, phone numbers,
 * etc.
 */
/* This is exported to public API (for Java) */
int dictionary_is_entity(Dictionary dict, const char * str)
{
	if (word_contains(dict, str, ENTITY_MARKER) == 1)
		return 1;
	return 0;
}

/* ========================= END OF FILE ============================== */
