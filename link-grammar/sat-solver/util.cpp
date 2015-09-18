#include "util.hpp"

extern "C" {
#include "api-structures.h"
#include "structures.h"
};

/**
 * Free all the connectors and disjuncts of a specific linkage.
 */
void free_linkage_connectors_and_disjuncts(Linkage lkg)
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

Exp* null_exp()
{
  static Exp e;

  if (e.type) return &e;
  e.u.l = NULL;
  e.type = AND_type;
  return &e;
}

void add_anded_exp(Exp*& orig, Exp* addit)
{
    if (orig == NULL)
    {
      orig = addit;
    } else {
      // flist is orig
      E_list* flist = (E_list*)xalloc(sizeof(E_list));
      flist->e = orig;
      flist->next = NULL;

      // elist is addit, orig
      E_list* elist = (E_list*)xalloc(sizeof(E_list));
      elist->next = flist;
      elist->e = addit;

      // The updated orig is addit & orig
      orig = (Exp*)xalloc(sizeof(Exp));
      orig->type = AND_type;
      orig->cost = 0.0;
      orig->u.l = elist;
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
