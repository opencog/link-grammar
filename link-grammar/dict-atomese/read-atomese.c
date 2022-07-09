/*
 * read-atomese.c
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
#include "lookup-atomese.h"

#define ATOMESE_DICT "storage.dict"
#define STORAGE_NODE_STRING "storage-node"

Dictionary dictionary_create_from_atomese(const char *dictdir)
{
	/* Read basic configuration */
	char *cfg_name = join_path (dictdir, ATOMESE_DICT);
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

	String_set *ss = cfgd->string_set;
	cfgd->string_set = NULL;
	dfine_s cfg_defines = cfgd->dfine;
	memset(&cfgd->dfine, 0, sizeof(dfine_s));

	/* It's a temporary, we don't need it any more. */
	dictionary_delete(cfgd);
	cfgd = NULL;

	/* --------------------------------------------- */
	Dictionary dict = (Dictionary) malloc(sizeof(struct Dictionary_s));
	memset(dict, 0, sizeof(struct Dictionary_s));

	/* Language and locale stuff */
	dict->string_set = ss;
	dict->dfine = cfg_defines;

	const char* lang = linkgrammar_get_dict_define(dict, "dictionary-lang");
	dict->lang = string_set_add(lang, dict->string_set);
	dictionary_setup_locale(dict);
	lgdebug(D_USER_FILES, "Debug: Language: %s\n", dict->lang);

	dict->spell_checker = NULL;
	dict->base_knowledge = NULL;
	dict->hpsg_knowledge = NULL;
	dict->root = NULL;

	/* Setup the affix table */
	/* XXX FIXME -- need to pull this from atomspace, too. */
	char * affix_name = join_path (dictdir, "4.0.affix");
	dict->affix_table = dictionary_six(lang, affix_name, NULL, NULL, NULL, NULL);
	if (dict->affix_table == NULL)
	{
		prt_error("Error: Could not open affix file %s\n", affix_name);
		free(affix_name);
		goto failure;
	}
	free(affix_name);
	if (!afdict_init(dict)) goto failure;

	const char* sto_node =
		linkgrammar_get_dict_define(dict, STORAGE_NODE_STRING);
	if (NULL == sto_node) goto failure;

	/* Set up the server connection */
	as_open(dict, sto_node);

	/* Install backend methods */
	dict->lookup_list = as_lookup_list;
	dict->lookup_wild = as_lookup_wild;
	dict->free_lookup = as_free_llist;
	dict->exists_lookup = as_boolean_lookup;
	dict->clear_cache = as_clear_cache;
	dict->close = as_close;

	// If MI values are missing, every disjunct gets a cost of 4.0
	// Search for MISSING_MI in the other file.
	dict->default_max_disjunct_cost = 10.0;

	dict->dynamic_lookup = true;
	condesc_init(dict, 1<<8);

	dict->Exp_pool = pool_new(__func__, "Exp", /*num_elements*/4096,
	                          sizeof(Exp), /*zero_out*/false,
	                          /*align*/false, /*exact*/false);

	if (!dictionary_setup_defines(dict))
		goto failure;

#if 0
	/* Initialize word categories, for text generation. */
	if (dictionary_generation_request(dict))
		add_categories(dict);
#endif

	return dict;

failure:
	dictionary_delete(dict);
	return NULL;
}

void as_free_llist(Dictionary dict, Dict_node *llist)
{
	Dict_node * dn;
	while (llist != NULL)
	{
		dn = llist->right;
		free(llist);
		llist = dn;
	}
}

#endif /* HAVE_ATOMESE */
