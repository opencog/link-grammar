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
#ifndef _CONSTITUENTSH_
#define _CONSTITUENTSH_

#include "link-includes.h"

/* Invariant: Leaf if child==NULL */
typedef struct CNode_s CNode;
struct CNode_s {
  char  * label;
  CNode * child;
  CNode * next;
  int   start, end;
};

CNode * linkage_constituent_tree(Linkage linkage);
void    linkage_free_constituent_tree(CNode * n);
char *  linkage_print_constituent_tree(Linkage linkage, int mode);

#endif
