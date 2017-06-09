/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2013 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>

#include "print-util.h"
#include "utilities.h"
#include "wcwidth.h"

/**
 * Return the width, in text-column-widths, of the utf8-encoded
 * string.  This is needed when printing formatted strings.
 * European languages will typically have widths equal to the
 * `mblen` value below (returned by mbsrtowcs); they occupy one
 * column-width per code-point.  The CJK ideographs occupy two
 * column-widths per code-point. No clue about what happens for
 * Arabic, or others.  See wcwidth.c for details.
 */
size_t utf8_strwidth(const char *s)
{
	mbstate_t mbss;
	wchar_t ws[MAX_LINE];
	size_t mblen, glyph_width=0, i;

	memset(&mbss, 0, sizeof(mbss));

#ifdef _WIN32
	mblen = MultiByteToWideChar(CP_UTF8, 0, s, -1, ws, MAX_LINE) - 1;
#else
	mblen = mbsrtowcs(ws, &s, MAX_LINE, &mbss);
#endif /* _WIN32 */

	for (i=0; i<mblen; i++)
	{
		glyph_width += mk_wcwidth(ws[i]);
	}
	return glyph_width;
}

/* ============================================================= */

/* Note: As in the rest of the LG library, we assume here C99 compliance. */
void vappend_string(dyn_str * string, const char *fmt, va_list args)
{
#define TMPLEN 1024 /* Big enough for a possible error message, see below */
	char temp_buffer[TMPLEN];
	char *temp_string = temp_buffer;
	size_t templen;
	va_list copy_args;

	va_copy(copy_args, args);
	templen = vsnprintf(temp_string, TMPLEN, fmt, copy_args);
	va_end(copy_args);

	if ((int)templen < 0) goto error;
	// if (fmt[0] == '(') { errno=2; goto error;} /* Test the error reporting. */

	if (templen >= TMPLEN)
	{
		/* TMPLEN is too small - use a bigger buffer. Couldn't actually
		 * find any example of entering this code with TMPLEN=1024... */
		temp_string = alloca(templen+1);
		templen = vsnprintf(temp_string, templen+1, fmt, args);
		if ((int)templen < 0) goto error;
	}
	va_end(args);

	dyn_strcat(string, temp_string);
	return;

error:
	{
		/* Some error has occurred */
		const char msg[] = "[vappend_string(): ";
		strcpy(temp_buffer, msg);
		strerror_r(errno, temp_buffer+sizeof(msg)-1, TMPLEN-sizeof(msg));
		strcat(temp_buffer, "]");
		dyn_strcat(string, temp_string);
		return;
	}
}

void append_string(dyn_str * string, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	vappend_string(string, fmt, args);
}

size_t append_utf8_char(dyn_str * string, const char * mbs)
{
	/* Copy exactly one multi-byte character to buf */
	char buf[10];
	size_t n = utf8_next(mbs);

	assert(n<10, "Multi-byte character is too long!");
	strncpy(buf, mbs, n);
	buf[n] = 0;
	dyn_strcat(string, buf);
	return n;
}
