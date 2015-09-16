#include "util.hpp"

extern "C" {
#include "api-structures.h"
#include "structures.h"
#include "disjunct-utils.h"
};

/**
 * Free all the connectors and disjuncts of a specific linkage.
 */
void free_linkage(Linkage lkg)
{
  // Free the connectors
  for(size_t i = 0; i < lkg->num_links; i++) {
    free(lkg->link_array[i].rc);
    free(lkg->link_array[i].lc);
  }
  // Free the disjuncts
  for (size_t i = 0; i < lkg->num_words; i++) {
    free_disjuncts(lkg->chosen_disjuncts[i]);
  }
}

bool isEndingInterpunction(const char* str)
{
  return strcmp(str, ".") == 0 ||
    strcmp(str, "?") == 0 ||
    strcmp(str, "!") == 0;
}

const char* word(Sentence sent, int w)
{
  // XXX FIXME this is fundamentally wrong, should explore all alternatives!
  return sent->word[w].alternatives[0];
}
