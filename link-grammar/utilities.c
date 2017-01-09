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
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <locale.h>
#ifdef HAVE_LOCALE_T_IN_XLOCALE_H
#include <xlocale.h>
#endif /* HAVE_LOCALE_T_IN_XLOCALE_H */

#ifndef _WIN32
	#include <unistd.h>
	#include <langinfo.h>
#else
	#include <windows.h>
	#include <Shlwapi.h> /* For PathRemoveFileSpecA(). */
	#include <direct.h>  /* For getcwd(). */
#endif /* _WIN32 */

#ifdef USE_PTHREADS
	#include <pthread.h>
#endif /* USE_PTHREADS */

#include "string-set.h"
#include "structures.h"
#include "utilities.h"

#ifdef _WIN32
	#define DIR_SEPARATOR "\\"
#else
	#define DIR_SEPARATOR "/"
#endif /*_WIN32 */

#define IS_DIR_SEPARATOR(ch) (DIR_SEPARATOR[0] == (ch))
#if !defined(DICTIONARY_DIR) || defined(__MINGW32__)
	#define DEFAULTPATH NULL
#else
	#define DEFAULTPATH DICTIONARY_DIR
#endif

/* This file contains certain general utilities. */
int    verbosity;
/* debug and test should not be NULL since they can be used before they
 * are assigned a value by parse_options_get_...() */
char * debug = (char *)"";
char * test = (char *)"";

/* ============================================================= */
/* String utilities */

char *safe_strdup(const char *u)
{
	if (u)
		return strdup(u);
	return NULL;
}

/**
 * Copies as much of v into u as it can assuming u is of size usize
 * guaranteed to terminate u with a '\0'.
 */
void safe_strcpy(char *u, const char * v, size_t usize)
{
	strncpy(u, v, usize-1);
	u[usize-1] = '\0';
}

/**
 * A version of strlcpy, for those systems that don't have it.
 */
size_t lg_strlcpy(char * dest, const char *src, size_t size)
{
	size_t i=0;
	while ((i<size) && (src[i] != 0x0))
	{
		dest[i] = src[i];
		i++;
	}
	if (i < size) { dest[i] = 0x0; size = i; }
	else if (0 < size) { size --; dest[size] = 0x0;}
	return size;
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

/**
 * Prints string `s`, aligned to the left, in a field width `w`.
 * If the width of `s` is shorter than `w`, then the remainder of
 * field is padded with blanks (on the right).
 */
void left_print_string(FILE * fp, const char * s, int w)
{
	int width = w + strlen(s) - utf8_strwidth(s);
	fprintf(fp, "%-*s", width, s);
}

#ifndef HAVE_STRNDUP
/* Emulates glibc's strndup() */
char *
strndup (const char *str, size_t size)
{
	size_t len;
	char *result = (char *) NULL;

	if ((char *) NULL == str) return (char *) NULL;

	len = strlen (str);
	if (!len) return strdup ("");
	if (size > len) size = len;

	result = (char *) malloc ((size + 1) * sizeof (char));
	memcpy (result, str, size);
	result[size] = 0x0;
	return result;
}
#endif /* !HAVE_STRNDUP */

/* ============================================================= */
/* UTF8 utilities */

/** Returns length of UTF8 character.
 * Current algo is based on the first character only.
 * If pointer is not pointing at first char, no not a valid value, returns 0.
 * Returns 0 for NULL as well.
 */
int utf8_charlen(const char *xc)
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

/* Implemented n wcwidth.c */
extern int mk_wcwidth(wchar_t);

/**
 * Return the width, in text-column-widths, of the utf8-encoded
 * string.  This is needed when printing formatted strings.
 * European langauges will typically have widths equal to the
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
void downcase_utf8_str(char *to, const char * from, size_t usize, locale_t locale_t)
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
		prt_error("Error: Invalid UTF-8 string!");
		return;
	}
	c = towlower_l(c, locale_t);
	nbl = wctomb_check(low, c);

	/* Check for error on an in-place copy */
	if ((nbh < nbl) && (to == from))
	{
		/* I'm to lazy to fix this */
		prt_error("Error: can't downcase UTF-8 string!");
		return;
	}

	/* Downcase */
	for (i=0; i<nbl; i++) { to[i] = low[i]; }

	if ((nbh == nbl) && (to == from)) return;

	from += nbh;
	to += nbl;
	safe_strcpy(to, from, usize-nbl);
}

/**
 * Upcase the first letter of the word.
 * XXX FIXME This works 'most of the time', but is not technically correct.
 * This is because towlower() and towupper() are locale dependent, and also
 * because the byte-counts might not match up, e.g. German ß and SS.
 * The correct long-term fix is to use ICU or glib g_utf8_strup(), etc.
 */
void upcase_utf8_str(char *to, const char * from, size_t usize, locale_t locale_t)
{
	wchar_t c;
	int i, nbl, nbh;
	char low[MB_LEN_MAX];
	mbstate_t mbs;

	memset(&mbs, 0, sizeof(mbs));
	nbh = mbrtowc (&c, from, MB_CUR_MAX, &mbs);
	if (nbh < 0)
	{
		prt_error("Error: Invalid UTF-8 string!");
		return;
	}
	c = towupper_l(c, locale_t);
	nbl = wctomb_check(low, c);

	/* Check for error on an in-place copy */
	if ((nbh < nbl) && (to == from))
	{
		/* I'm to lazy to fix this */
		prt_error("Error: can't upcase UTF-8 string!");
		return;
	}

	/* Upcase */
	for (i=0; i<nbl; i++) { to[i] = low[i]; }

	if ((nbh == nbl) && (to == from)) return;

	from += nbh;
	to += nbl;
	safe_strcpy(to, from, usize-nbl);
}

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

#ifdef USE_PTHREADS
static pthread_key_t space_key;
static pthread_once_t space_key_once = PTHREAD_ONCE_INIT;

static void fini_memusage(void)
{
	space_t *s = (space_t *) pthread_getspecific(space_key);
	if (s)
	{
		free(s);
		pthread_setspecific(space_key, NULL);
	}
	pthread_key_delete(space_key);
	space_key = 0;
}

static void space_key_alloc(void)
{
	int rc = pthread_key_create(&space_key, free);
	if (0 == rc)
		atexit(fini_memusage);
}
#else
static space_t space;
#endif /* USE_PTHREADS */

static space_t * do_init_memusage(void)
{
	space_t *s;

#ifdef USE_PTHREADS
	s = (space_t *) malloc(sizeof(space_t));
	pthread_setspecific(space_key, s);
#else
	s = &space;
#endif  /* USE_PTHREADS */

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
#ifdef USE_PTHREADS
	pthread_once(&space_key_once, space_key_alloc);
#else
	static bool mem_inited = false;
	if (mem_inited) return;
	mem_inited = true;
#endif /* USE_PTHREADS */
	do_init_memusage();
}

static inline space_t *getspace(void)
{
#ifdef USE_PTHREADS
	space_t *s = pthread_getspecific(space_key);
	if (s) return s;
	return do_init_memusage();
#else
	return &space;
#endif /* USE_PTHREADS */
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
		prt_error("Fatal Error: Ran out of space. (int)");
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
		prt_error("Fatal Error: Ran out of space. (ext)");
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

/* =========================================================== */
/* File path and dictionary open routines below */

char * join_path(const char * prefix, const char * suffix)
{
	char * path;
	size_t path_len, prel;

	path_len = strlen(prefix) + 1 /* len(DIR_SEPARATOR) */ + strlen(suffix);
	path = (char *) malloc(path_len + 1);

	strcpy(path, prefix);

	/* Windows is allergic to multiple path separators, so append one
	 * only if the prefix isn't already terminated by a path sep.
	 */
	prel = strlen(path);
	if (0 < prel && path[prel-1] != DIR_SEPARATOR[0])
	{
		path[prel] = DIR_SEPARATOR[0];
		path[prel+1] = '\0';
	}
	strcat(path, suffix);

	return path;
}

/* global - but that's OK, since this is set only during initialization,
 * and is is thenceforth a read-only item. So it doesn't need to be
 * locked.
 */
static char * custom_data_dir = NULL;

void dictionary_set_data_dir(const char * path)
{
	if (custom_data_dir) free (custom_data_dir);
	custom_data_dir = safe_strdup(path);
}

char * dictionary_get_data_dir(void)
{
	char * data_dir = NULL;

	if (custom_data_dir != NULL) {
		data_dir = safe_strdup(custom_data_dir);
		return data_dir;
	}

#ifdef _WIN32
	/* Dynamically locate invocation directory of our program.
	 * Non-ASCII characters are not supported (files will not be found). */
	char prog_path[MAX_PATH_NAME];

	if (!GetModuleFileNameA(NULL, prog_path, sizeof(prog_path)))
	{
		prt_error("Warning: GetModuleFileName error %d", (int)GetLastError());
	}
	else
	{
		if (NULL == prog_path)
		{
			/* Can it happen? */
			prt_error("Warning: GetModuleFileName returned a NULL program path!");
		}
		else
		{
			if (!PathRemoveFileSpecA(prog_path))
			{
				prt_error("Warning: Cannot get directory from program path '%s'!",
				          prog_path);
			}
			else
			{
				/* Unconvertible characters are marked as '?' */
				const char *unsupported = (NULL != strchr(prog_path, '?')) ?
					" (containing unsupported character)" : "";

				lgdebug(D_USER_FILES, "Debug: Directory of executable: %s%s\n",
				        unsupported, prog_path);
				data_dir = safe_strdup(prog_path);
			}
		}
	}
#endif /* _WIN32 */

	return data_dir;
}

/**
 * Locate a data file and open it.
 *
 * This function is used to open a dictionary file or a word file,
 * or any associated data file (like a post process knowledge file).
 *
 * It works as follows.  If the file name begins with a "/", then
 * it's assumed to be an absolute file name and it tries to open
 * that exact file.
 *
 * Otherwise, it looks for the file in a sequence of directories, as
 * specified in the dictpath array, until it finds it.
 *
 * If it is still not found, it may be that the user specified a relative
 * path, so it tries to open the exact file.
 *
 * Associated data files are looked in the *same* directory in which the
 * first one was found (typically "en/4.0.dict").  The private static
 * "path_found" serves as a directory path cache which records where the
 * first file was found.  The goal here is to avoid insanity due to
 * user's fractured installs.
 * If the filename argument is NULL, the function just invalidates this
 * directory path cache.
 */
#define NOTFOUND(fp) ((NULL == (fp)) ? " (Not found)" : "")
void * object_open(const char *filename,
                   void * (*opencb)(const char *, const void *),
                   const void * user_data)
{
	static char *path_found; /* directory path cache */
	char *completename = NULL;
	void *fp = NULL;
	char *data_dir = NULL;
	const char **path = NULL;

	if (NULL == filename)
	{
		/* Invalidate the directory path cache */
		free(path_found);
		path_found = NULL;
		return NULL;
	}

	if (NULL == path_found)
	{
		data_dir = dictionary_get_data_dir();
		if (debug_level(D_USER_FILES))
		{
			char cwd[MAX_PATH_NAME];
			char *cwdp = getcwd(cwd, sizeof(cwd));
			printf("Debug: Current directory: %s\n", NULL == cwdp ? "NULL": cwdp);
			printf("Debug: Last-resort data directory: %s\n",
					  data_dir ? data_dir : "NULL");
		}
	}

	/* Look for absolute filename.
	 * Unix: starts with leading slash.
	 * Windows: starts with C:\  except that the drive letter may differ.
	 * Note that only native windows C library uses backslashes; mingw
	 * seems to use forward-slash, from what I can tell.
	 */
	if ((filename[0] == '/')
#ifdef _WIN32
		|| ((filename[1] == ':')
			 && ((filename[2] == '\\') || (filename[2] == '/')))
		|| (filename[0] == '\\') /* UNC path */
#endif /* _WIN32 */
	   )
	{
		/* opencb() returns NULL if the file does not exist. */
		fp = opencb(filename, user_data);
		lgdebug(D_USER_FILES, "Debug: Opening file %s%s\n", filename, NOTFOUND(fp));
	}
	else
	{
		/* A path list in which to search for dictionaries.
		 * path_found, data_dir or DEFAULTPATH may be NULL. */
		const char *dictpath[] =
		{
			path_found,
			".",
			"." DIR_SEPARATOR "data",
			"..",
			".." DIR_SEPARATOR "data",
			data_dir,
			DEFAULTPATH,
		};
		size_t i = sizeof(dictpath)/sizeof(dictpath[0]);

		for (path = dictpath; i-- > 0; path++)
		{
			if (NULL == *path) continue;

			free(completename);
			completename = join_path(*path, filename);
			fp = opencb(completename, user_data);
			lgdebug(D_USER_FILES, "Debug: Opening file %s%s\n", completename, NOTFOUND(fp));
			if ((NULL != fp) || (NULL != path_found)) break;
		}
	}

	if (NULL == fp)
	{
		fp = opencb(filename, user_data);
		lgdebug(D_USER_FILES, "Debug: Opening file %s%s\n", filename, NOTFOUND(fp));
	}
	else if (NULL == path_found)
	{
		size_t i;

		path_found = strdup((NULL != completename) ? completename : filename);
		if (0 < verbosity)
			prt_error("Info: Dictionary found at %s", path_found);
		for (i = 0; i < 2; i++)
		{
			char *root = strrchr(path_found, DIR_SEPARATOR[0]);
			if (NULL != root) *root = '\0';
		}
	}

	free(data_dir);
	free(completename);
	return fp;
}
#undef NOTFOUND

static void *dict_file_open(const char *fullname, const void *how)
{
	return fopen(fullname, how);
}

FILE *dictopen(const char *filename, const char *how)
{
	return object_open(filename, dict_file_open, how);
}

/* ======================================================== */

/**
 * Check to see if a file exists.
 */
bool file_exists(const char * dict_name)
{
	bool retval = false;
	int fd;
	struct stat buf;

	/* On Windows, 'b' (binary mode) is mandatory, otherwise fstat file length
	 * is confused by crlf counted as one byte. POSIX systems just ignore it. */
	FILE *fp = dictopen(dict_name, "rb");

	if (fp == NULL)
		return false;

	/* Get the file size, in bytes. */
	fd = fileno(fp);
	fstat(fd, &buf);
	if (0 < buf.st_size) retval = true;

	fclose(fp);
	return retval;
}

/**
 * Read in the whole stinkin file. This routine returns
 * malloced memory, which should be freed as soon as possible.
 */
char *get_file_contents(const char * dict_name)
{
	int fd;
	size_t tot_size;
	int left;
	struct stat buf;
	char * contents, *p;

	/* On Windows, 'b' (binary mode) is mandatory, otherwise fstat file length
	 * is confused by crlf counted as one byte. POSIX systems just ignore it. */
	FILE *fp = dictopen(dict_name, "rb");

	if (fp == NULL)
		return NULL;

	/* Get the file size, in bytes. */
	fd = fileno(fp);
	fstat(fd, &buf);
	tot_size = buf.st_size;

	contents = (char *) malloc(sizeof(char) * (tot_size+7));

	/* Now, read the whole file. */
	p = contents;
	*p = '\0';
	left = tot_size + 7;
	while (1)
	{
		char *rv = fgets(p, left, fp);
		if (NULL == rv || feof(fp))
			break;
		while (*p != '\0') { p++; left--; }
		if (left < 0)
			 break;
	}

	fclose(fp);

	if (left < 0)
	{
		prt_error("Error: File size is insane!");
		free(contents);
		return NULL;
	}

	return contents;
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
						 "force-setting to en_US.UTF-8", locale, codeset);
		}
		locale = setlocale(LC_CTYPE, "en_US.UTF-8");
		if (NULL == locale)
		{
			prt_error("Warning: Program locale en_US.UTF-8 could not be set; "
			          "force-setting to C.UTF-8");
			locale = setlocale(LC_CTYPE, "C.UTF-8");
			if (NULL == locale)
			{
				prt_error("Warning: Could not set a UTF-8 program locale; "
				          "program may malfunction");
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
		          "Error %d", (int)lcid, (int)GetLastError());
		return NULL;
	}
	strcpy(locale, lbuf);
	strcat(locale, "-");

	if (0 >= GetLocaleInfoA(lcid, LOCALE_SISO3166CTRYNAME, lbuf, sizeof(lbuf)))
	{
		prt_error("Error: GetLocaleInfoA LOCALE_SISO3166CTRYNAME LCID=%d: "
		          "Error %d", (int)lcid, (int)GetLastError());
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

/* ============================================================= */
/* Alternatives utilities */

static const char ** resize_alts(const char **arr, size_t len)
{
	arr = realloc(arr, (len+2) * sizeof(char *));
	arr[len+1] = NULL;
	return arr;
}

size_t altlen(const char **arr)
{
	size_t len = 0;
	if (arr)
		while (arr[len] != NULL) len++;
	return len;
}

void altappend(Sentence sent, const char ***altp, const char *w)
{
	size_t n = altlen(*altp);

	*altp = resize_alts(*altp, n);
	(*altp)[n] = string_set_add(w, sent->string_set);
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
