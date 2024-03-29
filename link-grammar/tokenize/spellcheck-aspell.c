/*************************************************************************/
/* Copyright (c) 2009 Vikas N. Kumar                                     */
/* Copyright (c) 2009 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifdef HAVE_ASPELL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aspell.h>
#if HAVE_PTHREAD
#include <pthread.h>
#else
#define pthread_mutex_lock(x)
#define pthread_mutex_ulock(x)
#endif /* HAVE_PTHREAD */

#include "link-includes.h"
#include "error.h"                      // lgdebug
#include "spellcheck.h"

#define ASPELL_LANG_KEY  "lang"
/* FIXME: Move to a definition file (affix file?). */
static const char *spellcheck_lang_mapping[] =
{
/* link-grammar language , Aspell language key */
	"en", "en_US",
	"ru", "ru_RU",
	"he", "he_IL",
	"de", "de_DE",
	"lt", "lt_LT",
};

struct linkgrammar_aspell
{
	AspellConfig *config;
	AspellSpeller *speller;
};

/**
 * create a new spell-checker for the language 'lang'
 */
void * spellcheck_create(const char * lang)
{
	struct linkgrammar_aspell *aspell = NULL;
	size_t i = 0;
	AspellCanHaveError *spell_err = NULL;

	for (i = 0; i < sizeof(spellcheck_lang_mapping)/sizeof(char *); i += 2)
	{
		if (0 != strcmp(lang, spellcheck_lang_mapping[i])) continue;
		aspell = (struct linkgrammar_aspell *)malloc(sizeof(struct linkgrammar_aspell));
		if (!aspell)
		{
			prt_error("Error: Out of memory - aspell not used.\n");
			break;
		}
		aspell->config = NULL;
		aspell->speller = NULL;
		aspell->config = new_aspell_config();
		if (aspell_config_replace(aspell->config, ASPELL_LANG_KEY,
					spellcheck_lang_mapping[i]) == 0)
		{
			prt_error("Error: Failed to set language in aspell: %s\n", lang);
			delete_aspell_config(aspell->config);
			free(aspell);
			aspell = NULL;
			break;
		}
		spell_err = new_aspell_speller(aspell->config);
		if (aspell_error_number(spell_err) != 0)
		{
			if (strstr(aspell_error_message(spell_err), "No word lists") == 0)
			{
				prt_error("Error: new_aspell_speller: %s\n",
				          aspell_error_message(spell_err));
			}
			else
			{
				lgdebug(D_USER_FILES, "Warning: new_aspell_speller: %s\n",
				        aspell_error_message(spell_err));
			}
			delete_aspell_can_have_error(spell_err);
			delete_aspell_config(aspell->config);
			free(aspell);
			aspell = NULL;
			break;
		}
		aspell->speller = to_aspell_speller(spell_err);
		break;
	}
	return aspell;
}

/**
 * Free memory structures used with spell-checker 'chk'
 */
void spellcheck_destroy(void * chk)
{
	struct linkgrammar_aspell *aspell = (struct linkgrammar_aspell *)chk;
	if (aspell)
	{
		delete_aspell_speller(aspell->speller);
		delete_aspell_config(aspell->config);
		free(aspell);
		aspell = NULL;
	}
}

/**
 * Ask the spell-checker if the spelling looks good.
 * Return true if the spelling is good, else false.
 */
bool spellcheck_test(void * chk, const char * word)
{
	int val = 0;
	struct linkgrammar_aspell *aspell = (struct linkgrammar_aspell *)chk;
	if (aspell && aspell->speller)
	{
		/* This returns 1 if the word is in the aspell dict.
		 * i.e. is spelled OK. It returns 0 if the word is not
		 * in the aspell dict. It can also return -1 on failure.
		 */
		val = aspell_speller_check(aspell->speller, word, -1);
	}
	return (val == 1);
}

// Despite having a thread-compatible API, it appears that apsell
// is not actually thread-safe. Bummer.
#if HAVE_PTHREAD
static pthread_mutex_t aspell_lock = PTHREAD_MUTEX_INITIALIZER;
#endif /* HAVE_PTHREAD */

int spellcheck_suggest(void * chk, char ***sug, const char * word)
{
	struct linkgrammar_aspell *aspell = (struct linkgrammar_aspell *)chk;
	if (!sug)
	{
		prt_error("Error: spellcheck_suggest: Corrupt pointer.\n");
		return 0;
	}

	if (aspell && aspell->speller)
	{
		const AspellWordList *list = NULL;
		AspellStringEnumeration *elem = NULL;
		const char *aword = NULL;
		unsigned int size, i;
		char **array = NULL;

		pthread_mutex_lock(&aspell_lock);
		list = aspell_speller_suggest(aspell->speller, word, -1);
		elem = aspell_word_list_elements(list);
		size = aspell_word_list_size(list);

		/* allocate an array of char* for returning back to link-parser */
		array = (char **)malloc(sizeof(char *) * size);
		if (!array)
		{
			prt_error("Error: spellcheck_suggest: Out of memory.\n");
			delete_aspell_string_enumeration(elem);
			pthread_mutex_unlock(&aspell_lock);
			return 0;
		}

		i = 0;
		while ((aword = aspell_string_enumeration_next(elem)) != NULL)
		{
			array[i++] = strdup(aword);
		}
		delete_aspell_string_enumeration(elem);
		*sug = array;
		pthread_mutex_unlock(&aspell_lock);
		return size;
	}
	return 0;
}

void spellcheck_free_suggest(void *chk, char **sug, int size)
{
	for (int i = 0; i < size; ++i) free(sug[i]);
	free(sug);
}

#endif /* #ifdef HAVE_ASPELL */
