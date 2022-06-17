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

#include "connectors.h"  // for connector_set_delete
#include "dialect.h"
#include "dict-affix.h"
#include "dict-api.h"
#include "dict-common.h"
#include "dict-defines.h"
#include "disjunct-utils.h"
#include "file-utils.h"                // free_categories_from_disjunct_array
#include "post-process/pp_knowledge.h" // Needed only for pp_close !!??
#include "regex-morph.h"
#include "string-set.h"
#include "tokenize/anysplit.h"
#include "tokenize/spellcheck.h"

#include "dict-sql/read-sql.h"
#include "dict-file/read-dict.h"
#include "dict-file/word-file.h"
#include "dict-atomese/read-atomese.h"

/* Stems, by definition, end with ".=x" (when x is usually an empty
 * string, i.e. ".="). The STEMSUBSCR definition in the affix file
 * may include endings with other x values, when x serves as a word
 * subscript, e.g. ".=a".  */
#define STEM_MARK '='

/* ======================================================================== */
/* Identifying dictionary word formats */

/**
 * Return TRUE if the word seems to be in stem form.
 * Stems are signified by including = sign which is preceded by the subscript
 * mark.  Examples (. represented the subscript mark): word.= word.=[!]
 */
bool is_stem(const char* w)
{
	const char *subscrmark = get_word_subscript(w);

	if (NULL == subscrmark) return false;
	if (subscrmark == w) return false;
	if (STEM_MARK != subscrmark[1]) return false;
	return true;
}

bool is_macro(const char *w)
{
	if (w[0] == '<')
	{
		char *end = strchr(w, '>');
		if (end == NULL) return false;
		if ((end[1] == '\0') || (end[1] == SUBSCRIPT_MARK)) return true;
	}
	return false;
}

bool is_wall(const char *s)
{
	if (0 == strncmp(s, LEFT_WALL_WORD, sizeof(LEFT_WALL_WORD)-1))
	{
		if (s[sizeof(LEFT_WALL_WORD)-1] == '\0' ||
			(s[sizeof(LEFT_WALL_WORD)-1] == SUBSCRIPT_MARK)) return true;
	}
	if (0 == strncmp(s, RIGHT_WALL_WORD, sizeof(RIGHT_WALL_WORD)-1))
	{
		if (s[sizeof(RIGHT_WALL_WORD)-1] == '\0' ||
			(s[sizeof(RIGHT_WALL_WORD)-1] == SUBSCRIPT_MARK)) return true;
	}
	return false;
}

/* ======================================================================== */

Dictionary dictionary_create_default_lang(void)
{
	Dictionary dictionary = NULL;
	char * lang = get_default_locale(); /* E.g. ll_CC.UTF_8 or ll-CC */

	if (lang && *lang)
	{
		lang[strcspn(lang, "_-")] = '\0';
		dictionary = dictionary_create_lang(lang);
	}

	/* Fall back to English if no default locale or no matching dict. */
	if ((NULL == dictionary) && ((lang == NULL) || (0 != strcmp(lang, "en"))))
	{
		dictionary = dictionary_create_lang("en");
	}

	free(lang);
	return dictionary;
}

Dictionary dictionary_create_lang(const char * lang)
{
	Dictionary dictionary = NULL;

	object_open(NULL, NULL, NULL); /* Invalidate the directory path cache */

	/* If an sql database exists, try to read that. */
	if (check_db(lang))
	{
#if HAVE_SQLITE3
		dictionary = dictionary_create_from_db(lang);
#else
		return NULL;
#endif /* HAVE_SQLITE3 */
	}

	else if (check_atomspace(lang))
	{
#if HAVE_ATOMESE
		dictionary = dictionary_create_from_atomese(lang);
#else
		return NULL;
#endif /* HAVE_ATOMESE */
	}

	/* Fallback to a plain-text dictionary */
	if (NULL == dictionary)
	{
		dictionary = dictionary_create_from_file(lang);
	}

	return dictionary;
}

const char * dictionary_get_lang(Dictionary dict)
{
	if (!dict) return "";
	return dict->lang;
}

/* ======================================================================== */
/* Dictionary lookup stuff */

/**
 * dictionary_lookup_list() - get list of matching words in the dictionary.
 *
 * Returns a pointer to a list of dict_nodes for matching words in the
 * dictionary.
 *
 * This list is made up of Dict_nodes, linked by their right pointers.
 * The exp, file and string fields are copied from the dictionary.
 *
 * The returned list must be freed with free_lookup_list().
 */
Dict_node * dictionary_lookup_list(const Dictionary dict, const char *s)
{
	return dict->lookup_list(dict, s);
}

Dict_node * dictionary_lookup_wild(const Dictionary dict, const char *s)
{
	return dict->lookup_wild(dict, s);
}

void free_lookup_list(const Dictionary dict, Dict_node *llist)
{
	dict->free_lookup(dict, llist);
}

bool dict_has_word(const Dictionary dict, const char *s)
{
	return dict->lookup(dict, s);
}

/**
 * Return true if word is in dictionary, or if word is matched by
 * regex.
 */
bool dictionary_word_is_known(const Dictionary dict, const char * word)
{
	const char * regex_name;
	if (dict_has_word(dict, word)) return true;

	regex_name = match_regex(dict->regex_root, word);
	if (NULL == regex_name) return false;

	return dict_has_word(dict, regex_name);
}

/**
 * Return the dictionary Category array.
 */
const Category *dictionary_get_categories(const Dictionary dict)
{
	if (dict->category == NULL) return NULL;
	return dict->category + 1; /* First entry is not used */
}

/* ======================================================================== */
/* the following functions are for handling deletion */

#ifdef USEFUL_BUT_NOT_CURRENTLY_USED
/**
 * Returns true if it finds a non-idiom dict_node in a file that matches
 * the string s.
 *
 * Also sets parent and to_be_deleted appropriately.
 * Note: this function is used in only one place: delete_dictionary_words()
 * which is, itself, not currently used ...
 */
static bool find_one_non_idiom_node(Dict_node * p, Dict_node * dn,
                                   const char * s,
                                   Dict_node **parent, Dict_node **to_be_deleted)
{
	int m;
	if (dn == NULL) return false;
	m = dict_order_bare(s, dn);
	if (m <= 0) {
		if (find_one_non_idiom_node(dn, dn->left, s, parent, to_be_deleted)) return true;
	}
/*	if ((m == 0) && (!is_idiom_word(dn->string)) && (dn->file != NULL)) { */
	if ((m == 0) && (!is_idiom_word(dn->string))) {
		*to_be_deleted = dn;
		*parent = p;
		return true;
	}
	if (m >= 0) {
		if (find_one_non_idiom_node(dn, dn->right, s, parent, to_be_deleted)) return true;
	}
	return false;
}

static void set_parent_of_node(Dictionary dict,
                               Dict_node *p,
                               Dict_node * del,
                               Dict_node * newnode)
{
	if (p == NULL) {
		dict->root = newnode;
	} else {
		if (p->left == del) {
			p->left = newnode;
		} else if (p->right == del) {
			p->right = newnode;
		} else {
			assert(false, "Dictionary broken?");
		}
	}
}

/**
 * This deletes all the non-idiom words of the dictionary that match
 * the given string.  Returns true if some deleted, false otherwise.
 *
 * XXX Note: this function is not currently used anywhere in the code,
 * but it could be useful for general dictionary editing.
 */
int delete_dictionary_words(Dictionary dict, const char * s)
{
	Dict_node *pred, *pred_parent;
	Dict_node *parent, *to_be_deleted;

	if (!find_one_non_idiom_node(NULL, dict->root, s, &parent, &to_be_deleted)) return false;
	for(;;) {
		/* now parent and to_be_deleted are set */
		if (to_be_deleted->file != NULL) {
			to_be_deleted->file->changed = true;
		}
		if (to_be_deleted->left == NULL) {
			set_parent_of_node(dict, parent, to_be_deleted, to_be_deleted->right);
			free(to_be_deleted);
		} else {
			pred_parent = to_be_deleted;
			pred = to_be_deleted->left;
			while(pred->right != NULL) {
				pred_parent = pred;
				pred = pred->right;
			}
			to_be_deleted->string = pred->string;
			to_be_deleted->file = pred->file;
			to_be_deleted->exp = pred->exp;
			set_parent_of_node(dict, pred_parent, pred, pred->left);
			free(pred);
		}
		if (!find_one_non_idiom_node(NULL, dict->root, s, &parent, &to_be_deleted)) return true;
	}
}
#endif /* USEFUL_BUT_NOT_CURRENTLY_USED */

static void free_dict_node_recursive(Dict_node * dn)
{
	if (dn == NULL) return;
	free_dict_node_recursive(dn->left);
	free_dict_node_recursive(dn->right);
	free(dn);
}

static void free_dictionary(Dictionary dict)
{
	free_dict_node_recursive(dict->root);
	free_Word_file(dict->word_file_header);
	pool_delete(dict->Exp_pool);
}

static void affix_list_delete(Dictionary dict)
{
	int i;
	Afdict_class * atc;
	for (i=0, atc = dict->afdict_class; i < AFDICT_NUM_ENTRIES; i++, atc++)
	{
		if (atc->string) free(atc->string);
	}
	free(dict->afdict_class);
	dict->afdict_class = NULL;
}

void dictionary_delete(Dictionary dict)
{
	if (!dict) return;

	if (verbosity >= D_USER_INFO) {
		prt_error("Info: Freeing dictionary %s\n", dict->name);
	}

	if (dict->affix_table != NULL) {
		affix_list_delete(dict->affix_table);
		dictionary_delete(dict->affix_table);
	}
	spellcheck_destroy(dict->spell_checker);
	if ((locale_t) 0 != dict->lctype) {
		freelocale(dict->lctype);
	}

	condesc_delete(dict);

	if (dict->close) dict->close(dict);

	pp_knowledge_close(dict->base_knowledge);
	pp_knowledge_close(dict->hpsg_knowledge);
	string_set_delete(dict->string_set);

	free_dialect(dict->dialect);
	free(dict->dialect_tag.name);
	string_id_delete(dict->dialect_tag.set);
	if (dict->macro_tag != NULL) free(dict->macro_tag->name);
	free(dict->macro_tag);
	string_id_delete(dict->define.set);
	free(dict->define.name);
	free(dict->define.value);
	free_regexs(dict->regex_root);
	free_anysplit(dict);
	free_dictionary(dict);

	/* Free sentence generation stuff. */
	for (unsigned int i = 1; i <= dict->num_categories; i++)
		free(dict->category[i].word);
	free(dict->category);

	free(dict);
	object_open(NULL, NULL, NULL); /* Free the directory path cache */
}

/* ======================================================================== */

/**
 * Initialize generation mode, if requested.
 * Since the dictionary_create*() functions don't support Parse_Options as
 * an argument, use the "test" parse-option, which is in a global variable:
 * If it contains "generate", enable generation mode.
 * If an argument "walls" is also supplied ("generate:walls"), then
 * set a wall generation indication.
 *
 * @param dict Set the generation mode in this dictionary.
 * @return \c true if generation mode has been set, \c false otherwise.
 */
bool dictionary_generation_request(const Dictionary dict)
{
	const char *generation_mode = test_enabled("generate");
	if (generation_mode != NULL)
	{
		dict->generate_walls =
			feature_enabled(generation_mode, "walls", NULL) != NULL;
		dict->spell_checker = NULL; /* Disable spell-checking. */

		return true;
	}

	return false;
}
