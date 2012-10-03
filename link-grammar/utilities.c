/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright 2008, 2009 Linas Vepstas                                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
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
#include <unistd.h>


#ifdef USE_PTHREADS
#include <pthread.h>
#endif

#include "api.h"

#ifdef ENABLE_BINRELOC
#include "prefix.h"
#endif /* BINRELOC */

#ifdef _WIN32
#  include <windows.h>
#  define DIR_SEPARATOR '\\'
#  define PATH_SEPARATOR ';'
#else
#  define DIR_SEPARATOR '/'
#  define PATH_SEPARATOR ':'
#endif

#define IS_DIR_SEPARATOR(ch) (DIR_SEPARATOR == (ch))
#ifdef _MSC_VER
#define DICTIONARY_DIR "."
#endif
#define DEFAULTPATH DICTIONARY_DIR

/* This file contains certain general utilities. */
int   verbosity;

/* ============================================================= */
/* String utilities */

char *safe_strdup(const char *u)
{
	if(u)
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
 * prints s then prints the last |t|-|s| characters of t.
 * if s is longer than t, it truncates s.
 */
void left_print_string(FILE * fp, const char * s, const char * t)
{
	int i, j, k;
	j = strlen(t);
	k = strlen(s);
	for (i=0; i<j; i++) {
		if (i<k) {
			fprintf(fp, "%c", s[i]);
		} else {
			fprintf(fp, "%c", t[i]);
		}
	}
}

#ifdef _WIN32  /* should be !defined(HAVE_STRTOK_R) */

char *
strtok_r (char *s, const char *delim, char **saveptr)
{
	char *p;

	if (s == NULL)
		s = *saveptr;

	if (s == NULL)
		return NULL;

	/* Skip past any delimiters. */
	/* while (*s && strchr (delim, *s)) s++; */
	s += strspn(s, delim);

	if (*s == '\0')
	{
		*saveptr = NULL;
		return NULL;
	}

	/* Look for end of the token. */
	/* p = s; while (*p && !strchr (delim, *p)) p++; */
	p = strpbrk(s, delim);
	if (p == NULL)
	{
		*saveptr = NULL;
		return s;
	}

	*p = 0x0;
	*saveptr = p+1;

	return s;
}

#endif /* _WIN32 should be !HAVE_STROTOK_R */

#ifdef _WIN32  /* should be !defined(HAVE_STRNDUP) */

/* Emulates glibc's strndup() */
char *
strndup (char *str, size_t size)
{
	char *result = (char *) NULL;

	if ((char *) NULL == str) return (char *) NULL;

	size_t len = strlen (str);
	if (!len) return strdup ("");
	if (size > len) size = len;

	result = (char *) malloc ((size + 1) * sizeof (char));
	memcpy (result, str, size);
	result[size] = 0x0;
	return result;
}

#endif /* _WIN32 should be !HAVE_STRNDUP */

/* ============================================================= */
/* UTF8 utilities */

/**
 * Downcase the first letter of the word.
 */
void downcase_utf8_str(char *to, const char * from, size_t usize)
{
	wchar_t c;
	int i, nbl, nbh;
	char low[MB_LEN_MAX];
	mbstate_t mbss;

	nbh = mbtowc (&c, from, MB_CUR_MAX);
	c = towlower(c);
	memset(&mbss, 0, sizeof(mbss));
	nbl = wctomb_check(low, c, &mbss);

	/* Check for error on an in-place copy */
	if ((nbh < nbl) && (to == from))
	{
		/* I'm to lazy to fix this */
		prt_error("Error: can't downcase multi-byte string!\n");
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
 */
void upcase_utf8_str(char *to, const char * from, size_t usize)
{
	wchar_t c;
	int i, nbl, nbh;
	char low[MB_LEN_MAX];
	mbstate_t mbss;

	nbh = mbtowc (&c, from, MB_CUR_MAX);
	c = towupper(c);
	memset(&mbss, 0, sizeof(mbss));
	nbl = wctomb_check(low, c, &mbss);

	/* Check for error on an in-place copy */
	if ((nbh < nbl) && (to == from))
	{
		/* I'm to lazy to fix this */
		prt_error("Error: can't upcase multi-byte string!\n");
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
	size_t max_external_space_used;
	size_t external_space_in_use;
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
	s->max_external_space_used = 0;
	s->external_space_in_use = 0;

	return s;
}

void init_memusage(void)
{
#ifdef USE_PTHREADS
	pthread_once(&space_key_once, space_key_alloc);
#else
	static int mem_inited = FALSE;
	if (mem_inited) return;
	mem_inited = TRUE;
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
#endif /* TRACK_SPACE_USAGE */
	if ((p == NULL) && (size != 0))
	{
		prt_error("Fatal Error: Ran out of space.\n");
		abort();
		exit(1);
	}
	return p;
}

void * xrealloc(void *p, size_t oldsize, size_t newsize)
{
#ifdef TRACK_SPACE_USAGE
	space_t *s = getspace();
	s->space_in_use -= oldsize;
#endif /* TRACK_SPACE_USAGE */
	p = realloc(p, newsize);
	if ((p == NULL) && (newsize != 0))
	{
		prt_error("Fatal Error: Ran out of space on realloc.\n");
		abort();
		exit(1);
	}
#ifdef TRACK_SPACE_USAGE
	s->space_in_use += newsize;
	if (s->max_space_used < s->space_in_use) s->max_space_used = s->space_in_use;
#endif /* TRACK_SPACE_USAGE */
	return p;
}

#ifdef TRACK_SPACE_USAGE
void xfree(void * p, size_t size)
{
	getspace()->space_in_use -= size;
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
#endif /* TRACK_SPACE_USAGE */

	if ((p == NULL) && (size != 0))
	{
		prt_error("Fatal Error: Ran out of space.\n");
		abort();
		exit(1);
	}
	return p;
}

#ifdef TRACK_SPACE_USAGE
void exfree(void * p, size_t size)
{
	getspace()->external_space_in_use -= size;
	free(p);
}
#endif /* TRACK_SPACE_USAGE */

/* =========================================================== */
/* File path and dictionary open routines below */

char * join_path(const char * prefix, const char * suffix)
{
	char * path;
	int path_len;

	path_len = strlen(prefix) + 1 /* len(DIR_SEPARATOR) */ + strlen(suffix);
	path = (char *) malloc(path_len + 1);

	strcpy(path, prefix);
	path[strlen(path)+1] = '\0';
	path[strlen(path)] = DIR_SEPARATOR;
	strcat(path, suffix);

	return path;
}

#ifdef _WIN32
/* borrowed from glib */
/* Used only for Windows builds */
static char*
path_get_dirname (const char *file_name)
{
	register char *base;
	register int len;

	base = strrchr (file_name, DIR_SEPARATOR);
#ifdef _WIN32
	{
		char *q = strrchr (file_name, '/');
		if (base == NULL || (q != NULL && q > base))
			base = q;
	}
#endif
	if (!base)
	{
#ifdef _WIN32
		if (is_utf8_alpha (file_name) && file_name[1] == ':')
		{
			char drive_colon_dot[4];

			drive_colon_dot[0] = file_name[0];
			drive_colon_dot[1] = ':';
			drive_colon_dot[2] = '.';
			drive_colon_dot[3] = '\0';

			return safe_strdup (drive_colon_dot);
		}
#endif
		return safe_strdup (".");
	}

	while (base > file_name && IS_DIR_SEPARATOR (*base))
		base--;

#ifdef _WIN32
	/* base points to the char before the last slash.
	 *
	 * In case file_name is the root of a drive (X:\) or a child of the
	 * root of a drive (X:\foo), include the slash.
	 *
	 * In case file_name is the root share of an UNC path
	 * (\\server\share), add a slash, returning \\server\share\ .
	 *
	 * In case file_name is a direct child of a share in an UNC path
	 * (\\server\share\foo), include the slash after the share name,
	 * returning \\server\share\ .
	 */
	if (base == file_name + 1 && is_utf8_alpha (file_name) && file_name[1] == ':')
		base++;
	else if (IS_DIR_SEPARATOR (file_name[0]) &&
	         IS_DIR_SEPARATOR (file_name[1]) &&
	         file_name[2] &&
	         !IS_DIR_SEPARATOR (file_name[2]) &&
	         base >= file_name + 2)
	{
		const char *p = file_name + 2;
		while (*p && !IS_DIR_SEPARATOR (*p))
			p++;
		if (p == base + 1)
		{
			len = (int) strlen (file_name) + 1;
			base = (char *)malloc(len + 1);
			strcpy (base, file_name);
			base[len-1] = DIR_SEPARATOR;
			base[len] = 0;
			return base;
		}
		if (IS_DIR_SEPARATOR (*p))
		{
			p++;
			while (*p && !IS_DIR_SEPARATOR (*p))
				p++;
			if (p == base + 1)
				base++;
		}
	}
#endif

	len = (int) 1 + base - file_name;

	base = (char *)malloc(len + 1);
	memmove (base, file_name, len);
	base[len] = 0;

	return base;
}
#endif /* _WIN32 */

/* global - but thats OK, since this is set only during initialization,
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
#ifdef _WIN32
	HINSTANCE hInstance;
#endif
	char * data_dir = NULL;

	if (custom_data_dir != NULL) {
		data_dir = safe_strdup(custom_data_dir);
		return data_dir;
	}
	
#ifdef ENABLE_BINRELOC
	data_dir = safe_strdup (BR_DATADIR("/link-grammar"));
#elif defined(_WIN32)
	/* Dynamically locate library and return containing directory */
	hInstance = GetModuleHandle("link-grammar.dll");
	if (hInstance != NULL)
	{
		char dll_path[MAX_PATH];

		if (GetModuleFileName(hInstance, dll_path, MAX_PATH))
		{
#ifdef _DEBUG
			prt_error("Info: GetModuleFileName=%s\n", (dll_path ? dll_path : "NULL"));
#endif
			data_dir = path_get_dirname(dll_path);
		}
	}
	else
	{
		/* BHayes added else block for when link-grammar.dll is not found 11apr11 */
		/* This requests module handle for the current program */
		hInstance = GetModuleHandle(NULL);
		if (hInstance != NULL)
		{
			char prog_path16[MAX_PATH];
			printf("  found ModuleHandle for current program so try to get Filename for current program \n");
			if (GetModuleFileName(hInstance, prog_path16, MAX_PATH))
			{
				char * data_dir16 = NULL;
				// convert 2-byte to 1-byte encoding of prog_path
				char prog_path[MAX_PATH/2];
				int i, k;
				for (i = 0; i < MAX_PATH; i++)
				{
					k = i + i;
					if (prog_path16[k] == 0 )
					{
						prog_path[i] = 0;
						break; // found the string ending null char
					}
					// XXX this cannot possibly be correct for UTF-16 filepaths!! ?? FIXME
					prog_path[i] = prog_path16[k];
				}
#ifdef _DEBUG
				prt_error("Info: GetModuleFileName=%s\n", (prog_path ? prog_path : "NULL"));
#endif
				data_dir16 = path_get_dirname(prog_path);
				if (data_dir16 != NULL)
				{
					printf("   found dir for current prog %s\n", data_dir16);
				}
				return data_dir16;  // return path of data directory here instead of below
			} else {
				printf("   FAIL GetModuleFileName for current program \n");
			}
		}
   }  /* BHayes - end added else block */
#endif

	return data_dir;
}

/**
 * object_open() -- dictopen() - open a dictionary
 *
 * This function is used to open a dictionary file or a word file,
 * or any associated data file (like a post process knowledge file).
 *
 * It works as follows.  If the file name begins with a "/", then
 * it's assumed to be an absolute file name and it tries to open
 * that exact file.
 *
 * If the filename does not begin with a "/", then it uses the
 * dictpath mechanism to find the right file to open.  This looks
 * for the file in a sequence of directories until it finds it.  The
 * sequence of directories is specified in a dictpath string, in
 * which each directory is followed by a ":".
 */
void * object_open(const char *filename,
                   void * (*opencb)(const char *, void *),
                   void * user_data)
{
	char completename[MAX_PATH_NAME+1];
	char fulldictpath[MAX_PATH_NAME+1];
	static char prevpath[MAX_PATH_NAME+1] = "";
	static int first_time_ever = 1;
	char *pos, *oldpos;
	void *fp;

	/* Record the first path ever used, so that we can recycle it */
	if (first_time_ever)
	{
		strncpy (prevpath, filename, MAX_PATH_NAME);
		prevpath[MAX_PATH_NAME] = 0;
		pos = strrchr(prevpath, DIR_SEPARATOR);
		if (pos) *pos = 0;
		pos = strrchr(prevpath, DIR_SEPARATOR);
		if (pos) *(pos+1) = 0;
		first_time_ever = 0;
	}

	/* Look for absolute filename.
	 * Unix: starts with leading slash.
	 * Windows: starts with C:\  except that the drive letter may differ.
	 */
	if ((filename[0] == '/') || ((filename[1] == ':') && (filename[2] == '\\')))
	{
		/* fopen returns NULL if the file does not exist. */
		fp = opencb(filename, user_data);
		if (fp) return fp;
	}

	{
		char * data_dir = dictionary_get_data_dir();
#ifdef _DEBUG
		prt_error("Info: data_dir=%s\n", (data_dir?data_dir:"NULL"));
#endif
		if (data_dir) {
			snprintf(fulldictpath, MAX_PATH_NAME,
			         "%s%c%s%c", data_dir, PATH_SEPARATOR,
			                    DEFAULTPATH, PATH_SEPARATOR);
			free(data_dir);
		}
		else {
			/* Always make sure that it ends with a path
			 * separator char for the below while() loop.
			 * For unix, this should look like:
			 * /usr/share/link-grammar:.:data:..:../data:
			 * For windows:
			 * C:\SOMWHERE;.;data;..;..\data;
			 */
			snprintf(fulldictpath, MAX_PATH_NAME,
			         "%s%c%s%c%s%c%s%c%s%c%s%c%s%c",
				 prevpath, PATH_SEPARATOR,
				 DEFAULTPATH, PATH_SEPARATOR,
				 ".", PATH_SEPARATOR,
				 "data", PATH_SEPARATOR,
				 "..", PATH_SEPARATOR,
				 "..", DIR_SEPARATOR, "data", PATH_SEPARATOR);
		}
	}

	/* Now fulldictpath is our dictpath, where each entry is
	 * followed by a ":" including the last one */

	oldpos = fulldictpath;
	while ((pos = strchr(oldpos, PATH_SEPARATOR)) != NULL)
	{
		strncpy(completename, oldpos, (pos-oldpos));
		*(completename+(pos-oldpos)) = DIR_SEPARATOR;
		strcpy(completename+(pos-oldpos)+1, filename);
#ifdef _DEBUG
		prt_error("Info: object_open() trying %s\n", completename);
#endif
		if ((fp = opencb(completename, user_data)) != NULL) {
			return fp;
		}
		oldpos = pos+1;
	}
	return NULL;
}

/* XXX static global variable used during dictionary open */
static char *path_found = NULL;

static void * dict_file_open(const char * fullname, void * user_data)
{
	const char * how = (const char *) user_data;
	FILE * fh =  fopen(fullname, how);
	if (fh && NULL == path_found)
	{
		path_found = strdup (fullname);
		prt_error("Info: Dictionary found at %s\n", fullname);
	}
	return (void *) fh;
}

FILE *dictopen(const char *filename, const char *how)
{
	FILE * fh = NULL;
	void * ud = (void *) how;

	/* If not the first time through, look for the other dictionaries
	 * in the *same* directory in which the first one was found.
	 * (The first one is typcailly "en/4.0.dict")
	 * The global "path_found" records where the first dict was found.
	 * The goal here is to avoid fractured install insanity.
	 */
	if (path_found)
	{
		size_t sz = strlen (path_found) + strlen(filename) + 1;
		char * fullname = (char *) malloc (sz);
		strcpy(fullname, path_found);
		strcat(fullname, filename);
		fh = (FILE *) object_open(fullname, dict_file_open, ud);
		free(fullname);
	}
	else
	{
		fh = (FILE *) object_open(filename, dict_file_open, ud);
		if (path_found)
		{
			char * root = strstr(path_found, filename);
			*root = 0;
		}
	}
	return fh;
}

/* ======================================================== */

/**
 * Read in the whole stinkin file.
 * We read it as wide chars, one char at a time, so that any LC_CTYPE
 * setting are obeyed.  Later, during dictionary grokking, thesse will
 * be converted to the internal UTF8 format.
 */
wchar_t *get_file_contents(const char * dict_name)
{
	int fd;
	size_t tot_size;
	int left;
	struct stat buf;
	wchar_t * contents, *p;

	FILE *fp = dictopen(dict_name, "r");
	if (fp == NULL)
		return NULL;

	/* Get the file size, in bytes. */
	fd = fileno(fp);
	fstat(fd, &buf);
	tot_size = buf.st_size;

	/* This is surely more than we need. But that's OK, better
	 * too much than too little.  */
	contents = (wchar_t *) malloc(sizeof(wchar_t) * (tot_size+1));

	/* Now, read the whole file. */
	p = contents;
	left = tot_size + 1;
	while (1)
	{
		wchar_t *rv = fgetws(p, left, fp);
		if (NULL == rv || feof(fp))
			break;
		while (*p != 0x0) { p++; left--; }
		if (left < 0)
			 break;
	}

  	fclose(fp);

	if (left < 0)
	{
		prt_error("Fatal Error: File size is insane!\n");
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
	LCID lcid;
	LANGID langid;
	char *ev;
	int primary, sub;
	char bfr[64];
	char iso639[10];
	char iso3166[10];
	const char *script = NULL;

	/* Let the user override the system settings through environment
	 * variables, as on POSIX systems. Note that in GTK+ applications
	 * since GTK+ 2.10.7 setting either LC_ALL or LANG also sets the
	 * Win32 locale and C library locale through code in gtkmain.c.
	 */
	if (((ev = getenv ("LC_ALL")) != NULL && ev[0] != '\0')
	 || ((ev = getenv ("LC_MESSAGES")) != NULL && ev[0] != '\0')
	 || ((ev = getenv ("LANG")) != NULL && ev[0] != '\0'))
		return safe_strdup (ev);

	lcid = GetThreadLocale ();

	if (!GetLocaleInfo (lcid, LOCALE_SISO639LANGNAME, iso639, sizeof (iso639)) ||
	    !GetLocaleInfo (lcid, LOCALE_SISO3166CTRYNAME, iso3166, sizeof (iso3166)))
		return safe_strdup ("C");

	/* Strip off the sorting rules, keep only the language part.	*/
	langid = LANGIDFROMLCID (lcid);

	/* Split into language and territory part.	*/
	primary = PRIMARYLANGID (langid);
	sub = SUBLANGID (langid);

	/* Handle special cases */
	switch (primary)
	{
		case LANG_AZERI:
			switch (sub)
			{
				case SUBLANG_AZERI_LATIN:
					script = "@Latn";
					break;
				case SUBLANG_AZERI_CYRILLIC:
					script = "@Cyrl";
					break;
			}
			break;
		case LANG_SERBIAN:								/* LANG_CROATIAN == LANG_SERBIAN */
			switch (sub)
			{
				case SUBLANG_SERBIAN_LATIN:
				case 0x06: /* Serbian (Latin) - Bosnia and Herzegovina */
					script = "@Latn";
					break;
			}
			break;
		case LANG_UZBEK:
			switch (sub)
			{
				case SUBLANG_UZBEK_LATIN:
					script = "@Latn";
					break;
				case SUBLANG_UZBEK_CYRILLIC:
					script = "@Cyrl";
					break;
			}
			break;
	}

	strcat (bfr, iso639);
	strcat (bfr, "_");
	strcat (bfr, iso3166);

	if (script)
		strcat (bfr, script);

	return safe_strdup (bfr);
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

/* ========================== END OF FILE =================== */
