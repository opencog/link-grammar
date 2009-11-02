/*************************************************************************/
/* Copyright (c) 2009 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>
#include "link-includes.h"
#include "spellcheck-hun.h"

#ifdef HAVE_HUNSPELL

#ifndef HUNSPELL_DICT_DIR
#define HUNSPELL_DICT_DIR (char *)0
#endif /* HUNSPELL_DICT_DIR */

static const char *hunspell_dict_dirs[] = {
	"/usr/share/myspell/dicts",
	"/usr/share/hunspell/dicts",
	"/usr/local/share/myspell/dicts",
	"/usr/local/share/hunspell/dicts",
	HUNSPELL_DICT_DIR	
};

static const char *spellcheck_lang_mapping[] = {
	"en" /* link-grammar language */, "en-US" /* hunspell filename */,
	"en" /* link-grammar language */, "en_US" /* hunspell filename */
};

#define FPATHLEN 256
static char hunspell_aff_file[FPATHLEN];
static char hunspell_dic_file[FPATHLEN];

#include <hunspell.h>
#include <string.h>

void * spellcheck_create(const char * lang)
{
	size_t i = 0, j = 0;
	Hunhandle *h = NULL;

	memset(hunspell_aff_file, 0, FPATHLEN);
	memset(hunspell_dic_file, 0, FPATHLEN);
	for (i = 0; i < sizeof(spellcheck_lang_mapping)/sizeof(char *); i += 2)
	{
		if (0 != strcmp(lang, spellcheck_lang_mapping[i])) continue;

		/* check in each hunspell_dict_dir if the files exist */
		for (j = 0; j < sizeof(hunspell_dict_dirs); ++j)
		{
			/* if the directory name is NULL then ignore */
			if (hunspell_dict_dirs[j] == NULL) continue;

			snprintf(hunspell_aff_file, FPATHLEN, "%s/%s.aff", hunspell_dict_dirs[j],
					spellcheck_lang_mapping[i+1]);
			snprintf(hunspell_dic_file, FPATHLEN, "%s/%s.dic", hunspell_dict_dirs[j],
					spellcheck_lang_mapping[i+1]);
			h = Hunspell_create(hunspell_aff_file, hunspell_dic_file);
			/* if hunspell handle was created break from loop */
			if (h != NULL)
				break;
		}
		/* if hunspell handle was created break from loop */
		if (h != NULL) break;
	}
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
int spellcheck_test(void * chk, const char * word)
{
	if (NULL == chk)
	{
		prt_error("Error: no spell-check handle specified!\n");
		return 0;
	}

	return Hunspell_spell((Hunhandle *)chk, word);
}

int spellcheck_suggest(void * chk, char ***sug, const char * word)
{
	if (NULL == chk)
	{
		prt_error("Error: no spell-check handle specified!\n");
		return 0;
	}

	return Hunspell_suggest((Hunhandle *)chk, sug, word);
}

void spellcheck_free_suggest(char **sug, int size)
{
	free(sug);
}
#elif defined HAVE_ASPELL
#include <aspell.h>
#include <stdlib.h>
#include <string.h>

#define ASPELL_LANG_KEY  "lang"
static const char *spellcheck_lang_mapping[] = {
	"en" /* link-grammar language */, "en_US" /* Aspell language key */
};

struct linkgrammar_aspell {
	AspellConfig *config;
	AspellSpeller *speller;
};

void * spellcheck_create(const char * lang)
{
	struct linkgrammar_aspell *aspell = NULL;
	size_t i = 0;
	AspellCanHaveError *spell_err = NULL;

	for (i = 0; i < sizeof(spellcheck_lang_mapping)/sizeof(char *); i += 2)
	{
		if (0 != strcmp(lang, spellcheck_lang_mapping[i])) continue;
		aspell = (struct linkgrammar_aspell *)malloc(sizeof(struct linkgrammar_aspell));
		if (!aspell) {
			prt_error("Error: out of memory. Aspell not used.\n");
			aspell = NULL;
			break;
		}
		aspell->config = NULL;
		aspell->speller = NULL;
		aspell->config = new_aspell_config();
		if (aspell_config_replace(aspell->config, ASPELL_LANG_KEY,
					spellcheck_lang_mapping[i]) == 0) {
			prt_error("Error: failed to set language in aspell: %s\n", lang);
			delete_aspell_config(aspell->config);
			free(aspell);
			aspell = NULL;
			break;
		}
		spell_err = new_aspell_speller(aspell->config);
		if (aspell_error_number(spell_err) != 0) {
			prt_error("Error: Aspell: %s\n", aspell_error_message(spell_err));
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

void spellcheck_destroy(void * chk)
{
	struct linkgrammar_aspell *aspell = (struct linkgrammar_aspell *)chk;
	if (aspell) {
		delete_aspell_speller(aspell->speller);
		delete_aspell_config(aspell->config);
		free(aspell);
		aspell = NULL;
	}
}
int spellcheck_test(void * chk, const char * word)
{
	int val = 0;
	struct linkgrammar_aspell *aspell = (struct linkgrammar_aspell *)chk;
	if (aspell && aspell->speller)  {
		/* this can return -1 on failure */
		val = aspell_speller_check(aspell->speller, word, -1);
	}	
	return (val == 1) ? 1 : 0;
}
int spellcheck_suggest(void * chk, char ***sug, const char * word)
{
	struct linkgrammar_aspell *aspell = (struct linkgrammar_aspell *)chk;
	if (!sug) {
		prt_error("Error: Aspell. Corrupt pointer.\n");
		return 0;
	}
	if (aspell && aspell->speller) {
		const AspellWordList *list = NULL;
		AspellStringEnumeration *elem = NULL;
		const char *aword = NULL;
		unsigned int size, i;
		char **array = NULL;

		list = aspell_speller_suggest(aspell->speller, word, -1);
		elem = aspell_word_list_elements(list);
		size = aspell_word_list_size(list);
		/* allocate an array of char* for returning back to link-parser
		 */
		array = (char **)malloc(sizeof(char *) * size);
		if (!array) {
			prt_error("Error: Aspell. Out of memory.\n");
			delete_aspell_string_enumeration(elem);
			return 0;
		}
		i = 0;
		while ((aword = aspell_string_enumeration_next(elem)) != NULL) {
			array[i++] = strdup(aword);
		}
		delete_aspell_string_enumeration(elem);
		*sug = array;
		return size;
	}
	return 0;
}
void spellcheck_free_suggest(char **sug, int size)
{
	int i = 0;
	for (i = 0; i < size; ++i) {
		free(sug[i]);
		sug[i] = NULL;
	}
	free(sug);
}

#else
void * spellcheck_create(const char * lang) { return NULL; }
void spellcheck_destroy(void * chk) {}
int spellcheck_test(void * chk, const char * word) { return 0; }
int spellcheck_suggest(void * chk, char ***sug, const char * word) { return 0; }
void spellcheck_free_suggest(char **sug, int size) {}

#endif /* #ifdef HAVE_HUNSPELL */
