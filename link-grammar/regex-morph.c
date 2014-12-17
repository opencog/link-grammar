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

/* On MS Windows, regex.h fails to pull in size_t, so work around this by
 * including <stddef.h> before <regex.h> (<sys/types.h> is not enough) */
#include <stddef.h>
#include <regex.h>
#include "api-structures.h"
#include "dict-api.h"
#include "link-includes.h"
#include "regex-morph.h"
#include "structures.h"

/**
 * Support for the regular-expression based token matching system
 * using standard POSIX regex.
 */

/**
 * Notify an error message according to the error code.
 */
static void prt_regerror(const char *msg, const Regex_node *re, int rc)
{
	const size_t errbuf_size = regerror(rc, re->re, NULL, 0);
	char * const errbuf = malloc(errbuf_size);

	/*
	prt_error("Error: Failed to compile regex '%s' (%s) at %d: %s\n",
					re->pattern, re->name, erroroffset, error);
	*/
	regerror(rc, re->re, errbuf, errbuf_size);
	prt_error("Error: %s: \"%s\" (%s): %s", msg, re->pattern, re->name, errbuf);
	free(errbuf);
}

/**
 * Compiles all the given regexs. Returns 0 on success,
 * else an error code.
 */
int compile_regexs(Regex_node *re, Dictionary dict)
{
	regex_t *preg;
	int rc;

	while (re != NULL)
	{
		/* If re->re non-null, assume compiled already. */
		if(re->re == NULL)
		{
			/* Compile with default options (0) and default character
			 * tables (NULL). */
			/* re->re = pcre_compile(re->pattern, 0, &error, &erroroffset, NULL); */
			preg = (regex_t *) malloc (sizeof(regex_t));
			re->re = preg;
			rc = regcomp(preg, re->pattern, REG_EXTENDED);
			if (rc)
			{
				prt_regerror("Failed to compile regex", re, rc);
				return rc;
			}

			/* Check that the regex name is defined in the dictionary. */
			if ((NULL != dict) && !boolean_dictionary_lookup(dict, re->name))
			{
				/* TODO: better error handing. Maybe remove the regex? */
				prt_error("Error: Regex name %s not found in dictionary!\n",
				       re->name);
			}
		}
		re = re->next;
	}
	return 0;
}

/**
 * Tries to match each regex in turn to word s.
 * On match, returns the name of the first matching regex.
 * If no match is found, returns NULL.
 */
const char *match_regex(const Regex_node *re, const char *s)
{
	int rc;

	while (re != NULL)
	{
		if (re->re == NULL)
		{
			/* Re not compiled; if this happens, it's likely an
			 *  internal error, but nevermind for now.  */
			continue;
		}
		/* Try to match with no extra data (NULL), whole str (0 to strlen(s)),
		 * and default options (second 0). */
		/* int rc = pcre_exec(re->re, NULL, s, strlen(s), 0,
		 *                    0, ovector, PCRE_OVEC_SIZE); */

		rc = regexec((regex_t*) re->re, s, 0, NULL, 0);
		if (0 == rc)
		{
			return re->name; /* match found. just return--no multiple matches. */
		}
		else if (rc != REG_NOMATCH)
		{
			/* We have an error. TODO: more appropriate error handling.*/
			prt_regerror("Regex matching error", re, rc);
		}
		re = re->next;
	}
	return NULL; /* no matches. */
}

/**
 * Delete associated storage
 */
void free_regexs(Regex_node *re)
{
	while (re != NULL)
	{
		Regex_node *next = re->next;
		regfree((regex_t *)re->re);
		free(re->re);
		free(re->name);
		free(re->pattern);
		free(re);
		re = next;
	}
}
