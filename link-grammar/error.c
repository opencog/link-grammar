/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2009 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
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
	/* As we are printing to stderr, make sure that stdout has been
	 * written out first. */
	fflush(stdout);

	fprintf(stderr, "link-grammar: ");
	vfprintf(stderr, fmt, args);

	if ((Info != sev) && ec->sent != NULL)
	{
		size_t i, j;
		const char **a, **b;

#if 0
		/* Previous code. Documenting its problem:
		 * In the current library version (using Wordgraph) it may print a
		 * nonsense sequence of morphemes if the words have been split to
		 * morphemes in various ways, because the "alternatives" array doesn't
		 * hold real alternatives any more (see example in the comments of
		 * print_sentence_word_alternatives()).
		 *
		 * We could print the first path in the Wordgraph, analogous to what we
		 * did here, but (same problem as printing alternatives[0] only) it may
		 * not contain all the words, including those that failed (because they
		 * are in another path). */

		fprintf(stderr, "\tFailing sentence was:\n\t");
		for (i=0; i<ec->sent->length; i++)
		{
			fprintf(stderr, "%s ", ec->sent->word[i].alternatives[0]);
		}
#else
		/* The solution is just to print all the sentence tokenized subwords in
		 * their order in the sentence, without duplications. */

		fprintf(stderr,
		        "\tFailing sentence contains the following words/morphemes:\n\t");
		for (i=0; i<ec->sent->length; i++)
		{
			for (a = ec->sent->word[i].alternatives; NULL != *a; a++)
			{
				bool next_word = false;

				if (0 == strcmp(*a, EMPTY_WORD_MARK)) continue;
				for (j=0; j<ec->sent->length; j++)
				{
					for (b = ec->sent->word[j].alternatives; NULL != *b; b++)
					{
						/* print only the first occurrence. */
						if (0 == strcmp(*a, *b))
						{
							next_word = true;
							if (a != b) break;
							fprintf(stderr, "%s ", *a);
							break;
						}
					}
					if (next_word) break;
				}
			}
		}
#endif
	}
	fprintf(stderr, "\n");
	fflush(stderr); /* In case some OS does some strange thing */
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

#define MAX_FUNCTION_NAME_SIZE 128
/**
 * Check whether the given feature is enabled. It is considered
 * enabled if it is found in the comma-separated list of features.
 * This list, if not empty, has a leading and a trailing comma.
 */
bool feature_enabled(const char * list, const char * feature)
{
	char buff[MAX_FUNCTION_NAME_SIZE] = ",";
	size_t len;

	strncpy(buff+1, feature, sizeof(buff)-2);
	len = strlen(feature);
	if (len < sizeof(buff)-2)
	{
		buff[len+1] = ',';
		buff[len+2] = '\0';
	}
	else
	{
		buff[sizeof(buff)-1] = '\0';
	}
	return NULL != strstr(list, buff);
}


/* ============================================================ */
