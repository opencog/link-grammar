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
#include "dict-common/dict-structures.h"  /* For Exp, Exp_list */
#include "histogram.h"  /* Count_bin */

/* The following define the names of the special strings in the dictionary. */
#define LEFT_WALL_WORD   ("LEFT-WALL")
#define RIGHT_WALL_WORD  ("RIGHT-WALL")

#define UNKNOWN_WORD "UNKNOWN-WORD"

/*      Some size definitions.  Reduce these for small machines */
/* MAX_WORD is large, because Unicode entries can use a lot of space */
#define MAX_WORD 180          /* maximum number of bytes in a word */

#define MAX_LINE 500          /* maximum width of print area */

#define UNLIMITED_LEN 255
#define SHORT_LEN 6

/* Word subscripts come after the subscript mark (ASCII ETX)
 * In the dictionary, a dot is used; but that dot interferes with dots
 * in the input stream, and so we convert dictionary dots into the
 * subscript mark, which we don't expect to see in user input.
 */
#define SUBSCRIPT_MARK '\3'
#define SUBSCRIPT_DOT '.'

typedef struct gword_set gword_set;

/* On a 64-bit machine, this struct should be exactly 4*8=32 bytes long.
 * Lets try to keep it that way.
 */
struct Connector_struct
{
	int16_t hash;
	uint8_t length_limit;
	             /* If this is a length limited connector, this
	                gives the limit of the length of the link
	                that can be used on this connector.  Since
	                this is strictly a function of the connector
	                name, efficiency is the only reason to store
	                this.  If no limit, the value is set to 255. */
	uint8_t nearest_word;
	             /* The nearest word to my left (or right) that
	                this could ever connect to.  Computed by
	                setup_connectors() */
	bool multi;  /* TRUE if this is a multi-connector */
	uint8_t lc_start;     /* lc start position (or 0) - for match speedup. */
	uint8_t uc_length;    /* uc part length - for match speedup. */
	uint8_t uc_start;     /* uc start position - for match speedup. */
	Connector * next;
	const char * string; /* The connector name w/o the direction mark, e.g. AB */

	/* Hash table next pointer, used only during pruning. */
	union
	{
		Connector * tableNext;
		const gword_set *originating_gword;
	};
};

static inline void connector_set_string(Connector *c, const char *s)
{
	c->string = s;
	c->hash = -1;
}
static inline const char * connector_get_string(Connector *c)
{
	return c->string;
}

//#ifdef DEBUG
#define VERIFY_MATCH_LIST
//#endif
struct Disjunct_struct
{
	Disjunct *next;
	Connector *left, *right;
	double cost;
	bool marked;               /* unmarked disjuncts get deleted */
	/* match_left, right used only during parsing, for the match list. */
	bool match_left, match_right;
#ifdef VERIFY_MATCH_LIST
	int match_id;              /* verify the match list integrity */
#endif
	gword_set *originating_gword; /* List of originating gwords */
	const char * string;       /* subscripted dictionary word */
};

typedef struct X_node_struct X_node;
struct X_node_struct
{
	const char * string;       /* the word itself */
	Exp * exp;
	X_node *next;
	const Gword *word;         /* originating Wordgraph word */
};

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
