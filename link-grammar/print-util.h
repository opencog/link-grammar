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
#ifndef _PRINTUTILH_
#define _PRINTUTILH_

typedef struct String_s String;
struct String_s {
    unsigned int allocated;  /* Unsigned so VC++ doesn't complain about comparisons */
    char * p;
    char * eos;
};

String * String_create();
int append_string(String * string, char *fmt, ...);

#endif


