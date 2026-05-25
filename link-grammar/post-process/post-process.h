/*************************************************************************/
/* Copyright (c) 2004                                                    */
/*        Daniel Sleator, David Temperley, and John Lafferty             */
/* Copyright (c) 2014 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/
/**********************************************************************
  Calling paradigm:
   . call post_process_new() with the handle of a knowledge set. This
     returns a handle, used for all subsequent calls to post-process.
   . Do for each sentence:
       - Do for each generated linkage of a sentence:
             + call post_process_scan_linkage()
       - Do for each generated linkage of a sentence:
             + call do_post_process()
       - Call post_process_free()
***********************************************************************/

#ifndef _POSTPROCESS_H_
#define _POSTPROCESS_H_

#include <stdint.h>

#include "api-types.h"
#include "link-includes.h"

typedef struct PP_data_s PP_data;

#define PP_FAILURE_MAX_LINKS 64

typedef enum
{
	PP_FAILURE_NONE,
	PP_FAILURE_CONTAINS_ONE,
	PP_FAILURE_CONTAINS_NONE,
	PP_FAILURE_CONTAINS_ONE_GLOBAL,
	PP_FAILURE_MUST_FORM_CYCLE,
	PP_FAILURE_BOUNDED
} PP_failure_type;

typedef enum
{
	PP_DOMAIN_NONE,
	PP_DOMAIN_REGULAR,
	PP_DOMAIN_URFL,
	PP_DOMAIN_URFL_ONLY,
	PP_DOMAIN_LEFT
} PP_domain_kind;

typedef struct PP_failure_s
{
	PP_failure_type type;
	const char *message;
	const char *selector;
	const char **criteria;
	int domain;
	unsigned int domain_start_link;
	unsigned int domain_links[PP_FAILURE_MAX_LINKS];
	size_t num_domain_links;
	unsigned int selector_links[PP_FAILURE_MAX_LINKS];
	size_t num_selector_links;
	unsigned int criterion_links[PP_FAILURE_MAX_LINKS];
	size_t num_criterion_links;
	unsigned int offending_links[PP_FAILURE_MAX_LINKS];
	size_t num_offending_links;
	bool has_domain_start_link;
	bool truncated;
} PP_failure;

Postprocessor * post_process_new(pp_knowledge *);
void post_process_free(Postprocessor *);

void post_process_lkgs(Sentence, Parse_Options);
void post_process_lkgs_with_failures(Sentence, Parse_Options,
                                     PP_failure *, size_t);
void post_process_reset(Postprocessor *);
void post_process_set_metric_rule_suppression(Postprocessor *, bool, bool, bool);
const PP_failure *post_process_get_failure(Postprocessor *);
const PP_failure *post_process_find_failure(Sentence, Linkage, Parse_Options);
bool post_process_must_form_cycle(Postprocessor *, Linkage, PP_failure *);

void     do_post_process(Postprocessor *, Linkage, bool);
void     post_process_free_data(PP_data * ppd);
bool     post_process_match(const char *, const char *);  /* utility function */
bool     post_process_link_ignored(Postprocessor *, const char *);
bool     post_process_link_must_form_cycle(Postprocessor *, const char *);
bool     post_process_link_restricted(Postprocessor *, const char *);
PP_domain_kind post_process_link_domain_kind(Postprocessor *, const char *);
bool     post_process_link_domain_starter(Postprocessor *, const char *);
bool     post_process_link_domain_contains(Postprocessor *, const char *);
uint64_t post_process_parse_contains_one_selector_mask(Postprocessor *,
                                                       const char *);
uint64_t post_process_parse_contains_one_criterion_mask(Postprocessor *,
                                                        const char *);
uint64_t post_process_parse_contains_one_active_mask(Postprocessor *);
size_t   post_process_parse_contains_one_rule_count(Postprocessor *);
const char *post_process_parse_contains_one_rule_message(Postprocessor *,
                                                         size_t);
uint64_t post_process_parse_contains_one_global_selector_mask(Postprocessor *,
                                                              const char *);
uint64_t post_process_parse_contains_one_global_criterion_mask(Postprocessor *,
                                                               const char *);
size_t   post_process_parse_contains_one_global_rule_count(Postprocessor *);
const char *post_process_parse_contains_one_global_rule_message(
                                                        Postprocessor *,
                                                        size_t);
uint64_t post_process_parse_contains_none_selector_mask(Postprocessor *,
                                                        const char *);
uint64_t post_process_parse_contains_none_forbidden_mask(Postprocessor *,
                                                         const char *);
uint64_t post_process_parse_contains_none_active_mask(Postprocessor *);
size_t   post_process_parse_contains_none_rule_count(Postprocessor *);
const char *post_process_parse_contains_none_rule_message(Postprocessor *,
                                                          size_t);
bool     post_process_has_parse_constraints(Postprocessor *);

void compute_domain_names(Linkage);
void linkage_free_pp_domains(Linkage);

#endif
