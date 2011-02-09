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

#ifndef _LINK_GRAMMAR_WORD_UTILS_H_
#define _LINK_GRAMMAR_WORD_UTILS_H_

#include <link-grammar/api-types.h>

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
int connector_hash(Connector * c);

/* Link utilities ... */
Link *      excopy_link(Link *);
void        exfree_link(Link *);


/* Connector-set utilities ... */
Connector_set * connector_set_create(Exp *e);
void connector_set_delete(Connector_set * conset);
int match_in_connector_set(Sentence, Connector_set *conset, Connector * c, int d);
int word_has_connector(Dict_node *, const char *, int);


/* Dictionary utilities ... */
int word_contains(Dictionary dict, const char * word, const char * macro);
Dict_node * list_whole_dictionary(Dict_node *, Dict_node *);

/**
 * hash function. Based on some tests, this seems to be an almost
 * "perfect" hash, in that almost all hash buckets have the same size!
 */
static inline int pair_hash(int log2_table_size,
                int lw, int rw, Connector *le, Connector *re, int cost)
{
	int table_size = (1 << log2_table_size);
	unsigned int i = 0;

	i += 1 << cost;
	i += 1 << (lw % (log2_table_size-1));
	i += 1 << (rw % (log2_table_size-1));
	i += ((unsigned int) le) >> 2;
	i += ((unsigned int) le) >> log2_table_size;
	i += ((unsigned int) re) >> 2;
	i += ((unsigned int) re) >> log2_table_size;
	i += i >> log2_table_size;

   return i & (table_size-1);
}
#endif /* _LINK_GRAMMAR_WORD_UTILS_H_ */
