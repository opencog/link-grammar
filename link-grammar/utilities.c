/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright 2008, 2009, 2013 Linas Vepstas                              */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#ifdef _WIN32
#define _CRT_RAND_S
#endif /* _WIN32 */
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdarg.h>
#include <locale.h>
#ifdef HAVE_XLOCALE_H
#include <xlocale.h>
#endif /* HAVE_XLOCALE_H */

#ifndef _WIN32
	// #include <unistd.h>
	#include <langinfo.h>
#else
	#include <windows.h>
#endif /* _WIN32 */

#include "utilities.h"

/* This file contains general utilities that fix, enhance OS-provided
 * API's, esp ones that the OS forgot to provide, or managed to break.
 */

/* ============================================================= */
/* String utilities */

/* Windows, POSIX and GNU have different ideas about strerror_r().  hence
 * use our own function that uses the available system function and is
 * consistent. It doesn't try to mimic exactly any version of
 * strerror_r(). */
#ifdef _WIN32
void lg_strerror(int err_no, char *buf, size_t len)
{
	if (strerror_s(buf, len, err_no) != 0)
		strerror_s(buf, len, errno); /* errno got set by previous strerror_s() */
}
#else
#if HAVE_STRERROR_R

#if STRERROR_R_CHAR_P
/* Using the GNU version. */
void lg_strerror(int err_no, char *buf, size_t len)
{
	char *errstr = strerror_r(err_no, buf, len);
	strncpy(buf, errstr, len);
	buf[len-1] = '\0';
}
#else /* !STRERROR_R_CHAR_P */
/* Using the XSI version. */
void lg_strerror(int err_no, char *buf, size_t len)
{
	errno = 0;
	if ((strerror_r(err_no, buf, len) == EINVAL) || (errno == EINVAL))
		snprintf(buf, len, "Unknown error %d", err_no);
}
#endif /* STRERROR_R_CHAR_P */

#else /* !STRERROR_R */
/* No strerror_r()??? No thread-safe error message - use a workaround.
 * (FIXME Could check if threads are not supported and use strerror(),
 * else protect strerror().) */
void lg_strerror(int err_no, char *buf, size_t len)
{
	snprintf(buf, len, "Error %d", err_no);
}
#endif /* STRERROR_R */
#endif /* _WIN32 */

char *safe_strdup(const char *u)
{
	if (u)
		return strdup(u);
	return NULL;
}

/**
 * A version of strlcpy, for those systems that don't have it.
 */
/*	$OpenBSD: strlcpy.c,v 1.12 2015/01/15 03:54:12 millert Exp $	*/
/*
 * Copyright (c) 1998, 2015 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * Copy string src to buffer dst of size dsize.  At most dsize-1
 * chars will be copied.  Always NUL terminates (unless dsize == 0).
 * Returns strlen(src); if retval >= dsize, truncation occurred.
 */
size_t
lg_strlcpy(char * restrict dst, const char * restrict src, size_t dsize)
{
	const char *osrc = src;
	size_t nleft = dsize;

	/* Copy as many bytes as will fit. */
	if (nleft != 0) {
		while (--nleft != 0) {
			if ((*dst++ = *src++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src. */
	if (nleft == 0) {
		if (dsize != 0)
			*dst = '\0';      /* NUL-terminate dst */
		while (*src++)
			;
	}

	return(src - osrc - 1); /* count does not include NUL */
}

/**
 * Catenates as much of v onto u as it can assuming u is of size usize
 * guaranteed to terminate u with a '\0'.  Assumes u and v are null
 * terminated.
 */
void safe_strcat(char *u, const char *v, size_t usize)
{
	strncat(u, v, usize-strlen(u)-1);
	u[usize-1] = '\0';
}

#ifndef HAVE_STRNDUP
/* Emulates glibc's strndup() */
char *
strndup (const char *str, size_t size)
{
	size_t len;
	char *result;

	len = strlen (str);
	if (!len) return strdup ("");
	if (size > len) size = len;

	result = (char *) malloc ((size + 1) * sizeof (char));
	memcpy (result, str, size);
	result[size] = 0x0;
	return result;
}
#endif /* !HAVE_STRNDUP */

#ifndef HAVE_STRTOK_R
/*
 * public domain strtok_r() by Charlie Gordon
 * from comp.lang.c  9/14/2007
 *     http://groups.google.com/group/comp.lang.c/msg/2ab1ecbb86646684
 *
 *     Declaration that it's public domain:
 *     http://groups.google.com/group/comp.lang.c/msg/7c7b39328fefab9c
 */
char* strtok_r(char *str, const char *delim, char **nextp)
{
	char *ret;

	if (str == NULL) str = *nextp;
	str += strspn(str, delim);
	if (*str == '\0') return NULL;
	ret = str;
	str += strcspn(str, delim);
	if (*str) *str++ = '\0';
	*nextp = str;

	return ret;
}
#endif /* !HAVE_STRTOK_R */

/* ============================================================= */
/* UTF8 utilities */

#ifdef _WIN32
/**
 * (Experimental) Implementation of mbrtowc for Windows.
 * This is required because the other, commonly available implementations
 * seem to not work very well, based on user reports.  Someone who is
 * really, really good at windows programming needs to review this stuff!
 */
size_t lg_mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps)
{
	int nb, nb2;

	if (NULL == s) return 0;
	if (0 == n) return -2;
	if (0 == *s) { *pwc = 0; return 0; }

	nb = utf8_charlen(s);
	if (0 == nb) return 0;
	if (0 > nb) return nb;
	nb2 = MultiByteToWideChar(CP_UTF8, 0, s, nb, NULL, 0);
	nb2 = MultiByteToWideChar(CP_UTF8, 0, s, nb, pwc, nb2);
	if (0 == nb2) return (size_t)-1;
	return nb;
}

/**
 * Emulate rand_r() using rand_s() in a way that is enough for our needs.
 * Windows doesn't have rand_r(), and its rand_s() is different: It
 * returns an error indication and not the random number like rand_r().
 * The value it returns is through its argument.
 *
 * Note that "#define _CRT_RAND_S" is needed before "#include <stdlib.h>".
 */
int rand_r(unsigned int *s)
{
	rand_s(s);
	if (*s > INT_MAX) *s -= INT_MAX;

	return *s;
}
#endif /* _WIN32 */

static int wctomb_check(char *s, wchar_t wc)
{
	int nr;
#ifdef _WIN32
	nr = WideCharToMultiByte(CP_UTF8, 0, &wc, 1, NULL, 0, NULL, NULL);
	nr = WideCharToMultiByte(CP_UTF8, 0, &wc, 1, s, nr, NULL, NULL);
	if (0 == nr) return -1;
#else
	mbstate_t mbss;
	memset(&mbss, 0, sizeof(mbss));
	nr = wcrtomb(s, wc, &mbss);
	if (nr < 0) {
		prt_error("Fatal Error: unknown character set %s\n", nl_langinfo(CODESET));
		exit(1);
	}
#endif /* _WIN32 */
	return nr;
}

/**
 * Downcase the first letter of the word.
 * XXX FIXME This works 'most of the time', but is not technically correct.
 * This is because towlower() and towupper() are locale dependent, and also
 * because the byte-counts might not match up, e.g. German ß and SS.
 * The correct long-term fix is to use ICU or glib g_utf8_strup(), etc.
 */
void downcase_utf8_str(char *to, const char * from, size_t usize, locale_t locale)
{
	wchar_t c;
	int i, nbl, nbh;
	char low[MB_LEN_MAX];
	mbstate_t mbs;

	/* Make sure it doesn't contain garbage in case of an error */
	if (to != from) strcpy(to, from);

	memset(&mbs, 0, sizeof(mbs));
	nbh = mbrtowc (&c, from, MB_CUR_MAX, &mbs);
	if (nbh < 0)
	{
		prt_error("Error: Invalid UTF-8 string!\n");
		return;
	}
	c = towlower_l(c, locale);
	nbl = wctomb_check(low, c);

	/* Check for error on an in-place copy */
	if ((nbh < nbl) && (to == from))
	{
		/* I'm to lazy to fix this */
		prt_error("Error: can't downcase UTF-8 string!\n");
		return;
	}

	/* Downcase */
	for (i=0; i<nbl; i++) { to[i] = low[i]; }

	if ((nbh == nbl) && (to == from)) return;

	from += nbh;
	to += nbl;
	lg_strlcpy(to, from, usize-nbl);
}

#if 0
/**
 * Upcase the first letter of the word.
 * XXX FIXME This works 'most of the time', but is not technically correct.
 * This is because towlower() and towupper() are locale dependent, and also
 * because the byte-counts might not match up, e.g. German ß and SS.
 * The correct long-term fix is to use ICU or glib g_utf8_strup(), etc.
 */
void upcase_utf8_str(char *to, const char * from, size_t usize, locale_t locale)
{
	wchar_t c;
	int i, nbl, nbh;
	char low[MB_LEN_MAX];
	mbstate_t mbs;

	memset(&mbs, 0, sizeof(mbs));
	nbh = mbrtowc (&c, from, MB_CUR_MAX, &mbs);
	if (nbh < 0)
	{
		prt_error("Error: Invalid UTF-8 string!\n");
		return;
	}
	c = towupper_l(c, locale);
	nbl = wctomb_check(low, c);

	/* Check for error on an in-place copy */
	if ((nbh < nbl) && (to == from))
	{
		/* I'm to lazy to fix this */
		prt_error("Error: can't upcase UTF-8 string!\n");
		return;
	}

	/* Upcase */
	for (i=0; i<nbl; i++) { to[i] = low[i]; }

	if ((nbh == nbl) && (to == from)) return;

	from += nbh;
	to += nbl;
	lg_strlcpy(to, from, usize-nbl);
}
#endif

#ifdef NO_ALIGNED_MALLOC
#if __GNUC__
#warning No aligned alloc found (using malloc() instead).
#endif
#endif /* NO_ALIGNED_MALLOC */

#ifdef HAVE_POSIX_MEMALIGN
void *aligned_alloc(size_t alignment, size_t size)
{
	void *ptr;
	errno = posix_memalign(&ptr, alignment, size);
	return ptr;
}
#endif /* HAVE_POSIX_MEMALIGN */

/* ============================================================= */
/* Memory alloc routines below. These routines attempt to keep
 * track of how much space is getting used during a parse.
 *
 * This code is probably obsolescent, and should probably be dumped.
 * No one (that I know of) looks at the space usage; its one of the
 * few areas that needs pthreads -- it would be great to just get
 * rid of it (and thus get rid of pthreads).
 */

#ifdef TRACK_SPACE_USAGE
typedef struct
{
	size_t max_space_used;
	size_t space_in_use;
	size_t num_xallocs;
	size_t num_xfrees;
	size_t max_outstanding_xallocs;
	size_t max_external_space_used;
	size_t external_space_in_use;
	size_t num_exallocs;
	size_t num_exfrees;
	size_t max_outstanding_exallocs;
} space_t;

static TLS space_t space;
static space_t * do_init_memusage(void)
{
	space_t *s = &space;

	s->max_space_used = 0;
	s->space_in_use = 0;
	s->num_xallocs = 0;
	s->num_xfrees = 0;
	s->max_outstanding_xallocs = 0;
	s->max_external_space_used = 0;
	s->external_space_in_use = 0;
	s->num_exallocs = 0;
	s->num_exfrees = 0;
	s->max_outstanding_exallocs = 0;

	return s;
}

void init_memusage(void)
{
	static bool mem_inited = false;
	if (mem_inited) return;
	mem_inited = true;
	do_init_memusage();
}

static inline space_t *getspace(void)
{
	return &space;
}

/**
 * space used but not yet freed during parse
 */
size_t get_space_in_use(void)
{
	return getspace()->space_in_use;
}

/**
 * maximum space used during the parse
 */
size_t get_max_space_used(void)
{
	return getspace()->max_space_used;
}
#else /* TRACK_SPACE_USAGE */
void init_memusage(void) {}
size_t get_space_in_use(void) { return 0; }
size_t get_max_space_used(void) { return 0; }
#endif /* TRACK_SPACE_USAGE */

/**
 * alloc some memory, and keep track of the space allocated.
 */
void * xalloc(size_t size)
{
	void * p = malloc(size);

#ifdef TRACK_SPACE_USAGE
	space_t *s = getspace();
	s->space_in_use += size;
	if (s->max_space_used < s->space_in_use) s->max_space_used = s->space_in_use;
	s->num_xallocs ++;
	if (s->max_outstanding_xallocs < (s->num_xallocs - s->num_xfrees))
		s->max_outstanding_xallocs = (s->num_xallocs - s->num_xfrees);

#endif /* TRACK_SPACE_USAGE */
	if ((p == NULL) && (size != 0))
	{
		prt_error("Fatal Error: Ran out of space. (int)\n");
		abort();
		exit(1);
	}
	return p;
}

#ifdef TRACK_SPACE_USAGE
void xfree(void * p, size_t size)
{
	space_t *s = getspace();
	s->space_in_use -= size;
	s->num_xfrees ++;

	free(p);
}
#endif /* TRACK_SPACE_USAGE */

void * exalloc(size_t size)
{
	void * p = malloc(size);
#ifdef TRACK_SPACE_USAGE
	space_t *s = getspace();
	s->external_space_in_use += size;
	if (s->max_external_space_used < s->external_space_in_use)
		s->max_external_space_used = s->external_space_in_use;
	s->num_exallocs ++;
	if (s->max_outstanding_exallocs < (s->num_exallocs - s->num_exfrees))
		s->max_outstanding_exallocs = (s->num_exallocs - s->num_exfrees);
#endif /* TRACK_SPACE_USAGE */

	if ((p == NULL) && (size != 0))
	{
		prt_error("Fatal Error: Ran out of space. (ext)\n");
		abort();
		exit(1);
	}
	return p;
}

#ifdef TRACK_SPACE_USAGE
void exfree(void * p, size_t size)
{
	space_t *s = getspace();
	s->external_space_in_use -= size;
	s->num_exfrees ++;
	free(p);
}
#endif /* TRACK_SPACE_USAGE */

/* =========================================================== */
/* Simple, cheap, easy dynamic string. */

dyn_str* dyn_str_new(void)
{
	dyn_str *ds = malloc(sizeof(dyn_str));
	ds->len = 250;
	ds->end = 0;
	ds->str = malloc(ds->len);
	ds->str[0] = 0x0;
	return ds;
}

void dyn_str_delete(dyn_str* ds)
{
	free(ds->str);
	free(ds);
}

char * dyn_str_take(dyn_str* ds)
{
	char * rv = ds->str;
	free(ds);
	return rv;
}

void dyn_strcat(dyn_str* ds, const char *str)
{
	size_t l = strlen(str);
	if (ds->end+l+1 >= ds->len)
	{
		ds->len = 2 * ds->len + l;
		ds->str = realloc(ds->str, ds->len);
	}
	strcpy (ds->str+ds->end, str);
	ds->end += l;
}

/// Trim away trailing whitespace.
void dyn_trimback(dyn_str* ds)
{
	size_t tail = ds->end;
	while (0 < tail && ' ' == ds->str[--tail]) {}

	ds->end = ++tail;
	ds->str[tail] = 0x0;
}

const char * dyn_str_value(dyn_str* s)
{
	return s->str;
}

/* ======================================================== */
/* Locale routines */

#ifdef HAVE_LOCALE_T
/**
 * Create a locale object from the given locale string.
 * @param locale Locale string, in the native OS format.
 * @return Locale object for the given locale
 * Note: It has to be freed by freelocale().
 */
locale_t newlocale_LC_CTYPE(const char *locale)
{
	locale_t locobj;
#ifdef _WIN32
	locobj = _create_locale(LC_CTYPE, locale);
#else
	locobj = newlocale(LC_CTYPE_MASK, locale, (locale_t)0);
#endif /* _WIN32 */
	return locobj;
}
#endif /* HAVE_LOCALE_T */

/**
 * Check that the given locale known by the system.
 * In case we don't have locale_t, actually set the locale
 * in order to find out if it is fine. This side effect doesn't cause
 * harm, as the locale would be set up to that value anyway shortly.
 * @param locale Locale string
 * @return True if known, false if unknown.
 */
bool try_locale(const char *locale)
{
#ifdef HAVE_LOCALE_T
		locale_t ltmp = newlocale_LC_CTYPE(locale);
		if ((locale_t)0 == ltmp) return false;
		freelocale(ltmp);
#else
		lgdebug(D_USER_FILES, "Debug: Setting program's locale \"%s\"", locale);
		if (NULL == setlocale(LC_CTYPE, locale))
		{
			lgdebug(D_USER_FILES, " failed!\n");
			return false;
		}
		lgdebug(D_USER_FILES, ".\n");
#endif /* HAVE_LOCALE_T */

		return true;
}

/**
 * Ensure that the program's locale has a UTF-8 codeset.
 */
void set_utf8_program_locale(void)
{
#ifndef _WIN32
	/* The LG library doesn't use mbrtowc_l(), since it doesn't exist in
	 * the dynamic glibc (2.22). mbsrtowcs_l() could also be used, but for
	 * some reason it exists only in the static glibc.
	 * In order that mbrtowc() will work for any UTF-8 character, UTF-8
	 * codeset is ensured. */
	const char *codeset = nl_langinfo(CODESET);
	if (!strstr(codeset, "UTF") && !strstr(codeset, "utf"))
	{
		const char *locale = setlocale(LC_CTYPE, NULL);
		/* Avoid an initial spurious message. */
		if ((0 != strcmp(locale, "C")) && (0 != strcmp(locale, "POSIX")))
		{
			prt_error("Warning: Program locale \"%s\" (codeset %s) was not UTF-8; "
						 "force-setting to en_US.UTF-8\n", locale, codeset);
		}
		locale = setlocale(LC_CTYPE, "en_US.UTF-8");
		if (NULL == locale)
		{
			prt_error("Warning: Program locale en_US.UTF-8 could not be set; "
			          "force-setting to C.UTF-8\n");
			locale = setlocale(LC_CTYPE, "C.UTF-8");
			if (NULL == locale)
			{
				prt_error("Warning: Could not set a UTF-8 program locale; "
				          "program may malfunction\n");
			}
		}
	}
#endif /* !_WIN32 */
}

#ifdef _WIN32
static char *
win32_getlocale (void)
{
	char lbuf[10];
	char locale[32];

	LCID lcid = GetThreadLocale();

	if (0 >= GetLocaleInfoA(lcid, LOCALE_SISO639LANGNAME, lbuf, sizeof(lbuf)))
	{
		prt_error("Error: GetLocaleInfoA LOCALE_SENGLISHLANGUAGENAME LCID=%d: "
		          "Error %d\n", (int)lcid, (int)GetLastError());
		return NULL;
	}
	strcpy(locale, lbuf);
	strcat(locale, "-");

	if (0 >= GetLocaleInfoA(lcid, LOCALE_SISO3166CTRYNAME, lbuf, sizeof(lbuf)))
	{
		prt_error("Error: GetLocaleInfoA LOCALE_SISO3166CTRYNAME LCID=%d: "
		          "Error %d\n", (int)lcid, (int)GetLastError());
		return NULL;
	}
	strcat(locale, lbuf);

	return strdup(locale);
}
#endif /* _WIN32 */

char * get_default_locale(void)
{
	const char *lc_vars[] = {"LC_ALL", "LC_CTYPE", "LANG", NULL};
	char *ev;
	const char **evname;
	char *locale = NULL;

	for(evname = lc_vars; NULL != *evname; evname++)
	{
		ev = getenv(*evname);
		if ((NULL != ev) && ('\0' != ev[0])) break;
	}
	if (NULL != *evname)
	{
		locale = ev;
		lgdebug(D_USER_FILES, "Debug: Environment locale \"%s=%s\"\n", *evname, ev);
#ifdef _WIN32
		/* If compiled with MSVC/MinGW, we still support running under Cygwin. */
		const char *ostype = getenv("OSTYPE");
		if ((NULL != ostype) && (0 == strcmp(ostype, "cygwin")))
		{
			/* Convert to Windows style locale */
			locale = strdupa(locale);
			locale[strcspn(locale, "_")] = '-';
			locale[strcspn(locale, ".@")] = '\0';
		}
#endif /* _WIN32 */
	}
	else
	{
		lgdebug(D_USER_FILES, "Debug: Environment locale not set\n");
#ifdef _WIN32
		locale = win32_getlocale();
		if (NULL == locale)
			lgdebug(D_USER_FILES, "Debug: Cannot find user default locale\n");
		else
			lgdebug(D_USER_FILES, "Debug: User default locale \"%s\"\n", locale);
		return locale; /* Already strdup'ed */
#endif /* _WIN32 */
	}

	return safe_strdup(locale);
}

#ifdef HAVE_LOCALE_T
static void free_C_LC_NUMERIC(void);

static locale_t get_C_LC_NUMERIC(void)
{
	static locale_t locobj;

	if ((locale_t)0 != locobj) return locobj;

#ifdef _WIN32
	locobj = _create_locale(LC_NUMERIC, "C");
#else
	locobj = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
#endif /* _WIN32 */

	atexit(free_C_LC_NUMERIC);

	return locobj;
}

static void free_C_LC_NUMERIC(void)
{
	freelocale(get_C_LC_NUMERIC());
}
#endif /* HAVE_LOCALE_T */

/* FIXME: Rewrite to directly convert scaled integer strings (only). */
bool strtodC(const char *s, float *r)
{
	char *err;

#ifdef HAVE_LOCALE_T
	double val = strtod_l(s, &err, get_C_LC_NUMERIC());
#else
	/* dictionary_setup_locale() invokes setlocale(LC_NUMERIC, "C") */
	double val = strtod(s, &err);
#endif /* HAVE_LOCALE_T */

	if ('\0' != *err) return false; /* *r unaffected */

	*r = val;
	return true;
}

/* ============================================================= */
/* Alternatives utilities */

size_t altlen(const char **arr)
{
	size_t len = 0;
	if (arr)
		while (arr[len] != NULL) len++;
	return len;
}

/* ============================================================= */

#ifdef __MINGW32__
/*
 * Since _USE_MINGW_ANSI_STDIO=1 is used in order to support C99 STDIO
 * including the %z formats, MinGW uses its own *printf() functions (and not
 * the Windows ones). However, its printf()/fprintf() functions cannot write
 * UTF-8 to the console (to files/pipes they write UTF-8 just fine).  It
 * turned out the problem is that they use the putchar() of Windows, which
 * doesn't support writing UTF-8 only when writing to the console!  This
 * problem is not fixed even in Windows 10 and the latest MinGW in Cygwin
 * 2.5.2.
 *
 * The workaround implemented here is to reimplement the corresponding MinGW
 * internal functions, and use fputs() to write the result.
 *
 * (Reimplementing printf()/fprintf() this way didn't work even with the
 * compilation flag -fno-builtin .)
 */

int __mingw_vfprintf (FILE * __restrict__ stream, const char * __restrict__ fmt,
                      va_list vl)
{
	int n = vsnprintf(NULL, 0, fmt, vl);
	if (0 > n) return n;
	char *buf = malloc(n+1);
	n = vsnprintf(buf, n+1, fmt, vl);
	if (0 > n)
	{
		free(buf);
		return n;
	}

	n = fputs(buf, stdout);
	free(buf);
	return n;
}

int __mingw_vprintf (const char * __restrict__ fmt, va_list vl)
{
	return __mingw_vfprintf(stdout, fmt, vl);
}
#endif /* __MINGW32__ */
/* ============================================================= */
