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

/* The _Win32 definitions are for native-Windows compilers.  This includes
 * MSVC (only versions >=14 are supported) and MINGW (under MSYS or Cygwin).
 * The _WIN32 definitions are not for Cygwin, which doesn't define _WIN32. */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>
#ifdef HAVE_LOCALE_T_IN_XLOCALE_H
#include <xlocale.h>
#endif /* HAVE_LOCALE_T_IN_XLOCALE_H */

#include "link-includes.h"

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
#ifndef alloca
# define alloca __builtin_alloca
#endif /* !alloca */
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

#ifndef TLS
#ifdef _MSC_VER
#define TLS __declspec(thread)
#else
#define TLS
#endif /* _MSC_VER */
#endif /* !TLS */

#ifdef _MSC_VER
/* These definitions are incorrect, as these functions are different(!)
 * (non-standard functionality).
 * See http://stackoverflow.com/questions/27754492 . Fortunately,
 * MSVC 14 supports C99 functions so these definitions are now unneeded.
 * (LG library compilation is unsupported for previous MSVC versions.)
 * (Left here for documentation.) */
#if 0
/* MS Visual C uses non-standard string function names */
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif

#define HAVE__ALIGNED_MALLOC 1

/* Avoid plenty of: warning C4090: 'function': different 'const' qualifiers.
 * This happens, for example, when the argument is "const void **". */
#define free(x) free((void *)x)
#define realloc(x, s) realloc((void *)x, s)
#define memcpy(x, y, s) memcpy((void *)x, (void *)y, s)
#define qsort(x, y, z, w) qsort((void *)x, y, z, w)
#endif /* _MSC_VER */

#if defined(HAVE_LOCALE_T_IN_LOCALE_H) || defined(HAVE_LOCALE_T_IN_XLOCALE_H)
#define HAVE_LOCALE_T 1
#endif /* HAVE_LOCALE_T_IN_LOCALE_H || HAVE_LOCALE_T_IN_XLOCALE_H) */

#if defined _MSC_VER || defined __cplusplus
/* "restrict" is not a part of ISO C++.
 * C++ compilers usually know it as __restrict. */
#define restrict __restrict
#endif

#ifdef _WIN32
#include <windows.h>
#include <mbctype.h>

/* Compatibility definitions. */
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#ifndef strncasecmp
#define strncasecmp(a,b,s) strnicmp((a),(b),(s))
#endif

#undef rand_r  /* Avoid (a bad) definition on MinGW */
int rand_r(unsigned int *);
#ifndef __MINGW32__
/* No strtok_s in XP/2003 and their strtok_r is incompatible.
 * Hence HAVE_STRTOK_R will not be defined and our own one will be used. */
#if _WINVER != 0x501 /* XP */ && _WINVER != 0x502 /* Server 2003 */
#define strtok_r strtok_s
#define HAVE_STRTOK_R
#endif /* _WINVER != XP|2003 */

/* There is no ssize_t definition in native Windows. */
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;

/* Native windows has locale_t, and hence HAVE_LOCALE_T is defined here.
 * However, MinGW currently doesn't have locale_t. If/when it has locale_t,
 * "configure" will define HAVE_LOCALE_T for it. */
#define HAVE_LOCALE_T
#endif

#ifdef HAVE_LOCALE_T
#define locale_t _locale_t
#define iswupper_l  _iswupper_l
#define iswalpha_l  _iswalpha_l
#define iswdigit_l  _iswdigit_l
#define iswspace_l  _iswspace_l
#define towlower_l  _towlower_l
#define towupper_l  _towupper_l
#define strtod_l    _strtod_l
#define freelocale _free_locale
#endif /* HAVE_LOCALE_T */

/* strndup() is missing in Windows. */
char * strndup (const char *str, size_t size);

/* Users report that the default mbrtowc that comes with windows and/or
 * cygwin just doesn't work very well. So we use our own custom version,
 * instead.
 */
#ifdef mbrtowc
#undef mbrtowc
#endif
size_t lg_mbrtowc(wchar_t *, const char *, size_t n, mbstate_t *ps);
#define mbrtowc(w,s,n,x) lg_mbrtowc(w,s,n,x)
#endif /* _WIN32 */

/* MSVC isspace asserts in debug mode, and mingw sometime returns true,
 * when passed utf8. OSX returns TRUE on char values 0x85 and 0xa0).
 * Since it is defined to return TRUE only on 6 characters, all of which
 * are in the range [0..127], just limit its arguments to 7 bits. */
#define lg_isspace(c) ((0 < c) && (c < 127) && isspace(c))

void lg_strerror(int err_no, char *buf, size_t len);

#if defined(__sun__)
int strncasecmp(const char *s1, const char *s2, size_t n);
/* This does not appear to be in string.h header file in sunos
   (Or in linux when I compile with -ansi) */
#endif

/* Cygwin < 2.6.0 doesn't have locale_t. */
#ifdef HAVE_LOCALE_T
locale_t newlocale_LC_CTYPE(const char *);
#else
typedef int locale_t;
#define iswupper_l(c, l) iswupper(c)
#define iswalpha_l(c, l) iswalpha(c)
#define iswdigit_l(c, l) iswdigit(c)
#define iswspace_l(c, l) iswspace(c)
#define towlower_l(c, l) towlower(c)
#define towupper_l(c, l) towupper(c)
#define freelocale(l)
#endif /* HAVE_LOCALE_T */

#if HAVE__ALIGNED_MALLOC
#define aligned_alloc(alignment, size) _aligned_malloc (size, alignment)
#define aligned_free(p) _aligned_free(p)
#undef HAVE_POSIX_MEMALIGN

#elif HAVE_ALIGNED_ALLOC
#define aligned_free(p) free(p)
#undef HAVE_POSIX_MEMALIGN

#elif HAVE_POSIX_MEMALIGN
/* aligned_alloc() emulation will be defined in utilities.c. */
void *aligned_alloc(size_t alignment, size_t size);
#define aligned_free(p) free(p)

#else
/* Fallback to just malloc(), as alignment is not critical here. */
#define NO_ALIGNED_MALLOC /* For generating a warning in utilities.c. */
#define aligned_alloc(alignment, size) malloc(size)
#define aligned_free(p) free(p)
#endif /* HAVE__ALIGNED_MALLOC */

#define ALIGN(size, alignment) (((size)+(alignment-1))&~(alignment-1))

#define STR(x) #x
#define STRINGIFY(x) STR(x)

#if !defined(MIN)
#define MIN(X,Y)  ( ((X) < (Y)) ? (X) : (Y))
#endif
#if !defined(MAX)
#define MAX(X,Y)  ( ((X) > (Y)) ? (X) : (Y))
#endif

/* In the following, the arguments should not have side effects.
 * FIXME: Detect in "configure" and check HAVE_* */
#ifndef strdupa
#define strdupa(s) strcpy(alloca(strlen(s)+1), s)
#endif
#ifndef strndupa
#define strndupa(s, n) _strndupa3(alloca((n)+1), s, n)
static inline char *_strndupa3(char *new_s, const char *s, size_t n)
{
	strncpy(new_s, s, n);
	new_s[n] = '\0';

	return new_s;
}
#endif

/* From ccan array_size.h and build_assert.h, which are under a CC0 license */
#define BUILD_ASSERT_OR_ZERO(cond) (sizeof(char [1 - 2*!(cond)]) - 1)
#if !defined(ARRAY_SIZE)
/**
 * ARRAY_SIZE: Get the number of elements in a visible array
 * @param arr The array whose size you want.
 *
 * This does not work on pointers, or arrays declared as [], or
 * function parameters.  With correct compiler support, such usage
 * will cause a build error (see build_assert).
 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + _array_size_chk(arr))

#if HAVE_BUILTIN_TYPES_COMPATIBLE_P && HAVE_TYPEOF
/* Two gcc extensions.
 * &a[0] degrades to a pointer: a different type from an array */
#define _array_size_chk(arr)
	BUILD_ASSERT_OR_ZERO(!__builtin_types_compatible_p(typeof(arr), \
							typeof(&(arr)[0])))
#else
#define _array_size_chk(arr) 0
#endif
#endif /* !defined(ARRAY_SIZE) */

/* The GCC version we need must be >= 4.7, because it has to
 * support C11. So it already supports all the features below. */

/* Optimizations etc. that only gcc understands */
#if __GNUC__
#define GCC_DIAGNOSTIC
#define UNREACHABLE(x) (__extension__ ({if (x) __builtin_unreachable();}))
#define GNUC_MALLOC __attribute__ ((__malloc__))
#define GNUC_UNUSED __attribute__ ((__unused__))
#define GNUC_NORETURN __attribute__ ((__noreturn__))
#define NO_SAN __attribute__ ((no_sanitize_address, no_sanitize_undefined))

/* Define when configuring with ASAN/UBSAN - for fast dict load (of course
 * only when not debugging dict code.) */
#ifdef NO_SAN_DICT
#undef NO_SAN_DICT
#define NO_SAN_DICT NO_SAN
#else
#define NO_SAN_DICT
#endif

#ifndef DONT_EXPECT
#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)
#endif

#else
#define UNREACHABLE(x)
#define GNUC_MALLOC
#define GNUC_UNUSED
#define GNUC_NORETURN
#define NO_SAN_DICT

#define likely(x) x
#define unlikely(x) x
#endif


/* Apply a pragma to a specific code section only.
 * XXX According to the GCC docs, we cannot use here something like
 * "#ifdef HAVE_x". Also -Wunknown-pragmas & -Wno-unknown-warning-option
 * don't work in this situation. So "-Wmaybe-uninitialized", which
 * is not recognized by clang, is defined separately. */
#ifdef GCC_DIAGNOSTIC

#ifdef HAVE_MAYBE_UNINITIALIZED
#define PRAGMA_MAYBE_UNINITIALIZED \
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
#else
#define PRAGMA_MAYBE_UNINITIALIZED \
	_Pragma("GCC diagnostic push")
#endif /* HAVE_MAYBE_UNINITIALIZED */

#define PRAGMA_START(x) \
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Wunknown-pragmas\"") \
	_Pragma(#x)
#define PRAGMA_END _Pragma("GCC diagnostic pop")
#else
#define PRAGMA_START(x)
#define PRAGMA_END
#define PRAGMA_MAYBE_UNINITIALIZED
#endif /* GCC_DIAGNOSTIC */

/**
 * Return the length, in codepoints/glyphs, of the utf8-encoded
 * string.  The string is assumed to be null-terminated.
 * This is needed when splitting words into morphemes.
 */
static inline size_t utf8_strlen(const char *s)
{
	mbstate_t mbss;
	memset(&mbss, 0, sizeof(mbss));
#if _WIN32
	return MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0)-1;
#else
	return mbsrtowcs(NULL, &s, 0, &mbss);
#endif /* _WIN32 */
}

/** Returns length of UTF8 character.
 * Current algo is based on the first character only.
 * If pointer is not pointing at first char, or not a valid value, returns -1.
 * Returns 0 for NULL.
 */
static inline int utf8_charlen(const char *xc)
{
	unsigned char c;

	c = (unsigned char) *xc;

	if (c == 0) return 0;
	if (c < 0x80) return 1;
	if ((c >= 0xc2) && (c < 0xe0)) return 2; /* First byte of a code point U +0080 - U +07FF */
	if ((c >= 0xe0) && (c < 0xf0)) return 3; /* First byte of a code point U +0800 - U +FFFF */
	if ((c >= 0xf0) && (c <= 0xf4)) return 4; /* First byte of a code point U +10000 - U +10FFFF */
	return -1; /* Fallthrough -- not the first byte of a code-point. */
}

/**
 * Copy `n` utf8 characters from `src` to `dest`.
 * Return the number of bytes actually copied.
 * The `dest` must have enough room to hold the copy.
 */
static inline size_t utf8_strncpy(char *dest, const char *src, size_t n)
{
	size_t b = 0;
	while (0 < n)
	{
		size_t k = utf8_charlen(src);
		if (0 > (ssize_t)k) return 0; /* XXX Maybe print error. */
		b += k;
		while (0 < k) { *dest = *src; dest++; src++; k--; }
		n--;
		if (0x0 == *src) break;
	}

	return b;
}

static inline int is_utf8_upper(const char *s, locale_t dict_locale)
{
	mbstate_t mbs;
	wchar_t c;
	int nbytes;

	memset(&mbs, 0, sizeof(mbs));
	nbytes = mbrtowc(&c, s, MB_CUR_MAX, &mbs);
	if (nbytes < 0) return 0;  /* invalid mb sequence */
	if (iswupper_l(c, dict_locale)) return nbytes;
	return 0;
}

static inline int is_utf8_alpha(const char *s, locale_t dict_locale)
{
	mbstate_t mbs;
	wchar_t c;
	int nbytes;

	memset(&mbs, 0, sizeof(mbs));
	nbytes = mbrtowc(&c, s, MB_CUR_MAX, &mbs);
	if (nbytes < 0) return 0;  /* invalid mb sequence */
	if (iswalpha_l(c, dict_locale)) return nbytes;
	return 0;
}

static inline int is_utf8_digit(const char *s, locale_t dict_locale)
{
	mbstate_t mbs;
	wchar_t c;
	int nbytes;

	memset(&mbs, 0, sizeof(mbs));
	nbytes = mbrtowc(&c, s, MB_CUR_MAX, &mbs);
	if (nbytes < 0) return 0;  /* invalid mb sequence */
	if (iswdigit_l(c, dict_locale)) return nbytes;
	return 0;
}

static inline int is_utf8_space(const char *s, locale_t dict_locale)
{
	mbstate_t mbs;
	wchar_t c;
	int nbytes;

	memset(&mbs, 0, sizeof(mbs));
	nbytes = mbrtowc(&c, s, MB_CUR_MAX, &mbs);
	if (nbytes < 0) return 0;  /* invalid mb sequence */
	if (iswspace_l(c, dict_locale)) return nbytes;

	/* 0xc2 0xa0 is U+00A0, c2 a0, NO-BREAK SPACE */
	/* For some reason, iswspace doesn't get this */
	if ((2==nbytes) && ((0xff & s[0]) == 0xc2) && ((0xff & s[1]) == 0xa0)) return 2;
	if ((2==nbytes) && (c == 0xa0)) return 2;
	return 0;
}

#if 0 /* Not in use. */
static inline const char * skip_utf8_upper(const char * s, locale_t dict_locale)
{
	int nb = is_utf8_upper(s, dict_locale);
	while (nb)
	{
		s += nb;
		nb = is_utf8_upper(s, dict_locale);
	}
	return s;
}

/**
 * Return true if the initial upper-case letters of the
 * two input strings match. Comparison stops when
 * both strings descend to lowercase.
 */
static inline bool utf8_upper_match(const char * s, const char * t,
                                    locale_t dict_locale)
{
	mbstate_t mbs, mbt;
	wchar_t ws, wt;
	int ns, nt;

	memset(&mbs, 0, sizeof(mbs));
	memset(&mbt, 0, sizeof(mbt));

	ns = mbrtowc(&ws, s, MB_CUR_MAX, &mbs);
	nt = mbrtowc(&wt, t, MB_CUR_MAX, &mbt);
	if (ns < 0 || nt < 0) return false;  /* invalid mb sequence */
	while (iswupper_l(ws, dict_locale) || iswupper_l(wt, dict_locale))
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
#endif /* Not in use. */

void downcase_utf8_str(char *to, const char * from, size_t usize, locale_t);
#if 0
void upcase_utf8_str(char *to, const char * from, size_t usize, locale_t);
#endif

size_t lg_strlcpy(char * restrict dst, const char * restrict src, size_t dsize);
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
void dyn_trimback(dyn_str*);
char * dyn_str_take(dyn_str*);
const char * dyn_str_value(dyn_str*);
size_t dyn_strlen(dyn_str*);

size_t altlen(const char **);

/* routines for allocating basic objects */
void init_memusage(void);
void * xalloc(size_t) GNUC_MALLOC;
void * exalloc(size_t) GNUC_MALLOC;

/* Tracking the space usage can help with debugging */
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
void set_utf8_program_locale(void);
bool try_locale(const char *);
bool strtodC(const char *, float *);

/**
 * Returns the smallest power of two that is at least i and at least 1
 */
static inline size_t next_power_of_two_up(size_t i)
{
	size_t j=1;
	while (j<i) j <<= 1;
	return j;
}

#endif /* _LINK_GRAMMAR_UTILITIES_H_ */
