/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/
bool separate_sentence(Sentence, Parse_Options);
void build_sentence_expressions(Sentence, Parse_Options);
bool sentence_in_dictionary(Sentence);
#ifdef USE_ANYSPLIT
void add_alternative(Sentence sent,
                     int prefnum, const char **prefix,
                     int stemnum, const char **stem,
                     int suffnum, const char **suffix);
#endif
