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
int   build_parse_set(Sentence sent, int cost, Parse_Options opts);
void  free_parse_set(Sentence sent);
void  extract_links(int index, int cost, Parse_info * pi);
void  build_current_linkage(Parse_info * pi);
void  advance_parse_set(Parse_info * pi);
void  init_x_table(Sentence sent);
