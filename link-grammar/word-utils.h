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
void exfree_connectors(Connector *);
Connector * copy_connectors(Connector *);
Connector * excopy_connectors(Connector * c);

/* Link utilities ... */
Link *      excopy_link(Link *);
void        exfree_link(Link *);


/* Connector-set utilities ... */
Connector_set * connector_set_create(Exp *e);
void connector_set_delete(Connector_set * conset);
Boolean match_in_connector_set(count_context_t*, Connector_set*, Connector*, int);
Boolean word_has_connector(Dict_node *, const char *, char);
const char * word_only_connector(Dict_node *);


/* Dictionary utilities ... */
Boolean word_contains(Dictionary dict, const char * word, const char * macro);

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

/**
 * This hash function only looks at the leading upper case letters of
 * the connector string, and the label fields.  This ensures that if two
 * strings match (formally), then they must hash to the same place.
 */
static inline int connector_hash(Connector * c)
{
	const char *s;
	unsigned int i;

	if (-1 != c->hash) return c->hash;

	/* For most situations, all three hashes are very nearly equal;
	 * as to which is faster depends on the parsed text.
	 * For both English and Russian, there are about 100 pre-defined
	 * connectors, and another 2K-4K autogen'ed ones (the IDxxx idiom
	 * connectors, and the LLxxx suffix connectors for Russian).
	 * Turns out the cost of settting up the hash table dominates the
	 * cost of collistions. */
#ifdef USE_DJB2
	/* djb2 hash */
	i = 5381;
#ifdef USE_FAT_LINKAGES
	i = ((i << 5) + i) + (0xff & c->label);
#endif /* USE_FAT_LINKAGES */
	s = c->string;
	while (isupper((int) *s)) /* connector tables cannot contain UTF8, yet */
	{
		i = ((i << 5) + i) + *s;
		s++;
	}
	i += i>>14;
#endif /* USE_DJB2 */

#define USE_JENKINS
#ifdef USE_JENKINS
	/* Jenkins one-at-a-time hash */
	i = 0;
	s = c->string;
	while (isupper((int) *s)) /* connector tables cannot contain UTF8, yet */
	{
		i += *s;
		i += (i<<10);
		i ^= (i>>6);
		s++;
	}
	i += (i << 3);
	i ^= (i >> 11);
	i += (i << 15);
#endif /* USE_JENKINS */

#ifdef USE_SDBM
	/* sdbm hash */
#ifdef USE_FAT_LINKAGES
	i = (0xff & c->label);
#else
	i = 0;
#endif /* USE_FAT_LINKAGES */
	s = c->string;
	while (isupper((int) *s))
	{
		i = *s + (i << 6) + (i << 16) - i;
		s++;
	}
#endif /* USE_SDBM */

	c->hash = i;
	return i;
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
