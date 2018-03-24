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

#include <link-grammar/link-features.h>
#include "link-includes.h"

LINK_BEGIN_DECLS

/* Forward decls */
typedef struct Dict_node_struct Dict_node;
typedef struct Conn_struct Conn;
typedef struct Exp_struct Exp;
typedef struct E_list_struct E_list;
typedef struct Word_file_struct Word_file;
typedef struct condesc_struct condesc_t;

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
 * A basic connector. The `string` is the (encoded) connector name,
 * `dir` indicates the connector direction, and `multi` is true if it
 * is a ulti-connector.
 */
struct Conn_struct
{
	char dir;      /* The connector connects to: '-': the left; '+': the right */
	bool multi;    /* TRUE if a multi-connector (for connector)  */
	const char * string;
	condesc_t * desc;  /* Compiled description of connector */
};

/**
 * The E_list and Exp structures defined below comprise the expression
 * trees that are stored in the dictionary.  The expression has a type
 * (OR_type, AND_type or CONNECTOR_type).  If it is not a terminal it
 * has a list (an E_list) of children. Else "string" is the connector,
 * and "dir" indicates its direction.
 */
struct Exp_struct
{
	Exp_type type; /* One of three types: AND, OR, or connector. */
	union {
		Conn c;
		E_list * l;           /* Only needed for non-terminals */
	} u;
	double cost;   /* The cost of using this expression.
	                  Only used for non-terminals */
	Exp * next;    /* Used only for mem management,for freeing */
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
