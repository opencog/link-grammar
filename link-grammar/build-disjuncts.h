/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2012 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "api-types.h"
#include "structures.h"

void build_sentence_disjuncts(Sentence sent, double cost_cutoff);
X_node *   build_word_expressions(Dictionary dict, const char *);
Disjunct * build_disjuncts_for_exp(Exp*, const char*, double cost_cutoff);

unsigned int count_disjunct_for_dict_node(Dict_node *dn);

#ifdef DEBUG
void prt_exp(Exp *, int);
#endif
