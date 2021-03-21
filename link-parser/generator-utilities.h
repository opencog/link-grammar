#include "link-grammar/dict-common/dict-api.h"

void dump_categories(const Dictionary, const Category *);
const char *cond_subscript(const char *, bool);
const char *select_word(const Category *, const Category_cost *, WordIdx);
