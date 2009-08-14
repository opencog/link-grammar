/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

/* verbosity global is held in utilities.c */
extern int verbosity;          /* the verbosity level for error messages */

/* size of random table for computing the
   hash functions.  must be a power of 2 */
#define RTSIZE 256

extern unsigned int randtable[RTSIZE];   /* random table for hashing */

