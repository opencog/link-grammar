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
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>

#ifndef _WIN32
	#include <unistd.h>
#else
	#include <windows.h>
	#include <Shlwapi.h> /* For PathRemoveFileSpecA(). */
	#include <direct.h>  /* For getcwd(). */
#endif /* _WIN32 */

#ifdef USE_PTHREADS
	#include <pthread.h>
#endif

#include "string-set.h"
#include "structures.h"
#include "utilities.h"

#ifdef _WIN32
	#define DIR_SEPARATOR "\\"
#else
	#define DIR_SEPARATOR "/"
#endif

#define IS_DIR_SEPARATOR(ch) (DIR_SEPARATOR[0] == (ch))
#ifndef DICTIONARY_DIR
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
 * Prints s in a field width of string ti, with no truncation.
 * FIXME: make t a number.
 * (In a previous version of this function s got truncated to the
 * field width.)
 */
void left_print_string(FILE * fp, const char * s, const char * t)
{
	int width = strlen(t) + strlen(s) - utf8_strlen(s);
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

#if defined(_MSC_VER) || defined(__MINGW32__)

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
	if (0 == nb2) return 0;
	return nb;
}
#endif /* defined(_MSC_VER) || defined(__MINGW32__) */

static int wctomb_check(char *s, wchar_t wc)
{
	int nr;
#if defined(_MSC_VER) || defined(__MINGW32__)
	nr = WideCharToMultiByte(CP_UTF8, 0, &wc, 1, NULL, 0, NULL, NULL);
	nr = WideCharToMultiByte(CP_UTF8, 0, &wc, 1, s, nr, NULL, NULL);
	if (nr < 0) {
		wprintf(L"Fatal Error: wctomb_check failed: %d %ls\n", nr, &wc);
		exit(1);
	}
	/* XXX TODO Perhaps the below needs to be uncommented?  .. tucker says no ... */
	/* nr = nr / sizeof(wchar_t); */
#else
	mbstate_t mbss;
	memset(&mbss, 0, sizeof(mbss));
	nr = wcrtomb(s, wc, &mbss);
	if (nr < 0) {
		prt_error("Fatal Error: unknown character set %s\n", nl_langinfo(CODESET));
		exit(1);
	}
#endif
	return nr;
}

/**
 * Downcase the first letter of the word.
 * XXX FIXME This works 'most of the time', but is not technically correct.
 * This is because towlower() and towupper() are locale dependent, and also
 * because the byte-counts might not match up, e.g. German ß and SS.
 * The correct long-term fix is to use ICU or glib g_utf8_strup(), etc.
 */
void downcase_utf8_str(char *to, const char * from, size_t usize)
{
	wchar_t c;
	int i, nbl, nbh;
	char low[MB_LEN_MAX];
	mbstate_t mbs;

	to[0] = '\0'; /* Make sure it doesn't contain garbage in case of an error */
	memset(&mbs, 0, sizeof(mbs));
	nbh = mbrtowc (&c, from, MB_CUR_MAX, &mbs);
	if (nbh < 0)
	{
		prt_error("Error: Invalid multi-byte string!");
		return;
	}
	c = towlower(c);
	nbl = wctomb_check(low, c);

	/* Check for error on an in-place copy */
	if ((nbh < nbl) && (to == from))
	{
		/* I'm to lazy to fix this */
		prt_error("Error: can't downcase multi-byte string!");
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
void upcase_utf8_str(char *to, const char * from, size_t usize)
{
	wchar_t c;
	int i, nbl, nbh;
	char low[MB_LEN_MAX];
	mbstate_t mbs;

	memset(&mbs, 0, sizeof(mbs));
	nbh = mbrtowc (&c, from, MB_CUR_MAX, &mbs);
	if (nbh < 0)
	{
		prt_error("Error: Invalid multi-byte string!");
		return;
	}
	c = towupper(c);
	nbl = wctomb_check(low, c);

	/* Check for error on an in-place copy */
	if ((nbh < nbl) && (to == from))
	{
		/* I'm to lazy to fix this */
		prt_error("Error: can't upcase multi-byte string!");
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
#endif

static space_t * do_init_memusage(void)
{
	space_t *s;

#ifdef USE_PTHREADS
	s = (space_t *) malloc(sizeof(space_t));
	pthread_setspecific(space_key, s);
#else
	s = &space;
#endif

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
#endif
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
#endif
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

#if defined(_WIN32)
	/* Dynamically locate invocation directory of our program.
	 * Non-ASCII characters are not supported (files will not be found). */
	char prog_path[MAX_PATH_NAME];

	if (!GetModuleFileNameA(NULL, prog_path, sizeof(prog_path)))
	{
		prt_error("Warning: GetModuleFileName error %d\n", GetLastError());
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
				char *unsuppored = (NULL != strchr(prog_path, '?')) ?
					" (containing unsupported character)" : "";

				lgdebug(D_USER_FILES, "Info: Directory of executable: %s%s\n",
				        unsuppored, prog_path);
				data_dir = safe_strdup(prog_path);
			}
		}
	}
#endif

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
			printf("Info: Current directory: %s\n", NULL == cwdp ? "NULL": cwdp);
			printf("Info: Last-resort data directory: %s\n",
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
#if defined(_WIN32) || defined(_MSC_VER) || defined(__MINGW32__)
		|| ((filename[1] == ':')
			 && ((filename[2] == '\\') || (filename[2] == '/')))
		|| (filename[0] == '\\') /* UNC path */
#endif
	   )
	{
		/* opencb() returns NULL if the file does not exist. */
		fp = opencb(filename, user_data);
		lgdebug(D_USER_FILES, "Info: Opening file %s%s\n", filename, NOTFOUND(fp));
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
			lgdebug(D_USER_FILES, "Info: Opening file %s%s\n", completename, NOTFOUND(fp));
			if ((NULL != fp) || (NULL != path_found)) break;
		}
	}

	if (NULL == fp)
	{
		fp = opencb(filename, user_data);
		lgdebug(D_USER_FILES, "Info: Opening file %s%s\n", filename, NOTFOUND(fp));
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
	left = tot_size + 7;
	while (1)
	{
		char *rv = fgets(p, left, fp);
		if (NULL == rv || feof(fp))
			break;
		while (*p != 0x0) { p++; left--; }
		if (left < 0)
			 break;
	}

	fclose(fp);

	if (left < 0)
	{
		prt_error("Fatal Error: File size is insane!");
		exit(1);
	}

	return contents;
}


/* ======================================================== */
/* Locale routines */

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
		          "Error %d", lcid, GetLastError());
		return NULL;
	}
	strcpy(locale, lbuf);
	strcat(locale, "-");

	if (0 >= GetLocaleInfoA(lcid, LOCALE_SISO3166CTRYNAME, lbuf, sizeof(lbuf)))
	{
		prt_error("Error: GetLocaleInfoA LOCALE_SISO3166CTRYNAME LCID=%d: "
		          "Error %d", lcid, GetLastError());
		return NULL;
	}
	strcat(locale, lbuf);

	return strdup(locale);
}
#endif

char * get_default_locale(void)
{
	char * locale, * needle;

	locale = NULL;

#ifdef _WIN32
	if(!locale)
		locale = win32_getlocale ();
#endif

	if(!locale)
		locale = safe_strdup (getenv ("LANG"));

#if defined(HAVE_LC_MESSAGES)
	if(!locale)
		locale = safe_strdup (setlocale (LC_MESSAGES, NULL));
#endif

	if(!locale)
		locale = safe_strdup (setlocale (LC_ALL, NULL));

	if(!locale || strcmp(locale, "C") == 0) {
		free(locale);
		locale = safe_strdup("en");
	}

	/* strip off "@euro" from en_GB@euro */
	if ((needle = strchr (locale, '@')) != NULL)
		*needle = '\0';

	/* strip off ".UTF-8" from en_GB.UTF-8 */
	if ((needle = strchr (locale, '.')) != NULL)
		*needle = '\0';

	/* strip off "_GB" from en_GB */
	if ((needle = strchr (locale, '_')) != NULL)
		*needle = '\0';

	return locale;
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
