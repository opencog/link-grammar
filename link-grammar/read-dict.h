/********************************************************************************/
/* Copyright (c) 2004                                                           */
/* Daniel Sleator, David Temperley, and John Lafferty                           */
/* All rights reserved                                                          */
/*                                                                              */
/* Use of the link grammar parsing system is subject to the terms of the        */
/* license set forth in the LICENSE file included with this software,           */ 
/* and also available at http://www.link.cs.cmu.edu/link/license.html           */
/* This license allows free redistribution and use in source and binary         */
/* forms, with or without modification, subject to certain conditions.          */
/*                                                                              */
/********************************************************************************/
int open_dictionary(char * dict_path_name, Dictionary dict);
int  read_dictionary(Dictionary dict);
void dict_display_word_info(Dictionary dict, char * s);
void print_dictionary_data(Dictionary dict);
void print_dictionary_words(Dictionary dict);
void print_expression(Exp *);
int  boolean_dictionary_lookup(Dictionary dict, char *);
int  boolean_abridged_lookup(Dictionary dict, char *);
int  delete_dictionary_words(Dictionary dict, char *);
void free_lookup_list(void); 
            /* really doesn't need to be called outside 
               of the dictionary lookup code.  */
Dict_node * dictionary_lookup(Dictionary dict, char *);    
            /* remember, this returns a list  */
Dict_node * abridged_lookup(Dictionary dict, char *);
Dict_node * insert_dict(Dictionary dict, Dict_node * n, Dict_node * newnode);
void        free_dictionary(Dictionary dict);
Exp *       Exp_create(Dictionary dict);
