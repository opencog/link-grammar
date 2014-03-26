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

#include "dict-api.h"
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
/* Dictionary lookup stuff */

/**
 * dictionary_lookup_list() - get list of matching words in the dictionary.
 *
 * Returns a pointer to a list of dict_nodes for matching words in the
 * dictionary.  Matches include words that appear in idioms.  To exclude
 * idioms, use abridged_lookup_list() to obtain matches.
 *
 * This list is made up of Dict_nodes, linked by their right pointers.
 * The exp, file and string fields are copied from the dictionary.
 *
 * The returned list must be freed with free_lookup_list().
 */
Dict_node * dictionary_lookup_list(Dictionary dict, const char *s)
{
	return dict->lookup_list(dict, s);
}

void free_lookup_list(Dictionary dict, Dict_node *llist)
{
	dict->free_lookup(dict, llist);
}

Boolean boolean_dictionary_lookup(Dictionary dict, const char *s)
{
	return dict->lookup(dict, s);
}

/**
 * Return true if word is in dictionary, or if word is matched by
 * regex.
 */
Boolean find_word_in_dict(Dictionary dict, const char * word)
{
	const char * regex_name;
	if (boolean_dictionary_lookup (dict, word)) return TRUE;

	regex_name = match_regex(dict, word);
	if (NULL == regex_name) return FALSE;

	return boolean_dictionary_lookup(dict, regex_name);
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
static Boolean find_one_non_idiom_node(Dict_node * p, Dict_node * dn,
                                   const char * s,
                                   Dict_node **parent, Dict_node **to_be_deleted)
{
	int m;
	if (dn == NULL) return FALSE;
	m = dict_order_bare(s, dn);
	if (m <= 0) {
		if (find_one_non_idiom_node(dn, dn->left, s, parent, to_be_deleted)) return TRUE;
	}
/*	if ((m == 0) && (!is_idiom_word(dn->string)) && (dn->file != NULL)) { */
	if ((m == 0) && (!is_idiom_word(dn->string))) {
		*to_be_deleted = dn;
		*parent = p;
		return TRUE;
	}
	if (m >= 0) {
		if (find_one_non_idiom_node(dn, dn->right, s, parent, to_be_deleted)) return TRUE;
	}
	return FALSE;
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
			assert(FALSE, "Dictionary broken?");
		}
	}
}

/**
 * This deletes all the non-idiom words of the dictionary that match
 * the given string.  Returns TRUE if some deleted, FALSE otherwise.
 *
 * XXX Note: this function is not currently used anywhere in the code,
 * but it could be useful for general dictionary editing.
 */
int delete_dictionary_words(Dictionary dict, const char * s)
{
	Dict_node *pred, *pred_parent;
	Dict_node *parent, *to_be_deleted;

	if (!find_one_non_idiom_node(NULL, dict->root, s, &parent, &to_be_deleted)) return FALSE;
	for(;;) {
		/* now parent and to_be_deleted are set */
		if (to_be_deleted->file != NULL) {
			to_be_deleted->file->changed = TRUE;
		}
		if (to_be_deleted->left == NULL) {
			set_parent_of_node(dict, parent, to_be_deleted, to_be_deleted->right);
			free_dict_node(to_be_deleted);
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
			free_dict_node(pred);
		}
		if (!find_one_non_idiom_node(NULL, dict->root, s, &parent, &to_be_deleted)) return TRUE;
	}
}
#endif /* USEFUL_BUT_NOT_CURRENTLY_USED */

static void free_Word_file(Word_file * wf)
{
	Word_file *wf1;

	for (;wf != NULL; wf = wf1) {
		wf1 = wf->next;
		xfree((char *) wf, sizeof(Word_file));
	}
}

/**
 * The following two functions free the Exp s and the
 * E_lists of the dictionary.  Not to be confused with
 * free_E_list in utilities.c
 */
static void free_Elist(E_list * l)
{
	E_list * l1;

	for (; l != NULL; l = l1) {
		l1 = l->next;
		xfree(l, sizeof(E_list));
	}
}

static inline void exp_free(Exp * e)
{
	xfree((char *)e, sizeof(Exp));
}

static inline void free_dict_node(Dict_node *dn)
{
	xfree((char *)dn, sizeof(Dict_node));
}


static void free_Exp_list(Exp * e)
{
	Exp * e1;
	for (; e != NULL; e = e1)
	{
		e1 = e->next;
		if (e->type != CONNECTOR_type)
		{
		   free_Elist(e->u.l);
		}
		exp_free(e);
	}
}

static void free_dict_node_recursive(Dict_node * dn)
{
	if (dn == NULL) return;
	free_dict_node_recursive(dn->left);
	free_dict_node_recursive(dn->right);
	free_dict_node(dn);
}

static void free_dictionary(Dictionary dict)
{
	free_dict_node_recursive(dict->root);
	free_Word_file(dict->word_file_header);
	free_Exp_list(dict->exp_list);
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

	if (dict->close) dict->close(dict);

	post_process_close(dict->postprocessor);
	post_process_close(dict->constituent_pp);
	string_set_delete(dict->string_set);
	free_regexs(dict);
	free_dictionary(dict);
	xfree(dict, sizeof(struct Dictionary_s));
}

/* ======================================================================== */
