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

#ifndef _POSTPROCESSH_
#define _POSTPROCESSH_

#define PP_FIRST_PASS  1
#define PP_SECOND_PASS 2

/* Postprocessor * post_process_open(char *dictname, char *path);  this is in api-prototypes.h */

void     post_process_free_data(PP_data * ppd);
void     post_process_close_sentence(Postprocessor *);
void     post_process_scan_linkage(Postprocessor * pp, Parse_Options opts,
				   Sentence sent , Sublinkage * sublinkage);
PP_node *post_process(Postprocessor * pp, Parse_Options opts, 
		      Sentence sent, Sublinkage *, int cleanup);
int      post_process_match(char *s, char *t);  /* utility function */

void          free_d_type(D_type_list * dtl);
D_type_list * copy_d_type(D_type_list * dtl);


#endif
