#include "util.hpp"

extern "C" {
#include "api-structures.h"
#include "disjunct-utils.h"
#include "linkage/linkage.h"
#include "memory-pool.h"
#include "utilities.h"
};

/**
 * Free all the connectors and disjuncts of a specific linkage.
 */
void free_linkage_connectors_and_disjuncts(Linkage lkg)
{
  // Free the connectors
  for(size_t i = 0; i < lkg->lasz; i++) {
    free(lkg->link_array[i].rc);
    free(lkg->link_array[i].lc);
  }
  // Free the disjuncts
  for (size_t i = 0; i < lkg->cdsz; i++) {
    free_disjuncts(lkg->chosen_disjuncts[i]);
  }
}

/**
 * Free all the connectors and disjuncts of all the linkages.
 */
void sat_free_linkages(Sentence sent, LinkageIdx next_linkage_index)
{
  Linkage lkgs = sent->lnkages;

  for (LinkageIdx li = 0; li < next_linkage_index; li++) {
    free_linkage_connectors_and_disjuncts(&lkgs[li]);
    free_linkage(&lkgs[li]);
  }
  free(lkgs);
  sent->lnkages = NULL;
  sent->num_linkages_alloced = 0;
}

Exp* null_exp()
{
  static Exp e;

  e.operand_next = NULL;
  e.type = AND_type;
  e.operand_first = NULL;

  return &e;
}

/* FIXME:
 * 1. Remove unused Exp functions when disjuncts are directly built.
 * 2. Unify remaining Exp functions with the similar ones in read-dict.c. */

/**
 * Allocate a new Exp node.
 */
Exp* Exp_create(Sentence sent)
{
  Exp* e = (Exp*)pool_alloc(sent->Exp_pool);
  return e;
}

static Exp* make_unary_node(Sentence sent, Exp* e)
{
  Exp* n;
  n = Exp_create(sent);
  n->type = AND_type;  /* these must be AND types */
  n->operand_next = NULL;
  n->cost = 0.0;
  n->operand_first = e;
  return n;
}

/**
 * Duplicate the given Exp node.
 * This is needed in case it is to be participate more than once in an
 * expression (one or more), so its operand_first and operand_next may be
 * different.
 */
Exp *Exp_create_dup(Sentence sent, Exp *old_e)
{
  Exp *new_e = Exp_create(sent);

  *new_e = *old_e;

  return new_e;
}

/**
 * Prepend the CONNECTOR node addit to the AND node orig.
 * (If orig is NULL, first create an AND node.)
 * Return the result.
 */
void add_anded_exp(Sentence sent, Exp*& orig, Exp* addit)
{
    if (orig == NULL)
    {
      orig = make_unary_node(sent, Exp_create_dup(sent, addit));
      orig->operand_first->operand_next = NULL;
    }
    else
    {
      // Prepend addit to the existing operands
      Exp *orig_operand_first = orig->operand_first;
      orig->operand_first = Exp_create_dup(sent, addit);
      orig->operand_first->operand_next = orig_operand_first;
    }

    // The updated orig is addit & orig
}

#if 0
bool isEndingInterpunction(const char* str)
{
  return strcmp(str, ".") == 0 ||
    strcmp(str, "?") == 0 ||
    strcmp(str, "!") == 0;
}
#endif
