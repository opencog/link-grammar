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
#ifndef _ERRORH_
#define _ERRORH_

typedef enum {
    NODICT=1,
    DICTPARSE,
    WORDFILE,
    SEPARATE,
    NOTINDICT,
    BUILDEXPR,
    INTERNALERROR,
}   LP_error_type;

void lperror(int lperr, char *fmt, ...);
void error(char *fmt, ...);

#endif

