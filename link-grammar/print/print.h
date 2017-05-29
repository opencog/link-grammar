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

#ifndef _PRINT_H
#define _PRINT_H

#include "link-includes.h"

#define LEFT_WALL_DISPLAY  ("LEFT-WALL")  /* the string to use to show the wall */
#define RIGHT_WALL_DISPLAY ("RIGHT-WALL") /* the string to use to show the wall */

typedef struct String_s String;

void   print_disjunct_counts(Sentence sent);
void   print_expression_sizes(Sentence sent);
struct tokenpos;
void   print_sentence_word_alternatives(Sentence sent, bool debugprint,
       void (*display)(Dictionary, const char *), struct tokenpos *);
void print_with_subscript_dot(const char *);

// Used for debug/error printing
void print_sentence_context(Sentence, dyn_str*);

#endif /* _PRINT_H */
