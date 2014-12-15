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
   . call post_process_open() with the name of a knowledge file. This
     returns a handle, used for all subsequent calls to post-process.
   . Do for each sentence:
       - Do for each generated linkage of sentence:
             + call post_process_scan_linkage()
       - Do for each generated linkage of sentence:
             + call post_process()
       - Call post_process_close_sentence() 
***********************************************************************/

#ifndef _POSTPROCESS_H_
#define _POSTPROCESS_H_

#include "api-types.h"
#include "structures.h"

#define PP_FIRST_PASS  1
#define PP_SECOND_PASS 2

void     post_process_free_data(PP_data * ppd);
void     post_process_close_sentence(Postprocessor *);
void     post_process_scan_linkage(Postprocessor * pp, Parse_Options opts,
                                   Sentence sent, Linkage sublinkage);
PP_node *do_post_process(Postprocessor * pp, Parse_Options opts, 
                         Sentence sent, Linkage);
bool     post_process_match(const char *s, const char *t);  /* utility function */

bool sane_linkage_morphism(Sentence sent, size_t lk, Parse_Options opts);

#endif
