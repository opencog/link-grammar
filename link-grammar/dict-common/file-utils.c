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

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>                   // fstat

#ifndef _MSC_VER
	#include <unistd.h>
#else
	#include <windows.h>
	#include <Shlwapi.h>                 // PathRemoveFileSpecA
	#include <direct.h>                  // getcwd
#endif /* _MSC_VER */

#include <stdlib.h>
#include <string.h>

#include "file-utils.h"
#include "error.h"                      // verbosity_level
#include "link-includes.h"
#include "utilities.h"                  // lg_strerror_r

#ifdef _WIN32
	#define DIR_SEPARATOR "\\"
#else
	#define DIR_SEPARATOR "/"
#endif /*_WIN32 */

#ifndef DICTIONARY_DIR
#define DICTIONARY_DIR NULL
#endif

/* =========================================================== */
/* File path and dictionary open routines below */

#define MAX_PATH_NAME 200     /* file names (including paths)
                                 should not be longer than this */

/**
 * Find the location of the last directory separator (Windows/POSIX).
 * Return NULL if none found.
 * It doesn't modify its argument, but const is not used because
 * it is to be called also with non-const argument.
 */
char *find_last_dir_separator(char *path)
{
	char *dirsep = NULL;
	size_t pathlen = strlen(path);

	for (size_t p = pathlen; p > 0; p--)
		if (('/' == path[p]) || ('\\' == path[p])) return &path[p];

	return dirsep;
}

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
	if (0 < prel && (path[prel-1] != '/') && (path[prel-1] != '\\'))
	{
		path[prel] = '/';
		path[prel+1] = '\0';
	}
	strcat(path, suffix);

	return path;
}

/* global - but that's OK, since this is set only during initialization,
 * and is is thenceforth a read-only item. So it doesn't need to be
 * locked.  XXX Assuming the user actually does set it only once,
 * and doesn't hammer on this inside of a thread loop, in which case
 * it will race and abort with a double-free error.
 */
static char * custom_data_dir = NULL;

static void free_custom_data_dir(void) {
	free(custom_data_dir);
}

void dictionary_set_data_dir(const char * path)
{
	if (custom_data_dir)
		free(custom_data_dir);
	else
		atexit(free_custom_data_dir);
	custom_data_dir = safe_strdup(path);
}

char * dictionary_get_data_dir(void)
{
	char * data_dir = NULL;

	if (custom_data_dir != NULL) {
		data_dir = safe_strdup(custom_data_dir);
		return data_dir;
	}

	return NULL;
}

#ifdef _MSC_VER
static const char *get_dictionary_dir(bool);
static void free_dictionary_dir(void)
{
	get_dictionary_dir(false);
}
#endif // _MSC_VER

/**
 * Return the path of the system dictionary directory.
 * If DICTIONARY_DIR is an absolute path (must be so on systems other than
 * Windows) then it is directly used.
 * On Windows (currently implemented on MSVC only), a relative
 * DICTIONARY_DIR is combined with the location of this library DLL, and
 * if it includes a drive letter or is a UNC path then it must be
 * absolute. It shouldn't end with a directory separator.
 * @param find If true, find the system dictionary directory; else free it
 * if needed.
 * @return The actual dictionary directory.
 */
static const char *get_dictionary_dir(bool find)
{
#ifndef _MSC_VER
		return DICTIONARY_DIR;
#else
	/* We are on Windows. DICTIONARY_DIR, if defined, can be relative or
	 * absolute. An absolute path must start with a drive letter or be a
	 * UNC path, and a relative path should not start with a drive letter
	 * or be a UNC path.
	 * In case it is not so, the result is undefined.  If it is a relative
	 * path, it is concatenated to the program location. */
	static const char *dictionary_dir;

	if (!find) {
		free((void *)dictionary_dir);
		return NULL;
	}

	/* If we already found it, just return it. */
	if (NULL != dictionary_dir) return dictionary_dir;

	dictionary_dir = DICTIONARY_DIR;
	/* If it is NULL or "", assume it is relative to the current directory. */
	if ((NULL == dictionary_dir) || ('\0' == dictionary_dir[1]))
		dictionary_dir = ".";

	/* DICTIONARY_DIR is already an absolute path - return it.
	 * Note: UNC paths are supposed to be absolute. */
	if (0 == strncmp(dictionary_dir, "\\\\", 2)) return DICTIONARY_DIR;
	if (0 == strncmp(dictionary_dir, "//", 2)) return DICTIONARY_DIR;

	/* It includes a drive letter - suppose it's absolute so just return it. */
	if ((strlen(dictionary_dir) > 2) && (':' == dictionary_dir[1]))
		return DICTIONARY_DIR;

	/* Here DICTIONARY_DIR is a relative path (to be concatenated to the
	 * location of this library DLL), or an absolute path w/o a drive
	 * letter (to be prepended with the drive letter of our program).
	 * Non-ASCII characters in the DLL location are not supported (files
	 * will not be found).
	 * Note: In case of an error, we return dictionary_dir.
	 * Is there anything better to do? */
	char dll_path[MAX_PATH_NAME] = "";
	HMODULE dll_hm = NULL;

	/* First find the module handle of this library DLL. */
	if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
	                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                       (LPCSTR) &get_dictionary_dir, &dll_hm))
	{
		prt_error("Warning: GetModuleHandleEx error %d\n", (int)GetLastError());
		return dictionary_dir; /* Is there anything better to do? */
	}

	/* Then use it to find the DLL path. */
	if (!GetModuleFileNameA(dll_hm, dll_path, sizeof(dll_path)))
	{
		prt_error("Warning: GetModuleFileNameA error %d\n", (int)GetLastError());
		return dictionary_dir;
	}

	if ('\0' == dll_path[0])
	{
		/* Can it happen? */
		prt_error("Warning: GetModuleFileNameA didn't return a path!\n");
		return dictionary_dir;
	}

	if (!PathRemoveFileSpecA(dll_path))
	{
		prt_error("Warning: Cannot get directory from LG DLL path '%s'!\n",
					 dll_path);
		return dictionary_dir;
	}

	/* Unconvertible characters are marked as '?' */
	if (NULL != strchr(dll_path, '?'))
	{
		prt_error("Warning: Directory of LG DLL (%s) "
		          "contains unsupported characters\n", dll_path);
		return dictionary_dir;
	}

	/* Just a sanity check of the directory length. */
	if (strlen(dll_path) < 3)
	{
		prt_error("Warning: DLL directory name '%s' too short!\n", dll_path);
		return dictionary_dir;
	}

	lgdebug(D_USER_FILES, "Debug: Directory of LG DLL: %s\n", dll_path);

	char *combined_dictionary_dir;

	if (('\\' == dictionary_dir[0]) || ('/' == dictionary_dir[0]))
	{
		/* DICTIONARY_DIR is an absolute path without a drive
		 * (UNC paths already returned unmodified, above).
		 * Prepend the drive or the host of our program location. */
		size_t prefix_len = 0;

		if (dll_path[1] == ':')
		{
			/* "X:path" */
			prefix_len = 2;
		}
		else
		{
			/* \\host\path */
			const char *hostend = strchr(dll_path+3, '\\');
			if (NULL == hostend)
				hostend = strchr(dll_path+3, '/');
			if (NULL != hostend)
				prefix_len = (size_t)(hostend - dll_path);
		}
		size_t len = prefix_len + strlen(dictionary_dir) + 1;
		combined_dictionary_dir = malloc(len);
		strncpy(combined_dictionary_dir, dll_path, prefix_len);
		strcpy(combined_dictionary_dir + prefix_len, dictionary_dir);
	}
	else
	{
		size_t len = strlen(dll_path)+1+strlen(dictionary_dir)+1;
		combined_dictionary_dir = malloc(len);
		strcpy(combined_dictionary_dir, dll_path);
		strcat(combined_dictionary_dir, "\\"); /* Mixing / and \ is allowed. */
		strcat(combined_dictionary_dir, dictionary_dir);
	}

	dictionary_dir = combined_dictionary_dir;
	atexit(free_dictionary_dir);
	lgdebug(D_USER_FILES, "Debug: Using dictionary directory '%s'\n",
	        dictionary_dir);
	return dictionary_dir;
#endif /* _MSC_VER */
}

static void *dict_file_open(const char *fullname, const void *how)
{
	return fopen(fullname, how);
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
	/* Dictionary data directory path cache -- per-thread storage. */
	static TLS char *path_found;
	char *completename = NULL;
	void *fp = NULL;
	char *data_dir = NULL;
	const char *dictionary_dir = NULL;
	const char **path = NULL;

	if (NULL == filename)
	{
		/* Invalidate the dictionary data directory path cache. */
		char *pf = path_found;
		path_found = NULL;
		free(pf);
		return NULL;
	}

	if (NULL == path_found)
	{
		dictionary_dir = get_dictionary_dir(true);
		data_dir = dictionary_get_data_dir();
		if (verbosity_level(D_USER_FILES))
		{
			char cwd[MAX_PATH_NAME];
			char *cwdp = getcwd(cwd, sizeof(cwd));
			prt_error("Debug: Current directory: %s\n", NULL == cwdp ? "NULL": cwdp);
			prt_error("Debug: Data directory: %s\n",
					  data_dir ? data_dir : "NULL");
			prt_error("Debug: System data directory: %s\n",
					  dictionary_dir ? dictionary_dir : "NULL");
		}
	}

	/* Look for absolute filename.
	 * Unix: starts with leading slash.
	 * Windows: starts with C:\  except that the drive letter may differ. */
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
			"./data",
			"..",
			"../data",
			data_dir,
			dictionary_dir,
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
		char *pfnd = strdup((NULL != completename) ? completename : filename);
		if ((0 < verbosity) && (dict_file_open == opencb))
			prt_error("Info: Dictionary found at %s\n", pfnd);
		for (size_t i = 0; i < 2; i++)
		{
			char *root = find_last_dir_separator(pfnd);
			if (NULL != root) *root = '\0';
		}
		path_found = pfnd;
		lgdebug(D_USER_FILES, "Debug: Using dictionary path \"%s\"\n", path_found);
	}

	free(data_dir);
	free(completename);
	return fp;
}
#undef NOTFOUND

FILE *dictopen(const char *filename, const char *how)
{
	return object_open(filename, dict_file_open, how);
}

/*
 * XXX - dict_file_open() cannot be used due to the Info printout
 * of opening a dictionary.
 */
static void *data_file_open(const char *fullname, const void *how)
{
	return fopen(fullname, how);
}

/**
 * Open a file in the dictionary search path.
 * Experimental API (may be unstable).
 * @param filename Filename to be opened.
 * @return FILE pointer, or NULL if the file was no found.
 */
FILE *linkgrammar_open_data_file(const char *filename)
{
	object_open(NULL, NULL, NULL); /* Invalidate the directory path cache */
	return object_open(filename, data_file_open, "r");
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
	size_t tot_read = 0;
	struct stat buf;
	char * contents;

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

	/* Now, read the whole file.
	 * Normally, a single fread() call below reads the whole file. */
	while (1)
	{
		size_t read_size = fread(contents, 1, tot_size+7, fp);

		if (0 == read_size)
		{
			bool err = (0 != ferror(fp));

			if (err)
			{
				char errbuf[64];

				lg_strerror_r(errno, errbuf, sizeof(errbuf));
				fclose(fp);
				prt_error("Error: %s: Read error (%s)\n", dict_name, errbuf);
				free(contents);
				return NULL;
			}
			fclose(fp);
			break;
		}
		tot_read += read_size;
	}

	if (tot_read > tot_size+6)
	{
		prt_error("Error: %s: File size is insane (%zu)!\n", dict_name, tot_size);
		free(contents);
		return NULL;
	}

	contents[tot_read] = '\0';
	return contents;
}

/* ============================================================= */
