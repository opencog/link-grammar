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

#include "api-types.h"
#include "link-includes.h"

Postprocessor * post_process_new(pp_knowledge *);
void post_process_free(Postprocessor *);

PP_data* pp_data_new(void);
void     post_process_free_data(PP_data *);
PP_node *do_post_process(Postprocessor *, PP_data*, Linkage, bool);
bool     post_process_match(const char *, const char *);  /* utility function */

void linkage_free_pp_info(Linkage);

void linkage_set_domain_names(Postprocessor*, PP_data *, Linkage);
void exfree_domain_names(PP_info *);

void post_process_lkgs(Sentence, Parse_Options);

#endif
