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


Dictionary dictionary_create_from_atomese(const char *dictdir)
{
	Dictionary cfgd;
	define_s cfg_defines;

	Dictionary dict;
	const char* url;

	/* Read basic configuration */
	char *cfg_name = join_path (dictdir, "cogserver.dict");
	cfgd = dictionary_six(dictdir, cfg_name, NULL, NULL, NULL, NULL);
	if (cfgd == NULL)
	{
		prt_error("Error: Could not open cogserver configuration file %s\n",
			cfg_name);
		free(cfg_name);
		return NULL;
	}
	free(cfg_name);

	cfg_defines = cfgd->define;
	memset(&cfgd->define, 0, sizeof(define_s));

	/* It's a temporary, we don't need it any more. */
	dictionary_delete(cfgd);
	cfgd = NULL;

	/* --------------------------------------------- */
	dict = (Dictionary) malloc(sizeof(struct Dictionary_s));
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
	dict->db_handle = NULL;

	url = linkgrammar_get_dict_define(dict, COGSERVER_URL);
	if (NULL == url)
	{
		dictionary_delete(cfgd);
		return NULL;
	}

#if 0
	/* Set up the database */
	dict->db_handle = object_open(dict->name, db_open, NULL);

	dict->lookup_list = db_lookup_list;
	dict->lookup_wild = db_lookup_wild;
	dict->free_lookup = db_free_llist;
	dict->lookup = db_lookup;
	dict->close = db_close;
#endif

	condesc_init(dict, 1<<8);

	dict->Exp_pool = pool_new(__func__, "Exp", /*num_elements*/4096,
	                          sizeof(Exp), /*zero_out*/false,
	                          /*align*/false, /*exact*/false);

#if 0
	/* Setup the affix table */
	char *affix_name = join_path (lang, "4.0.affix");
	dict->affix_table = dictionary_six(lang, affix_name, NULL, NULL, NULL, NULL);
	if (dict->affix_table == NULL)
	{
		prt_error("Error: Could not open affix file %s\n", affix_name);
		free(affix_name);
		goto failure;
	}
	free(affix_name);
	if (!afdict_init(dict))
		goto failure;

	if (!dictionary_setup_defines(dict))
		goto failure;

	/* Initialize word categories, for text generation. */
	if (dictionary_generation_request(dict))
		add_categories(dict);
#endif

	return dict;

#if 0
failure:
	dictionary_delete(dict);
	return NULL;
#endif
}

#endif /* HAVE_ATOMESE */
