/***************************************************************************/
/* Copyright (c) 2012 Linas Vepstas                                        */
/* All rights reserved                                                     */
/*                                                                         */
/* Use of the link grammar parsing system is subject to the terms of the   */
/* license set forth in the LICENSE file included with this software.      */
/* This license allows free redistribution and use in source and binary    */
/* forms, with or without modification, subject to certain conditions.     */
/*                                                                         */
/***************************************************************************/

/**
 * Arghhhh. This hacks around multiple stupidities in readline/editline.
 * 1) most versions of editline don't have wide-char support.
 * 2) No versions of editline have UTF8 support.
 * So basically readline() is just plain broken.
 * So hack one up, using the wide-char interfaces.  This is a hack. Argh.
 *
 * Double-arghh.  Current versions of readline hang in an infinite loop
 * on __read_nocancel() in read_char() called from el_wgets() (line 92
 * below) when the input is "He said 《 This is bull shit 》" Notice
 * the unicode angle-brackets.
 */

#include "lg_readline.h"

#ifdef HAVE_EDITLINE
#include <string.h>
#include <histedit.h>
#include <stdlib.h>

#ifdef HAVE_WIDECHAR_EDITLINE
#include <stdbool.h>
#include <fcntl.h>                     // open flags
#if defined O_DIRECTORY && defined O_PATH
#include <unistd.h>                    // fchdir
#include <errno.h>
#endif

#include "command-line.h"
#include "parser-utilities.h"

extern Switch default_switches[];
static const Switch **sorted_names; /* sorted command names */
static wchar_t * wc_prompt = NULL;
static wchar_t * prompt(EditLine *el)
{
	return wc_prompt;
}

/**
 * Try to complete the wide string in \p input, whose length is \p len.
 *
 * @param input Non-NUL-terminated wide string with length \p len.
 * @param len Number of wide characters in \p input.
 * @param is_help \p input is the argument of an !help command.
 * @return 3 types of values:
 *  <code>""</code> There are no completions.
 *  <code>NULL</code> Input not supported, or choices help has been printed.
 *  A NUL-terminated byte string (to be used as a completion).
 */
static char *complete_command(const wchar_t *input, size_t len, bool is_help)
{
	const Switch **start = NULL;
	const Switch **end;
	const Switch **match;
	const char *prev;
	size_t addlen;
	bool is_assignment = false; /* marking for the help facility */

	if ((1 < len) && L'=' == input[len-1] && !is_help)
	{
		/* This is a variable assignment without a value.
		 * Arrange for displaying an help line (only - no completion). */
		is_assignment = true;
		len--; /* disregard the ending '=' to enable command search. */
	}

	/* Our commands are ASCII strings.
	 * So for simplicity, convert the input to an ASCII string. */
	char *astr = malloc(len+1);
	for (size_t i = 0; i < len; i++)
	{
		if (input[i] <= 0 || input[i] > 127)
		{
			free(astr);
			return NULL; /* unsupported input */
		}
		astr[i] = (char)input[i];
	}
	astr[len] = '\0';

	/* Find the possible completions. */
	for (match = sorted_names; NULL != *match; match++)
	{
		if (UNDOC[0] == (*match)->description[0]) continue;
		if (0 == strncmp(astr, (*match)->string, len))
		{
			if (NULL == start)
			{
				start = match;
				addlen = strlen((*match)->string) - len;
			}
			else
			{
				/* There is more than one match. Find the maximal common
				 * additional substring that we can append. */
				for (; addlen  > 0; addlen--)
				{
					if (0 == strncmp((*match)->string+len, prev+len, addlen))
					    break;
				}
			}
			prev = (*match)->string;
		}
		else if (NULL != start) break;
	}
	free(astr);
	end = match;

	if (NULL == start) return strdup(""); /* nothing found - no completions */
	int cnum = end - start;

	/* Show a possible completion list in a form of 1-line command help,
	 * for these cases:
	 * 1. Multiple completions with no common substring to add.
	 * 2. An assignment command.
	 */
	if (((cnum > 1) && (0 == addlen)) || is_assignment)
	{
		bool all_commands = true;

		printf("\n");
		if (is_assignment)
		{
			for (size_t i = 0; &start[i] < end; i++)
				if ((Cmd != start[i]->param_type)) all_commands = false;
		}
		do
		{
			if (UNDOC[0] == (*start)->description[0]) continue;
			if (is_assignment && all_commands)
			{
				printf("\"%s\" is not a user variable\n", (*start)->string);
			}
			else
			{
				if (!is_assignment || (Cmd != (*start)->param_type))
					display_1line_help(*start, /*is_completion*/true);
			}
		}
		while (++start < end);

		return NULL;
	}

	/* Here addlen>0, so we have 1 or more possible completions. */
	char *addstr = malloc(addlen + 2); /* + '=' + '\0' */
	strncpy(addstr, (*start)->string + len, addlen);

	if ((1 == cnum) && !is_help)
	{
		/* A single completion.  Indicate a success:
		 * - For a non-Boolean user variable appending '='.
		 * - For a command or a Boolean append ' '. */
		addstr[addlen++] =
			((Cmd != (*start)->param_type) && (Bool != (*start)->param_type))
			? '=' : ' ';
	}
	else
	{
		/* Multiple completions. The maximal common substring will be returned. */
	}

	addstr[addlen] = '\0';
	return addstr;
}

static int by_byteorder(const void *a, const void *b)
{
	const Switch * const *sa = a;
	const Switch * const *sb = b;

	return strcmp((*sa)->string, (*sb)->string);
}

static void build_command_list(const Switch ds[])
{
	size_t cl_num = 0;
	for (size_t i = 0; NULL != ds[i].string; i++)
	{
		if (UNDOC[0] == ds[i].description[0]) continue;
		cl_num++;
	}
	sorted_names = malloc((cl_num+1) * sizeof(*sorted_names));

	int j = 0;
	for (size_t i = 0; NULL != ds[i].string; i++)
	{
		if (UNDOC[0] == ds[i].description[0]) continue;
		sorted_names[j++] = &ds[i];
	}
	sorted_names[cl_num] = NULL;
	qsort(sorted_names, cl_num, sizeof(*sorted_names), by_byteorder);
}

/**
 * Complete variables / commands and also file name after "!file ".
 * Filenames with blanks are not supported well.
 * This is partially a problem of editline, so there is no point to
 * fix that.
 *
 * There is a special treatment of the !help command since it can be
 * abbreviated up to one character and its argument position also
 * needs a command completion (which is slightly different - no '=' is
 * appended to variables).
 */
static unsigned char lg_complete(EditLine *el, int ch)
{
	const LineInfoW *li = el_wline(el);
	const wchar_t *word_start; /* the word to be completed */
	const wchar_t *word_end;
	size_t word_len;
	const wchar_t *ctemp;
	unsigned char rc;
	bool is_file_command = false;
	bool is_help_command = false;

	/* A command must start with '!' on column 0. */
	if (L'!' != li->buffer[0])
		return CC_ERROR;

	/* Allow for whitespace after the initial "!" */
	for (ctemp = li->buffer+1; ctemp < li->lastchar; ctemp++)
		if (!iswspace(*ctemp)) break;

	word_start = ctemp;

	/* Find the word (if any) end. */
	for (ctemp = word_start; ctemp < li->lastchar; ctemp++)
		if (iswspace(*ctemp)) break;

	word_end = ctemp;
	word_len = word_end - word_start;

	if (0 < word_len)
	{
		/* Don't try to complete in a middle of word. */
		if ((li->cursor >= word_start) && ((li->cursor < word_end)))
			return CC_ERROR;

		/* Now check if it is !help or !file (or their abbreviation), as
		 * these commands need an argument completion. Whitespace is
		 * needed after these commands as else this is a command completion. */
		if (iswspace(*word_end))
		{
			if (0 == wcsncmp(word_start, L"help", word_len))
				is_help_command = true;
			else if (0 == wcsncmp(word_start, L"file", word_len))
				is_file_command = true;

			if (is_file_command || is_help_command)
			{
				/* Check if the command has an argument. */
				for (ctemp = word_end + 1; ctemp < li->lastchar; ctemp++)
					if (!iswspace(*ctemp)) break;
				word_start = ctemp;
				for (ctemp = word_start; ctemp < li->lastchar; ctemp++)
					if (iswspace(*ctemp)) break;
				word_end = ctemp;
				word_len = word_end - word_start;
			}
		}
	}

	/* Cannot complete if the cursor is not directly after the word. */
	if ((0 < word_len) && li->cursor != word_end) return CC_ERROR;

	/* Cannot complete if there is non-whitespace after the cursor. */
	for (ctemp = word_end; ctemp < li->lastchar; ctemp++)
		if (!iswspace(*ctemp)) return CC_ERROR;

	if (is_file_command)
	{
		/* The code before _el_fn_complete() supports printing the
		 * completion possibilities without a directory name prefix, as
		 * normally displayed by most programs. The code after
		 * _el_fn_complete() restores its temporary changes. */
#if defined O_DIRECTORY && defined O_PATH
		wchar_t *dn_end = wcsrchr(word_start, L'/');
		int cwdfd = -1;

		if (dn_end != NULL)
		{
			cwdfd = open(".", O_DIRECTORY|O_PATH);
			if (cwdfd == -1)
			{
				printf("Error: Open current directory: %s\n", strerror(errno));
			}
			else
			{
				wchar_t *wdirname = wcsdup(word_start);
				wdirname[dn_end - word_start] = L'\0';

				/* Original buffer is const, but never mind. Restored below. */
				*dn_end = L' '; /* Not saved - supposing it was L'/' */

				size_t byte_len = wcstombs(NULL, wdirname, 0);
				if (byte_len == (size_t)-1)
				{
					printf("Error: Unable to process non-ASCII in directory name.\n");
					free(wdirname);
					return CC_ERROR;
				}
				char *dirname = malloc(byte_len + 1);
				wcstombs(dirname, wdirname, byte_len + 1);
				free(wdirname);
				char *eh_dirname = expand_homedir(dirname);
				free(dirname);
				int chdir_status = chdir(eh_dirname);
				free(eh_dirname);
				if (chdir_status == -1)
				{
					*dn_end = L'/';
				}
			}
		}
#endif

		rc = _el_fn_complete(el, ch);

#if defined O_DIRECTORY && defined O_PATH
		if (cwdfd != -1)
		{
			if (fchdir(cwdfd) < 0)
			{
				/* This shouldn't happen, unless maybe the directory to which
				 * cwdfd reveres becomes unreadable after cwdfd is created. */
				printf("\nfchdir(): Cannot change directory back: %s\n",
				       strerror(errno));
			}
			close(cwdfd);
			*dn_end = L'/';
		}
#endif

		/* CC_NORM is returned if there is no possible completion. */
		return (rc == CC_NORM) ? CC_ERROR : rc;
	}

	char *completion = complete_command(word_start, word_len, is_help_command);
	if (NULL == completion)
		rc = CC_REDISPLAY; /* completions got printed - redraw the input line */
	else if ('\0' == completion[0])
		rc =  CC_NORM;     /* nothing to complete */
	else if (el_insertstr(el, completion) == -1)
		rc = CC_ERROR;     /* no more space in the line buffer */
	else
		rc = CC_REFRESH;   /* there is a completion */

	free(completion);
	return (rc == CC_NORM) ? CC_ERROR : rc;
}

char *lg_readline(const char *mb_prompt)
{
	static bool is_init = false;
	static HistoryW *hist = NULL;
	static HistEventW ev;
	static EditLine *el = NULL;
	static char *mb_line;

	size_t byte_len;
	const wchar_t *wc_line;
	char *nl;

	if (!is_init)
	{
		size_t sz;
#define HFILE ".lg_history"
		is_init = true;

		sz = mbstowcs(NULL, mb_prompt, 0) + 4;
		wc_prompt = malloc (sz*sizeof(wchar_t));
		mbstowcs(wc_prompt, mb_prompt, sz);

		hist = history_winit();    /* Init built-in history */
		el = el_init("link-parser", stdin, stdout, stderr);
		history_w(hist, &ev, H_SETSIZE, 100);
		history_w(hist, &ev, H_SETUNIQUE, 1);
		el_wset(el, EL_HIST, history_w, hist);
		history_w(hist, &ev, H_LOAD, HFILE);

		el_set(el, EL_SIGNAL, 1); /* Restore tty setting on returning to shell */

		/* By default, it comes up in vi mode, with the editor not in
		 * insert mode; and even when in insert mode, it drops back to
		 * command mode at the drop of a hat. Totally confusing/lame. */
		el_set(el, EL_EDITOR, "emacs");
		el_wset(el, EL_PROMPT, prompt); /* Set the prompt function */

		el_set(el, EL_ADDFN, "lg_complete", "command completion", lg_complete);
		el_set(el, EL_BIND, "^I", "lg_complete", NULL);

		build_command_list(default_switches);

		el_source(el, NULL); /* Source the user's defaults file. */
	}

	int numc = 1; /*  Uninitialized at libedit. */
	wc_line = el_wgets(el, &numc);

	/* Received end-of-file */
	if (numc <= 0)
	{
		el_end(el);
		history_wend(hist);
		free(wc_prompt);
		wc_prompt = NULL;
		hist = NULL;
		el = NULL;
		is_init = false;
		return NULL;
	}

	if (1 < numc)
	{
		history_w(hist, &ev, H_ENTER, wc_line);
		history_w(hist, &ev, H_SAVE, HFILE);
	}
	/* fwprintf(stderr, L"==> got %d %ls", numc, wc_line); */

	byte_len = wcstombs(NULL, wc_line, 0) + 4;
	free(mb_line);
	if (byte_len == (size_t)-1)
	{
		printf("Error: Unable to process UTF8 in input string.\n");
		mb_line = strdup(""); /* Just ignore it. */
		return mb_line;
	}
	mb_line = malloc(byte_len);
	wcstombs(mb_line, wc_line, byte_len);

	/* In order to be compatible with regular libedit, we have to
	 * strip away the trailing newline, if any. */
	nl = strchr(mb_line, '\n');
	if (nl) *nl = 0x0;

	return mb_line;
}

#else /* HAVE_WIDECHAR_EDITLINE */

#include <editline/readline.h>

char *lg_readline(const char *prompt)
{
	static char *pline;

	free(pline);
	pline = readline(prompt);

	/* Save non-blank lines */
	if (pline && *pline)
		add_history(pline);

	return pline;
}
#endif /* HAVE_WIDECHAR_EDITLINE */
#endif /* HAVE_EDITLINE */
