
/**
 * Private functions for dictionary maintainers. Coude outside of
 * this directory should not call these functions.
 */
#ifndef _DICT_IMPL_H_
#define _DICT_IMPL_H_

#include "link-includes.h"
#include "dict-common/dict-common.h"        // For Afdict_class

void dictionary_setup_locale(Dictionary dict);
bool dictionary_setup_defines(Dictionary dict);
void afclass_init(Dictionary dict);
bool afdict_init(Dictionary dict);
void affix_list_add(Dictionary afdict, Afdict_class *, const char *);

#endif /* _DICT_IMPL_H_ */
