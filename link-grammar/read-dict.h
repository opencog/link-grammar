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

/* forwardd decls */
typedef char Boolean;
typedef struct Dict_node_struct Dict_node;
typedef struct Exp_struct Exp;
typedef struct E_list_struct E_list;

/** 
 * Types of Exp_struct structures
 */
#define OR_type 0
#define AND_type 1
#define CONNECTOR_type 2

/** 
 * The E_list and Exp structures defined below comprise the expression
 * trees that are stored in the dictionary.  The expression has a type
 * (AND, OR or TERMINAL).  If it is not a terminal it has a list
 * (an E_list) of children.
 */
struct Exp_struct
{
    Exp * next; /* Used only for mem management,for freeing */
    char type;  /* One of three types, see above */
    char dir;   /* '-' means to the left, '+' means to right (for connector) */
    char multi; /* TRUE if a multi-connector (for connector)  */
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


Boolean read_dictionary(Dictionary dict);
void dict_display_word_info(Dictionary dict, const char *);
void dict_display_word_expr(Dictionary dict, const char *);
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
void        free_dictionary(Dictionary dict);
Exp *       Exp_create(Dictionary dict);

Dictionary dictionary_create_from_utf8(const char * input);

Dictionary
dictionary_six(const char * lang, const char * dict_name,
                const char * pp_name, const char * cons_name,
                const char * affix_name, const char * regex_name);


LINK_END_DECLS

#endif /* _LG_READ_DICT_H_ */
