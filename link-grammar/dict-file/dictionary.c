/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright 2008, 2009, 2012 Linas Vepstas <linasvepstas@gmail.com>     */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "api-structures.h"
#include "dict-common/dialect.h"      // dialect_alloc
#include "dict-common/dict-affix.h"
#include "dict-common/dict-affix-impl.h"
#include "dict-common/dict-api.h"
#include "dict-common/dict-common.h"
#include "dict-common/dict-defines.h"
#include "dict-common/dict-internals.h"
#include "dict-common/dict-locale.h"
#include "dict-common/dict-utils.h"
#include "dict-common/file-utils.h"
#include "dict-common/idiom.h"
#include "dict-common/regex-morph.h"
#include "dict-ram/dict-ram.h"
#include "post-process/pp_knowledge.h"
#include "read-dialect.h"
#include "read-dict.h"
#include "read-regex.h"
#include "string-set.h"
#include "tokenize/anysplit.h"        // Initialize anysplit here ...
#include "tokenize/spellcheck.h"      // Initialize spellcheck here ...

/***************************************************************
*
* Routines for manipulating Dictionary
*
****************************************************************/

/**
 * If word has a connector, return it.
 * If word has more than one connector, return NULL.
 */
static const char * word_only_connector(Dict_node * dn)
{
	Exp * e = dn->exp;
	if (CONNECTOR_type == e->type)
		return e->condesc->string;
	return NULL;
}

static void load_affix(Dictionary afdict, Dict_node *dn, int l)
{
	Dict_node * dnx = NULL;
	for (; NULL != dn; dn = dnx)
	{
		const char *string;
		const char *con = word_only_connector(dn);
		if (NULL == con)
		{
			/* ??? should we support here more than one class? */
			prt_error("Warning: Word \"%s\" found near line %d of %s.\n"
			          "\tWord has more than one connector.\n"
			          "\tThis word will be ignored.\n",
			          dn->string, afdict->line_number, afdict->name);
			return;
		}

		/* The affix files serve a dual purpose: they indicate both
		 * what a unit is, connector-wise, and what is strippable, as
		 * a string.  When the unit is an 'idiom' (i.e. two words,
		 * e.g. base_pair or degrees_C) then only the first word can
		 * be stripped away from a run-on expression (e.g. "86degrees C")
		 */
		if (contains_underbar(dn->string))
		{
			char *p;
			char *writeable_string = strdupa(dn->string);
			p = writeable_string+1;
			while (*p != '_' && *p != '\0') p++;
			*p = '\0';
			string = writeable_string;
		}
		else
		{
			string = dn->string;
		}

		affix_list_add(afdict, afdict_find(afdict, con,
		               /*notify_err*/true), string);

		dnx = dn->left;
		free(dn);
	}
}

/**
 * Dummy lookup function for the affix dictionary -
 * compile_regexs() needs it.
 */
static bool return_true(Dictionary dict, const char *name)
{
	return true;
}

/**
 * Process the regex file.
 * We have to compile regexs using the dictionary locale,
 * so make a temporary locale swap. XXX FIXME: Thread safety.
 */
static bool load_regexes(Dictionary dict, const char *regex_name)
{
	if (!read_regex_file(dict, regex_name)) return false;

	const char *locale = setlocale(LC_CTYPE, NULL); /* Save current locale. */
	locale = strdupa(locale); /* setlocale() uses its own memory. */
	setlocale(LC_CTYPE, dict->locale);
	lgdebug(+D_DICT, "Regexs locale \"%s\"\n", setlocale(LC_CTYPE, NULL));

	if (!compile_regexs(dict->regex_root, dict))
	{
		locale = setlocale(LC_CTYPE, locale);         /* Restore the locale. */
		assert(NULL != locale, "Cannot restore program locale");
		return false;
	}
	locale = setlocale(LC_CTYPE, locale);            /* Restore the locale. */
	assert(NULL != locale, "Cannot restore program locale");

	return true;
}

/**
 * Read dictionary entries from a wide-character string "input".
 * All other parts are read from files.
 */
#define D_DICT 10
static Dictionary
dictionary_six_str(const char * lang,
                   const char * input,
                   const char * dict_name,
                   const char * pp_name, const char * cons_name,
                   const char * affix_name, const char * regex_name)
{
	const char * t;
	Dictionary dict;
	size_t Exp_pool_size;

	dict = (Dictionary) malloc(sizeof(struct Dictionary_s));
	memset(dict, 0, sizeof(struct Dictionary_s));

	dict->line_number = 1;

	/* Language and file-name stuff */
	dict->string_set = string_set_create();
	t = find_last_dir_separator((char *)lang);
	t = (NULL == t) ? lang : t+1;
	dict->lang = string_set_add(t, dict->string_set);
	lgdebug(D_USER_FILES, "Debug: Language: %s\n", dict->lang);
	dict->name = string_set_add(dict_name, dict->string_set);

	if (NULL != affix_name)
	{
		if (dictionary_generation_request(dict))
		{
			const size_t initial_allocation = 256;
			dict->num_categories_alloced = initial_allocation;
			dict->category = malloc(sizeof(*dict->category) * initial_allocation);
		}
		else
		{
			dict->spell_checker = spellcheck_create(dict->lang);
		}

#if defined HAVE_HUNSPELL || defined HAVE_ASPELL
		/* FIXME: Move to spellcheck-*.c */
		if (verbosity_level(D_USER_BASIC) && (NULL == dict->spell_checker))
			prt_error("Info: %s: Spell checker disabled.\n", dict->lang);
#endif
		memset(dict->current_idiom, 'A', IDIOM_LINK_SZ-1);
		dict->current_idiom[IDIOM_LINK_SZ-1] = 0;

		dict->insert_entry = insert_list;
		dict->lookup_list = dict_node_lookup;
		dict->lookup_wild = dict_node_wild_lookup;
		dict->free_lookup = dict_node_free_lookup;
		dict->exists_lookup = dict_node_exists_lookup;
		dict->clear_cache = dict_node_noop;
		dict->start_lookup = dict_lookup_noop;
		dict->end_lookup = dict_lookup_noop;

		dict->dialect_tag.set = string_id_create();
		condesc_init(dict, 1<<13);
		Exp_pool_size = 1<<13;

		if (!test_enabled("no-macro-tag"))
		{
			dict->macro_tag = malloc(sizeof(*dict->macro_tag));
			memset(dict->macro_tag, 0, sizeof(*dict->macro_tag));
		}
	}
	else
	{
		/*
		 * Affix dictionary.
		 */
		afclass_init(dict);
		dict->insert_entry = load_affix;
		dict->exists_lookup = return_true;
		condesc_init(dict, 1<<9);
		Exp_pool_size = 1<<5;
	}

	dict->dfine.set = string_id_create();

	dict->Exp_pool = pool_new(__func__, "Exp", /*num_elements*/Exp_pool_size,
	                          sizeof(Exp), /*zero_out*/false,
	                          /*align*/false, /*exact*/false);

	/* Read dictionary from the input string. */

	dict->input = input;
	dict->pin = dict->input;
	if (!read_dictionary(dict))
	{
		goto failure;
	}

	if (NULL == affix_name)
	{
		/*
		 * The affix table is handled alone in this invocation.
		 * Skip the rest of processing!
		 * FIXME: The dictionary creating stuff needs a rearrangement.
		 */
		return dict;
	}

	if (dict->dialect_tag.num == 0)
	{
		string_id_delete(dict->dialect_tag.set);
		dict->dialect_tag.set = NULL;
	}

	if (!dictionary_setup_defines(dict))
		goto failure;

	if (!load_regexes(dict, regex_name)) goto failure;

	dict->affix_table = dictionary_six(lang, affix_name, NULL, NULL, NULL, NULL);
	if (dict->affix_table == NULL)
	{
		prt_error("Error: Could not open affix file %s\n", affix_name);
		goto failure;
	}
	if (! afdict_init(dict))
		goto failure;

	if (! anysplit_init(dict->affix_table))
		goto failure;

	dict->base_knowledge  = pp_knowledge_open(pp_name);
	dict->hpsg_knowledge  = pp_knowledge_open(cons_name);

	condesc_setup(dict);

	// Special-case hack.
	if ((0 == strncmp(dict->lang, "any", 3)) ||
	    (NULL != dict->affix_table->anysplit))
		dict->shuffle_linkages = true;

	return dict;

failure:
	dictionary_delete(dict);
	return NULL;
}

/**
 * Use filenames of six different files to put together the dictionary.
 */
NO_SAN_DICT
Dictionary
dictionary_six(const char * lang, const char * dict_name,
               const char * pp_name, const char * cons_name,
               const char * affix_name, const char * regex_name)
{
	Dictionary dict;

	char* input = get_file_contents(dict_name);
	if (NULL == input)
	{
		prt_error("Error: Could not open dictionary \"%s\"\n", dict_name);
		return NULL;
	}

	dict = dictionary_six_str(lang, input, dict_name, pp_name,
	                          cons_name, affix_name, regex_name);

	free_file_contents(input);
	return dict;
}

Dictionary dictionary_create_from_file(const char * lang)
{
	Dictionary dictionary;

	init_memusage();
	if (lang && *lang)
	{
		char * dict_name;
		char * pp_name;
		char * cons_name;
		char * affix_name;
		char * regex_name;

		dict_name = join_path(lang, "4.0.dict");
		pp_name = join_path(lang, "4.0.knowledge");
		cons_name = join_path(lang, "4.0.constituent-knowledge");
		affix_name = join_path(lang, "4.0.affix");
		regex_name = join_path(lang, "4.0.regex");

		dictionary = dictionary_six(lang, dict_name, pp_name, cons_name,
		                            affix_name, regex_name);

		free(regex_name);
		free(affix_name);
		free(cons_name);
		free(pp_name);
		free(dict_name);

		if (dictionary != NULL)
		{
			char *dialect_name = join_path(lang, "4.0.dialect");
			if (!dialect_file_read(dictionary, dialect_name))
			{
				dictionary_delete(dictionary);
				dictionary = NULL;
			}
			else if ((dictionary->dialect == NULL) ||
			         (dictionary->dialect->num_table_tags == 0))
			{
				/* No or empty dialect file. */
				free_dialect(dictionary->dialect);
				dictionary->dialect = NULL;
			}
			free(dialect_name);
		}
	}
	else
	{
		prt_error("Error: No language specified!\n");
		dictionary = NULL;
	}

	return dictionary;
}


#ifdef NOTDEF
/**
 * Use "string" as the input dictionary. All of the other parts,
 * including post-processing, affix table, etc, are NULL.
 * This routine is intended for unit-testing ONLY.
 *
 * FIXME? (Not used for now.)
 * 1. get_default_locale() returns locale, not language.
 * 2. "lang" memory-leak on success.
 */
Dictionary dictionary_create_from_utf8(const char * input)
{
	Dictionary dictionary = NULL;
	char * lang;

	init_memusage();

	lang = get_default_locale();
	if (lang && *lang) {
		dictionary = dictionary_six_str(lang, input, "string",
		                                NULL, NULL, NULL, NULL);
		free(lang);
	} else {
		/* Default to en when locales are broken (e.g. WIN32) */
		dictionary = dictionary_six_str("en", input, "string",
		                                NULL, NULL, NULL, NULL);
	}

	return dictionary;
}
#endif // NOTDEF

