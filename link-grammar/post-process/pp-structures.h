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

#ifndef _PP_STRUCTURES_H_
#define _PP_STRUCTURES_H_

#include "api-types.h"

/* Post-processing-related structures */

/* The following two structs comprise what is returned by post_process(). */
typedef struct D_type_list_struct D_type_list;
struct D_type_list_struct
{
	D_type_list * next;
	int type;
};

struct PP_node_struct
{
	size_t dtsz;
	D_type_list **d_type_array;
	const char *violation;
};

/* Davy added these */
struct List_o_links_struct
{
	size_t link;     /* the link number */
	size_t word;     /* the word at the other end of this link */
	List_o_links * next;
};

/* from pp_linkset.c */
typedef struct pp_linkset_node_s
{
	const char *str;
	struct pp_linkset_node_s *next;
} pp_linkset_node;

struct pp_linkset_s
{
	unsigned int hash_table_size;
	unsigned int population;
	pp_linkset_node **hash_table;    /* data actually lives here */
};


/* from pp_lexer.c */
typedef struct pp_label_node_s
{
	/* linked list of strings associated with a label in the table */
	const char *str;
	struct pp_label_node_s *next;
} pp_label_node;                 /* next=NULL: end of list */


/* from pp_knowledge.c */
typedef struct StartingLinkAndDomain_s
{
	const char *starting_link;
	int   domain;       /* domain which the link belongs to (-1: terminator)*/
} StartingLinkAndDomain;

typedef struct pp_rule_s
{
	/* Holds a single post-processing rule. Since rules come in many
	   flavors, not all fields of the following are always relevant  */
	const char *selector; /* name of link to which rule applies      */
	pp_linkset *link_set; /* handle to set of links relevant to rule */
	int   link_set_size;  /* size of this set                        */
	int   domain;         /* type of domain to which rule applies    */
	const char  **link_array; /* array holding the spelled-out names */
	const char  *msg;     /* explanation (NULL=end sentinel in array)*/
	int use_count;        /* Number of times rule has been applied   */
} pp_rule;

typedef struct PPLexTable_s PPLexTable;
struct pp_knowledge_s
{
	PPLexTable *lt; /* Internal rep'n of sets of strings from knowledge file */
	const char *path; /* Name of file we loaded from */

	/* handles to sets of links specified in knowledge file. These constitute
	   auxiliary data, necessary to implement the rules, below. See comments
	   in post-process.c for a description of these. */
	pp_linkset *domain_starter_links;
	pp_linkset *urfl_domain_starter_links;
	pp_linkset *urfl_only_domain_starter_links;
	pp_linkset *domain_contains_links;
	pp_linkset *must_form_a_cycle_links;
	pp_linkset *restricted_links;
	pp_linkset *ignore_these_links;
	pp_linkset *left_domain_starter_links;

	/* arrays of rules specified in knowledge file */
	pp_rule *form_a_cycle_rules;
	pp_rule *contains_one_rules;
	pp_rule *contains_none_rules;
	pp_rule *bounded_rules;

	size_t n_form_a_cycle_rules;
	size_t n_contains_one_rules;
	size_t n_contains_none_rules;
	size_t n_bounded_rules;

	size_t nStartingLinks;
	pp_linkset *set_of_links_starting_bounded_domain;
	StartingLinkAndDomain *starting_link_lookup_table;
	String_set *string_set;
};

#endif
