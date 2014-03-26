/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright 2008, 2009, 2012-2014 Linas Vepstas <linasvepstas@gmail.com>*/
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "dict-common.h"
#include "externs.h"
#include "regex-morph.h"
#include "spellcheck.h"
#include "string-set.h"
#include "structures.h"
#include "utilities.h"
#include "word-utils.h"
#include "dict-sql/read-sql.h"
#include "dict-file/read-dict.h"

/* ======================================================================== */
/* Replace the right-most dot with SUBSCRIPT_MARK */
void patch_subscript(char * s)
{
	char *ds, *de;
	ds = strrchr(s, SUBSCRIPT_DOT);
	if (!ds) return;

	/* a dot at the end or a dot followed by a number is NOT
	 * considered a subscript */
	de = ds + 1;
	if (*de == '\0') return;
	if (isdigit((int)*de)) return;
	*ds = SUBSCRIPT_MARK;
}

/* ======================================================================== */

Dictionary dictionary_create_default_lang(void)
{
	Dictionary dictionary;
	char * lang;

	lang = get_default_locale();
	if (lang && *lang) {
		dictionary = dictionary_create_lang(lang);
		free(lang);
	} else {
		/* Default to en when locales are broken (e.g. WIN32) */
		dictionary = dictionary_create_lang("en");
	}

	return dictionary;
}

Dictionary dictionary_create_lang(const char * lang)
{
	Dictionary dictionary = NULL;

	/* If an sql database exists, try to read that. */
	if (check_db(lang))
	{
		dictionary = dictionary_create_from_db(lang);
	}

	/* Fallback to a plain-text dictionary */
	if (NULL == dictionary)
	{
		dictionary = dictionary_create_from_file(lang);
	}

	return dictionary;
}

/* ======================================================================== */

static void affix_list_delete(Dictionary dict)
{
	int i;
	for (i=0; i<dict->l_strippable; i++)
	{
		free((char *)dict->strip_left[i]);
	}
	for (i=0; i<dict->r_strippable; i++)
	{
		free((char *)dict->strip_right[i]);
	}
	for (i=0; i<dict->u_strippable; i++)
	{
		free((char *)dict->strip_units[i]);
	}
	xfree(dict->strip_right, dict->r_strippable * sizeof(char *));
	xfree(dict->strip_left, dict->l_strippable * sizeof(char *));
	xfree(dict->strip_units, dict->u_strippable * sizeof(char *));

	/* The prefix and suffix words are in the string set,
	 * and are deleted here. */
	xfree(dict->suffix, dict->s_strippable * sizeof(char *));
	xfree(dict->prefix, dict->p_strippable * sizeof(char *));
	xfree(dict->mprefix, dict->mp_strippable * sizeof(char *));
	xfree(dict->sane_morphism, dict->sm_total * sizeof(char *));
}

void dictionary_delete(Dictionary dict)
{
	if (!dict) return;

	if (verbosity > 0) {
		prt_error("Info: Freeing dictionary %s", dict->name);
	}

#ifdef USE_CORPUS
	lg_corpus_delete(dict->corpus);
#endif

	if (dict->affix_table != NULL) {
		affix_list_delete(dict->affix_table);
		dictionary_delete(dict->affix_table);
	}
	spellcheck_destroy(dict->spell_checker);

#ifdef USE_FAT_LINKAGES
	connector_set_delete(dict->andable_connector_set);
#endif /* USE_FAT_LINKAGES */
	connector_set_delete(dict->unlimited_connector_set);

	post_process_close(dict->postprocessor);
	post_process_close(dict->constituent_pp);
	string_set_delete(dict->string_set);
	free_regexs(dict);
	free_dictionary(dict);
	xfree(dict, sizeof(struct Dictionary_s));
}

/* ======================================================================== */

