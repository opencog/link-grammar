/*************************************************************************/
/* Copyright (c) 2013, 2014 Linas Vepstas                                */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "dict-common.h"
#include "structures.h"
#include "utilities.h"
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
	Boolean have_db = FALSE;

	have_db = check_db(lang);

	if (!have_db)
	{
		dictionary = dictionary_file_create_lang(lang);
	}

	return dictionary;
}

