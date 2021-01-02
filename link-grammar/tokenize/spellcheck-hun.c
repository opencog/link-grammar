/*************************************************************************/
/* Copyright (c) 2009 Linas Vepstas                                      */
/* Copyright (c) 2009 Vikas N. Kumar                                     */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifdef HAVE_HUNSPELL

#ifdef __MINGW32__
#define LGPTHREAD(x) x
#else
#define LGPTHREAD(x)
#endif // __MINGW32__

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#ifdef __MINGW32__
#define LGPTHREAD(x) x
#else
#define LGPTHREAD(x)
#endif // __MINGW32__

#include "link-includes.h"
#include "error.h"
#include "spellcheck.h"

#ifndef HUNSPELL_DICT_DIR
#define HUNSPELL_DICT_DIR (char *)0
#endif /* HUNSPELL_DICT_DIR */

static const char *hunspell_dict_dirs[] = {
	"/usr/share/myspell/dicts",
	"/usr/share/hunspell/dicts",
	"/usr/local/share/myspell/dicts",
	"/usr/local/share/hunspell/dicts",
	"/usr/share/myspell",
	"/usr/share/hunspell",
	"/usr/local/share/myspell",
	"/usr/local/share/hunspell",
	HUNSPELL_DICT_DIR
};

static const char *spellcheck_lang_mapping[] = {
/* link-grammar language, Hunspell filename */
	"en", "en-US",
	"en", "en_US",
	"ru", "ru-RU",
	"ru", "ru_RU",
	"he", "he-IL",
	"he", "he_IL",
	"de", "de-DE",
	"de", "de_DE",
	"lt", "lt-LT",
	"lt", "lt_LT",
};

#include <hunspell.h>
#include <string.h>

void * spellcheck_create(const char * lang)
{
	size_t i = 0, j = 0;
	Hunhandle *h = NULL;

#define FPATHLEN 1024
	static char hunspell_aff_file[FPATHLEN];
	static char hunspell_dic_file[FPATHLEN];

	static pthread_mutex_t findpath_lock = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&findpath_lock);

	if (hunspell_aff_file[0] != '\0')
	{
		h = Hunspell_create(hunspell_aff_file, hunspell_dic_file);
		if (h == NULL)
			prt_error("Error: Hunspell_create(%s, %s): Unexpected failure\n",
			          hunspell_aff_file, hunspell_dic_file);
	}
	else
	{
		for (i = 0; i < sizeof(spellcheck_lang_mapping)/sizeof(char *); i += 2)
		{
			if (0 != strcmp(lang, spellcheck_lang_mapping[i])) continue;

			/* check in each hunspell_dict_dir if the files exist */
			for (j = 0; j < sizeof(hunspell_dict_dirs)/sizeof(char *); ++j)
			{
				FILE *fh;
				/* if the directory name is NULL then ignore */
				if (hunspell_dict_dirs[j] == NULL) continue;

				snprintf(hunspell_aff_file, FPATHLEN, "%s/%s.aff", hunspell_dict_dirs[j],
				         spellcheck_lang_mapping[i+1]);
				snprintf(hunspell_dic_file, FPATHLEN, "%s/%s.dic", hunspell_dict_dirs[j],
				         spellcheck_lang_mapping[i+1]);

				/* Some versions of Hunspell_create() will succeed even if
				 * there are no dictionary files. So test for permissions.
				 */
				fh = fopen(hunspell_aff_file, "r");
				if (fh) fclose (fh);
				else continue;

				fh = fopen(hunspell_dic_file, "r");
				if (fh) fclose (fh);
				else continue;

				h = Hunspell_create(hunspell_aff_file, hunspell_dic_file);
				if (h == NULL)
					prt_error("Error: Hunspell_create(%s, %s): Unexpected failure\n",
					          hunspell_aff_file, hunspell_dic_file);
				/* if hunspell handle was created break from loop */
				if (h != NULL)
					break;
			}
			/* if hunspell handle was created break from loop */
			if (h != NULL) break;
		}
		if (h == NULL)
		{
			if ((hunspell_aff_file[0] != '\0') && verbosity_level(D_USER_FILES))
				prt_error("Warning: Cannot find hunspell language files\n");
			hunspell_aff_file[0] = '\0';
		}
	}
	pthread_mutex_unlock(&findpath_lock);
	return h;
}

void spellcheck_destroy(void * chk)
{
	Hunhandle *h = (Hunhandle *) chk;
	Hunspell_destroy(h);
}

/**
 * Return boolean: 1 if spelling looks good, else zero
 */
bool spellcheck_test(void * chk, const char * word)
{
	if (NULL == chk)
	{
		prt_error("Error: no spell-check handle specified!\n");
		return 0;
	}

	return (bool) Hunspell_spell((Hunhandle *)chk, word);
}

LGPTHREAD(static pthread_mutex_t hunspell_lock = PTHREAD_MUTEX_INITIALIZER;)

int spellcheck_suggest(void * chk, char ***sug, const char * word)
{
	if (NULL == chk)
	{
		prt_error("Error: no spell-check handle specified!\n");
		return 0;
	}

	LGPTHREAD(pthread_mutex_lock(&hunspell_lock);)
	int rc = Hunspell_suggest((Hunhandle *)chk, sug, word);
	LGPTHREAD(pthread_mutex_unlock(&hunspell_lock);)
	return rc;
}

void spellcheck_free_suggest(void *chk, char **sug, int size)
{
	Hunspell_free_list((Hunhandle *)chk, &sug, size);
}

#endif /* #ifdef HAVE_HUNSPELL */
