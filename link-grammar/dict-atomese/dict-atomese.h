/*
 * dict-atomese.h
 *
 * Dictionary configuration for AtomSpace access.
 *
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 */

#ifndef _LG_DICT_ATOMESE_H_
#define _LG_DICT_ATOMESE_H_

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/persist/api/StorageNode.h>

extern "C" {
#include <link-grammar/link-includes.h>
};

using namespace opencog;


/**
 * lg_config_atomspace() - Provide custom configuration for the dict.
 * This can be called, prior to opening the dictionary, to over-ride
 * the config settings in the `storage.dict` file.
 *
 * If `asp` is not null, then it will be used as the AtomSpace to work
 * with; else a private AtomSpace will be used. This AtomSpace will get
 * EvaluationLinks denoting the pairings between LG connectors and the
 * matching AS concept of connectors.
 *
 * If `stnp` is not null, then it will be used as the StorageNode from
 * which dictionary data is fetched.
 *
 * If `asp` is null, but `stnp` is not, then a private atomspace will be
 * used, and `stnp` will be used instead of the configuration in the
 * config file.
 *
 * If `asp` is not null, but `stnp` is null, then it is assumed that all
 * of the dictionary data is already available in the AtomSpace; there
 * will be no fetching of it from storage. The setting in the config
 * file will be ignored.
 */
link_public_api(void)
	lg_config_atomspace(AtomSpacePtr asp, StorageNodePtr stnp);

#endif // _LG_DICT_ATOMESE_H_
