/********************************************************************************/
/* Copyright (c) 2004                                                           */
/* Daniel Sleator, David Temperley, and John Lafferty                           */
/* All rights reserved                                                          */
/*                                                                              */
/* Use of the link grammar parsing system is subject to the terms of the        */
/* license set forth in the LICENSE file included with this software,           */ 
/* and also available at http://www.link.cs.cmu.edu/link/license.html           */
/* This license allows free redistribution and use in source and binary         */
/* forms, with or without modification, subject to certain conditions.          */
/*                                                                              */
/********************************************************************************/
int  maxcost_of_sentence(Sentence sent);
void build_sentence_disjuncts(Sentence sent, int cost_cutoff);
void print_disjunct_list(Disjunct *);
X_node *   build_word_expressions(Sentence sent, char *);
Disjunct * build_disjuncts_for_dict_node(Dict_node *);
