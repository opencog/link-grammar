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
#include "dict-api.h"
#include "dict-common.h"
#include "externs.h"
#include "idiom.h"
#include "read-dict.h"
#include "read-regex.h"
#include "regex-morph.h"
#include "spellcheck.h"
#include "string-set.h"
#include "structures.h"
#include "utilities.h"
#include "word-utils.h"
#include "dict-sql/read-sql.h"  /* Temporary hack */


/***************************************************************
*
* Routines for manipulating Dictionary
*
****************************************************************/

/* Units will typically have a ".u" at the end. Get
 * rid of it, as otherwise stripping is messed up. */
static inline char * deinflect(const char * str)
{
	size_t len;
	char *s;
	char *p = strrchr(str, SUBSCRIPT_MARK);
	if (!p || (p == str)) return strdup(str);

	len = p - str;
	s = (char *)malloc(len + 1);
	strncpy(s, str, len);
	s[len] = '\0';
	return s;
}

/* The affix dictionary is represented as a dynamically allocated array with
 * an element for each class (connector type) in the affix file. Each element
 * has a pointer to an array of strings which are the punctuation/affix
 * names. */

const char const * afdict_classname[] = { AFDICT_CLASSNAMES };

/**
 * Find the affix table entry for given connector name.
 * If the connector name is not in the table, return NULL.
 */
static Afdict_class afdict_find(Dictionary afdict, const char * con)
{
	const char ** ac;

	for (ac = afdict_classname; ac < &afdict_classname[AFDICT_END]; ac++)
	{
		if (0 == strcmp(*ac, con))
			return &afdict->afdict_class[ac - afdict_classname];
	}
	return NULL;
}

static void affix_list_resize(Afdict_class ac)
{
	size_t old_mem_elems = ac->mem_elems;

	ac->mem_elems += AFFIX_COUNT_MEM_INCREMENT;
	ac->string = xrealloc((void *)ac->string, old_mem_elems,
	 ac->mem_elems * sizeof(*ac->string));
}

static void affix_list_add(Dictionary afdict, const char * name,
		const char * affix)
{
	Afdict_class ac = afdict_find(afdict, name);

	if (NULL == ac)
	{
		prt_error("Warning: Unknown class name %s found near line %d of %s.\n"
				  "\tThis class name will be ignored.",
				  name, afdict->line_number, afdict->name);
		return;
	}
	if (ac->length == ac->mem_elems)
		affix_list_resize(ac);
	ac->string[ac->length] = string_set_add(affix, afdict->string_set);
	ac->length++;
}

static void load_affix(Dictionary afdict, Dict_node *dn, int l)
{
	char *string;
	const char *con;

	for (; NULL != dn; dn = dn->left)
	{
		if (contains_underbar(dn->string))
		{
			if (NULL == dn->file)
			{
				prt_error("Warning: Idiom %s found near line %d of %s.\n"
						  "\tIt will be ignored since it is meaningless there.",
						  dn->string, afdict->line_number, afdict->name);
			}
			return; /* Don't load idioms */
		}

		con = word_only_connector(dn);
		if (NULL == con)
		{
			/* ??? should we support here more than one class? */
			prt_error("Warning: Word \"%s\" found near line %d of %s.\n"
					  "\tWord has more than one connector.\n"
					  "\tThis word will be ignored.",
					  dn->string, afdict->line_number, afdict->name);
			return;
		}
		string = deinflect(dn->string);
		affix_list_add(afdict, con, string);
		free(string);
	}
}

static int revcmplen(const void *a, const void *b)
{
	return strlen(*(char * const *)b) - strlen(*(char * const *)a);
}

/**
 * Initialize several classes.
 * In case of a future dynamic change of the affix table, this
 * function needs to be invoked again after the affix table structure
 * is re-constructed.
 */
static bool afdict_init(Dictionary afdict)
{
	Afdict_class ac;

	if (0) verbosity = 5; /* debug - !verbosity is not set so early for now */
	if (4 < verbosity)
	{
		size_t l;

		for (ac = afdict->afdict_class; ac->length; ac++)
		{
				lgdebug(+0, "Class %s:", afdict_classname[ac-afdict->afdict_class]);
				for (l = 0; l < ac->length; l++)
					lgdebug(0, " '%s'", ac->string[l]);
				lgdebug(0, "\n");
		}
	}

	/* Store the SANEMORPHISM regex in the unused (up to now)
	 * regex_root element of the affix dictionary, and precompile it */
	assert(NULL == afdict->regex_root, "SM regex is already assigned");
	ac = AFCLASS(afdict, AFDICT_SANEMORPHISM);
	if (0 != ac->length)
	{
		int rc;

		Regex_node * sm_re = (Regex_node *) malloc(sizeof(Regex_node));
		char rebuf[MAX_WORD + 16] = "^((";

		/* The regex is converted to: ^((original-regex)b)+$ */
		strcat(rebuf, ac->string[0]);
		strcat(rebuf, ")b)+$");

		afdict->regex_root = sm_re;
		sm_re->pattern = strdup(rebuf);
		sm_re->name = strdup(afdict_classname[AFDICT_SANEMORPHISM]);
		sm_re->re = NULL;
		sm_re->next = NULL;
		rc = compile_regexs(afdict);
		if (rc) {
			prt_error("Error: sane_morphism: Failed to compile "
			          "regex '%s' in file %s, return code %d\n",
			          afdict_classname[AFDICT_SANEMORPHISM], afdict->name, rc);
			return FALSE;
		}
		lgdebug(+5, "%s regex %s\n",
		        afdict_classname[AFDICT_SANEMORPHISM], sm_re->pattern);
	}

	/* pre-sort the MPRE list */
	ac = AFCLASS(afdict, AFDICT_MPRE);
	if (0 < ac->length)
	{
		/* Longer subwords have priority over shorter ones,
		 * reverse-sort by length.
		 * XXX mprefix_suffix() for Hebrew depends on that. */
		qsort(ac->string, ac->length, sizeof(char *), revcmplen);
	}

	{
		wchar_t * wqs;
		mbstate_t mbs;
		size_t i;
		int w;
		dyn_str * qs;
		const char *pqs;

		ac = AFCLASS(afdict, AFDICT_QUOTES);
		qs = dyn_str_new();
		for (i = 0; i < ac->length; i++)
			dyn_strcat(qs, ac->string[i]);

		/*
		 * Convert utf8 to wide chars before use.
		 * In case of error the result is undefined.
		 */
		pqs = qs->str;
		memset(&mbs, 0, sizeof(mbs));
		w = mbsrtowcs(NULL, &pqs, 0, &mbs);
		if (0 > w)
		{
			prt_error("Error: Affix dictionary: %s: "
						 "Invalid utf8 character\n", afdict_classname[AFDICT_QUOTES]);
			return FALSE;
		}

		/* Store the wide char version at the AFDICT_QUOTES entry.
		 */ 
		ac->string = xrealloc((void *)ac->string, ac->mem_elems,
		             sizeof(*wqs) * (w+1));
		ac->mem_elems =  sizeof(*wqs) * (w+1);
		wqs = (wchar_t *)ac->string;
		pqs = qs->str;
		(void)mbsrtowcs(wqs, &pqs, w, &mbs);
		wqs[w] = L'\0';

		dyn_str_delete(qs);
	}

	return TRUE;
}

static void free_llist(Dictionary dict, Dict_node *llist)
{
	free_lookup(llist);
}

/**
 * Dummy lookup function for the affix dictionary -
 * compile_regexs() needs needs it.
 */
static bool return_true(Dictionary dict, const char *name)
{
	return TRUE;
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
                   const char * input,
                   const char * dict_name,
                   const char * pp_name, const char * cons_name,
                   const char * affix_name, const char * regex_name)
{
	const char * t;
	Dictionary dict;
	Dict_node *dict_node;

	dict = (Dictionary) xalloc(sizeof(struct Dictionary_s));
	memset(dict, 0, sizeof(struct Dictionary_s));

	dict->num_entries = 0;
	dict->is_special = FALSE;
	dict->already_got_it = '\0';
	dict->line_number = 0;
	dict->root = NULL;
	dict->regex_root = NULL;
	dict->word_file_header = NULL;
	dict->exp_list = NULL;
	dict->affix_table = NULL;
	dict->recursive_error = FALSE;
	dict->version = NULL;
#ifdef HAVE_SQLITE
	dict->db_handle = NULL;
#endif

	/* Language and file-name stuff */
	dict->string_set = string_set_create();
	dict->lang = lang;
	t = strrchr (lang, '/');
	if (t) dict->lang = string_set_add(t+1, dict->string_set);
	dict->name = string_set_add(dict_name, dict->string_set);

	/*
	 * A special setup per dictionary type. The check here assumes the affix
	 * dictionary name contains "affix". FIXME: For not using this
	 * assumption, the dictionary creating stuff needs a rearrangement.
	 */
	if (0 == strstr(dict->name, "affix"))
	{
		/* To disable spell-checking, just set the checker to NULL */
		dict->spell_checker = spellcheck_create(dict->lang);
		dict->insert_entry = insert_list;

		dict->lookup_list = lookup_list;
		dict->free_lookup = free_llist;
		dict->lookup = boolean_lookup;
	}
	else
	{
		/*
		 * Affix dictionary.
		 */
		size_t i;

		dict->insert_entry = load_affix;
		dict->lookup = return_true;

		/* initialize the class table */
		dict->afdict_class = malloc(sizeof(*dict->afdict_class)*AFDICT_END);
		for (i = 0; i < AFDICT_END; i++)
		{
			dict->afdict_class[i].mem_elems = 0;
			dict->afdict_class[i].length = 0;
			dict->afdict_class[i].string = NULL;
		}

	}

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

	if (NULL == affix_name)
	{
		/*
		 * The affix table is handled alone in this invocation.
		 * Skip the rest of processing!
		 * FIXME: The dictionary creating stuff needs a rearrangement.
		 */
		return dict;
	}

	dict->affix_table = dictionary_six(lang, affix_name, NULL, NULL, NULL, NULL);
	if (dict->affix_table == NULL)
	{
		prt_error("Error: Could not open affix file %s", affix_name);
		goto failure;
	}
	if (! afdict_init(dict->affix_table))
		goto failure;

	if (read_regex_file(dict, regex_name)) goto failure;
	if (compile_regexs(dict)) goto failure;

#ifdef USE_CORPUS
	dict->corpus = lg_corpus_new();
#endif

	dict->left_wall_defined  = boolean_dictionary_lookup(dict, LEFT_WALL_WORD);
	dict->right_wall_defined = boolean_dictionary_lookup(dict, RIGHT_WALL_WORD);

	dict->empty_word_defined = boolean_dictionary_lookup(dict, EMPTY_WORD_MARK);

	dict->postprocessor   = post_process_open(pp_name);
	dict->constituent_pp  = post_process_open(cons_name);

	dict->unknown_word_defined = boolean_dictionary_lookup(dict, UNKNOWN_WORD);
	dict->use_unknown_word = TRUE;

#ifdef USE_FAT_LINKAGES
	dict_node = dictionary_lookup_list(dict, ANDABLE_CONNECTORS_WORD);
	if (dict_node != NULL) {
		dict->andable_connector_set = connector_set_create(dict_node->exp);
	} else {
		dict->andable_connector_set = NULL;
	}
	free_lookup(dict_node);
#endif /* USE_FAT_LINKAGES */

	dict_node = dictionary_lookup_list(dict, UNLIMITED_CONNECTORS_WORD);
	if (dict_node != NULL) {
		dict->unlimited_connector_set = connector_set_create(dict_node->exp);
	} else {
		dict->unlimited_connector_set = NULL;
	}
	free_lookup(dict_node);

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

	char* input = get_file_contents(dict_name);
	if (NULL == input)
	{
		prt_error("Error: Could not open dictionary %s", dict_name);
		return NULL;
	}

	dict = dictionary_six_str(lang, input, dict_name, pp_name,
				  cons_name, affix_name, regex_name);

	free(input);
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
	}
	else
	{
		prt_error("Error: No language specified!");
		dictionary = NULL;
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

