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
PPLexTable *pp_lexer_open(FILE *f);
void  pp_lexer_close                  (PPLexTable *lt);
int   pp_lexer_set_label              (PPLexTable *lt, const char *label);
int   pp_lexer_count_tokens_of_label  (PPLexTable *lt);
char *pp_lexer_get_next_token_of_label(PPLexTable *lt);
int   pp_lexer_count_commas_of_label  (PPLexTable *lt);
char **pp_lexer_get_next_group_of_tokens_of_label(PPLexTable *lt,int *n_toks);
