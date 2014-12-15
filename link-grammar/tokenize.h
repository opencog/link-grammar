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
bool flatten_wordgraph(Sentence, Parse_Options);
#ifdef USE_ANYSPLIT
Gword *issue_word_alternative(Sentence sent, Gword *unsplit_word,
                     const char *label,
                     int prefnum, const char **prefix,
                     int stemnum, const char **stem,
                     int suffnum, const char **suffix);
#endif

/* Wordgraph utilities */
/* FIXME? Move to a separate file. */
size_t wordgraph_pathpos_len(Wordgraph_pathpos *);
Wordgraph_pathpos *wordgraph_pathpos_resize(Wordgraph_pathpos *, size_t);
bool wordgraph_pathpos_append(Wordgraph_pathpos **, Gword *, bool, bool);
const Gword **wordgraph_hier_position(Gword *);
void print_hier_position(const Gword *);
bool in_same_alternative(Gword *, Gword *);
Gword *find_real_unsplit_word(Gword *, bool);
