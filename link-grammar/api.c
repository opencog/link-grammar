/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <math.h>
#include <link-grammar/api.h>
#include "disjuncts.h"
#include "error.h"
#include "preparation.h"
#include "read-regex.h"
#include "regex-morph.h"
#ifdef USE_SAT_SOLVER
#include "sat-solver/sat-encoder.h"
#endif
#include "corpus/corpus.h"
#include "spellcheck-hun.h"

/***************************************************************
*
* Routines for setting Parse_Options
*
****************************************************************/

static int VDAL_compare_parse(Linkage_info * p1, Linkage_info * p2)
{
	/* for sorting the linkages in postprocessing */
	if (p1->N_violations != p2->N_violations) {
		return (p1->N_violations - p2->N_violations);
	}
	else if (p1->unused_word_cost != p2->unused_word_cost) {
		return (p1->unused_word_cost - p2->unused_word_cost);
	}
	else if (p1->disjunct_cost != p2->disjunct_cost) {
		return (p1->disjunct_cost - p2->disjunct_cost);
	}
	else if (p1->and_cost != p2->and_cost) {
		return (p1->and_cost - p2->and_cost);
	}
	else {
		return (p1->link_cost - p2->link_cost);
	}
}

#ifdef USE_CORPUS
static int CORP_compare_parse(Linkage_info * p1, Linkage_info * p2)
{
	double diff = p1->corpus_cost - p2->corpus_cost;
	if (fabs(diff) < 1.0e-5) 
		return VDAL_compare_parse(p1, p2);
	if (diff < 0.0f) return -1;
	return 1;
}
#endif

/**
 * Create and initialize a Parse_Options object
 */
Parse_Options parse_options_create(void)
{
	Parse_Options po;

	init_memusage();
	po = (Parse_Options) xalloc(sizeof(struct Parse_Options_s));

	/* Here's where the values are initialized */
	po->verbosity	 = 1;
	po->linkage_limit = 100;
	po->disjunct_cost = MAX_DISJUNCT_COST;
	po->min_null_count = 0;
	po->max_null_count = 0;
	po->null_block = 1;
	po->islands_ok = FALSE;
	po->use_spell_guess = TRUE;
	po->cost_model.compare_fn = &VDAL_compare_parse;
	po->cost_model.type = VDAL;
	po->short_length = 6;
	po->all_short = FALSE;
	po->twopass_length = 30;
	po->max_sentence_length = 170;
	po->resources = resources_create();
	po->display_short = TRUE;
	po->display_word_subscripts = TRUE;
	po->display_link_subscripts = TRUE;
	po->display_walls = FALSE;
	po->display_union = FALSE;
	po->allow_null = TRUE;
	po->echo_on = FALSE;
	po->batch_mode = FALSE;
	po->panic_mode = FALSE;
	po->screen_width = 79;
	po->display_on = TRUE;
	po->display_postscript = FALSE;
	po->display_constituents = 0;
	po->display_bad = FALSE;
	po->display_disjuncts = FALSE;
	po->display_links = FALSE;
	po->display_senses = FALSE;

	return po;
}

int parse_options_delete(Parse_Options  opts)
{
	resources_delete(opts->resources);
	xfree(opts, sizeof(struct Parse_Options_s));
	return 0;
}

void parse_options_set_cost_model_type(Parse_Options opts, int cm)
{
	switch(cm) {
	case VDAL:
		opts->cost_model.type = VDAL;
		opts->cost_model.compare_fn = &VDAL_compare_parse;
		break;
	case CORPUS:
#ifdef USE_CORPUS
		opts->cost_model.type = CORPUS;
		opts->cost_model.compare_fn = &CORP_compare_parse;
#else
		prt_error("Error: Source code compiled with cost model 'CORPUS' disabled.\n");
#endif
		break;
	default:
		prt_error("Error: Illegal cost model: %d\n", cm);
	}
}

int  parse_options_get_cost_model_type(Parse_Options opts)
{
	return opts->cost_model.type;
}

void parse_options_set_verbosity(Parse_Options opts, int dummy)
{
	opts->verbosity = dummy;
	verbosity = opts->verbosity;
	/* this is one of the only global variables. */
}

int parse_options_get_verbosity(Parse_Options opts) {
	return opts->verbosity;
}

void parse_options_set_linkage_limit(Parse_Options opts, int dummy) {
	opts->linkage_limit = dummy;
}
int parse_options_get_linkage_limit(Parse_Options opts) {
	return opts->linkage_limit;
}

void parse_options_set_disjunct_cost(Parse_Options opts, int dummy) {
	opts->disjunct_cost = dummy;
}
int parse_options_get_disjunct_cost(Parse_Options opts) {
	return opts->disjunct_cost;
}

void parse_options_set_min_null_count(Parse_Options opts, int val) {
	opts->min_null_count = val;
}
int parse_options_get_min_null_count(Parse_Options opts) {
	return opts->min_null_count;
}

void parse_options_set_max_null_count(Parse_Options opts, int val) {
	opts->max_null_count = val;
}
int parse_options_get_max_null_count(Parse_Options opts) {
	return opts->max_null_count;
}


void parse_options_set_null_block(Parse_Options opts, int dummy) {
	opts->null_block = dummy;
}
int parse_options_get_null_block(Parse_Options opts) {
	return opts->null_block;
}

void parse_options_set_islands_ok(Parse_Options opts, int dummy) {
	opts->islands_ok = dummy;
}

int parse_options_get_islands_ok(Parse_Options opts) {
	return opts->islands_ok;
}

void parse_options_set_spell_guess(Parse_Options opts, int dummy) {
	opts->use_spell_guess = dummy;
}

int parse_options_get_spell_guess(Parse_Options opts) {
	return opts->use_spell_guess;
}

void parse_options_set_short_length(Parse_Options opts, int short_length) {
	opts->short_length = short_length;
}

int parse_options_get_short_length(Parse_Options opts) {
	return opts->short_length;
}

void parse_options_set_all_short_connectors(Parse_Options opts, int val) {
	opts->all_short = val;
}

int parse_options_get_all_short_connectors(Parse_Options opts) {
	return opts->all_short;
}

void parse_options_set_max_parse_time(Parse_Options opts, int dummy) {
	opts->resources->max_parse_time = dummy;
}

int parse_options_get_max_parse_time(Parse_Options opts) {
	return opts->resources->max_parse_time;
}

void parse_options_set_max_memory(Parse_Options opts, int dummy) {
	opts->resources->max_memory = dummy;
}

int parse_options_get_max_memory(Parse_Options opts) {
	return opts->resources->max_memory;
}

void parse_options_set_max_sentence_length(Parse_Options opts, int dummy) {
	opts->max_sentence_length = dummy;
}

int parse_options_get_max_sentence_length(Parse_Options opts) {
	return opts->max_sentence_length;
}

void parse_options_set_echo_on(Parse_Options opts, int dummy) {
	opts->echo_on = dummy;
}

int parse_options_get_echo_on(Parse_Options opts) {
	return opts->echo_on;
}

void parse_options_set_batch_mode(Parse_Options opts, int dummy) {
	opts->batch_mode = dummy;
}

int parse_options_get_batch_mode(Parse_Options opts) {
	return opts->batch_mode;
}

void parse_options_set_panic_mode(Parse_Options opts, int dummy) {
	opts->panic_mode = dummy;
}

int parse_options_get_panic_mode(Parse_Options opts) {
	return opts->panic_mode;
}

void parse_options_set_allow_null(Parse_Options opts, int dummy) {
	opts->allow_null = dummy;
}

int parse_options_get_allow_null(Parse_Options opts) {
	return opts->allow_null;
}

void parse_options_set_screen_width(Parse_Options opts, int dummy) {
	opts->screen_width = dummy;
}

int parse_options_get_screen_width(Parse_Options opts) {
	return opts->screen_width;
}


void parse_options_set_display_on(Parse_Options opts, int dummy) {
	opts->display_on = dummy;
}

int parse_options_get_display_on(Parse_Options opts) {
	return opts->display_on;
}

void parse_options_set_display_postscript(Parse_Options opts, int dummy) {
	opts->display_postscript = dummy;
}

int parse_options_get_display_postscript(Parse_Options opts)
{
	return opts->display_postscript;
}

void parse_options_set_display_constituents(Parse_Options opts, int dummy)
{
	if ((dummy < 0) || (dummy > 3)) {
		prt_error("Possible values for constituents: \n"
	             "   0 (no display)\n"
	             "   1 (treebank style, multi-line indented)\n"
	             "   2 (flat tree, square brackets)\n"
	             "   3 (flat treebank style)\n");
	   opts->display_constituents = 0;
	}
	else opts->display_constituents = dummy;
}

int parse_options_get_display_constituents(Parse_Options opts)
{
	return opts->display_constituents;
}

void parse_options_set_display_bad(Parse_Options opts, int dummy) {
	opts->display_bad = dummy;
}

int parse_options_get_display_bad(Parse_Options opts) {
	return opts->display_bad;
}

void parse_options_set_display_disjuncts(Parse_Options opts, int dummy) {
	opts->display_disjuncts = dummy;
}

int parse_options_get_display_disjuncts(Parse_Options opts) {
	return opts->display_disjuncts;
}

void parse_options_set_display_links(Parse_Options opts, int dummy) {
	opts->display_links = dummy;
}

int parse_options_get_display_links(Parse_Options opts) {
	return opts->display_links;
}

void parse_options_set_display_senses(Parse_Options opts, int dummy) {
	opts->display_senses = dummy;
}

int parse_options_get_display_senses(Parse_Options opts) {
	return opts->display_senses;
}

void parse_options_set_display_walls(Parse_Options opts, int dummy) {
	opts->display_walls = dummy;
}

int parse_options_get_display_walls(Parse_Options opts) {
	return opts->display_walls;
}

int parse_options_get_display_union(Parse_Options opts) {
	return opts->display_union;
}

void parse_options_set_display_union(Parse_Options opts, int dummy) {
	opts->display_union = dummy;
}

int parse_options_timer_expired(Parse_Options opts) {
	return resources_timer_expired(opts->resources);
}

int parse_options_memory_exhausted(Parse_Options opts) {
	return resources_memory_exhausted(opts->resources);
}

int parse_options_resources_exhausted(Parse_Options opts) {
	return (resources_timer_expired(opts->resources) || resources_memory_exhausted(opts->resources));
}

void parse_options_reset_resources(Parse_Options opts) {
	resources_reset(opts->resources);
}


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
	char * p = strchr(s, '.');
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

/**
 * The following function is dictionary_create with an extra
 * paramater called "path". If this is non-null, then the path
 * used to find the file is taken from that path. Otherwise,
 * the path is taken from the dict_name.  This is only needed
 * because an affix_file is opened by a recursive call to this
 * function.
 */
static Dictionary
dictionary_six(const char * lang, const char * dict_name,
                const char * pp_name, const char * cons_name,
                const char * affix_name, const char * regex_name)
{
	const char * t;
	Dictionary dict;
	Dict_node *dict_node;

	init_memusage();
	init_randtable();

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

	dict->fp = dictopen(dict->name, "r");
	if (dict->fp == NULL)
	{
		prt_error("Error: Could not open dictionary %s\n", dict_name);
		goto failure;
	}

	if (!read_dictionary(dict))
	{
		fclose(dict->fp);
		goto failure;
	}
	fclose(dict->fp);

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

#if DONT_USE_REGEX_GUESSING
	dict->capitalized_word_defined = boolean_dictionary_lookup(dict, PROPER_WORD);
	dict->pl_capitalized_word_defined = boolean_dictionary_lookup(dict, PL_PROPER_WORD);

	dict->hyphenated_word_defined = boolean_dictionary_lookup(dict, HYPHENATED_WORD);
	dict->number_word_defined = boolean_dictionary_lookup(dict, NUMBER_WORD);

	dict->ing_word_defined = boolean_dictionary_lookup(dict, ING_WORD);
	dict->s_word_defined = boolean_dictionary_lookup(dict, S_WORD);
	dict->ed_word_defined = boolean_dictionary_lookup(dict, ED_WORD);
	dict->ly_word_defined = boolean_dictionary_lookup(dict, LY_WORD);
#endif /* DONT_USE_REGEX_GUESSING */

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

Dictionary
dictionary_create(const char * dict_name, const char * pp_name,
                  const char * cons_name, const char * affix_name)
{
	return dictionary_six("en", dict_name, pp_name, cons_name, affix_name, NULL);
}

Dictionary dictionary_create_lang(const char * lang)
{
	Dictionary dictionary;

	if(lang && *lang)
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
	if(lang && *lang) {
		dictionary = dictionary_create_lang(lang);
		free(lang);
	} else {
		/* Default to en when locales are broken (e.g. WIN32) */
		dictionary = dictionary_create_lang("en");
	}

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

/***************************************************************
*
* Routines for postprocessing
*
****************************************************************/

static Linkage_info * linkage_info_new(int num_to_alloc)
{
	Linkage_info *link_info;
	link_info = (Linkage_info *) xalloc(num_to_alloc * sizeof(Linkage_info));
	memset(link_info, 0, num_to_alloc * sizeof(Linkage_info));
	return link_info;
}

static void linkage_info_delete(Linkage_info *link_info, int sz)
{
	int i,j;

	for (i=0; i<sz; i++)
	{
		Linkage_info *lifo = &link_info[i];
		int nwords = lifo->nwords;
		for (j=0; j<nwords; j++)
		{
			if (lifo->disjunct_list_str[j])
				free(lifo->disjunct_list_str[j]);
		}
		free(lifo->disjunct_list_str);
#ifdef USE_CORPUS
		lg_sense_delete(lifo);
#endif
	}
	xfree(link_info, sz);
}

static void free_andlists(Sentence sent)
{
	int L;
	Andlist * andlist, * next;
	for(L=0; L<sent->num_linkages_post_processed; L++) {
		/* printf("%d ", sent->link_info[L].canonical);  */
		/* if (sent->link_info[L].canonical==0) continue; */
		andlist = sent->link_info[L].andlist;
		while(1) {
			if(andlist == NULL) break;
			next = andlist->next;
			xfree((char *) andlist, sizeof(Andlist));
			andlist = next;
		}
	}
	/* printf("\n"); */
}

static void free_post_processing(Sentence sent)
{
	if (sent->link_info != NULL) {
		/* postprocessing must have been done */
		free_andlists(sent);
		linkage_info_delete(sent->link_info, sent->num_linkages_alloced);
		sent->link_info = NULL;
	}
}

static void post_process_linkages(Sentence sent, Parse_Options opts)
{
	int *indices;
	int in, block_bottom, block_top;
	int N_linkages_found, N_linkages_alloced;
	int N_linkages_post_processed, N_valid_linkages;
	int overflowed, only_canonical_allowed;
	Linkage_info *link_info;
	int canonical;

	free_post_processing(sent);

	overflowed = build_parse_set(sent, sent->null_count, opts);
	print_time(opts, "Built parse set");

	if (overflowed)
	{
		/* We know that sent->num_linkages_found is bogus, possibly negative */
		sent->num_linkages_found = opts->linkage_limit;
		if (1 < opts->verbosity)
		{
			err_ctxt ec;
			ec.sent = sent;
			err_msg(&ec, Warn, "Warning: Count overflow.\n"
			  "Considering a random subset of %d of an unknown and large number of linkages\n",
				opts->linkage_limit);
		}
	}
	N_linkages_found = sent->num_linkages_found;

	if (sent->num_linkages_found == 0)
	{
		sent->num_linkages_alloced = 0;
		sent->num_linkages_post_processed = 0;
		sent->num_valid_linkages = 0;
		sent->link_info = NULL;
		return;
	}

	if (N_linkages_found > opts->linkage_limit)
	{
		N_linkages_alloced = opts->linkage_limit;
		if (opts->verbosity > 1)
		{
			err_ctxt ec;
			ec.sent = sent;
			err_msg(&ec, Warn, "Warning: Considering a random subset of %d of %d linkages\n",
				  N_linkages_alloced, N_linkages_found);
		}
	}
	else
	{
		N_linkages_alloced = N_linkages_found;
	}

	link_info = linkage_info_new(N_linkages_alloced);
	N_valid_linkages = 0;

	/* Generate an array of linkage indices to examine */
	indices = (int *) xalloc(N_linkages_alloced * sizeof(int));
	if (overflowed)
	{
		for (in=0; in < N_linkages_alloced; in++)
		{
			indices[in] = -(in+1);
		}
	}
	else
	{
		sent->rand_state = N_linkages_found + sent->length;
		for (in=0; in<N_linkages_alloced; in++)
		{
			double frac = (double) N_linkages_found;
			frac /= (double) N_linkages_alloced;
			block_bottom = (int) (((double) in) * frac);
			block_top = (int) (((double) (in+1)) * frac);
			indices[in] = block_bottom + 
				(rand_r(&sent->rand_state) % (block_top-block_bottom));
		}
	}

	only_canonical_allowed = !(overflowed || (N_linkages_found > 2*opts->linkage_limit));
	/* When we're processing only a small subset of the linkages,
	 * don't worry about restricting the set we consider to be
	 * canonical ones.  In the extreme case where we are only
	 * generating 1 in a million linkages, it's very unlikely
	 * that we'll hit two symmetric variants of the same linkage
	 * anyway.
	 */
	/* (optional) first pass: just visit the linkages */
	/* The purpose of these two passes is to make the post-processing
	 * more efficient.  Because (hopefully) by the time you do the
	 * real work in the 2nd pass you've pruned the relevant rule set
	 * in the first pass.
	 */
	if (sent->length >= opts->twopass_length)
	{
		for (in=0; (in < N_linkages_alloced) &&
				   (!resources_exhausted(opts->resources)); in++)
		{
			extract_links(indices[in], sent->null_count, sent->parse_info);
			if (set_has_fat_down(sent))
			{
				if (only_canonical_allowed && !is_canonical_linkage(sent)) continue;
				analyze_fat_linkage(sent, opts, PP_FIRST_PASS);
			}
			else
			{
				analyze_thin_linkage(sent, opts, PP_FIRST_PASS);
			}
		}
	}

	/* second pass: actually perform post-processing */
	N_linkages_post_processed = 0;
	for (in=0; (in < N_linkages_alloced) &&
			   (!resources_exhausted(opts->resources)); in++)
	{
		Linkage_info *lifo = &link_info[N_linkages_post_processed];
		extract_links(indices[in], sent->null_count, sent->parse_info);
		if (set_has_fat_down(sent))
		{
			canonical = is_canonical_linkage(sent);
			if (only_canonical_allowed && !canonical) continue;
			*lifo = analyze_fat_linkage(sent, opts, PP_SECOND_PASS);
			lifo->fat = TRUE;
			lifo->canonical = canonical;
		}
		else
		{
			*lifo = analyze_thin_linkage(sent, opts, PP_SECOND_PASS);
			lifo->fat = FALSE;
			lifo->canonical = TRUE;
		}
		if (0 == lifo->N_violations)
		{
			N_valid_linkages++;
		}
		lifo->index = indices[in];
		lg_corpus_score(sent, lifo);
		N_linkages_post_processed++;
	}

	print_time(opts, "Postprocessed all linkages");
	qsort((void *)link_info, N_linkages_post_processed, sizeof(Linkage_info),
		  (int (*)(const void *, const void *)) opts->cost_model.compare_fn);

	if (!resources_exhausted(opts->resources))
	{
		if ((N_linkages_post_processed == 0) &&
		    (N_linkages_found > 0) &&
		    (N_linkages_found < opts->linkage_limit))
		{
			/* With the current parser, the following sentence will elicit
			 * this error:
			 *
			 * Well, say, Joe, you can be Friar Tuck or Much the miller's
			 * son, and lam me with a quarter-staff; or I'll be the Sheriff
			 * of Nottingham and you be Robin Hood a little while and kill
			 * me.
			 */
			err_ctxt ec;
			ec.sent = sent;
			err_msg(&ec, Error, "Error: None of the linkages is canonical\n"
			          "\tN_linkages_post_processed=%d "
			          "N_linkages_found=%d\n",
			          N_linkages_post_processed,
			          N_linkages_found);
		}
	}

	if (opts->verbosity > 1)
	{
		err_ctxt ec;
		ec.sent = sent;
		err_msg(&ec, Info, "Info: %d of %d linkages with no P.P. violations\n",
				N_valid_linkages, N_linkages_post_processed);
	}

	print_time(opts, "Sorted all linkages");

	sent->num_linkages_alloced = N_linkages_alloced;
	sent->num_linkages_post_processed = N_linkages_post_processed;
	sent->num_valid_linkages = N_valid_linkages;
	sent->link_info = link_info;

	xfree(indices, N_linkages_alloced * sizeof(int));
	/*if(N_valid_linkages == 0) free_andlists(sent); */
}

/***************************************************************
*
* Routines for creating and destroying processing Sentences
*
****************************************************************/

Sentence sentence_create(const char *input_string, Dictionary dict)
{
	Sentence sent;

	sent = (Sentence) xalloc(sizeof(struct Sentence_s));
	bzero(sent, sizeof(struct Sentence_s));
	sent->dict = dict;
	sent->length = 0;
	sent->num_linkages_found = 0;
	sent->num_linkages_alloced = 0;
	sent->num_linkages_post_processed = 0;
	sent->num_valid_linkages = 0;
	sent->link_info = NULL;
	sent->deletable = NULL;
	sent->effective_dist = NULL;
	sent->num_valid_linkages = 0;
	sent->null_count = 0;
	sent->parse_info = NULL;
	sent->string_set = string_set_create();

	sent->q_pruned_rules = FALSE;
	sent->is_conjunction = NULL;

	sent->dptr = NULL;
	sent->deletable = NULL;

	/* Make a copy of the input */
	sent->orig_sentence = string_set_add (input_string, sent->string_set);

	return sent;
}

static int split_sentence(Sentence sent, Parse_Options opts)
{
	int i;
	Dictionary dict = sent->dict;

	if (!separate_sentence(sent, opts))
	{
		return -1;
	}

	sent->q_pruned_rules = FALSE; /* for post processing */
	sent->is_conjunction = (char *) xalloc(sizeof(char)*sent->length);
	set_is_conjunction(sent);
	initialize_conjunction_tables(sent);

	for (i=0; i<sent->length; i++)
	{
		/* in case we free these before they set to anything else */
		sent->word[i].x = NULL;
		sent->word[i].d = NULL;
	}

	if (!(dict->unknown_word_defined && dict->use_unknown_word))
	{
		if (!sentence_in_dictionary(sent)) {
			return -1;
		}
	}

	return 0;
}

void sentence_delete(Sentence sent)
{
	if (!sent) return;
	/* free_andlists(sent); */
	free_sentence_disjuncts(sent);
	free_sentence_expressions(sent);
	string_set_delete(sent->string_set);
	free_parse_set(sent);
	free_post_processing(sent);
	post_process_close_sentence(sent->dict->postprocessor);
	free_deletable(sent);
	free_effective_dist(sent);
	free_count(sent);
	free_analyze(sent);
	if (sent->is_conjunction) xfree(sent->is_conjunction, sizeof(char)*sent->length);
	xfree((char *) sent, sizeof(struct Sentence_s));
}

int sentence_length(Sentence sent)
{
	if (!sent) return 0;
	return sent->length;
}

const char * sentence_get_word(Sentence sent, int index)
{
	if (!sent) return NULL;
	return sent->word[index].string;
}

const char * sentence_get_nth_word(Sentence sent, int index)
{
	if (!sent) return NULL;
	return sent->word[index].string;
}

int sentence_null_count(Sentence sent) {
	if (!sent) return 0;
	return sent->null_count;
}

int sentence_num_linkages_found(Sentence sent) {
	if (!sent) return 0;
	return sent->num_linkages_found;
}

int sentence_num_valid_linkages(Sentence sent) {
	if (!sent) return 0;
	return sent->num_valid_linkages;
}

int sentence_num_linkages_post_processed(Sentence sent) {
	if (!sent) return 0;
	return sent->num_linkages_post_processed;
}

int sentence_num_violations(Sentence sent, int i) {
	if (!sent) return 0;
	return sent->link_info[i].N_violations;
}

int sentence_and_cost(Sentence sent, int i) {
	if (!sent) return 0;
	return sent->link_info[i].and_cost;
}

int sentence_disjunct_cost(Sentence sent, int i) {
	if (!sent) return 0;
	return sent->link_info[i].disjunct_cost;
}

int sentence_link_cost(Sentence sent, int i) {
	if (!sent) return 0;
	return sent->link_info[i].link_cost;
}

int sentence_nth_word_has_disjunction(Sentence sent, int i)
{
	if (!sent) return 0;
	return (sent->parse_info->chosen_disjuncts[i] != NULL);
}

int sentence_parse(Sentence sent, Parse_Options opts)
{
	int rc;
	int nl;
	s64 total;

	verbosity = opts->verbosity;

	/* Cleanup stuff previously allocated. This is because some free 
	 * routines depend on sent-length, which might change in different
	 * parse-opts settings. 
	 */
	free_deletable(sent);

	rc = split_sentence(sent, opts);
	if (rc) return -1;

	/* Tokenize, look up each word in the dictionary, collect up all
	 * plausible disjunct expressions for each word. */
	if (!build_sentence_expressions(sent, opts)) {
		sent->num_valid_linkages = 0;
		return 0;
	}

	/* Initialize/free any leftover garbage */
	free_sentence_disjuncts(sent);
	resources_reset_space(opts->resources);

	if (resources_exhausted(opts->resources)) {
		sent->num_valid_linkages = 0;
		return 0;
	}

	init_analyze(sent);
	init_count(sent);

	/* Expressions were previously set up during the tokenize stage. */
	expression_prune(sent);
	print_time(opts, "Finished expression pruning");
#ifdef USE_SAT_SOLVER
	sat_parse(sent, opts);
#else

	/* Build lists of disjuncts */
	prepare_to_parse(sent, opts);

	init_fast_matcher(sent);
	init_table(sent);

	/* A parse set may have been already been built for this sentence,
	 * if it was previously parsed.  If so we free it up before
	 * building another.  */
	free_parse_set(sent);
	init_x_table(sent);

	for (nl = opts->min_null_count; nl<=opts->max_null_count ; ++nl)
	{
		if (resources_exhausted(opts->resources)) break;
		sent->null_count = nl;
		total = do_parse(sent, sent->null_count, opts);

		/* Give up if the parse count is overflowing */
		if (PARSE_NUM_OVERFLOW < total) break;

		sent->num_linkages_found = (int) total;
		print_time(opts, "Counted parses");
		post_process_linkages(sent, opts);
		if (sent->num_valid_linkages > 0) break;
	}

	free_table(sent);
	free_fast_matcher(sent);
#endif /* USE_SAT_SOLVER */
	print_time(opts, "Finished parse");

	return sent->num_valid_linkages;
}

/***************************************************************
*
* Routines which allow user access to Linkages.
*
****************************************************************/

Linkage linkage_create(int k, Sentence sent, Parse_Options opts)
{
	Linkage linkage;

	if ((k >= sent->num_linkages_post_processed) || (k < 0)) return NULL;

	/* Using exalloc since this is external to the parser itself. */
	linkage = (Linkage) exalloc(sizeof(struct Linkage_s));

	linkage->num_words = sent->length;
	linkage->word = (const char **) exalloc(linkage->num_words*sizeof(char *));
	linkage->current = 0;
	linkage->num_sublinkages=0;
	linkage->sublinkage = NULL;
	linkage->unionized = FALSE;
	linkage->sent = sent;
	linkage->opts = opts;
	linkage->info = &sent->link_info[k];
	linkage->dis_con_tree = NULL;

	extract_links(sent->link_info[k].index, sent->null_count, sent->parse_info);
	compute_chosen_words(sent, linkage);

	if (set_has_fat_down(sent))
	{
		extract_fat_linkage(sent, opts, linkage);
	}
	else
	{
		extract_thin_linkage(sent, opts, linkage);
	}

	if (sent->dict->postprocessor != NULL)
	{
	   linkage_post_process(linkage, sent->dict->postprocessor);
	}

	return linkage;
}

int linkage_get_current_sublinkage(Linkage linkage) { 
	return linkage->current; 
} 

int linkage_set_current_sublinkage(Linkage linkage, int index)
{
	if ((index < 0) ||
		(index >= linkage->num_sublinkages))
	{
		return 0;
	}
	linkage->current = index;
	return 1;
}

static void exfree_pp_info(PP_info *ppi)
{
	if (ppi->num_domains > 0)
		exfree(ppi->domain_name, sizeof(const char *)*ppi->num_domains);
	ppi->domain_name = NULL;
	ppi->num_domains = 0;
}

void linkage_delete(Linkage linkage)
{
	int i, j;
	Sublinkage *s;

	/* Can happen on panic timeout or user error */
	if (NULL == linkage) return;

	for (i=0; i<linkage->num_words; ++i)
	{
		exfree((void *) linkage->word[i], strlen(linkage->word[i])+1);
	}
	exfree(linkage->word, sizeof(char *)*linkage->num_words);

	for (i=0; i<linkage->num_sublinkages; ++i)
	{
		s = &(linkage->sublinkage[i]);
		for (j=0; j<s->num_links; ++j) {
			exfree_link(s->link[j]);
		}
		exfree(s->link, sizeof(Link)*s->num_links);
		if (s->pp_info != NULL) {
			for (j=0; j<s->num_links; ++j) {
				exfree_pp_info(&s->pp_info[j]);
			}
			exfree(s->pp_info, sizeof(PP_info)*s->num_links);
			s->pp_info = NULL;
			post_process_free_data(&s->pp_data);
		}
		if (s->violation != NULL) {
			exfree((void *) s->violation, sizeof(char)*(strlen(s->violation)+1));
		}
	}
	exfree(linkage->sublinkage, sizeof(Sublinkage)*linkage->num_sublinkages);
	if (linkage->dis_con_tree)
		free_DIS_tree(linkage->dis_con_tree);
	exfree(linkage, sizeof(struct Linkage_s));
}

static int links_are_equal(Link *l, Link *m)
{
	return ((l->l == m->l) && (l->r == m->r) && (strcmp(l->name, m->name)==0));
}

static int link_already_appears(Linkage linkage, Link *link, int a)
{
	int i, j;

	for (i=0; i<a; ++i) {
		for (j=0; j<linkage->sublinkage[i].num_links; ++j) {
			if (links_are_equal(linkage->sublinkage[i].link[j], link)) return TRUE;
		}
	}
	return FALSE;
}

static PP_info excopy_pp_info(PP_info ppi)
{
	PP_info newppi;
	int i;

	newppi.num_domains = ppi.num_domains;
	newppi.domain_name = (const char **) exalloc(sizeof(const char *)*ppi.num_domains);
	for (i=0; i<newppi.num_domains; ++i)
	{
		newppi.domain_name[i] = ppi.domain_name[i];
	}
	return newppi;
}


static Sublinkage unionize_linkage(Linkage linkage)
{
	int i, j, num_in_union=0;
	Sublinkage u;
	Link *link;
	const char *p;

	for (i=0; i<linkage->num_sublinkages; ++i) {
		for (j=0; j<linkage->sublinkage[i].num_links; ++j) {
			link = linkage->sublinkage[i].link[j];
			if (!link_already_appears(linkage, link, i)) num_in_union++;
		}
	}

	u.link = (Link **) exalloc(sizeof(Link *)*num_in_union);
	u.num_links = num_in_union;
	zero_sublinkage(&u);

	u.pp_info = (PP_info *) exalloc(sizeof(PP_info)*num_in_union);
	u.violation = NULL;
	u.num_links = num_in_union;

	num_in_union = 0;

	for (i=0; i<linkage->num_sublinkages; ++i) {
		for (j=0; j<linkage->sublinkage[i].num_links; ++j) {
			link = linkage->sublinkage[i].link[j];
			if (!link_already_appears(linkage, link, i)) {
				u.link[num_in_union] = excopy_link(link);
				u.pp_info[num_in_union] = excopy_pp_info(linkage->sublinkage[i].pp_info[j]);
				if (((p=linkage->sublinkage[i].violation) != NULL) &&
					(u.violation == NULL)) {
					char *s = (char *) exalloc((strlen(p)+1)*sizeof(char));
					strcpy(s, p);
					u.violation = s;
				}
				num_in_union++;
			}
		}
	}

	return u;
}

int linkage_compute_union(Linkage linkage)
{
	int i, num_subs=linkage->num_sublinkages;
	Sublinkage * new_sublinkage, *s;

	if (linkage->unionized) {
		linkage->current = linkage->num_sublinkages-1;
		return 0;
	}
	if (num_subs == 1) {
		linkage->unionized = TRUE;
		return 1;
	}

	new_sublinkage =
		(Sublinkage *) exalloc(sizeof(Sublinkage)*(num_subs+1));

	for (i=0; i<num_subs; ++i) {
		new_sublinkage[i] = linkage->sublinkage[i];
	}
	exfree(linkage->sublinkage, sizeof(Sublinkage)*num_subs);
	linkage->sublinkage = new_sublinkage;

	/* Zero out the new sublinkage, then unionize it. */
	s = &new_sublinkage[num_subs];
	s->link = NULL;
	s->num_links = 0;
	zero_sublinkage(s);
	linkage->sublinkage[num_subs] = unionize_linkage(linkage);

	linkage->num_sublinkages++;

	linkage->unionized = TRUE;
	linkage->current = linkage->num_sublinkages-1;
	return 1;
}

int linkage_get_num_sublinkages(Linkage linkage) {
	return linkage->num_sublinkages;
}

int linkage_get_num_words(Linkage linkage)
{
	return linkage->num_words;
}

int linkage_get_num_links(Linkage linkage)
{
	int current = linkage->current;
	return linkage->sublinkage[current].num_links;
}

static inline int verify_link_index(Linkage linkage, int index)
{
	if ((index < 0) ||
		(index >= linkage->sublinkage[linkage->current].num_links))
	{
		return 0;
	}
	return 1;
}

int linkage_get_link_length(Linkage linkage, int index)
{
	Link *link;
	int word_has_link[MAX_SENTENCE];
	int i, length;
	int current = linkage->current;

	if (!verify_link_index(linkage, index)) return -1;

	for (i=0; i<linkage->num_words+1; ++i) {
		word_has_link[i] = FALSE;
	}

	for (i=0; i<linkage->sublinkage[current].num_links; ++i) {
		link = linkage->sublinkage[current].link[i];
		word_has_link[link->l] = TRUE;
		word_has_link[link->r] = TRUE;
	}

	link = linkage->sublinkage[current].link[index];
	length = link->r - link->l;
	for (i= link->l+1; i < link->r; ++i) {
		if (!word_has_link[i]) length--;
	}
	return length;
}

int linkage_get_link_lword(Linkage linkage, int index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return -1;
	link = linkage->sublinkage[linkage->current].link[index];
	return link->l;
}

int linkage_get_link_rword(Linkage linkage, int index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return -1;
	link = linkage->sublinkage[linkage->current].link[index];
	return link->r;
}

const char * linkage_get_link_label(Linkage linkage, int index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return NULL;
	link = linkage->sublinkage[linkage->current].link[index];
	return link->name;
}

const char * linkage_get_link_llabel(Linkage linkage, int index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return NULL;
	link = linkage->sublinkage[linkage->current].link[index];
	return link->lc->string;
}

const char * linkage_get_link_rlabel(Linkage linkage, int index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return NULL;
	link = linkage->sublinkage[linkage->current].link[index];
	return link->rc->string;
}

const char ** linkage_get_words(Linkage linkage)
{
	return linkage->word;
}

Sentence linkage_get_sentence(Linkage linkage)
{
	return linkage->sent;
}

const char * linkage_get_disjunct(Linkage linkage, int w)
{
	if (NULL == linkage->info->disjunct_list_str)
	{
		lg_compute_disjunct_strings(linkage->sent, linkage->info);
	}
	return linkage->info->disjunct_list_str[w];
}

const char * linkage_get_word(Linkage linkage, int w)
{
	return linkage->word[w];
}

int linkage_unused_word_cost(Linkage linkage)
{
	return linkage->info->unused_word_cost;
}

int linkage_disjunct_cost(Linkage linkage)
{
	return linkage->info->disjunct_cost;
}

int linkage_and_cost(Linkage linkage)
{
	return linkage->info->and_cost;
}

int linkage_link_cost(Linkage linkage)
{
	return linkage->info->link_cost;
}

double linkage_corpus_cost(Linkage linkage)
{
	return linkage->info->corpus_cost;
}

int linkage_get_link_num_domains(Linkage linkage, int index)
{
	PP_info *pp_info;
	if (!verify_link_index(linkage, index)) return -1;
	pp_info = &linkage->sublinkage[linkage->current].pp_info[index];
	return pp_info->num_domains;
}

const char ** linkage_get_link_domain_names(Linkage linkage, int index)
{
	PP_info *pp_info;
	if (!verify_link_index(linkage, index)) return NULL;
	pp_info = &linkage->sublinkage[linkage->current].pp_info[index];
	return pp_info->domain_name;
}

const char * linkage_get_violation_name(Linkage linkage)
{
	return linkage->sublinkage[linkage->current].violation;
}

int linkage_is_canonical(Linkage linkage)
{
	return linkage->info->canonical;
}

int linkage_is_improper(Linkage linkage)
{
	return linkage->info->improper_fat_linkage;
}

int linkage_has_inconsistent_domains(Linkage linkage)
{
	return linkage->info->inconsistent_domains;
}

void linkage_post_process(Linkage linkage, Postprocessor * postprocessor)
{
	int N_sublinkages = linkage_get_num_sublinkages(linkage);
	Parse_Options opts = linkage->opts;
	Sentence sent = linkage->sent;
	Sublinkage * subl;
	PP_node * pp;
	int i, j, k;
	D_type_list * d;

	for (i = 0; i < N_sublinkages; ++i)
	{
		subl = &linkage->sublinkage[i];
		if (subl->pp_info != NULL)
		{
			for (j = 0; j < subl->num_links; ++j)
			{
				exfree_pp_info(&subl->pp_info[j]);
			}
			post_process_free_data(&subl->pp_data);
			exfree(subl->pp_info, sizeof(PP_info)*subl->num_links);
		}
		subl->pp_info = (PP_info *) exalloc(sizeof(PP_info)*subl->num_links);
		for (j = 0; j < subl->num_links; ++j)
		{
			subl->pp_info[j].num_domains = 0;
			subl->pp_info[j].domain_name = NULL;
		}
		if (subl->violation != NULL)
		{
			exfree((void *)subl->violation, sizeof(char)*(strlen(subl->violation)+1));
			subl->violation = NULL;
		}

		if (linkage->info->improper_fat_linkage)
		{
			pp = NULL;
		}
		else
		{
			pp = post_process(postprocessor, opts, sent, subl, FALSE);
			/* This can return NULL, for example if there is no
			   post-processor */
		}

		if (pp == NULL)
		{
			for (j = 0; j < subl->num_links; ++j)
			{
				subl->pp_info[j].num_domains = 0;
				subl->pp_info[j].domain_name = NULL;
			}
		}
		else
		{
			for (j = 0; j < subl->num_links; ++j)
			{
				k = 0;
				for (d = pp->d_type_array[j]; d != NULL; d = d->next) k++;
				subl->pp_info[j].num_domains = k;
				if (k > 0)
				{
					subl->pp_info[j].domain_name = (const char **) exalloc(sizeof(const char *)*k);
				}
				k = 0;
				for (d = pp->d_type_array[j]; d != NULL; d = d->next)
				{
					char buff[5];
					sprintf(buff, "%c", d->type);
					subl->pp_info[j].domain_name[k] = string_set_add (buff, sent->string_set);

					k++;
				}
			}
			subl->pp_data = postprocessor->pp_data;
			if (pp->violation != NULL)
			{
				char * s = (char *) exalloc(sizeof(char)*(strlen(pp->violation)+1));
				strcpy(s, pp->violation);
				subl->violation = s;
			}
		}
	}
	post_process_close_sentence(postprocessor);
}

