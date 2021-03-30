#include "link-grammar/dict-common/dict-api.h"

void dump_categories(const Dictionary, const Category *);
void print_sentence(const Category*,
                    Linkage, size_t nwords, const char** words,
                    bool subscript, bool explode);
