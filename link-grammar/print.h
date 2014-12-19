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

#include "link-includes.h"

#define LEFT_WALL_DISPLAY  ("LEFT-WALL")  /* the string to use to show the wall */
#define RIGHT_WALL_DISPLAY ("RIGHT-WALL") /* the string to use to show the wall */

void   print_disjunct_counts(Sentence sent);
void   print_expression_sizes(Sentence sent);
struct tokenpos;
void   print_sentence_word_alternatives(Sentence sent, bool debugprint,
       void (*display)(Dictionary, const char *), struct tokenpos *);
void print_with_subscript_dot(const char *);
void print_chosen_disjuncts_words(const Linkage);
