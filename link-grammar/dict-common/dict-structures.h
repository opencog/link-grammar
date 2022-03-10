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

#ifndef SWIG
LINK_BEGIN_DECLS
#endif /* !SWIG */

/* Forward decls */
typedef struct Dict_node_struct Dict_node;
typedef struct Exp_struct Exp;
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

#ifndef SWIG
static const int cost_max_dec_places = 3;
static const float cost_epsilon = 1E-5;

#define EXPTAG_SZ 100 /* Initial size for the Exptag array. */
typedef enum { Exptag_none=0, Exptag_dialect, Exptag_macro } Exptag_type;

/**
 * The Exp structure defined below comprises the expression trees that are
 * stored in the dictionary. The expression has a type (OR_type, AND_type
 * or CONNECTOR_type). If it is not a terminal "operand_first" points to its
 * list of operands (when each of them points to the next one through
 * "operand_next"). Else "condesc" is the connector descriptor, when "dir"
 * indicates the connector direction.
 * For CONNECTOR_type, the "pos" filed is the expression ordinal position
 * (found when building the disjuncts, for "!!word/m").
 */
struct Exp_struct
{
	Exp_type type:8;      /* One of three types: AND, OR, or connector. */
	unsigned int unsued:8;
	unsigned int pos:16;  /* The position in the expression. */
	union
	{
		struct /* For non-terminals. */
		{
			Exptag_type tag_type:8; /* tag_id namespace. */
			unsigned int tag_id:24; /* Index in tag_type namespace. */
		};
		struct /* For connectors */
		{
			bool multi; /* TRUE if a multi-connector. */
			char dir;   /* Connects to the left ('-') or right ('+'). */
			unsigned char farthest_word; /* See Connector_struct. */
		};
	};
	float cost;            /* The cost of using this expression. */
	union
	{
		Exp *operand_first; /* First operand (for non-terminals). */
		condesc_t *condesc; /* Only needed if it's a connector. */
	};
	Exp *operand_next;     /* Next same-level operand. */
};
#endif /* !SWIG */

/* List of words in a dictionary category. */
typedef struct
{
	unsigned int num_words;
	const char* name;
	Exp *exp;
	char const ** word;
} Category;

/* List of disjuncts categories and their costs. */
typedef struct
{
	unsigned int num;    /* Index in the Category array. */
	float cost;          /* Corresponding disjunct cost. */
} Category_cost;

#ifndef SWIG
bool cost_eq(float cost1, float cost2);
const char *cost_stringify(float cost);
#endif /* !SWIG */

/* API to access the above structure. */
static inline Exp_type lg_exp_get_type(const Exp* exp) { return exp->type; }
static inline char lg_exp_get_dir(const Exp* exp) { return exp->dir; }
static inline bool lg_exp_get_multi(const Exp* exp) { return exp->multi; }
static inline double lg_exp_get_cost(const Exp* exp) { return exp->cost; }
static inline const Exp* lg_exp_operand_first(const Exp* exp)
                                             { return exp->operand_first; }
static inline const Exp* lg_exp_operand_next(const Exp* exp)
                                            { return exp->operand_next; }
link_public_api(const char *)
	lg_exp_get_string(const Exp*);
link_public_api(char *)
	lg_exp_stringify(const Exp *); /* To be freed by the caller. */
link_experimental_api(Exp *)
	lg_exp_resolve(Dictionary, const Exp *, Parse_Options);

/**
 * The dictionary is stored as a binary tree comprised of the following
 * nodes.  A list of these (via right pointers) is used to return
 * the result of a dictionary lookup.
 */
struct Dict_node_struct
{
	const char * string;  /* The word itself */
	const char * file;    /* The file the word came from (NULL if dict file) */
	Exp       * exp;
	Dict_node *left, *right;
};

#ifndef SWIG
LINK_END_DECLS
#endif /* !SWIG */

#endif /* _LG_DICT_STRUCTURES_H_ */
