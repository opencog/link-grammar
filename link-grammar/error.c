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
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include "error.h"

#define MSGSZ 1024

#ifdef _MSC_VER
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

DLLEXPORT int  lperrno = 0;
DLLEXPORT char lperrmsg[MSGSZ];

static const char * msg_of_lperror(int lperr)
{
	switch(lperr) {
	case NODICT:
		return "Could not open dictionary ";
	case DICTPARSE:
		return "Error parsing dictionary ";
	case WORDFILE:
		return "Error opening word file ";
	case SEPARATE:
		return "Error separating sentence ";
	case NOTINDICT:
		return "Sentence not in dictionary ";
	case CHARSET:
		return "Unable to process UTF8 input string in current locale ";
	case BUILDEXPR:
		return "Could not build sentence expressions ";
	case INTERNALERROR:
		return "Internal error.  Send mail to link-grammar@googlegroups.com ";
	default:
		return "";
	}
	return "";
}

void lperror(int lperr, const char *fmt, ...)
{
	char temp[MSGSZ] = "";
	char stack[MSGSZ] = "";

#if (defined _MSC_VER) && _MFC_VER < 0x0700
#define vsnprintf _vsnprintf
#endif		
	va_list args;

	va_start(args, fmt);
	vsnprintf(temp, MSGSZ, fmt, args);
	va_end(args);

	/* Maintain a stack of errors, earliest error first */
	strncat(lperrmsg, msg_of_lperror(lperr), MSGSZ-strlen(lperrmsg)-1);
	strncat(lperrmsg, temp, MSGSZ-strlen(lperrmsg)-1);
	strncat(lperrmsg, "\n", MSGSZ-strlen(lperrmsg)-1);
	lperrmsg[MSGSZ-1]=0; /* Null terminate at all costs */

	lperrno = lperr;
}

void lperror_clear(void)
{
	lperrmsg[0] = 0x0;
	lperrno = 0;
}

void error(const char *fmt, ...)
{
	int norr = errno;
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
	if (norr > 0)
	{
		perror(NULL);
		fprintf(stderr, "errno=%d\n\n", norr);
	}
	fflush(stdout);
	fprintf(stderr, "Parser quitting.\n");
	exit(1); /* Always fail and print out this file name */
}
