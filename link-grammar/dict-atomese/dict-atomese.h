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

extern "C" {
#include <link-grammar/link-includes.h>
};

using namespace opencog;
void dict_set_atomspace(Dictionary dict, AtomSpacePtr);

#endif // _LG_DICT_ATOMESE_H_
