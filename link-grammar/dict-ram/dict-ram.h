/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

Dict_node * strict_lookup_list(const Dictionary dict, const char *s);
Dict_node * dsw_tree_to_vine (Dict_node *root);
Dict_node * dsw_vine_to_tree (Dict_node *root, int size);

Dict_node * dict_node_insert(Dictionary dict, Dict_node *n, Dict_node *newnode);
Dict_node * dict_node_lookup(const Dictionary dict, const char *s);
Dict_node * dict_node_wild_lookup(Dictionary dict, const char *s);
bool dict_node_exists_lookup(Dictionary dict, const char *s);

void free_dictionary_root(Dictionary dict);

Exp * Exp_create(Pool_desc *);
Exp * Exp_create_dup(Pool_desc *, Exp *);
Exp * make_zeroary_node(Pool_desc *mp);
Exp * make_unary_node(Pool_desc *, Exp *);
Exp * make_join_node(Pool_desc *mp, Exp* nl, Exp* nr, Exp_type t);
Exp * make_and_node(Pool_desc *mp, Exp* nl, Exp* nr);
Exp * make_or_node(Pool_desc *mp, Exp* nl, Exp* nr);
Exp * make_optional_node(Pool_desc *mp, Exp *e);
Exp * make_connector_node(Dictionary dict, Pool_desc *mp,
                          const char* linktype, char dir, bool multi);

void add_define(Dictionary dict, const char *name, const char *value);

void add_category(Dictionary dict, Exp *e, Dict_node *dn, int n);

void print_dictionary_data(Dictionary dict);
void print_dictionary_defines(Dictionary dict);
