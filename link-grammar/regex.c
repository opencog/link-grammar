/*****************************************************************************/
/* Copyright (c) 2005                                                        */
/* Sampo Pyysalo                                                             */
/* All rights reserved                                                       */
/*                                                                           */
/* Use of the link grammar parsing system is subject to the terms of the     */
/* license set forth in the LICENSE file included with this software,        */
/* and also available at http://www.link.cs.cmu.edu/link/license.html        */
/* This license allows free redistribution and use in source and binary      */
/* forms, with or without modification, subject to certain conditions.       */
/*                                                                           */
/*****************************************************************************/

#include "link-includes.h"

/* Support for the regular-expression based token matching system
   using the PCRE regex library. If WITH_PCRE_REGEX is defined, the
   functions compile_regexs() and match_regex() interface with PCRE to
   prepare and match regular expressions.  If WITH_PCRE_REGEX is not
   defined, dummy versions of the functions are provided instead.
*/

#ifdef WITH_PCRE_REGEX

#include "../pcre-5.0/pcre.h"

/* Size of output vector for pcre_match. Must be a multiple of 3. */
#define PCRE_OVEC_SIZE 30

/* Compiles all the regexs in the Dictionary. Returns 1 one success,
 * 0 on failure.
 */
int compile_regexs(Dictionary dict) {
  const char *error;
  int erroroffset;

  Regex_node *re = dict->regex_root;
  while(re != NULL) {
    /* If re->re non-null, assume compiled already. */
    if(re->re == NULL) {
      /* Compile with default options (0) and default character
       * tables (NULL). */
      re->re = pcre_compile(re->pattern, 0, &error, &erroroffset, NULL);
      if(re->re == NULL) {
	lperror(DICTPARSE, ": Failed to compile regex '%s' (%s) at %d: %s\n",
		re->pattern, re->name, erroroffset, error);
	return 0;
      }
      /* Check that the regex name is defined in the dictionary. */
      if(!boolean_dictionary_lookup(dict, re->name)) {
	/* TODO: better error handing. Maybe remove the regex? */
	fprintf(stderr, "Regex name %s not found in dictionary!\n", re->name);
      }
    }
    re = re->next;
  }
  return 1;
}

/* Tries to match each regex in turn to word s.
 * On match, returns the name of the first matching regex.
 * If no match is found, returns NULL.
 */
char *match_regex(Dictionary dict, char *s) {
  int ovector[PCRE_OVEC_SIZE];

  Regex_node *re = dict->regex_root;
  while(re != NULL) {
    if(re->re == NULL) {
      /* Re not compiled; if this happens, it's likely an internal error, 
	 but nevermind for now.
      */
      continue;
    }
    /* Try to match with no extra data (NULL), whole str (0 to strlen(s)),
     * and default options (second 0). */
    int rc = pcre_exec(re->re, NULL, s, strlen(s), 0,
		       0, ovector, PCRE_OVEC_SIZE);

    if(rc > 0) {
      return re->name; /* match found. just return--no multiple matches. */
    } 
    else if(rc != PCRE_ERROR_NOMATCH) {
      /* We have an error. TODO: more appropriate error handling.*/
      fprintf(stderr,"Regex matching error %d occurred!\n", rc);
    }
    re = re->next;
  }
  return NULL; /* no matches. */
}

#else

/* if PCRE is not available, dummy functions provided instead. */
int compile_regexs(Dictionary dict) { return 1; }
char *match_regex(Dictionary dict, char *s) { return NULL; }

#endif
