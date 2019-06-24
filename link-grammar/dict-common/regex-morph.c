/*************************************************************************/
/* Copyright (c) 2005  Sampo Pyysalo                                     */
/* Copyright (c) 2009 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

/**
 * Support for the regular-expression based token matching system
 * using standard POSIX regex or PCRE2.
 * (Cannot use pcre2posix.h at this time because pcre2posix <= 10.32
 * don't have pcre2_regcomp() etc., and then regcomp() etc.
 * may bind to libc., e.g. when the Python bindings are used.)
 */

/* The regex include definitions must be checked here in reverse order
 * of their check in "configure.ac". */
#if HAVE_REGEX_H
/* On MS Windows, regex.h fails to pull in size_t, so work around this by
 * including <stddef.h> before <regex.h> (<sys/types.h> is not enough) */
#include <stddef.h>
#include <regex.h>
#elif HAVE_PCRE2_H
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#elif HAVE_TRE_REGEX_H
#include <tre/regex.h>
#else
#error No regex header file.
#endif
#include "api-structures.h"
#include "error.h"          /* verbosity */
#include "externs.h"        /* lgdebug() */
#include "dict-common/dict-api.h"
#include "dict-common/dict-common.h"
#include "dict-common/regex-morph.h"
#include "link-includes.h"

#if HAVE_PCRE2_H
typedef struct {
	pcre2_code *re_code;
	pcre2_match_data* re_md;
} regex_t;
#endif

/**
 * Notify an error message according to the error code.
 */
#define ERRBUFFLEN 120
static void prt_regerror(const char *msg, const Regex_node *re, int rc,
                         int erroffset)
{
#if HAVE_PCRE2_H
	PCRE2_UCHAR errbuf[ERRBUFFLEN];
	pcre2_get_error_message(rc, errbuf, ERRBUFFLEN);
#else
	char errbuf[ERRBUFFLEN];
	regerror(rc, re->re, errbuf, ERRBUFFLEN);
#endif /* HAVE_PCRE2_H */

	prt_error("Error: %s: \"%s\" (%s", msg, re->pattern, re->name);
	if (-1 != erroffset) prt_error(" at %d", erroffset);
	prt_error("): %s (%d)\n", errbuf, rc);
}

/**
 * Compile all the given regexs.
 * Return 0 on success, else an error code.
 */
int compile_regexs(Regex_node *rn, Dictionary dict)
{
	while (rn != NULL)
	{
		/* If rn->re non-null, assume compiled already. */
		if(rn->re == NULL)
		{
			int rc;
			regex_t *re = rn->re = malloc(sizeof(regex_t));

#if HAVE_PCRE2_H
			PCRE2_SIZE erroffset;
			re->re_code =
				pcre2_compile((PCRE2_SPTR)rn->pattern, PCRE2_ZERO_TERMINATED,
				              PCRE2_UTF|PCRE2_UCP, &rc, &erroffset, NULL);
			if (NULL != re->re_code)
			{
				rc = 0;
				re->re_md = pcre2_match_data_create(0, NULL);
				if (NULL == re->re_md) return -1; /* Unhandled for now. */
			}
#else
			const int erroffset = -1;

			/* REG_ENHANCED is needed for macOS to support \w etc. */
#ifndef REG_ENHANCED
#define REG_ENHANCED 0
#endif
			rc = regcomp(re, rn->pattern, REG_NOSUB|REG_EXTENDED|REG_ENHANCED);
#endif

			if (rc)
			{
				prt_regerror("Failed to compile regex", rn, rc ,erroffset);
				rn->re = NULL;
				return rc;
			}

			/* Check that the regex name is defined in the dictionary. */
			if ((NULL != dict) && !dict_has_word(dict, rn->name))
			{
				/* TODO: better error handing. Maybe remove the regex? */
				prt_error("Error: Regex name %s not found in dictionary!\n",
				       rn->name);
			}
		}
		rn = rn->next;
	}
	return 0;
}

/**
 * Try to match each regex in turn to word s.
 * On match, return the name of the first matching regex.
 * If no match is found, return NULL.
 */
#define D_MRE 6
const char *match_regex(const Regex_node *rn, const char *s)
{
	while (rn != NULL)
	{
		int rc;
		bool nomatch;
		bool match;
		regex_t *re = rn->re;

		/* Make sure the regex has been compiled. */
		assert(re);

#if HAVE_PCRE2_H
		rc = pcre2_match(re->re_code, (PCRE2_SPTR)s,
		                 PCRE2_ZERO_TERMINATED, /*startoffset*/0,
		                 PCRE2_NO_UTF_CHECK, re->re_md, NULL);
		match = (rc >= 0);
		nomatch = (rc == PCRE2_ERROR_NOMATCH);
#else
		rc = regexec(rn->re, s, 0, NULL, /*eflags*/0);
		match = (rc == 0);
		nomatch = (rc == REG_NOMATCH);
#endif
		if (match)
		{
			lgdebug(+D_MRE, "%s%s %s\n", &"!"[!rn->neg], rn->name, s);
			if (!rn->neg)
				return rn->name; /* Match found - return--no multiple matches. */

			/* Negative match - skip this regex name. */
			for (const char *nre_name = rn->name; rn->next != NULL; rn = rn->next)
			{
				if (strcmp(nre_name, rn->next->name) != 0) break;
			}
		}
		else if (!nomatch)
		{
			/* We have an error. */
			prt_regerror("Regex matching error", rn, rc, -1);
		}

		rn = rn->next;
	}
	return NULL; /* No matches. */
}
#undef D_MRE

/**
 * Delete associated storage
 */
void free_regexs(Regex_node *rn)
{
	while (rn != NULL)
	{
		Regex_node *next = rn->next;
		regex_t *re = rn->re;

		/* Prevent a crash in regfree() in case of a regex compilation error. */
		if (NULL != re)
		{
#if HAVE_PCRE2_H
			pcre2_match_data_free(re->re_md);
			pcre2_code_free(re->re_code);
#else
			regfree(re);
#endif
		}
		free(re);
		free(rn->name);
		free(rn->pattern);
		free(rn);
		rn = next;
	}
}
