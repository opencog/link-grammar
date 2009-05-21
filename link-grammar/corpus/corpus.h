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
void lg_corpus_linkage_senses(Linkage);

Sense *lg_sense_next(Sense *);
int lg_sense_get_index(Sense *);
const char * lg_sense_get_subscripted_word(Sense *);
const char * lg_sense_get_disjunct(Sense *);
const char * lg_sense_get_sense(Sense *);
double lg_sense_get_score(Sense *);
void lg_sense_delete(Linkage_info *);

#else /* USE_CORPUS */

static inline void lg_corpus_score(Sentence s, Linkage_info *li) {}
#endif /* USE_CORPUS */

#endif /* _LINKGRAMMAR_CORPUS_H */
