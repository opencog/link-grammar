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
void free_sentence_expressions(Sentence sent);
void free_sentence_disjuncts(Sentence sent);
void free_deletable(Sentence sent);
void free_effective_dist(Sentence sent);
void prepare_to_parse(Sentence sent, Parse_Options opts);
void install_fat_connectors(Sentence sent);
