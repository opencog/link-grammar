/*
 * read-atomese.cc
 *
 * Use a dictionary located in the OpenCog AtomSpace.
 *
 * The goal of using a dictionary in the AtomSpace is that no distinct
 * data export step is required.  The dictionary can dynamically update,
 * even as it is being used.
 *
 * Copyright (c) 2014, 2022 Linas Vepstas <linasvepstas@gmail.com>
 */

#ifdef HAVE_ATOMESE

#define D_ATOMSPACE 5 /* Verbosity levels for this module are 5 and 6. */

extern "C" {
#include "api-structures.h"
#include "connectors.h"
#include "dict-common/dict-api.h"
#include "dict-common/dict-common.h"
#include "dict-common/dict-impl.h"
#include "dict-common/dict-structures.h"
#include "dict-common/dict-utils.h"      // patch_subscript()
#include "dict-common/file-utils.h"
#include "dict-file/read-dict.h"         // dictionary_six()
#include "error.h"
#include "externs.h"
#include "memory-pool.h"
#include "string-set.h"
#include "tokenize/spellcheck.h"
#include "utilities.h"

#include "read-atomese.h"

}; // extern "C"

#define COGSERVER_URL "cogserver-url"


static bool as_lookup(Dictionary dict, const char *s)
{
printf("duuude called as_lookup for %s\n", s);
	return false;
}

static Dict_node * as_lookup_list(Dictionary dict, const char *s)
{
printf("duuude called as_lookup_list for %s\n", s);
	return NULL;
}

static Dict_node * as_lookup_wild(Dictionary dict, const char *s)
{
printf("duuude called as_lookup_wild for %s\n", s);
	return NULL;
}

static void as_free_llist(Dictionary dict, Dict_node *llist)
{
	Dict_node * dn;
	while (llist != NULL)
	{
		dn = llist->right;
		free(llist);
		llist = dn;
	}
}

static void as_close(Dictionary dict)
{
printf("duuude called as_close\n");
}

Dictionary dictionary_create_from_atomese(const char *dictdir)
{

	/* Read basic configuration */
	char *cfg_name = join_path (dictdir, "cogserver.dict");
	Dictionary cfgd =
		dictionary_six(dictdir, cfg_name, NULL, NULL, NULL, NULL);
	if (cfgd == NULL)
	{
		prt_error("Error: Could not open cogserver configuration file %s\n",
			cfg_name);
		free(cfg_name);
		return NULL;
	}
	free(cfg_name);

	define_s cfg_defines = cfgd->define;
	memset(&cfgd->define, 0, sizeof(define_s));

	/* It's a temporary, we don't need it any more. */
	dictionary_delete(cfgd);
	cfgd = NULL;

	/* --------------------------------------------- */
	const char* url;
	Dictionary dict = (Dictionary) malloc(sizeof(struct Dictionary_s));
	memset(dict, 0, sizeof(struct Dictionary_s));

	/* Language and locale stuff */
	dict->define = cfg_defines;
	dict->string_set = string_set_create();

	const char* lang = linkgrammar_get_dict_define(dict, "dictionary-lang");
	dict->lang = string_set_add(lang, dict->string_set);
	dictionary_setup_locale(dict);
	lgdebug(D_USER_FILES, "Debug: Language: %s\n", dict->lang);

	dict->spell_checker = nullptr;
	dict->base_knowledge = NULL;
	dict->hpsg_knowledge = NULL;

	/* Setup the affix table */
	/* XXX FIXME -- need to pull this from atomspace, too. */
	char * affix_name = join_path (lang, "4.0.affix");
	dict->affix_table = dictionary_six(lang, affix_name, NULL, NULL, NULL, NULL);
	if (dict->affix_table == NULL)
	{
		prt_error("Error: Could not open affix file %s\n", affix_name);
		free(affix_name);
		goto failure;
	}
	free(affix_name);
	if (!afdict_init(dict)) goto failure;

	url = linkgrammar_get_dict_define(dict, COGSERVER_URL);
	if (NULL == url) goto failure;

	/* Set up the server connection */
	dict->as_server = NULL;

	/* Install backend methods */
	dict->lookup_list = as_lookup_list;
	dict->lookup_wild = as_lookup_wild;
	dict->free_lookup = as_free_llist;
	dict->lookup = as_lookup;
	dict->close = as_close;

	condesc_init(dict, 1<<8);

	dict->Exp_pool = pool_new(__func__, "Exp", /*num_elements*/4096,
	                          sizeof(Exp), /*zero_out*/false,
	                          /*align*/false, /*exact*/false);


#if 0
	if (!dictionary_setup_defines(dict))
		goto failure;

	/* Initialize word categories, for text generation. */
	if (dictionary_generation_request(dict))
		add_categories(dict);
#endif

	return dict;

failure:
	dictionary_delete(dict);
	return NULL;
}

#endif /* HAVE_ATOMESE */
