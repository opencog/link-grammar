/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2009, 2013 Linas Vepstas                                */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _LINK_GRAMMAR_WORD_UTILS_H_
#define _LINK_GRAMMAR_WORD_UTILS_H_

#include "api-types.h"
#include "dict-structures.h"
#include "structures.h"

/* Exp utilities ... */
void free_Exp(Exp *);
void free_E_list(E_list *);
int  size_of_expression(Exp *);
Exp * copy_Exp(Exp *);
/* int exp_compare(Exp * e1, Exp * e2); */
/* int exp_contains(Exp * super, Exp * sub); */


/* X_node utilities ... */
X_node *    catenate_X_nodes(X_node *, X_node *);
void free_X_nodes(X_node *);


/* Connector utilities ... */
Connector * connector_new(void);
Connector * init_connector(Connector *c);
void free_connectors(Connector *);

/* Connector-set utilities ... */
Connector_set * connector_set_create(Exp *e);
void connector_set_delete(Connector_set * conset);
bool word_has_connector(Dict_node *, const char *, char);
const char * word_only_connector(Dict_node *);
bool match_in_connector_set(Connector_set*, Connector*, int dir);


/**
 * Returns TRUE if s and t match according to the connector matching
 * rules.  The connector strings must be properly formed, starting with
 * zero or one lower case letters, followed by one or more upper case
 * letters, followed by some other letters.
 *
 * The algorithm is symmetric with respect to a and b.
 *
 * Connectors starting with lower-case letters match ONLY if the initial
 * letters are DIFFERENT.  Otherwise, connectors only match if the
 * upper-case letters are the same, and the trailing lower case letters
 * are the same (or have wildcards).
 *
 * The initial lower-case letters allow an initial 'h' (denoting 'head
 * word') to match an initial 'd' (denoting 'dependent word'), while
 * rejecting a match 'h' to 'h' or 'd' to 'd'.  This allows the parser
 * to work with catena, instead of just links.
 */
static inline bool easy_match(const char * s, const char * t)
{
	char is = 0, it = 0;
	if (islower((int) *s)) { is = *s; s++; }
	if (islower((int) *t)) { it = *t; t++; }

	if (is != 0 && it != 0 && is == it) return false;

	while (isupper((int)*s) || isupper((int)*t))
	{
		if (*s != *t) return false;
		s++;
		t++;
	}

	while ((*s!='\0') && (*t!='\0'))
	{
		if ((*s == '*') || (*t == '*') || (*s == *t))
		{
			s++;
			t++;
		}
		else
			return false;
	}
	return true;
}


/* Dictionary utilities ... */
bool word_contains(Dictionary dict, const char * word, const char * macro);

static inline int string_hash(const char *s)
{
	unsigned int i;

	/* djb2 hash */
	i = 5381;
	while (*s)
	{
		i = ((i << 5) + i) + *s;
		s++;
	}
	return i;
}

int calculate_connector_hash(Connector *);

static inline int connector_hash(Connector * c)
{
	if (-1 != c->hash) return c->hash;
	return calculate_connector_hash(c);
}

/**
 * hash function. Based on some tests, this seems to be an almost
 * "perfect" hash, in that almost all hash buckets have the same size!
 */
static inline unsigned int pair_hash(unsigned int log2_table_size,
                            int lw, int rw,
                            const Connector *le, const Connector *re,
                            unsigned int cost)
{
	unsigned int table_size = (1 << log2_table_size);
	unsigned int i = 0;

#if 0
 	/* hash function. Based on some tests, this seems to be
	 * an almost "perfect" hash, in that almost all hash buckets
	 * have the same size! */
	i += 1 << cost;
	i += 1 << (lw % (log2_table_size-1));
	i += 1 << (rw % (log2_table_size-1));
	i += ((unsigned int) le) >> 2;
	i += ((unsigned int) le) >> log2_table_size;
	i += ((unsigned int) re) >> 2;
	i += ((unsigned int) re) >> log2_table_size;
	i += i >> log2_table_size;
#else
	/* sdbm-based hash */
	i = cost;
	i = lw + (i << 6) + (i << 16) - i;
	i = rw + (i << 6) + (i << 16) - i;
	i = ((unsigned long) le) + (i << 6) + (i << 16) - i;
	i = ((unsigned long) re) + (i << 6) + (i << 16) - i;
#endif

	return i & (table_size-1);
}
#endif /* _LINK_GRAMMAR_WORD_UTILS_H_ */
