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

#include <link-grammar/api.h>

int connector_hash(Connector * c, int i);

void free_disjuncts(Disjunct *);
void free_X_nodes(X_node *);
void free_connectors(Connector *);
void free_Exp(Exp *);
void free_E_list(E_list *);
int  size_of_expression(Exp *);

/* routines for copying basic objects */
Disjunct *  copy_disjunct(Disjunct * );
void        exfree_connectors(Connector *);
Link        excopy_link(Link);
void        exfree_link(Link);
Connector * copy_connectors(Connector *);
Connector * excopy_connectors(Connector * c);
Disjunct *  catenate_disjuncts(Disjunct *, Disjunct *);
X_node *    catenate_X_nodes(X_node *, X_node *);
Exp *       copy_Exp(Exp *);

Connector * init_connector(Connector *c);

void init_x_table(Sentence sent);

/* utility routines moved from parse.c */
int sentence_contains(Sentence sent, const char * s);
void set_is_conjunction(Sentence sent);
int sentence_contains_conjunction(Sentence sent);
int conj_in_range(Sentence sent, int lw, int rw);
int max_link_length(Connector * c);
void string_delete(char * p);

/* Connector_set routines */
Connector_set * connector_set_create(Exp *e);
void connector_set_delete(Connector_set * conset);
int match_in_connector_set(Connector_set *conset, Connector * c, int d);

Dict_node * list_whole_dictionary(Dict_node *, Dict_node *);
int word_has_connector(Dict_node *, const char *, int);
int word_contains(const char * word, const char * macro, Dictionary dict);
int is_past_tense_form(const char * str, Dictionary dict);
int is_entity(const char * str, Dictionary dict);

#endif /* _LINK_GRAMMAR_WORD_UTILS_H_ */
