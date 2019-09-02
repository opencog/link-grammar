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

#include <limits.h>
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
	size_t mblen;

#ifdef _WIN32
	mblen = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0) - 1;
#else
	mblen = mbsrtowcs(NULL, &s, 0, NULL);
#endif
	if ((int)mblen < 0)
	{
		prt_error("Warning: Error in utf8_strwidth(%s)\n", s);
		return 1 /* XXX */;
	}

	wchar_t *ws = alloca((mblen + 1) * sizeof(wchar_t));

#ifdef _WIN32
	MultiByteToWideChar(CP_UTF8, 0, s, -1, ws, mblen);
#else
	mbstate_t mbss;
	memset(&mbss, 0, sizeof(mbss));
	mbsrtowcs(ws, &s, mblen, &mbss);
#endif /* _WIN32 */

	int glyph_width = 0;
	for (size_t i = 0; i < mblen; i++)
	{
		int w = mk_wcwidth(ws[i]);

		// If w<0 then we do not know what the correct glyph
		// width should be for this codepoint. Many terminals
		// will print this with a weird boxed font that is two
		// columns wide, showing the hex value in it.
		if (w < 0) w = 2;
		glyph_width += w;
	}
	return glyph_width;
}

/**
 * Return the width, in text-column-widths, of the utf8-encoded
 * character. The will return a NEGATIVE VALUE if the character
 * width is not know (no glyph for a valid UTF-8 codepoint) or
 * if the input is not a valid UTF-8 character!
 *
 * The mbstate_t argument is not used, since we convert only from utf-8.
 * FIXME: This function (along with other places that use mbrtowc()) need
 * to be fixed for Windows (utf-16 wchar_t).
 * Use char32_t with mbrtoc32() instead of mbrtowc().
 */
int utf8_charwidth(const char *s)
{
	wchar_t wc;

	int n = mbrtowc(&wc, s, MB_LEN_MAX, NULL);
	if (n == 0) return 0;
	if (n < 0)
	{
		// prt_error("Error: charwidth(%s): mbrtowc() returned %d\n", s, n);
		return -2 /* Yes, we want this! It signals the error! */;
	}

	return mk_wcwidth(wc);
}

/**
 * Return the number of characters in the longest initial substring
 * which has a text-column-width of not greater than max_width.
 */
size_t utf8_chars_in_width(const char *s, size_t max_width)
{
	size_t total_bytes = 0;
	size_t glyph_width = 0;
	int n = 0;
	wchar_t wc;

	do
	{
		total_bytes += n;
		n = mbrtowc(&wc, s+total_bytes, MB_LEN_MAX, NULL);
		if (n == 0) break;
		if (n < 0)
		{
			// Allow for double-column-wide box-font printing
			// i.e. box with the hex code inside.
			glyph_width += 2;
			n = 1;
		}
		else
		{
			// If we are here, it was a valid UTF-8 code point,
			// but we do not know the width of the corresponding
			// glyph. Just like above, assume a double-wide box
			// font will be printed.
			int gw = mk_wcwidth(wc);
			if (0 <= gw)
				glyph_width += gw;
			else
				glyph_width += 2;
		}
		//printf("N %zu G %zu;", total_bytes, glyph_width);
	}
	while (glyph_width <= max_width);
	// printf("\n");

	return total_bytes;
}

/* ============================================================= */

/**
 * Append to a dynamic string with vprintf-like formatting.
 * @return The number of appended bytes, or a negative value on error.
 *
 * Note: As in the rest of the LG library, we assume here C99 library
 * compliance (without it, this code would be buggy).
 */
int vappend_string(dyn_str * string, const char *fmt, va_list args)
{
#define TMPLEN 1024 /* Big enough for a possible error message, see below */
	char temp_buffer[TMPLEN];
	char *temp_string = temp_buffer;
	int templen;
	va_list copy_args;

	va_copy(copy_args, args);
	templen = vsnprintf(temp_string, TMPLEN, fmt, copy_args);
	va_end(copy_args);

	if (templen < 0) goto error;
	if (0)
	{
		if (fmt[0] == '(') { errno=2; goto error;} /* Test the error reporting. */
	}

	if (templen >= TMPLEN)
	{
		/* TMPLEN is too small - use a bigger buffer. This may happen
		 * when printing dictionary words using !! with a wildcard
		 * or when debug-printing all the connectors. */
		temp_string = malloc(templen+1);
		templen = vsnprintf(temp_string, templen+1, fmt, args);
		if (templen < 0)
		{
			free(temp_string);
			goto error;
		}
	}
	va_end(args);

	patch_subscript_marks(temp_string);
	dyn_strcat(string, temp_string);
	if (templen >= TMPLEN) free(temp_string);
	return templen;

error:
	{
		/* Some error has occurred */
		const char msg[] = "[vappend_string(): ";
		strcpy(temp_buffer, msg);
		lg_strerror(errno, temp_buffer+sizeof(msg)-1, TMPLEN-sizeof(msg));
		strcat(temp_buffer, "]");
		dyn_strcat(string, temp_buffer);
		va_end(args);
		return templen;
	}
}

/**
 * Append to a dynamic string with printf-like formatting.
 * @return The number of appended bytes, or a negative value on error.
 */
int append_string(dyn_str * string, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	return vappend_string(string, fmt, args);
}

/**
 * Append exactly one UTF-8 character to the string.
 * Return the number of bytes to advance, until the
 * next UTF-8 character. This might NOT be the same as
 * number of bytes actually appended. Two things might
 * happen:
 * a) Invalid UTF-8 values are copied, but only one byte,
 *    followed by an additional blank.
 * b) Valid UTF-8 code-points that do not have a known
 *    glyph are copied, followed by an additional blank.
 * This additional blanks allows proper printing of these
 * two cases, by allowing the terminal to display with
 * "box fonts" - boxes containing hex code, usually two
 * column-widths wide.
 */
size_t append_utf8_char(dyn_str * string, const char * mbs)
{
	/* Copy exactly one multi-byte character to buf */
	char buf[12];
	assert('\0' != *mbs);
	int nb = utf8_charlen(mbs);
	int n = nb;
	if (n < 0) n = 1; // charlen is negative if its not a valid UTF-8

	assert((size_t)n<sizeof(buf), "Multi-byte character is too long!");
	memcpy(buf, mbs, n);

	// Whitepsace pad if its a bad value
	if (nb < 0) { buf[n] = ' '; n++; }

	// Whitespace pad if not a known UTF-8 glyph.
	if (0 < nb && utf8_charwidth(mbs) < 0) { buf[n] = ' '; n++; }

	// Null terminate.
	buf[n] = 0;
	dyn_strcat(string, buf);

	// How many bytes did we hoover in?
	n = nb;
	if (n < 0) n = 1; // advance exactly one byte, if invalid UTF-8
	return n;
}
