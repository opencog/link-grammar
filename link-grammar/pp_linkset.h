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
pp_linkset *pp_linkset_open(int size); 
void pp_linkset_close     (pp_linkset *ls);
void pp_linkset_clear     (pp_linkset *ls);
int  pp_linkset_add       (pp_linkset *ls, char *str);
int  pp_linkset_match     (pp_linkset *ls, char *str);
int  pp_linkset_match_bw  (pp_linkset *ls, char *str);
int  pp_linkset_population(pp_linkset *ls); 

