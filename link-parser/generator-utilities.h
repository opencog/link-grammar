#ifdef _MSC_VER
#define LINK_GRAMMAR_DLL_EXPORT 0
#endif /* _MSC_VER */

#include <dict-api.h>

void dump_categories(const Dictionary, const Category *);
size_t print_sentences(const Category*,
                       Linkage, size_t nwords, const char** words,
                       bool subscript, double max_samples);


extern int verbosity_level;

#define MAX_SENTENCE 254
