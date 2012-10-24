/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright 2008, 2009, 2012 Linas Vepstas <linasvepstas@gmail.com>     */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "api-structures.h"
#include "externs.h"
#include "read-dict.h"
#include "read-regex.h"
#include "regex-morph.h"
#include "spellcheck.h"
#include "string-set.h"
#include "structures.h"
#include "utilities.h"
#include "word-utils.h"


/***************************************************************
*
* Routines for manipulating Dictionary
*
****************************************************************/

/* Units will typically have a ".u" at the end. Get
 * rid of it, as otherwise stipping is messed up. */
static inline char * deinflect(const char * str)
{
	char * s = strdup(str);
	char * p = strrchr(s, '.');
	if (p && p != s) *p = 0x0;
	return s;
}

static void affix_list_create(Dictionary dict)
{
	int i, j, k, l, m;
	int r_strippable=0, l_strippable=0, u_strippable=0;
	int s_strippable=0, p_strippable=0;
	Dict_node * dn, * dn2, * start_dn;

	const char * rpunc_con = "RPUNC";
	const char * lpunc_con = "LPUNC";
	const char * units_con = "UNITS";

	/* Hmm SUF and PRE do not seem to be used at this time ... */
	const char * suf_con = "SUF";
	const char * pre_con = "PRE";

	dict->strip_left = NULL;
	dict->strip_right = NULL;
	dict->strip_units = NULL;
	dict->prefix = NULL;
	dict->suffix = NULL;

	/* Load affixes from the affix table.
	 */
	start_dn = list_whole_dictionary(dict->root, NULL);
	for (dn = start_dn; dn != NULL; dn = dn->right)
	{
		if (word_has_connector(dn, rpunc_con, 0)) r_strippable++;
		if (word_has_connector(dn, lpunc_con, 0)) l_strippable++;
		if (word_has_connector(dn, units_con, 0)) u_strippable++;
		if (word_has_connector(dn, suf_con, 0)) s_strippable++;
		if (word_has_connector(dn, pre_con, 0)) p_strippable++;
	}
	dict->strip_right = (const char **) xalloc(r_strippable * sizeof(char *));
	dict->strip_left = (const char **) xalloc(l_strippable * sizeof(char *));
	dict->strip_units = (const char **) xalloc(u_strippable * sizeof(char *));
	dict->suffix = (const char **) xalloc(s_strippable * sizeof(char *));
	dict->prefix = (const char **) xalloc(p_strippable * sizeof(char *));

	dict->r_strippable = r_strippable;
	dict->l_strippable = l_strippable;
	dict->u_strippable = u_strippable;
	dict->p_strippable = p_strippable;
	dict->s_strippable = s_strippable;

	i = 0;
	j = 0;
	k = 0;
	l = 0;
	m = 0;
	dn = start_dn;

	while (dn != NULL)
	{
		if (word_has_connector(dn, rpunc_con, 0))
		{
			dict->strip_right[i] = deinflect(dn->string);
			i++;
		}
		if (word_has_connector(dn, lpunc_con, 0))
		{
			dict->strip_left[j] = deinflect(dn->string);
			j++;
		}
		if (word_has_connector(dn, units_con, 0))
		{
			dict->strip_units[m] = deinflect(dn->string);
			m++;
		}
		if (word_has_connector(dn, suf_con, 0))
		{
			dict->suffix[k] = dn->string;
			k++;
		}
		if (word_has_connector(dn, pre_con, 0))
		{
			dict->prefix[l] = dn->string;
			l++;
		}
		dn2 = dn->right;
		dn->right = NULL;
		xfree(dn, sizeof(Dict_node));
		dn = dn2;
	}
}

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
	xfree(dict->suffix, dict->s_strippable * sizeof(char *));
	xfree(dict->prefix, dict->p_strippable * sizeof(char *));
}

static Dictionary
dictionary_six(const char * lang, const char * dict_name,
                const char * pp_name, const char * cons_name,
                const char * affix_name, const char * regex_name);

/**
 * Read dictionary entries from a wide-character string "input".
 * All other parts are read from files.
 */
static Dictionary
dictionary_six_str(const char * lang,
                const wchar_t * input,
                const char * dict_name,
                const char * pp_name, const char * cons_name,
                const char * affix_name, const char * regex_name)
{
	const char * t;
	Dictionary dict;
	Dict_node *dict_node;

	init_memusage();

	dict = (Dictionary) xalloc(sizeof(struct Dictionary_s));
	memset(dict, 0, sizeof(struct Dictionary_s));

	dict->string_set = string_set_create();

	dict->lang = lang;
	t = strrchr (lang, '/');
	if (t) dict->lang = string_set_add(t+1, dict->string_set);
	dict->name = string_set_add(dict_name, dict->string_set);

	dict->max_cost = 1000;
	dict->num_entries = 0;
	dict->is_special = FALSE;
	dict->already_got_it = '\0';
	dict->line_number = 1;
	dict->root = NULL;
	dict->word_file_header = NULL;
	dict->exp_list = NULL;
	dict->affix_table = NULL;
	dict->recursive_error = FALSE;

	/* To disable spell-checking, just set the cheker to NULL */
	dict->spell_checker = spellcheck_create(dict->lang);

	/* Read dictionary from the input string. */
	dict->input = input;
	dict->pin = dict->input;
	if (!read_dictionary(dict))
	{
		dict->pin = NULL;
		dict->input = NULL;
		goto failure;
	}
	dict->pin = NULL;
	dict->input = NULL;

	dict->affix_table = NULL;
	if (affix_name != NULL)
	{
		dict->affix_table = dictionary_six(lang, affix_name, NULL, NULL, NULL, NULL);
		if (dict->affix_table == NULL)
		{
			goto failure;
		}
		affix_list_create(dict->affix_table);
	}

	dict->regex_root = NULL;
	if (regex_name != NULL)
	{
		int rc;
		rc = read_regex_file(dict, regex_name);
		if (rc) goto failure;
		rc = compile_regexs(dict);
		if (rc) goto failure;
	}

#if USE_CORPUS
	dict->corpus = NULL;
	if (affix_name != NULL) /* Don't do this for the second time */
	{
		dict->corpus = lg_corpus_new();
	}
#endif

	dict->left_wall_defined  = boolean_dictionary_lookup(dict, LEFT_WALL_WORD);
	dict->right_wall_defined = boolean_dictionary_lookup(dict, RIGHT_WALL_WORD);
	dict->postprocessor	  = post_process_open(pp_name);
	dict->constituent_pp	 = post_process_open(cons_name);

	dict->unknown_word_defined = boolean_dictionary_lookup(dict, UNKNOWN_WORD);
	dict->use_unknown_word = TRUE;

	if ((dict_node = dictionary_lookup_list(dict, ANDABLE_CONNECTORS_WORD)) != NULL) {
		dict->andable_connector_set = connector_set_create(dict_node->exp);
	} else {
		dict->andable_connector_set = NULL;
	}
	free_lookup_list(dict_node);

	if ((dict_node = dictionary_lookup_list(dict, UNLIMITED_CONNECTORS_WORD)) != NULL) {
		dict->unlimited_connector_set = connector_set_create(dict_node->exp);
	} else {
		dict->unlimited_connector_set = NULL;
	}
	free_lookup_list(dict_node);

	return dict;

failure:
	string_set_delete(dict->string_set);
	xfree(dict, sizeof(struct Dictionary_s));
	return NULL;
}

/**
 * Use filenames of six different files to put together the dictionary.
 */
static Dictionary
dictionary_six(const char * lang, const char * dict_name,
                const char * pp_name, const char * cons_name,
                const char * affix_name, const char * regex_name)
{
   Dictionary dict;

	wchar_t* input = get_file_contents(dict_name);
	if (NULL == input)
	{
		prt_error("Error: Could not open dictionary %s\n", dict_name);
		goto failure;
	}

	dict = dictionary_six_str(lang, input, dict_name, pp_name,
	                          cons_name, affix_name, regex_name);
	if (NULL == dict)
	{
		free(input);
		goto failure;
	}
	free(input);
	return dict;

failure:
	string_set_delete(dict->string_set);
	xfree(dict, sizeof(struct Dictionary_s));
	return NULL;
}

Dictionary dictionary_create_lang(const char * lang)
{
	Dictionary dictionary;

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
	}
	else
	{
		prt_error("Error: No language specified!\n");
		dictionary = NULL;
	}

	return dictionary;
}

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

/**
 * Use "string" as the input dictionary. All of the other parts,
 * including post-processing, affix table, etc, are NULL.
 * This routine is itended for unit-testing ONLY.
 */
Dictionary dictionary_create_from_utf8(const char * input)
{
	Dictionary dictionary = NULL;
	char * lang;
	wchar_t *winput, *wp;
	size_t len;
	const char *p;
	int cv;

	/* Convert input string to wide chars. This is needed for locale
	 * compatibility with the dictionary read routines. */
	len = strlen(input) + 1;
	winput = (wchar_t*) malloc(len * sizeof(wchar_t));

	p = input;
	wp = winput;
	while (1)
	{
		cv = mbtowc(wp, p, 8);
		if (-1 == cv)
		{
			prt_error("Error: Conversion failure!\n");
		goto failure;
		}
		if (0 == cv)
			break;
		p += cv;
		wp++;
	}


	lang = get_default_locale();
	if (lang && *lang) {
		dictionary = dictionary_six_str(lang, winput, "string",
		                                NULL, NULL, NULL, NULL);
		free(lang);
	} else {
		/* Default to en when locales are broken (e.g. WIN32) */
		dictionary = dictionary_six_str("en", winput, "string",
		                                NULL, NULL, NULL, NULL);
	}

failure:
	free(winput);

	return dictionary;
}

int dictionary_delete(Dictionary dict)
{
	if (verbosity > 0) {
		prt_error("Info: Freeing dictionary %s\n", dict->name);
	}

#if USE_CORPUS
	lg_corpus_delete(dict->corpus);
#endif

	if (dict->affix_table != NULL) {
		affix_list_delete(dict->affix_table);
		dictionary_delete(dict->affix_table);
	}
	spellcheck_destroy(dict->spell_checker);

	connector_set_delete(dict->andable_connector_set);
	connector_set_delete(dict->unlimited_connector_set);

	post_process_close(dict->postprocessor);
	post_process_close(dict->constituent_pp);
	string_set_delete(dict->string_set);
	free_regexs(dict);
	free_dictionary(dict);
	xfree(dict, sizeof(struct Dictionary_s));

	return 0;
}

int dictionary_get_max_cost(Dictionary dict)
{
	return dict->max_cost;
}

/**
 * Support function for the old, deprecated API.
 * Do not use this function in new developemnt!
 */
Dictionary
dictionary_create(const char * dict_name, const char * pp_name,
                  const char * cons_name, const char * affix_name)
{
	return dictionary_six("en", dict_name, pp_name, cons_name, affix_name, NULL);
}

