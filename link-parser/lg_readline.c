/***************************************************************************/
/* Copyright (c) 2012 Linas Vepstas                                        */
/* All rights reserved                                                     */
/*                                                                         */
/* Use of the link grammar parsing system is subject to the terms of the   */
/* license set forth in the LICENSE file included with this software,      */
/* and also available at http://www.link.cs.cmu.edu/link/license.html      */
/* This license allows free redistribution and use in source and binary    */
/* forms, with or without modification, subject to certain conditions.     */
/*                                                                         */
/***************************************************************************/

#include "lg_readline.h"

#ifdef HAVE_EDITLINE
#ifdef EL_PROMPT_ESC

#include <histedit.h>
#include <stdlib.h>

/**
 * Arghhhh  readline() is not utf8-capable.  So hack one up, using the
 * wide-char interfaces.  This is a hack. Argh. 
 */

static wchar_t * wc_prompt = NULL;
static wchar_t * prompt(EditLine *el)
{
	return wc_prompt;
}


char *lg_readline(const char *mb_prompt)
{
	static int is_init = FALSE;
	static HistoryW *hist = NULL;
	static HistEventW ev;
	static EditLine *el = NULL;

	int numc;
	size_t byte_len;
	const wchar_t *wc_line;
	char *mb_line;

	if (!is_init)
	{
		size_t sz;
#define HFILE ".lg_history"
		is_init = TRUE;

		sz = mbstowcs(NULL, mb_prompt, 0) + 4;
		wc_prompt = malloc (sz*sizeof(wchar_t));
		mbstowcs(wc_prompt, mb_prompt, sz);

		hist = history_winit();    /* Init built-in history     */
		history_w(hist, &ev, H_SETSIZE, 20);  /* Remember 20 events       */
		history_w(hist, &ev, H_LOAD, HFILE);
		el = el_init("link-parser", stdin, stdout, stderr);
		el_wset(el, EL_HIST, history_w, hist);
		el_wset(el, EL_PROMPT_ESC, prompt, '\1'); /* Set the prompt function */
		el_source(el, NULL); /* Source the user's defaults file. */
	}

	wc_line = el_wgets(el, &numc);

	/* Received end-of-file */
	if (0 == numc) return NULL;

	history_w(hist, &ev, H_ENTER, wc_line);
	history_w(hist, &ev, H_SAVE, HFILE);
	/* fwprintf(stderr, L"==> got %d %ls", numc, wc_line); */

	byte_len = wcstombs(NULL, wc_line, 0) + 4;
	mb_line = malloc(byte_len);
	wcstombs(mb_line, wc_line, byte_len);
	
	return mb_line;
}

#else /* EL_PROMPT_ESC */
#endif /* EL_PROMPT_ESC*/
#endif /* HAVE_EDITLINE */
