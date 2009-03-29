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

#ifdef HAVE_HUNSPELL

#include <hunspell.h>
#include <stdio.h>

void * spellcheck_create(void)
{
	Hunhandle *h;
	h = Hunspell_create("/usr/share/myspell/dicts/en-US.aff",
	                    "/usr/share/myspell/dicts/en-US.dic");

	return h;
}

void spellcheck_destroy(void * chk)
{
	Hunhandle *h = chk;
	Hunspell_destroy(h);
}

#else
void * spellcheck_create(void)
{
	return NULL;
}

void spellcheck_destroy(void * chk) {}

#endif /* #ifdef HAVE_HUNSPELL */
