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
/* --------------------------------------------------------------------
 linkset.h
 6/96 ALB
 maintains sets of pointers to link names 
 stores either a copy of name, or the actual object. However,
 the memory used by *solid* objects is not freed!
--------------------------------------------------------------------- */

#ifndef _LINKSETH_
#define _LINKSETH_

#define LINKSET_SPARSENESS   2
#define LINKSET_MAX_SETS     512
#define LINKSET_DEFAULT_SEED 37

typedef struct linkset_node {   
    char   *str;
    struct linkset_node *next;
    char   solid;    /* should we delete the pointer when deleting the node?*/
} LINKSET_NODE;


/* stores information for one 'instance' of pset */
typedef struct {
    int hash_table_size;
    LINKSET_NODE **hash_table;    /* data actually lives here */
} LINKSET_SET;


/* declarations of exported functions */
int   linkset_open(const int size); 
void  linkset_clear(const int unit);
void  linkset_close(const int unit);
int   linkset_add(const int unit, char *str);
int   linkset_add_solid(const int unit, char *str);
int   linkset_remove(const int unit, char *str);
int   linkset_match(const int unit, char *str);
int   linkset_match_bw(const int unit, char *str);

#endif
