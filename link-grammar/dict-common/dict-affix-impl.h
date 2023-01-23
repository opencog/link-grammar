
/**
 * Private affix dictionary implementation.
 */
#ifndef _DICT_AFFIX_IMPL_H_
#define _DICT_AFFIX_IMPL_H_

#include "link-includes.h"
#include "dict-common/dict-common.h"        // For Afdict_class

void afclass_init(Dictionary dict);
bool afdict_init(Dictionary dict);
void affix_list_add(Dictionary afdict, Afdict_class *, const char *);

#endif /* _DICT_AFFIX_IMPL_H_ */
