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

/* The sentence currently being parsed: needed for reporting parser
 * errors.  This needs to be made thread-safe at some point.
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
	Sentence sentence;
	int is_info;

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

#ifdef USE_PTHREADS
	pthread_once(&sentence_key_once, sentence_key_alloc);
	sentence = pthread_getspecific(failing_sentence_key);
#else
	sentence = failing_sentence;
#endif
	is_info = (0 == strncmp(fmt, "Info:", 5));
	if (!is_info && sentence != NULL)
	{
		int i;
		fprintf(stderr, "\tFailing sentence was:\n\t");
		for (i=0; i<sentence->length; i++)
		{
			fprintf(stderr, "%s ", sentence->word[i].string);
		}
		fprintf(stderr, "\n");
	}
}
