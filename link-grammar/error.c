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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "error.h"

#ifdef _MSC_VER
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

/* These are deprecated, obsolete, and unused, but are still here 
 * because these are exported in the public API. Do not use these.
 */
DLLEXPORT int  lperrno = 0;
DLLEXPORT char lperrmsg[1];

void lperror_clear(void)
{
	lperrmsg[0] = 0x0;
	lperrno = 0;
}

/* The sentence currently being parsed: needed for reporting parser
 * errors.  This needs to be made thread-safe at some point.
 */
static const char * failing_sentence = NULL;

void error_report_set_sentence(const char * s)
{
	failing_sentence = s;
}

void prt_error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	int is_info = (0 == strncmp(fmt, "Info:", 5));
	if (!is_info && failing_sentence != NULL && failing_sentence[0] != 0x0)
	{
		fprintf(stderr, "\tFailing sentence was:\n");
		fprintf(stderr, "\t%s\n", failing_sentence);
	}
}
