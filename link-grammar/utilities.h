/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2009-2013 Linas Vepstas                                 */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */ 
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

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
# define alloca __builtin_alloca
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
# ifdef  __cplusplus
extern "C"
# endif
void *alloca (size_t);
#endif

#ifdef _WIN32
#include <windows.h>
#include <mbctype.h>

#ifdef _MSC_VER
/* The Microsoft Visual C compiler doesn't support the "inline" keyword.
 * But it does support _inline as a synonym... */
#define inline _inline

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
#define floorf(x) floor(x)
#define ceilf(x) ceil(x)

/* MS changed the name of rand_r to rand_s */
#define rand_r(seedp) rand_s(seedp)

#endif /* _MSC_VER */

/* Apparently, MinGW is also missing a variety of standard functions.
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

/* Users report that the default mbrtowc that comes with windows and/or
 * cygwin just doesn't work very well. So we use our own custom version,
 * instead.
 */
link_public_api(size_t) lg_mbrtowc(wchar_t *, const char *, size_t, mbstate_t *);
#ifdef mbrtowc
#undef mbrtowc
#endif
#define mbrtowc(w,s,n,x) lg_mbrtowc(w,s,n,x)
#endif /* _MSC_VER || __MINGW32__ */

#if __APPLE__
/* Junk, to keep the Mac OSX linker happy, because this is listed in
 * the link-grammar.def symbol export file.  */
void lg_mbrtowc(void);
#endif

/*
 * CYGWIN prior to version 1.7 did not have UTF8 support, or wide
 * chars ... However, MS Visual C does, as does MinGW.  Since
 * some users have both cygwin and MinGW installed, crap out the 
 * UTF8 code only when MinGW is missing (and the CYGWIN version
 * is very old) XXX This code is dangerous and should be removed.
 */
#if defined (__CYGWIN__) && !defined(__MINGW32__)
#if CYGWIN_VERSION_DLL_MAJOR < 1007
#error "Your Cygwin version is too old! Version 1.7 or later is needed for UTF8 support!"
#define mbstate_t char
#define mbrtowc(w,s,n,x)  ({*((char *)(w)) = *(s); 1;})
#define wcrtomb(s,w,x)    ({*((char *)(s)) = ((char)(w)); 1;})
#define mbrlen(s,n,m)     (1)
#define iswupper  isupper
#define iswalpha  isalpha
#define iswdigit  isdigit
#define iswspace  isspace
#define wchar_t   char
#define towlower  tolower
#define towupper  toupper
#endif /* CYGWIN_VERSION_DLL_MAJOR < 1007 */
#endif /* __CYGWIN__ and not __MINGW32__ */

#endif /* _WIN32 */

/* MSVC isspace asserts in debug mode, and mingw sometime returns true,
 * when passed utf8. Thus, limit to 7 bits for windows. */
#if defined(_MSC_VER) || defined(__MINGW32__)
  #define lg_isspace(c) ((0 < c) && (c < 127) && isspace(c))
#else
  #define lg_isspace isspace
#endif

#if defined(__sun__)
int strncasecmp(const char *s1, const char *s2, size_t n);
/* This does not appear to be in string.h header file in sunos
   (Or in linux when I compile with -ansi) */
#endif

#define STR(x) #x
#define STRINGIFY(x) STR(x)

#define FILELINE __FILE__ ":" STRINGIFY(__LINE__)

#if defined(_MSC_VER) || defined(__MINGW32__)
#define assert(ex, ...) {                                                   \
	if (!(ex)) {                                                             \
		prt_error("\nAssertion (" #ex ") failed at " FILELINE ": " __VA_ARGS__);  \
		fprintf(stderr, "\n");                                                \
		*((volatile int*) 0x0) = 42;  /* leave stack trace in debugger */     \
	}                                                                        \
}
#else
#define assert(ex, ...) {                                                   \
	if (!(ex)) {                                                             \
		prt_error("\nAssertion (" #ex ") failed at " FILELINE ": " __VA_ARGS__);  \
		fprintf(stderr, "\n");                                                \
		__builtin_trap();  /* leave stack trace in debugger */                \
	}                                                                        \
}
#endif

#if defined(__UCLIBC__)
#define fmaxf(a,b) ((a) > (b) ? (a) : (b))
#endif

#if !defined(MIN)
#define MIN(X,Y)  ( ((X) < (Y)) ? (X) : (Y))
#endif
#if !defined(MAX)
#define MAX(X,Y)  ( ((X) > (Y)) ? (X) : (Y))
#endif

/* Optimizations etc. that only gcc undersatands */
#if __GNUC__ > 2
#define GNUC_MALLOC __attribute__ ((malloc))
#define GNUC_UNUSED __attribute__ ((unused))
#else
#define GNUC_MALLOC
#define GNUC_UNUSED
#endif

/**
 * Return the length, in codepoints/glyphs, of the utf8-encoded 
 * string.  This is needed when printing strings in a formatted way.
 */
static inline size_t utf8_strlen(const char *s)
{
	mbstate_t mbss;
	memset(&mbss, 0, sizeof(mbss));
#if defined(_MSC_VER) || defined(__MINGW32__)
	return MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0)-1;
#else
	return mbsrtowcs(NULL, &s, 0, &mbss);
#endif
}

/**
 * Return the distance, in bytes, to the next character, in this
 * input utf8-encoded string
 */
static inline size_t utf8_next(const char *s)
{
	size_t len;
	mbstate_t mbs;
	memset(&mbs, 0, sizeof(mbs));
	len = mbrlen(s, MB_CUR_MAX, &mbs);
	if (len == (size_t)(-1) || len == (size_t)(-2)) {
		/* Too long or malformed sequence, step one byte. */
		return 1;
	}

	return len;
}

static inline int is_utf8_upper(const char *s)
{
	mbstate_t mbs;
	wchar_t c;
	int nbytes;

	memset(&mbs, 0, sizeof(mbs));
	nbytes = mbrtowc(&c, s, MB_CUR_MAX, &mbs);
	if (nbytes < 0) return 0;  /* invalid mb sequence */
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
	if (nbytes < 0) return 0;  /* invalid mb sequence */
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
	if (nbytes < 0) return 0;  /* invalid mb sequence */
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
	if (nbytes < 0) return 0;  /* invalid mb sequence */
	if (iswspace(c)) return nbytes;

	/* 0xc2 0xa0 is U+00A0, c2 a0, NO-BREAK SPACE */
	/* For some reason, iswspace doesn't get this */
	if ((2==nbytes) && ((0xff & s[0]) == 0xc2) && ((0xff & s[1]) == 0xa0)) return 2;
	if ((2==nbytes) && (c == 0xa0)) return 2;
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
 * Return true if the initial upper-case letters of the
 * two input strings match. Comparison stops when
 * both srings descend to lowercase.
 */
static inline bool utf8_upper_match(const char * s, const char * t)
{
	mbstate_t mbs, mbt;
	wchar_t ws, wt;
	int ns, nt;

	memset(&mbs, 0, sizeof(mbs));
	memset(&mbt, 0, sizeof(mbt));

	ns = mbrtowc(&ws, s, MB_CUR_MAX, &mbs);
	nt = mbrtowc(&wt, t, MB_CUR_MAX, &mbt);
	if (ns < 0 || nt < 0) return false;  /* invalid mb sequence */
	while (iswupper(ws) || iswupper(wt))
	{
		if (ws != wt) return false;
		s += ns;
		t += nt;
		ns = mbrtowc(&ws, s, MB_CUR_MAX, &mbs);
		nt = mbrtowc(&wt, t, MB_CUR_MAX, &mbt);
		if (ns < 0 || nt < 0) return false;  /* invalid mb sequence */
	}
	return true;
}

void downcase_utf8_str(char *to, const char * from, size_t usize);
void upcase_utf8_str(char *to, const char * from, size_t usize);

size_t lg_strlcpy(char * dest, const char *src, size_t size);
void safe_strcpy(char *u, const char * v, size_t usize);
void safe_strcat(char *u, const char *v, size_t usize);
char *safe_strdup(const char *u);

/* Simple, cheap, easy dynamic string. */
typedef struct
{
  char *str;
  size_t end;
  size_t len;
} dyn_str;

dyn_str* dyn_str_new(void);
void dyn_str_delete(dyn_str*);
void dyn_strcat(dyn_str*, const char*);

size_t altlen(const char **);
void altappend(Sentence, const char ***, const char *);

/* routines for allocating basic objects */
void init_memusage(void);
void * xalloc(size_t) GNUC_MALLOC;
void * exalloc(size_t) GNUC_MALLOC;

/* Tracking the space usage can help with debugging */
#if defined(_WIN32) || __APPLE__
  /* **MUST** define for win32, Mac OSX, because xfree is listed in
   * link-grammar.def and the win/osx linker fails if there is no
   * xfree. So, keep the linker happy. (xfree is used in both dict-file
   * and sat solver.)
   */
  #define TRACK_SPACE_USAGE 1
#endif /* defined(_WIN32) || __APPLE__ */

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
                   void * (*opencb)(const char *, const void *),
                   const void * user_data);

bool file_exists(const char * dict_name);
char * get_file_contents(const char *filename);

/**
 * Returns the smallest power of two that is at least i and at least 1
 */
static inline unsigned int next_power_of_two_up(unsigned int i)
{
   unsigned int j=1;
   while (j<i) j <<= 1;
   return j;
}

#endif
