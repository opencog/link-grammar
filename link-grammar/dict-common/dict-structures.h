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

#ifndef _LG_DICT_STRUCTURES_H_
#define _LG_DICT_STRUCTURES_H_

#include <stdint.h>
#include <link-grammar/link-features.h>
#include "link-includes.h"

LINK_BEGIN_DECLS

/* Forward decls */
typedef struct Dict_node_struct Dict_node;
typedef struct Exp_struct Exp;
typedef struct E_list_struct E_list;
typedef struct Word_file_struct Word_file;


/* For faster comparisons, the connector lc part is encoded into a number
 * and a mask. Each letter is encoded using LC_BITS bits. With 7 bits, it
 * is possible to encode up to 9 letters in an uint64_t. */
#define LC_BITS 7
#define LC_MASK ((1<<LC_BITS)-1)
typedef uint64_t lc_enc_t;

typedef uint16_t connector_hash_size; /* Change to uint32_t if needed. */

/* When connector_hash_size is uint16_t, the size of the following
 * struct on a 64-bit machine is 32 bytes.
 * FIXME: Make it 16 bytes by separating the info that is not needed
 * by do_count() into another structure (and some other minor changes). */
typedef struct
{
	lc_enc_t lc_letters;
	lc_enc_t lc_mask;

	const char *string;  /* The connector name w/o the direction mark, e.g. AB */
	// double *cost; /* Array of cost by length_limit (cost[0]: default) */
	connector_hash_size str_hash;
	union
	{
		connector_hash_size uc_hash;
		connector_hash_size uc_num;
	};
	uint8_t length_limit;
	                      /* If not 0, it gives the limit of the length of the
	                       * link that can be used on this connector type. The
	                       * value UNLIMITED_LEN specifies no limit.
	                       * If 0, short_length (a Parse_Option) is used. If
	                       * all_short==true (a Parse_Option), length_limit
	                       * is clipped to short_length. */
	char head_dependent;   /* 'h' for head, 'd' for dependent, or '\0' if none */

	/* For connector match speedup when sorting the connector table. */
	uint8_t uc_length;   /* uc part length */
	uint8_t uc_start;    /* uc start position */
} condesc_t;

/**
 * Types of Exp_struct structures
 */
typedef enum
{
	OR_type = 1,
	AND_type,
	CONNECTOR_type
} Exp_type;

/**
 * The E_list and Exp structures defined below comprise the expression
 * trees that are stored in the dictionary.  The expression has a type
 * (OR_type, AND_type or CONNECTOR_type).  If it is not a terminal it
 * has a list (an E_list) of children. Else "string" is the connector,
 * and "dir" indicates its direction.
 */
struct Exp_struct
{
	Exp * next;    /* Used only for mem management,for freeing */
	Exp_type type; /* One of three types: AND, OR, or connector. */
	char dir;      /* The connector connects to: '-': the left; '+': the right */
	bool multi;    /* TRUE if a multi-connector (for connector)  */
	union {
		E_list * l;           /* Only needed for non-terminals */
		condesc_t * condesc;  /* Only needed if it's a connector */
	} u;
	double cost;   /* The cost of using this expression.
	                  Only used for non-terminals */
};

struct E_list_struct
{
	E_list * next;
	Exp * e;
};

/**
 * The dictionary is stored as a binary tree comprised of the following
 * nodes.  A list of these (via right pointers) is used to return
 * the result of a dictionary lookup.
 */
struct Dict_node_struct
{
	const char * string;  /* The word itself */
	Word_file * file;     /* The file the word came from (NULL if dict file) */
	Exp       * exp;
	Dict_node *left, *right;
};

LINK_END_DECLS

#endif /* _LG_DICT_STRUCTURES_H_ */
