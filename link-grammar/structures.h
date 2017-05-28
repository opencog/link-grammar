/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2013 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _STRUCTURES_H_
#define _STRUCTURES_H_

#include <stdint.h>

#include "api-types.h"
#include "api-structures.h"
#include "histogram.h"  /* Count_bin */

/* The following define the names of the special strings in the dictionary. */
#define LEFT_WALL_WORD   ("LEFT-WALL")
#define RIGHT_WALL_WORD  ("RIGHT-WALL")

#define UNKNOWN_WORD "UNKNOWN-WORD"

/*      Some size definitions.  Reduce these for small machines */
/* MAX_WORD is large, because Unicode entries can use a lot of space */
#define MAX_WORD 180          /* maximum number of bytes in a word */

#define MAX_LINE 500          /* maximum width of print area */

/* Word subscripts come after the subscript mark (ASCII ETX)
 * In the dictionary, a dot is used; but that dot interferes with dots
 * in the input stream, and so we convert dictionary dots into the
 * subscript mark, which we don't expect to see in user input.
 */
#define SUBSCRIPT_MARK '\3'
#define SUBSCRIPT_DOT '.'

typedef struct X_node_struct X_node;

/**
 * Word, as represented shortly after tokenization, but before parsing.
 *
 * X_node* x:
 *    Contains a pointer to a list of expressions from the dictionary,
 *    Computed by build_sentence_expressions().
 *
 * Disjunct* d:
 *   Contains a pointer to a list of disjuncts for this word.
 *   Computed by: prepare_to_parse(), but modified by pruning and power
 *   pruning.
 */
struct Word_struct
{
	const char *unsplit_word;

	X_node * x;          /* Sentence starts out with these, */
	Disjunct * d;        /* eventually these get generated. */
	bool optional;       /* Linkage is optional. */

	const char **alternatives;
};

#endif
