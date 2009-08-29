#include <link-grammar/api.h>

#ifdef USE_SAT_SOLVER
int sat_parse(Sentence sent, Parse_Options  opts);
#else
static inline int sat_parse(Sentence sent, Parse_Options  opts) { return -1; }
#endif
