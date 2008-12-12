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

#ifdef USE_PTHREADS
#include <pthread.h>
#endif

#include "error.h"
#include "structures.h"
#include "api-structures.h"

#ifdef _MSC_VER
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

/* ============================================================ */

struct err_ctxt_s
{
	Sentence sent;
};

err_ctxt * error_context_new(const Sentence s)
{
	err_ctxt *ec;
	ec = (err_ctxt *) malloc(sizeof(err_ctxt));
	ec->sent = s;
	return ec;
}

void error_context_delete(err_ctxt *ec)
{
	ec->sent = NULL;
	free(ec);
}

static void verr_msg(err_ctxt *ec, severity sev, const char *fmt, va_list args)
{
	vfprintf(stderr, fmt, args);

	if ((Info != sev) && ec->sent != NULL)
	{
		int i;
		fprintf(stderr, "\tFailing sentence was:\n\t");
		for (i=0; i<ec->sent->length; i++)
		{
			fprintf(stderr, "%s ", ec->sent->word[i].string);
		}
		fprintf(stderr, "\n");
	}
}

void err_msg(err_ctxt *ec, severity sev, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	verr_msg(ec, sev, fmt, args);
	va_end(args);
}

/* ============================================================ */
/* These are deprecated, obsolete, and unused, but are still here 
 * because these are exported in the public API. Do not use these.
 */
DLLEXPORT int  lperrno = 0;
DLLEXPORT char lperrmsg[1];

extern void lperror_clear(void);
void lperror_clear(void)
{
	lperrmsg[0] = 0x0;
	lperrno = 0;
}

/* ============================================================ */
/* The sentence currently being parsed: needed for reporting parser
 * errors.  Deprecated, do not use in new code.
 */
#ifdef USE_PTHREADS
static pthread_key_t failing_sentence_key;
static pthread_once_t sentence_key_once = PTHREAD_ONCE_INIT;

static void sentence_key_alloc(void)
{
	pthread_key_create(&failing_sentence_key, NULL);
}

#else
static Sentence failing_sentence = NULL;
#endif

void error_report_set_sentence(const Sentence s)
{
#ifdef USE_PTHREADS
	pthread_once(&sentence_key_once, sentence_key_alloc);
	pthread_setspecific(failing_sentence_key, s);
#else
	failing_sentence = s;
#endif
}

void prt_error(const char *fmt, ...)
{
	severity sev;
	err_ctxt *ec;
	Sentence sentence;

#ifdef USE_PTHREADS
	pthread_once(&sentence_key_once, sentence_key_alloc);
	sentence = pthread_getspecific(failing_sentence_key);
#else
	sentence = failing_sentence;
#endif

	sev = Error;
	if (0 == strncmp(fmt, "Fatal", 5)) sev = Fatal;
	if (0 == strncmp(fmt, "Error:", 6)) sev = Error;
	if (0 == strncmp(fmt, "Warn", 4)) sev = Warn;
	if (0 == strncmp(fmt, "Info:", 5)) sev = Info;

	ec = error_context_new(sentence);
	va_list args;
	va_start(args, fmt);
	verr_msg(ec, sev, fmt, args);
	va_end(args);
	error_context_delete(ec);
}
