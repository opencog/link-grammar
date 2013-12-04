/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _LG_READ_DICT_H_
#define  _LG_READ_DICT_H_

#include <link-grammar/link-includes.h>

LINK_BEGIN_DECLS

/* forward decls */
typedef char Boolean;
typedef struct Dict_node_struct Dict_node;
typedef struct Exp_struct Exp;
typedef struct E_list_struct E_list;
typedef struct Word_file_struct Word_file;

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
 * (AND, OR or TERMINAL).  If it is not a terminal it has a list
 * (an E_list) of children.
 */
struct Exp_struct
{
    Exp * next; /* Used only for mem management,for freeing */
    Exp_type type;  /* One of three types, see above */
    char dir;   /* '-' means to the left, '+' means to right (for connector) */
    Boolean multi; /* TRUE if a multi-connector (for connector)  */
    union {
        E_list * l;           /* only needed for non-terminals */
        const char * string;  /* only needed if it's a connector */
    } u;
    float cost;   /* The cost of using this expression.
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
    const char * string;  /* the word itself */
    Word_file * file;    /* the file the word came from (NULL if dict file) */
    Exp       * exp;
    Dict_node *left, *right;
};

void print_dictionary_data(Dictionary dict);
void print_dictionary_words(Dictionary dict);
void print_expression(Exp *);

Boolean boolean_dictionary_lookup(Dictionary dict, const char *);
Boolean find_word_in_dict(Dictionary dict, const char *);

int  delete_dictionary_words(Dictionary dict, const char *);

Dict_node * dictionary_lookup_list(Dictionary dict, const char *);
Dict_node * abridged_lookup_list(Dictionary dict, const char *);
void free_lookup_list(Dict_node *);

Dict_node * insert_dict(Dictionary dict, Dict_node * n, Dict_node * newnode);

Dictionary dictionary_create_from_utf8(const char * input);

/* Below are not really for public consumption.
 * These need to be moved to a private header file. TODO XXX */
Exp *       Exp_create(Dictionary dict);
void        free_dictionary(Dictionary dict);
Boolean read_dictionary(Dictionary dict);
void add_empty_word(Dictionary dict, Dict_node *);


LINK_END_DECLS

#endif /* _LG_READ_DICT_H_ */
