/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */ 
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/
#ifndef _LINK_GRAMMAR_UTILITIES_H_
#define _LINK_GRAMMAR_UTILITIES_H_

#ifdef __CYGWIN__
#define _WIN32 1
#endif /* __CYGWIN__ */

#ifndef _WIN32
#include <langinfo.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __CYGWIN__
/* I was told that cygwin does not have these files. */ 
#include <wchar.h>
#include <wctype.h>
#endif

#if defined(__CYGWIN__) && defined(__MINGW32__)
/* Some users have CygWin and MinGW installed! 
 * In this case, use the MinGW versions of UTF-8 support. */
#include <wchar.h>
#include <wctype.h>
#endif

#include "error.h"


#ifdef _WIN32
#include <windows.h>

#ifdef _MSC_VER
/* The Microsoft Visual C compiler doesn't support the "inline" keyword. */
#define inline

/* MS Visual C does not have any function normally found in strings.h */
/* In particular, be careful to avoid including strings.h */

/* MS Visual C uses non-standard string function names */
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp _stricmp
#define strdup _strdup
#define strncasecmp(a,b,s) strnicmp((a),(b),(s))

/* MS Visual C does not support some C99 standard floating-point functions */
#define fmaxf(a,b) ((a) > (b) ? (a) : (b))

#endif /* _MSC_VER */

/* Appearently, MinGW is also missing a variety of standard fuctions.
 * Not surprising, since MinGW is intended for compiling Windows
 * programs on Windows.
 * MINGW is also known as MSYS */
#if defined(_MSC_VER) || defined(__MINGW32__)

/* No langinfo in Windows or MinGW */
#define nl_langinfo(X) ""

/* strtok_r() is missing in Windows */
char * strtok_r (char *s, const char *delim, char **saveptr);

/* strndup() is missing in Windows. */
char * strndup (const char *str, size_t size);

/* Windows doesn't have a thread-safe rand (!)
 * XXX FIXME -- this breaks thread safety on windows!
 * That means YOU, windows thread-using programmer!
 * Suck it up!
 */
#define rand_r(seedp) rand()
#endif /* _MSC_VER || __MINGW32__ */

/*
 * CYGWIN on Windows doesn't have UTF8 support, or wide chars ... 
 * However, MS Visual C appearently does, as does MinGW.  Since
 * some users have both cygwin and MinGW installed, crap out the 
 * UTF8 code only when MinGW is missing.
 */
#if defined (__CYGWIN__) && !defined(__MINGW32__)
#define mbstate_t char
#define mbrtowc(w,s,n,x)  ({*((char *)(w)) = *(s); 1;})
#define wcrtomb(s,w,x)    ({*((char *)(s)) = ((char)(w)); 1;})
#define iswupper  isupper
#define iswalpha  isalpha
#define iswdigit  isdigit
#define iswspace  isspace
#define wchar_t   char
#define wint_t    unsigned int  /* I think ?? not sure */
#define fgetwc    fgetc
#define WEOF      EOF
#define towlower  tolower
#define towupper  toupper
#endif /* __CYGWIN__ and not __MINGW32__ */

#endif /* _WIN32 */

#if defined(__sun__)
int strncasecmp(const char *s1, const char *s2, size_t n);
/* This does not appear to be in string.h header file in sunos
   (Or in linux when I compile with -ansi) */
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define assert(ex,string) {                                       \
    if (!(ex)) {                                                  \
        prt_error("Assertion failed: %s\n", string);              \
        exit(1);                                                  \
    }                                                             \
}

#if !defined(MIN)
#define MIN(X,Y)  ( ((X) < (Y)) ? (X) : (Y))
#endif
#if !defined(MAX)
#define MAX(X,Y)  ( ((X) > (Y)) ? (X) : (Y))
#endif

/* Optimizations that only gcc undersatands */
#if __GNUC__ > 2
#define GNUC_MALLOC __attribute__ ((malloc))
#else
#define GNUC_MALLOC
#endif

static inline int wctomb_check(char *s, wchar_t wc, mbstate_t *ps)
{
	int nr = wcrtomb(s, wc, ps);
	if (nr < 0) {
		prt_error("Fatal Error: unknown character set %s\n", nl_langinfo(CODESET));
		exit(1);
	}
	return nr;
}

static inline int is_utf8_upper(const char *s)
{
	mbstate_t mbs;
	wchar_t c;
	int nbytes;

	memset(&mbs, 0, sizeof(mbs));
	nbytes = mbrtowc(&c, s, MB_CUR_MAX, &mbs);
	if (iswupper(c)) return nbytes;
	return 0;
}

static inline int is_utf8_alpha(const char *s)
{
	mbstate_t mbs;
	wchar_t c;
	int nbytes;

	memset(&mbs, 0, sizeof(mbs));
	nbytes = mbrtowc(&c, s, MB_CUR_MAX, &mbs);
	if (iswalpha(c)) return nbytes;
	return 0;
}

static inline int is_utf8_digit(const char *s)
{
	mbstate_t mbs;
	wchar_t c;
	int nbytes;

	memset(&mbs, 0, sizeof(mbs));
	nbytes = mbrtowc(&c, s, MB_CUR_MAX, &mbs);
	if (iswdigit(c)) return nbytes;
	return 0;
}

static inline int is_utf8_space(const char *s)
{
	mbstate_t mbs;
	wchar_t c;
	int nbytes;

	memset(&mbs, 0, sizeof(mbs));
	nbytes = mbrtowc(&c, s, MB_CUR_MAX, &mbs);
	if (iswspace(c)) return nbytes;
	return 0;
}

static inline const char * skip_utf8_upper(const char * s)
{
	int nb = is_utf8_upper(s);
	while (nb)
	{
		s += nb;
		nb = is_utf8_upper(s);
	}
	return s;
}

/**
 * Return true if the intial upper-case letters of the
 * two input strings match. Comparison stops when
 * both srings descend to lowercase.
 */
static inline int utf8_upper_match(const char * s, const char * t)
{
	mbstate_t mbs, mbt;
	wchar_t ws, wt;
	int ns, nt;

	memset(&mbs, 0, sizeof(mbs));
	memset(&mbt, 0, sizeof(mbt));

	ns = mbrtowc(&ws, s, MB_CUR_MAX, &mbs);
	nt = mbrtowc(&wt, t, MB_CUR_MAX, &mbt);
	while (iswupper(ws) || iswupper(wt))
	{
		if (ws != wt) return FALSE;
		s += ns;
		t += nt;
		ns = mbrtowc(&ws, s, MB_CUR_MAX, &mbs);
		nt = mbrtowc(&wt, t, MB_CUR_MAX, &mbt);
	}
	return TRUE;
}

void downcase_utf8_str(char *to, const char * from, size_t usize);
void upcase_utf8_str(char *to, const char * from, size_t usize);

size_t lg_strlcpy(char * dest, const char *src, size_t size);
void safe_strcpy(char *u, const char * v, size_t usize);
void safe_strcat(char *u, const char *v, size_t usize);
char *safe_strdup(const char *u);

void left_print_string(FILE* fp, const char *, const char *);

/* routines for allocating basic objects */
void init_memusage(void);
void * xalloc(size_t) GNUC_MALLOC;
void * xrealloc(void *, size_t oldsize, size_t newsize) GNUC_MALLOC;
void * exalloc(size_t) GNUC_MALLOC;

#define TRACK_SPACE_USAGE
#ifdef TRACK_SPACE_USAGE
void xfree(void *, size_t);
void exfree(void *, size_t);
#else /* TRACK_SPACE_USAGE */
static inline void xfree(void *p, size_t sz) { free(p); }
static inline void exfree(void *p, size_t sz) { free(p); };
#endif /* TRACK_SPACE_USAGE */

size_t get_space_in_use(void);
size_t get_max_space_used(void);


char * get_default_locale(void);
char * join_path(const char * prefix, const char * suffix);

FILE * dictopen(const char *filename, const char *how);
void * object_open(const char *filename,
                   void * (*opencb)(const char *, void *),
                   void * user_data);

wchar_t * get_file_contents(const char *filename);

/**
 * Returns the smallest power of two that is at least i and at least 1
 */
static inline int next_power_of_two_up(int i)
{
   int j=1;
   while(j<i) j = j<<1;
   return j;
}

#endif
