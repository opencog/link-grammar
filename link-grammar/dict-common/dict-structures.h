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

#include "link-includes.h"

LINK_BEGIN_DECLS

/* Forward decls */
typedef struct Dict_node_struct Dict_node;
typedef struct Exp_struct Exp;
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

static const int cost_max_dec_places = 3;
static const double cost_epsilon = 1E-5;

#define EXPTAG_SZ 100 /* Initial size for the Exptag array. */
typedef enum { Exptag_none=0, Exptag_dialect, Exptag_macro } Exptag_type;

/**
 * The Exp structure defined below comprises the expression trees that are
 * stored in the dictionary. The expression has a type (OR_type, AND_type
 * or CONNECTOR_type). If it is not a terminal "operand_first" points to its
 * list of operands (when each of them points to the next one through
 * "operand_next"). Else "condesc" is the connector descriptor, when "dir"
 * indicates the connector direction.
 */
struct Exp_struct
{
	Exp *operand_next;    /* Next same-level operand. */
	Exp_type type:8;      /* One of three types: AND, OR, or connector. */
	bool multi;         /* TRUE if a multi-connector (for connector). */
	char dir;      /* The connector connects to the left ('-') or right ('+'). */
	union
	{
		Exptag_type tag_type:8;      /* tag_id namespace (for non-terminals). */
		unsigned char farthest_word; /* For connectors, see Connector_struct. */
	};
	unsigned int tag_id;  /* Index in tag_type namespace. */
	float cost;           /* The cost of using this expression. */
	union
	{
		Exp *operand_first; /* First operand (for non-terminals). */
		condesc_t *condesc; /* Only needed if it's a connector. */
	};
};


bool cost_eq(double cost1, double cost2);
const char *cost_stringify(double cost);

/* API to access the above structure. */
static inline Exp_type lg_exp_get_type(const Exp* exp) { return exp->type; }
static inline char lg_exp_get_dir(const Exp* exp) { return exp->dir; }
static inline bool lg_exp_get_multi(const Exp* exp) { return exp->multi; }
static inline double lg_exp_get_cost(const Exp* exp) { return exp->cost; }
static inline Exp* lg_exp_operand_first(Exp* exp) { return exp->operand_first; }
static inline Exp* lg_exp_operand_next(Exp* exp) { return exp->operand_next; }
link_public_api(const char *)
	lg_exp_get_string(const Exp*);
link_public_api(const char *)
	lg_exp_stringify(const Exp *); /* Result is in static variable. */

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
