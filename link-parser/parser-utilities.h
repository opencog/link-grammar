/***************************************************************************/
/* Copyright (c) 2016 Linas Vepstas                                        */
/* All rights reserved                                                     */
/*                                                                         */
/* Use of the link grammar parsing system is subject to the terms of the   */
/* license set forth in the LICENSE file included with this software.      */
/* This license allows free redistribution and use in source and binary    */
/* forms, with or without modification, subject to certain conditions.     */
/*                                                                         */
/***************************************************************************/

#ifndef _PARSER_UTILITIES_
#define _PARSER_UTILITIES_

#if !defined(MIN)
#define MIN(X,Y)  ( ((X) < (Y)) ? (X) : (Y))
#endif

#ifndef HAVE_EDITLINE

/* This if/endif block is a copy from link-grammar/utilities.h. */
#if __APPLE__
/* It appears that fgetc on Mac OS 10.11.3 "El Capitan" has a weird
 * or broken version of fgetc() that flubs reads of utf8 chars when
 * the locale is not set to "C" -- in particular, it fails for the
 * en_US.utf8 locale; see bug report #293
 * https://github.com/opencog/link-grammar/issues/293
 */
static inline int lg_fgetc(FILE *stream)
{
	char c[4];  /* general overflow paranoia */
	size_t nr = fread(c, 1, 1, stream);
	if (0 == nr) return EOF;
	return (int) c[0];
}

static inline int lg_ungetc(int c, FILE *stream)
{
	/* This should work, because we never unget past the newline char. */
	int rc = fseek(stream, -1, SEEK_CUR);
	if (rc) return EOF;
	return c;
}

#else
#define lg_fgetc   fgetc
#define lg_ungetc  ungetc
#endif

#endif // HAVE_EDITLINE

#ifdef _MSC_VER
#define strcasecmp _stricmp
#define strncasecmp(a,b,s) _strnicmp((a),(b),(s))
#endif

#endif // _PARSER_UTILITIES_
