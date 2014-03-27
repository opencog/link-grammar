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

void   print_disjunct_counts(Sentence sent);
void   print_expression_sizes(Sentence sent);
void   compute_chosen_words(Sentence sent, Linkage linkage);
struct tokenpos
{
	int wi;
	int ai;
} print_sentence_word_alternatives(Sentence sent, bool debugprint,
			void (*display)(Dictionary, const char *), const char * findtoken);
