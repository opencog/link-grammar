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
#include "structures.h"
#include "api-structures.h"

#ifdef _MSC_VER
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

/* ============================================================ */

static void verr_msg(err_ctxt *ec, severity sev, const char *fmt, va_list args)
	GNUC_PRINTF(3,0);

static void verr_msg(err_ctxt *ec, severity sev, const char *fmt, va_list args)
{
	fprintf(stderr, "link-grammar: ");
	vfprintf(stderr, fmt, args);

	if ((Info != sev) && ec->sent != NULL)
	{
		int i;
		fprintf(stderr, "\tFailing sentence was:\n\t");
		for (i=0; i<ec->sent->length; i++)
		{
			fprintf(stderr, "%s ", ec->sent->word[i].alternatives[0]);
		}
	}
	fprintf(stderr, "\n");
}

void err_msg(err_ctxt *ec, severity sev, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	verr_msg(ec, sev, fmt, args);
	va_end(args);
}

void prt_error(const char *fmt, ...)
{
	severity sev;
	err_ctxt ec;
	va_list args;

	sev = Error;
	if (0 == strncmp(fmt, "Fatal", 5)) sev = Fatal;
	if (0 == strncmp(fmt, "Error:", 6)) sev = Error;
	if (0 == strncmp(fmt, "Warn", 4)) sev = Warn;
	if (0 == strncmp(fmt, "Info:", 5)) sev = Info;

	ec.sent = NULL;
	va_start(args, fmt);
	verr_msg(&ec, sev, fmt, args);
	va_end(args);
}

/* ============================================================ */
