/***************************************************************************/
/* Copyright (c) 2016 Amir Plivatsky                                       */
/* Copyright (c) 2016 Linas Vepstas                                        */
/* All rights reserved                                                     */
/*                                                                         */
/* Use of the link grammar parsing system is subject to the terms of the   */
/* license set forth in the LICENSE file included with this software.      */
/* This license allows free redistribution and use in source and binary    */
/* forms, with or without modification, subject to certain conditions.     */
/*                                                                         */
/***************************************************************************/
#include "parser-utilities.h"

#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#endif

#ifdef _WIN32
/**
 * Get a line from the console in UTF-8.
 * This function bypasses the code page conversion and reads Unicode
 * directly from the console.
 * @return An input line from the console in UTF-8 encoding.
 */
char *get_console_line(void)
{
	static HANDLE console_handle = NULL;
	static EOF_encountered = false;
	wchar_t winbuf[MAX_INPUT];
	/* Worst-case: 4 bytes per UTF-8 char, 1 UTF-8 char per wchar_t char. */
	static char utf8inbuf[MAX_INPUT*4+1];

	if (EOF_encountered) return NULL;

	if (NULL == console_handle)
	{
		console_handle = CreateFileA("CONIN$", GENERIC_READ, FILE_SHARE_READ,
		                             NULL, OPEN_EXISTING, 0, NULL);
		if (INVALID_HANDLE_VALUE == console_handle)
		{
			prt_error("CreateFileA CONIN$: Error %d\n", GetLastError());
			return NULL;
		}
	}

	int nchar;
	if (!ReadConsoleW(console_handle, &winbuf, MAX_INPUT-1, &nchar, NULL))
	{
		printf("ReadConsoleW: Error %d\n", GetLastError());
		return NULL;
	}
	winbuf[nchar] = L'\0';

	nchar = WideCharToMultiByte(CP_UTF8, 0, winbuf, -1, utf8inbuf,
	                            sizeof(utf8inbuf), NULL, NULL);
	if (0 == nchar)
	{
		prt_error("Error: WideCharToMultiByte CP_UTF8 failed: Error %d",
		          GetLastError());
		return NULL;
	}

	/* Make sure we don't have conversion problems by searching for '�'. */
	const char *invlid_char  = strstr(utf8inbuf, "\xEF\xBF\xBD");
	if (NULL != invlid_char)
	{
		prt_error("Warning: Invalid input character encountered");
	}
	/* ^Z (EOF mark) is a character here. Convert it to an EOF indication. */
	char *ctrl_z = strchr(utf8inbuf, '\x1A');
	if (NULL != ctrl_z)
	{
		if (utf8inbuf == ctrl_z)   /* ^z at line start. */
			return NULL;            /* Immediate EOF. */
		*ctrl_z = '\0';            /* The returned line is up to the ^Z. */
		EOF_encountered = true;    /* Delayed EOF - on next invocation. */
	}
	else
	{
		/* Note that nchar includes the terminating NUL. */
		if (3 <= nchar)                  /* Avoid a crash - just in case. */
			utf8inbuf[nchar-3] = '\0';    /* Discard \r\n */
	}

	return utf8inbuf;
}

static int console_input_cp;
static int console_output_cp;
static void restore_console_cp(void)
{
	SetConsoleCP(console_input_cp);
	SetConsoleOutputCP(console_output_cp);
}

static BOOL CtrlHandler(DWORD fdwCtrlType)
{
	if ((CTRL_C_EVENT == fdwCtrlType) || (CTRL_BREAK_EVENT  == fdwCtrlType))
	{
		fprintf(stderr, "Interrupt\n");
		restore_console_cp();
		exit(2);
	}
	return FALSE;
}

#include <locale.h>
/**
 * Set the output conversion attributes for transparency.
 * This way UTF-8 output doesn't pass any conversion.
 */
void win32_set_utf8_output(void)
{
	if (-1 == _setmode(fileno(stdout), _O_BINARY))
	{
		prt_error("Warning: _setmode(fileno(stdout), _O_BINARY): %s",
			strerror(errno));
	}

	console_input_cp = GetConsoleCP();
	console_output_cp = GetConsoleOutputCP();
	atexit(restore_console_cp);
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
	{
		prt_error("Warning: Cannot not set code page restore handler");
	}
	/* For file output. It is too late for output pipes.
	 * If output pipe is desired, one case set CP_UTF8 by the
	 * command "chcp 65001" before invoking link-parser. */
	if (!SetConsoleCP(CP_UTF8))
	{
		prt_error("Warning: Cannot set input codepage %d (error %d)",
			CP_UTF8, GetLastError());
	}
	/* For Console output. */
	if (!SetConsoleOutputCP(CP_UTF8))
	{
		prt_error("Warning: Cannot set output codepage %d (error %d)",
			CP_UTF8, GetLastError());
	}
}
#endif
