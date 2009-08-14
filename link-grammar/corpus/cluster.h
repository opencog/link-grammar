/*
 * cluster.h
 *
 * Data for related-word clusters. Meant to expand disjunct covereage
 * for the case where a parse cannot be completed without ommitting 
 * a word.
 *
 * Copyright (c) 2009 Linas Vepstas <linasvepstas@gmail.com>
 */

#ifndef _LINKGRAMMAR_CLUSTER_H
#define _LINKGRAMMAR_CLUSTER_H

#ifdef USE_CORPUS

#include "../api-types.h"
#include "../link-includes.h"

Cluster * lg_cluster_new(void);
void lg_cluster_delete(Cluster *);

Disjunct * lg_cluster_get_disjuncts(Cluster *, const char * wrd);

#else /* USE_CORPUS */

static inline Cluster * lg_cluster_new(void) { return NULL; }
static inline void lg_cluster_delete(Cluster *c) {}
static inline Disjunct * lg_cluster_get_disjuncts(Cluster *c, const char * wrd) { return NULL; }

#endif /* USE_CORPUS */

#endif /* _LINKGRAMMAR_CLUSTER_H */
