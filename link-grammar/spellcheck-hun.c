/*************************************************************************/
/* Copyright (c) 2009 Linas Vepstas                                      */
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
#include "link-includes.h"
#include "spellcheck-hun.h"

#ifdef HAVE_HUNSPELL

#include <hunspell.h>
#include <string.h>

void * spellcheck_create(const char * lang)
{
	Hunhandle *h = NULL;
	/* XXX We desperately need something better than this! */
	if (0 == strcmp(lang, "en"))
	{
		h = Hunspell_create("/usr/share/myspell/dicts/en-US.aff",
		                    "/usr/share/myspell/dicts/en-US.dic");
	}

	return h;
}

void spellcheck_destroy(void * chk)
{
	Hunhandle *h = (Hunhandle *) chk;
	Hunspell_destroy(h);
}

/**
 * Return boolean: 1 if spelling looks good, else zero
 */
int spellcheck_test(void * chk, const char * word)
{
	if (NULL == chk)
	{
		prt_error("Error: no spell-check handle specified!\n");
		return 0;
	}

	return Hunspell_spell((Hunhandle *)chk, word);
}

int spellcheck_suggest(void * chk, char ***sug, const char * word)
{
	if (NULL == chk)
	{
		prt_error("Error: no spell-check handle specified!\n");
		return 0;
	}

	return Hunspell_suggest((Hunhandle *)chk, sug, word);
}

#else
void * spellcheck_create(const char * lang) { return NULL; }
void spellcheck_destroy(void * chk) {}
int spellcheck_test(void * chk, const char * word) { return 0; }
int spellcheck_suggest(void * chk, char ***sug, const char * word) { return 0; }

#endif /* #ifdef HAVE_HUNSPELL */
