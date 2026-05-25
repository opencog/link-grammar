/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.   */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _EXTRACT_LINKS_H
#define _EXTRACT_LINKS_H

#include "api-structures.h"
#include "link-includes.h"
#include "resources.h"

typedef struct extractor_s extractor_t;
typedef struct Metric_candidate_struct Metric_candidate;
typedef struct PP_failure_s PP_failure;

extractor_t* extractor_new(Sentence);
void free_extractor(extractor_t*);
void extractor_set_metric_enabled(extractor_t *, bool);
size_t extractor_finish_metric_bounded_domain_feedback_state(extractor_t *,
                                                            Metric_candidate *,
                                                            Linkage,
                                                            const PP_failure *);
void extractor_apply_metric_bounded_domain_feedback(extractor_t *);

bool build_parse_set(extractor_t*, Sentence,
                     fast_matcher_t*, count_context_t*,
                     unsigned int null_count, Parse_Options);

void extract_links(extractor_t*, Linkage);
bool extract_metric_links(extractor_t*, Linkage);
void extractor_trace_metric_candidate(extractor_t *, Linkage,
                                      const PP_failure *,
                                      Metric_candidate *, size_t);

void mark_used_disjuncts(extractor_t *, bool *);

// Uncomment to enable graphviz display of parse choice
// #define PC_DISPLAY
#ifdef PC_DISPLAY
void display_parse_choice(extractor_t *);
#endif

#endif /* _EXTRACT_LINKS_H */
