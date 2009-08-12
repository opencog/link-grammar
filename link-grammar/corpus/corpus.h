/*
 * corpus.h
 *
 * Data for corpus statistics, used to provide a parse ranking
 * to drive the SAT solver, as well as parse ranking with the
 * ordinary solver. 
 *
 * Copyright (c) 2008, 2009 Linas Vepstas <linasvepstas@gmail.com>
 */

#ifndef _LINKGRAMMAR_CORPUS_H
#define _LINKGRAMMAR_CORPUS_H

#ifdef USE_CORPUS

#include "../api-types.h"
#include "../link-includes.h"

Corpus * lg_corpus_new(void);
void lg_corpus_delete(Corpus *);

void lg_corpus_score(Sentence, Linkage_info *);
double lg_corpus_disjunct_score(Linkage linkage, int w);
void lg_corpus_linkage_senses(Linkage);

Sense * lg_get_word_sense(Linkage_info *, int word);
Sense * lg_sense_next(Sense *);
int lg_sense_get_index(Sense *);
const char * lg_sense_get_subscripted_word(Sense *);
const char * lg_sense_get_disjunct(Sense *);
const char * lg_sense_get_sense(Sense *);
double lg_sense_get_score(Sense *);
void lg_sense_delete(Linkage_info *);

#else /* USE_CORPUS */

static inline void lg_corpus_score(Sentence s, Linkage_info *li) {}
static inline void lg_corpus_linkage_senses(Linkage l) {}
static inline Sense * lg_get_word_sense(Linkage_info *lif, int word) { return NULL; }
static inline Sense * lg_sense_next(Sense *s ) {return NULL; }
static inline const char * lg_sense_get_sense(Sense *s) { return NULL; }
static inline double lg_sense_get_score(Sense *s) { return 0.0; }
static inline double lg_corpus_disjunct_score(Linkage linkage, int w) { return 998.0; }
#endif /* USE_CORPUS */

#endif /* _LINKGRAMMAR_CORPUS_H */
