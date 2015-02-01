/**
 * Encode link-grammar for solving with the SAT solver...
 */
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <iterator>
using std::cout;
using std::cerr;
using std::endl;

#include "sat-encoder.hpp"
#include "variables.hpp"
#include "word-tag.hpp"
#include "matrix-ut.hpp"
#include "clock.hpp"
#include "fast-sprintf.hpp"
#include "minisat/Solver.h"


extern "C" {
#include "analyze-linkage.h"
#include "build-disjuncts.h"
#include "dict-file/read-dict.h"
#include "extract-links.h"
#include "linkage.h"
#include "post-process.h"
#include "preparation.h"
#include "score.h"
#include "utilities.h"
}

// Macro DEBUG_print is used to dump to stdout information while debugging
#ifdef _DEBUG
#define DEBUG_print(x) (cout << x << endl)
#else
#define DEBUG_print(x)
#endif

/*-------------------------------------------------------------------------*
 *               C N F   C O N V E R S I O N                               *
 *-------------------------------------------------------------------------*/
void SATEncoder::generate_literal(Lit l) {
  vec<Lit> clause(1);
  clause[0] = l;
  add_clause(clause);
}

void SATEncoder::generate_and_definition(Lit lhs, vec<Lit>& rhs) {
  vec<Lit> clause;
  for (int i = 0; i < rhs.size(); i++) {
    clause.growTo(2);
    clause[0] = ~lhs;
    clause[1] = rhs[i];
    add_clause(clause);
  }

  for (int i = 0; i < rhs.size(); i++) {
    clause.growTo(2);
    clause[0] = ~rhs[i];
    clause[1] = lhs;
    add_clause(clause);
  }
}
void SATEncoder::generate_classical_and_definition(Lit lhs, vec<Lit>& rhs) {
  {
    vec<Lit> clause;
    for (int i = 0; i < rhs.size(); i++) {
      clause.growTo(2);

      clause[0] = ~lhs;
      clause[1] = rhs[i];
      add_clause(clause);
    }
  }

  {
    vec<Lit> clause(rhs.size() + 1);
    for (int i = 0; i < rhs.size(); i++) {
      clause[i] = ~rhs[i];
    }
    clause[rhs.size()] = lhs;
    add_clause(clause);
  }
}

void SATEncoder::generate_or_definition(Lit lhs, vec<Lit>& rhs) {
  {
    vec<Lit> clause;
    for (int i = 0; i < rhs.size(); i++) {
      clause.growTo(2);
      clause[0] = lhs;
      clause[1] = ~rhs[i];
      add_clause(clause);
    }
  }

  {
    vec<Lit> clause(rhs.size() + 1);
    for (int i = 0; i < rhs.size(); i++) {
      clause[i] = rhs[i];
    }
    clause[rhs.size()] = ~lhs;
    add_clause(clause);
  }
}

void SATEncoder::generate_conditional_lr_implication_or_definition(Lit condition, Lit lhs, vec<Lit>& rhs) {
  {
    vec<Lit> clause;
    for (int i = 0; i < rhs.size(); i++) {
      clause.growTo(2);
      clause[0] = lhs;
      clause[1] = ~rhs[i];
      add_clause(clause);
    }
  }

  {
    vec<Lit> clause(rhs.size() + 2);
    for (int i = 0; i < rhs.size(); i++) {
      clause[i] = rhs[i];
    }
    clause[rhs.size()] = ~lhs;
    clause[rhs.size()+1] = ~condition;
    add_clause(clause);
  }
}

void SATEncoder::generate_conditional_lr_implication_or_definition(Lit condition1, Lit condition2, Lit lhs, vec<Lit>& rhs) {
  {
    vec<Lit> clause;
    for (int i = 0; i < rhs.size(); i++) {
      clause.growTo(2);
      clause[0] = lhs;
      clause[1] = ~rhs[i];
      add_clause(clause);
    }
  }

  {
    vec<Lit> clause(rhs.size() + 3);
    for (int i = 0; i < rhs.size(); i++) {
      clause[i] = rhs[i];
    }
    clause[rhs.size()] = ~lhs;
    clause[rhs.size()+1] = ~condition1;
    clause[rhs.size()+2] = ~condition2;
    add_clause(clause);
  }
}

void SATEncoder::generate_xor_conditions(vec<Lit>& vect) {
  vec<Lit> clause;
  for (int i = 0; i < vect.size(); i++) {
    for (int j = i + 1; j < vect.size(); j++) {
      if (vect[i] == vect[j])
        continue;
      clause.growTo(2);
      clause[0] = ~vect[i];
      clause[1] = ~vect[j];
      add_clause(clause);
    }
  }
}

void SATEncoder::generate_and(vec<Lit>& vect) {
  for (int i = 0; i < vect.size(); i++) {
    generate_literal(vect[i]);
  }
}


void SATEncoder::generate_or(vec<Lit>& vect) {
  add_clause(vect);
}

void SATEncoder::generate_equivalence_definition(Lit l1, Lit l2) {
  {
    vec<Lit> clause(2);
    clause[0] = ~l1;
    clause[1] = l2;
    add_clause(clause);
  }
  {
    vec<Lit> clause(2);
    clause[0] = l1;
    clause[1] = ~l2;
    add_clause(clause);
  }
}


/*-------------------------------------------------------------------------*
 *                          E N C O D I N G                                *
 *-------------------------------------------------------------------------*/
void SATEncoder::encode() {
    Clock clock;
    generate_satisfaction_conditions();
    DEBUG_print(clock.elapsed());
    generate_linked_definitions();
    DEBUG_print(clock.elapsed());
    generate_planarity_conditions();
    DEBUG_print(clock.elapsed());

#ifdef _CONNECTIVITY_
    generate_connectivity();
    DEBUG_print(clock.elapsed());
#endif

    generate_encoding_specific_clauses();
    DEBUG_print(clock.elapsed());

    pp_prune();
    power_prune();
    DEBUG_print(clock.elapsed());

    _variables->setVariableParameters(_solver);
}



/*-------------------------------------------------------------------------*
 *                         W O R D - T A G S                               *
 *-------------------------------------------------------------------------*/
void SATEncoder::build_word_tags()
{
  for (size_t w = 0; w < _sent->length; w++) {
    _word_tags.push_back(WordTag(w, _variables, _sent, _opts));
    int dfs_position = 0;

    if (_sent->word[w].x == NULL) {
      DEBUG_print("Word ." << w << ".: " << _sent->word[w].unsplit_word << " (null)" <<  endl);
      continue;
    }

    bool join = _sent->word[w].x->next != NULL;
    Exp* exp = join ? join_alternatives(w) : _sent->word[w].x->exp;

#ifdef _DEBUG
    cout << "Word ." << w << ".: " << _sent->word[w].unsplit_word << endl;
    //print_expression(exp);
    cout << endl;
#endif

    char name[MAX_VARIABLE_NAME];
    sprintf(name, "w%zu", w);
    bool leading_right = true;
    bool leading_left = true;
    std::vector<int> eps_right, eps_left;

    _word_tags[w].insert_connectors(exp, dfs_position, leading_right,
         leading_left, eps_right, eps_left, name, true, 0, NULL);

    if (join)
      free_alternatives(exp);
  }

  for (size_t wl = 0; wl < _sent->length - 1; wl++) {
    for (size_t wr = wl + 1; wr < _sent->length; wr++) {
      _word_tags[wl].add_matches_with_word(_word_tags[wr]);
    }
  }
}

void SATEncoder::find_all_matches_between_words(size_t w1, size_t w2,
                                                std::vector<std::pair<const PositionConnector*, const PositionConnector*> >& matches) {
  const std::vector<PositionConnector>& w1_right = _word_tags[w1].get_right_connectors();
  std::vector<PositionConnector>::const_iterator i;
  for (i = w1_right.begin(); i != w1_right.end(); i++) {
    const std::vector<PositionConnector*>& w2_left_c = (*i).matches;
    std::vector<PositionConnector*>::const_iterator j;
    for (j = w2_left_c.begin(); j != w2_left_c.end(); j++) {
      if ((*j)->word == w2) {
        matches.push_back(std::pair<const PositionConnector*, const PositionConnector*>(&(*i), *j));
      }
    }
  }
}

bool SATEncoder::matches_in_interval(int wi, int pi, int l, int r) {
  for (int w = l; w < r; w++) {
    if (_word_tags[w].match_possible(wi, pi))
      return true;
  }
  return false;
}

/*-------------------------------------------------------------------------*
 *                     S A T I S F A C T I O N                             *
 *-------------------------------------------------------------------------*/

void SATEncoder::generate_satisfaction_conditions()
{
  for (size_t w = 0; w < _sent->length; w++) {
    if (_sent->word[w].x == NULL) {
      DEBUG_print("Word: " << _sent->word[w].unsplit_word << " : " << "(null)" << endl);
      handle_null_expression(w);
      continue;
    }

    bool join = _sent->word[w].x->next != NULL;
    Exp* exp = join ? join_alternatives(w) : _sent->word[w].x->exp;

#ifdef _DEBUG
    cout << "Word: " << _sent->word[w].unsplit_word << endl;
    //print_expression(exp);
    cout << endl;
#endif

    char name[MAX_VARIABLE_NAME];
    sprintf(name, "w%zu", w);

    determine_satisfaction(w, name);
    int dfs_position = 0;
    generate_satisfaction_for_expression(w, dfs_position, exp, name, 0);

    if (join)
      free_alternatives(exp);
  }
}


void SATEncoder::generate_satisfaction_for_expression(int w, int& dfs_position, Exp* e,
                                                      char* var, double parent_cost)
{
  E_list *l;
  double total_cost = parent_cost + e->cost;

  if (e->type == CONNECTOR_type) {
    dfs_position++;

    generate_satisfaction_for_connector(w, dfs_position, e, var);

    if (total_cost > _cost_cutoff) {
      Lit lhs = Lit(_variables->string_cost(var, e->cost));
      generate_literal(~lhs);
    }
  } else {
    if (e->type == AND_type) {
      if (e->u.l == NULL) {
        /* zeroary and */
        _variables->string_cost(var, e->cost);
        if (total_cost > _cost_cutoff) {
          generate_literal(~Lit(_variables->string_cost(var, e->cost)));
        }
      } else if (e->u.l != NULL && e->u.l->next == NULL) {
        /* unary and - skip */
        generate_satisfaction_for_expression(w, dfs_position, e->u.l->e, var, total_cost);
      } else {
        /* n-ary and */
        int i;

        char new_var[MAX_VARIABLE_NAME];
        char* last_new_var = new_var;
        char* last_var = var;
        while((*last_new_var = *last_var)) {
          last_new_var++;
          last_var++;
        }

        vec<Lit> rhs;
        for (i = 0, l=e->u.l; l!=NULL; l=l->next, i++) {
          // sprintf(new_var, "%sc%d", var, i)
          char* s = last_new_var;
          *s++ = 'c';
          fast_sprintf(s, i);
          rhs.push(Lit(_variables->string(new_var)));
        }

        Lit lhs = Lit(_variables->string_cost(var, e->cost));
        generate_and_definition(lhs, rhs);

        /* Preceeds */
        int dfs_position_tmp = dfs_position;
        for (l = e->u.l; l->next != NULL; l = l->next) {
          generate_conjunct_order_constraints(w, l->e, l->next->e, dfs_position_tmp);
        }

        /* Recurse */
        for (i = 0, l=e->u.l; l!=NULL; l=l->next, i++) {
          // sprintf(new_var, "%sc%d", var, i)
          char* s = last_new_var;
          *s++ = 'c';
          fast_sprintf(s, i);

          generate_satisfaction_for_expression(w, dfs_position, l->e, new_var, total_cost);
        }
      }
    } else if (e->type == OR_type) {
      if (e->u.l == NULL) {
        /* zeroary or */
        cerr << "Zeroary OR" << endl;
        exit(EXIT_FAILURE);
      } else if (e->u.l != NULL && e->u.l->next == NULL) {
        /* unary or */
        generate_satisfaction_for_expression(w, dfs_position, e->u.l->e, var, total_cost);
      } else {
        /* n-ary or */
        int i;

        char new_var[MAX_VARIABLE_NAME];
        char* last_new_var = new_var;
        char* last_var = var;
        while ((*last_new_var = *last_var)) {
          last_new_var++;
          last_var++;
        }

        vec<Lit> rhs;
        for (i = 0, l=e->u.l; l!=NULL; l=l->next, i++) {
          // sprintf(new_var, "%sc%d", var, i)
          char* s = last_new_var;
          *s++ = 'd';
          fast_sprintf(s, i);
          rhs.push(Lit(_variables->string(new_var)));
        }

        Lit lhs = Lit(_variables->string_cost(var, e->cost));
        generate_or_definition(lhs, rhs);
        generate_xor_conditions(rhs);

        /* Recurse */
        for (i = 0, l=e->u.l; l!=NULL; l=l->next, i++) {
          char* s = last_new_var;
          *s++ = 'd';
          fast_sprintf(s, i);

          generate_satisfaction_for_expression(w, dfs_position, l->e, new_var, total_cost);
        }
      }
    }
  }
}

Exp* SATEncoder::join_alternatives(int w)
{
  // join all alternatives using and OR_type node
  Exp* exp;
  E_list* or_list = NULL;;
  for (X_node* x = _sent->word[w].x; x != NULL; x = x->next) {
    E_list* new_node = (E_list*) xalloc(sizeof(E_list));
    new_node->e = x->exp;
    new_node->next = NULL;
    if (or_list == NULL) {
      or_list = new_node;
    } else {
      E_list *y;
      for (y = or_list; y->next != NULL; y = y->next)
        ;
      y->next = new_node;
    }
  }
  exp = (Exp*) xalloc(sizeof(Exp));
  exp->type = OR_type;
  exp->u.l = or_list;
  exp->cost = 0.0;

  return exp;
}

void SATEncoder::free_alternatives(Exp* exp)
{
  E_list *l = exp->u.l;
  while (l != NULL) {
    E_list* next = l->next;
    xfree(l, sizeof(E_list));
    l = next;
  }
  xfree(exp, sizeof(exp));
}


void SATEncoder::generate_link_cw_ordinary_definition(size_t wi, int pi,
                                                      Exp* e, size_t wj)
{
  const char* Ci = e->u.string;
  char dir = e->dir;
  double cost = e->cost;
  Lit lhs = Lit(_variables->link_cw(wj, wi, pi, Ci));

  char str[MAX_VARIABLE_NAME];
  sprintf(str, "w%zd", wj);
  Lit condition = Lit(_variables->string(str));

  vec<Lit> rhs;

  // Collect matches (wi, pi) with word wj
  std::vector<PositionConnector*>& matches = _word_tags[wi].get(pi)->matches;
  std::vector<PositionConnector*>::const_iterator i;
  for (i = matches.begin(); i != matches.end(); i++) {
    /* TODO: PositionConnector->matches[wj] */
    if ((*i)->word != wj)
      continue;

    if (dir == '+') {
      rhs.push(Lit(_variables->link_cost(wi, pi, Ci, e,
                                         (*i)->word, (*i)->position,
                                         (*i)->connector->string,
                                         (*i)->exp,
                                         cost + (*i)->cost)));
    } else if (dir == '-'){
      rhs.push(Lit(_variables->link((*i)->word, (*i)->position,
                                    (*i)->connector->string,
                                    (*i)->exp,
                                    wi, pi, Ci, e)));
    }
  }

  // Generate clauses
  DEBUG_print("--------- link_cw as ordinary down");
  generate_conditional_lr_implication_or_definition(condition, lhs, rhs);
  generate_xor_conditions(rhs);
  DEBUG_print("--------- end link_cw as ordinary down");
}

/*--------------------------------------------------------------------------*
 *               C O N J U N C T    O R D E R I N G                         *
 *--------------------------------------------------------------------------*/
int SATEncoder::num_connectors(Exp* e)
{
  if (e->type == CONNECTOR_type)
    return 1;
  else {
    int num = 0;
    for (E_list* l = e->u.l; l != NULL; l = l->next) {
      num += num_connectors(l->e);
    }
    return num;
  }
}

int SATEncoder::empty_connectors(Exp* e, char dir)
{
  if (e->type == CONNECTOR_type) {
    return e->dir != dir;
  } else if (e->type == OR_type) {
    for (E_list* l = e->u.l; l != NULL; l = l->next) {
      if (empty_connectors(l->e, dir))
        return true;
    }
    return false;
  } else if (e->type == AND_type) {
    for (E_list* l = e->u.l; l != NULL; l = l->next) {
      if (!empty_connectors(l->e, dir))
        return false;
    }
    return true;
  } else
    throw std::string("Unkown connector type");
}

int SATEncoder::non_empty_connectors(Exp* e, char dir)
{
  if (e->type == CONNECTOR_type) {
    return e->dir == dir;
  } else if (e->type == OR_type) {
    for (E_list* l = e->u.l; l != NULL; l = l->next) {
      if (non_empty_connectors(l->e, dir))
        return true;
    }
    return false;
  } else if (e->type == AND_type) {
    for (E_list* l = e->u.l; l != NULL; l = l->next) {
      if (non_empty_connectors(l->e, dir))
        return true;
    }
    return false;
  } else
    throw std::string("Unkown connector type");
}

bool SATEncoder::trailing_connectors_and_aux(int w, E_list* l, char dir, int& dfs_position,
                                             std::vector<PositionConnector*>& connectors)
{
  if (l == NULL) {
    return true;
  } else {
    int dfs_position_in = dfs_position;
    dfs_position += num_connectors(l->e);
    if (trailing_connectors_and_aux(w, l->next, dir, dfs_position, connectors)) {
      trailing_connectors(w, l->e, dir, dfs_position_in, connectors);
    }
    return empty_connectors(l->e, dir);
  }
}

void SATEncoder::trailing_connectors(int w, Exp* exp, char dir, int& dfs_position,
                                     std::vector<PositionConnector*>& connectors)
{
  if (exp->type == CONNECTOR_type) {
    dfs_position++;
    if (exp->dir == dir) {
      connectors.push_back(_word_tags[w].get(dfs_position));
    }
  } else if (exp->type == OR_type) {
    for (E_list* l = exp->u.l; l != NULL; l = l->next) {
      trailing_connectors(w, l->e, dir, dfs_position, connectors);
    }
  } else if (exp->type == AND_type) {
    trailing_connectors_and_aux(w, exp->u.l, dir, dfs_position, connectors);
  }
}

void SATEncoder::certainly_non_trailing(int w, Exp* exp, char dir, int& dfs_position,
                                       std::vector<PositionConnector*>& connectors, bool has_right) {
  if (exp->type == CONNECTOR_type) {
    dfs_position++;
    if (exp->dir == dir && has_right) {
      connectors.push_back(_word_tags[w].get(dfs_position));
    }
  } else if (exp->type == OR_type) {
    for (E_list* l = exp->u.l; l != NULL; l = l->next) {
      certainly_non_trailing(w, l->e, dir, dfs_position, connectors, has_right);
    }
  } else if (exp->type == AND_type) {
    if (has_right) {
      for (E_list* l = exp->u.l; l != NULL; l = l->next) {
        certainly_non_trailing(w, l->e, dir, dfs_position, connectors, true);
      }
    } else {
      for (E_list* l = exp->u.l; l != NULL; l = l->next) {
        has_right = false;
        for (E_list* m = l->next; m != NULL; m = m->next) {
          if (non_empty_connectors(m->e, dir) && !empty_connectors(m->e, dir)) {
            has_right = true;
            break;
          }
        }
        certainly_non_trailing(w, l->e, dir, dfs_position, connectors, has_right);
      }
    }
  }
}

void SATEncoder::leading_connectors(int w, Exp* exp, char dir, int& dfs_position, vector<PositionConnector*>& connectors) {
  if (exp->type == CONNECTOR_type) {
    dfs_position++;
    if (exp->dir == dir) {
      connectors.push_back(_word_tags[w].get(dfs_position));
    }
  } else if (exp->type == OR_type) {
    for (E_list* l = exp->u.l; l != NULL; l = l->next) {
      leading_connectors(w, l->e, dir, dfs_position, connectors);
    }
  } else if (exp->type == AND_type) {
    E_list* l;
    for (l = exp->u.l; l != NULL; l = l->next) {
      leading_connectors(w, l->e, dir, dfs_position, connectors);
      if (!empty_connectors(l->e, dir))
        break;
    }

    if (l != NULL) {
      for (l = l->next; l != NULL; l = l->next)
        dfs_position += num_connectors(l->e);
    }
  }
}

void SATEncoder::generate_conjunct_order_constraints(int w, Exp *e1, Exp* e2, int& dfs_position) {
  DEBUG_print("----- conjunct order constraints");
  int dfs_position_e1 = dfs_position;
  std::vector<PositionConnector*> last_right_in_e1, first_right_in_e2;
  trailing_connectors(w, e1, '+', dfs_position_e1, last_right_in_e1);

  int dfs_position_e2 = dfs_position_e1;
  leading_connectors(w, e2, '+', dfs_position_e2, first_right_in_e2);

  vec<Lit> clause;

  if (!last_right_in_e1.empty() && !first_right_in_e2.empty()) {
    std::vector<PositionConnector*>::iterator i, j;
    for (i = last_right_in_e1.begin(); i != last_right_in_e1.end(); i++) {
      std::vector<PositionConnector*>& matches_e1 = (*i)->matches;

      for (j = first_right_in_e2.begin(); j != first_right_in_e2.end(); j++) {
        std::vector<PositionConnector*>& matches_e2 = (*j)->matches;
        std::vector<PositionConnector*>::const_iterator m1, m2;

        std::vector<int> mw1;
        for (m1 = matches_e1.begin(); m1 != matches_e1.end(); m1++) {
          if ((m1+1) == matches_e1.end() || (*m1)->word != (*(m1 + 1))->word)
            mw1.push_back((*m1)->word);
        }

        std::vector<int> mw2;
        for (m2 = matches_e2.begin(); m2 != matches_e2.end(); m2++) {
          if ((m2+1) == matches_e2.end() || (*m2)->word != (*(m2 + 1))->word)
            mw2.push_back((*m2)->word);
        }

        std::vector<int>::const_iterator mw1i, mw2i;
        for (mw1i = mw1.begin(); mw1i != mw1.end(); mw1i++) {
          for (mw2i = mw2.begin(); mw2i != mw2.end(); mw2i++) {
            if (*mw1i >= *mw2i) {
              clause.growTo(2);
              clause[0] = ~Lit(_variables->link_cw(*mw1i, w, (*i)->position, (*i)->connector->string));
              clause[1] = ~Lit(_variables->link_cw(*mw2i, w, (*j)->position, (*j)->connector->string));
              add_clause(clause);
            }
          }
        }
      }
    }
  }

  DEBUG_print("----");
  dfs_position_e1 = dfs_position;
  std::vector<PositionConnector*> last_left_in_e1, first_left_in_e2;
  trailing_connectors(w, e1, '-', dfs_position_e1, last_left_in_e1);

  dfs_position_e2 = dfs_position_e1;
  leading_connectors(w, e2, '-', dfs_position_e2, first_left_in_e2);

  if (!last_left_in_e1.empty() && !first_left_in_e2.empty()) {
    std::vector<PositionConnector*>::iterator i, j;
    for (i = last_left_in_e1.begin(); i != last_left_in_e1.end(); i++) {
      std::vector<PositionConnector*>& matches_e1 = (*i)->matches;

      for (j = first_left_in_e2.begin(); j != first_left_in_e2.end(); j++) {
        std::vector<PositionConnector*>& matches_e2 = (*j)->matches;

        std::vector<PositionConnector*>::const_iterator m1, m2;


        std::vector<int> mw1;
        for (m1 = matches_e1.begin(); m1 != matches_e1.end(); m1++) {
          if ((m1+1) == matches_e1.end() || (*m1)->word != (*(m1 + 1))->word)
            mw1.push_back((*m1)->word);
        }

        std::vector<int> mw2;
        for (m2 = matches_e2.begin(); m2 != matches_e2.end(); m2++) {
          if ((m2+1) == matches_e2.end() || (*m2)->word != (*(m2 + 1))->word)
            mw2.push_back((*m2)->word);
        }

        std::vector<int>::const_iterator mw1i, mw2i;
        for (mw1i = mw1.begin(); mw1i != mw1.end(); mw1i++) {
          for (mw2i = mw2.begin(); mw2i != mw2.end(); mw2i++) {
            if (*mw1i <= *mw2i) {
              clause.growTo(2);
              clause[0] = ~Lit(_variables->link_cw(*mw1i, w, (*i)->position, (*i)->connector->string));
              clause[1] = ~Lit(_variables->link_cw(*mw2i, w, (*j)->position, (*j)->connector->string));
              add_clause(clause);
            }
          }
        }
      }
    }
  }

  dfs_position = dfs_position_e1;

  DEBUG_print("---end conjunct order constraints");
}



/*--------------------------------------------------------------------------*
 *                       C O N N E C T I V I T Y                            *
 *--------------------------------------------------------------------------*/

#ifdef _CONNECTIVITY_
void SATEncoder::generate_connectivity() {
  for (int w = 1; w < _sent->length; w++) {
    if (!_linkage_possible(0, w)) {
      vec<Lit> clause;
      clause.push(~Lit(_variables->con(w, 1)));
      generate_and(clause);
    }
    else {
      generate_equivalence_definition(Lit(_variables->con(w, 1)),
                                      Lit(_variables->linked(0, w)));
    }
  }

  for (int d = 2; d < _sent->length; d++) {
    for (int w = 1; w < _sent->length; w++) {
      Lit lhs = Lit(_variables->con(w, d));

      vec<Lit> rhs;
      rhs.push(Lit(_variables->con(w, d-1)));
      for (int w1 = 1; w1 < _sent->length; w1++) {
        if (w == w1)
          continue;

        if (!_linkage_possible(std::min(w, w1), std::max(w, w1))) {
          continue;
        }
        rhs.push(Lit(_variables->l_con(w, w1, d-1)));
      }
      generate_or_definition(lhs, rhs);


      /* lc definitions*/
      for (int w1 = 1; w1 < _sent->length; w1++) {
        if (w == w1)
          continue;

        int mi = std::min(w, w1), ma = std::max(w, w1);
        if (!_linked_possible(mi, ma))
          continue;

        Lit lhs = Lit(_variables->l_con(w, w1, d-1));
        vec<Lit> rhs(2);
        rhs[0] = Lit(_variables->linked(mi, ma));
        rhs[1] = Lit(_variables->con(w1, d-1));
        generate_classical_and_definition(lhs, rhs);
      }
    }
  }


  for (int w = 1; w < _sent->length; w++) {
    generate_literal(Lit(_variables->con(w, _sent->length-1)));
  }
}
#endif

void SATEncoder::dfs(int node, const MatrixUpperTriangle<int>& graph, int component, std::vector<int>& components) {
  if (components[node] != -1)
    return;

  components[node] = component;
  for (size_t other_node = node + 1; other_node < components.size(); other_node++) {
    if (graph(node, other_node))
      dfs(other_node, graph, component, components);
  }
  for (int other_node = 0; other_node < node; other_node++) {
    if (graph(other_node, node))
      dfs(other_node, graph, component, components);
  }
}


bool SATEncoder::connectivity_components(std::vector<int>& components) {
  // get satisfied linked(wi, wj) variables
  const std::vector<int>& linked_variables = _variables->linked_variables();
  std::vector<int> satisfied_linked_variables;
  for (std::vector<int>::const_iterator i = linked_variables.begin(); i != linked_variables.end(); i++) {
    int var = *i;
    if (_solver->model[var] == l_True) {
      satisfied_linked_variables.push_back(var);
    }
  }

  // build the connectivity graph
  MatrixUpperTriangle<int> graph(_sent->length, 0);
  std::vector<int>::const_iterator i;
  for (i = satisfied_linked_variables.begin(); i != satisfied_linked_variables.end(); i++) {
    graph.set(_variables->linked_variable(*i)->left_word,
              _variables->linked_variable(*i)->right_word, 1);
  }

  // determine the connectivity components
  components.resize(_sent->length);
  std::fill(components.begin(), components.end(), -1);
  for (size_t node = 0; node < _sent->length; node++)
    dfs(node, graph, node, components);
  bool connected = true;
  for (size_t node = 0; node < _sent->length; node++) {
    if (components[node] != 0) {
      connected = false;
    }
  }

  return connected;
}

void SATEncoder::generate_disconnectivity_prohibiting(std::vector<int> components) {
  // vector of unique components
  std::vector<int> different_components = components;
  std::sort(different_components.begin(), different_components.end());
  different_components.erase(std::unique(different_components.begin(), different_components.end()),
                             different_components.end());

  // Each connected component must contain a branch going out of it
  std::vector<int>::const_iterator c;
  for (c = different_components.begin(); c != different_components.end(); c++) {
    vec<Lit> clause;
    const std::vector<int>& linked_variables = _variables->linked_variables();
    for (std::vector<int>::const_iterator i = linked_variables.begin(); i != linked_variables.end(); i++) {
      int var = *i;
      const Variables::LinkedVar* lv = _variables->linked_variable(var);
      if ((components[lv->left_word] == *c && components[lv->right_word] != *c) ||
          (components[lv->left_word] != *c && components[lv->right_word] == *c)) {
        clause.push(Lit(var));
      }
    }
    _solver->addConflictingClause(clause);
    if (different_components.size() == 2)
      break;
  }
}

/*--------------------------------------------------------------------------*
 *                           P L A N A R I T Y                              *
 *--------------------------------------------------------------------------*/
void SATEncoder::generate_planarity_conditions()
{
  vec<Lit> clause;
  for (size_t wi1 = 0; wi1 < _sent->length; wi1++) {
    for (size_t wj1 = wi1+1; wj1 < _sent->length; wj1++) {
      for (size_t wi2 = wj1+1; wi2 < _sent->length; wi2++) {
        if (!_linked_possible(wi1, wi2))
          continue;
        for (size_t wj2 = wi2+1; wj2 < _sent->length; wj2++) {
          if (!_linked_possible(wj1, wj2))
            continue;
          clause.growTo(2);
          clause[0] = ~Lit(_variables->linked(wi1, wi2));
          clause[1] = ~Lit(_variables->linked(wj1, wj2));
          add_clause(clause);
        }
      }
    }
  }

  //  generate_linked_min_max_planarity();
}

/*--------------------------------------------------------------------------*
 *               P O W E R     P R U N I N G                                *
 *--------------------------------------------------------------------------*/

void SATEncoder::generate_epsilon_definitions()
{
  for (size_t w = 0; w < _sent->length; w++) {
    if (_sent->word[w].x == NULL) {
      continue;
    }

    bool join = _sent->word[w].x->next != NULL;
    Exp* exp = join ? join_alternatives(w) : _sent->word[w].x->exp;

    char name[MAX_VARIABLE_NAME];
    sprintf(name, "w%zu", w);

    int dfs_position;

    dfs_position = 0;
    generate_epsilon_for_expression(w, dfs_position, exp, name, true, '+');

    dfs_position = 0;
    generate_epsilon_for_expression(w, dfs_position, exp, name, true, '-');

    if (join)
      free_alternatives(exp);
  }
}

bool SATEncoder::generate_epsilon_for_expression(int w, int& dfs_position, Exp* e,
                                                 char* var, bool root, char dir)
{
  if (e->type == CONNECTOR_type) {
    dfs_position++;
    if (var != NULL) {
      if (e->dir == dir) {
        // generate_literal(-_variables->epsilon(name, var, e->dir));
        return false;
      } else {
        generate_equivalence_definition(Lit(_variables->epsilon(var, dir)),
                                        Lit(_variables->string(var)));
        return true;
      }
    }
  } else if (e->type == AND_type) {
    if (e->u.l == NULL) {
      /* zeroary and */
      generate_equivalence_definition(Lit(_variables->string(var)),
                                      Lit(_variables->epsilon(var, dir)));
      return true;
    } else
      if (e->u.l != NULL && e->u.l->next == NULL) {
        /* unary and - skip */
        return generate_epsilon_for_expression(w, dfs_position, e->u.l->e, var, root, dir);
      } else {
        /* Recurse */
        E_list* l;
        int i;
        bool eps = true;

        char new_var[MAX_VARIABLE_NAME];
        char* last_new_var = new_var;
        char* last_var = var;
        while ((*last_new_var = *last_var)) {
          last_new_var++;
          last_var++;
        }


        for (i = 0, l = e->u.l; l != NULL; l = l->next, i++) {
          // sprintf(new_var, "%sc%d", var, i)
          char* s = last_new_var;
          *s++ = 'c';
          fast_sprintf(s, i);

          if (!generate_epsilon_for_expression(w, dfs_position, l->e, new_var, false, dir)) {
            eps = false;
            break;
          }
        }

        if (l != NULL) {
          for (l = l->next; l != NULL; l = l->next)
            dfs_position += num_connectors(l->e);
        }

        if (!root && eps) {
          Lit lhs = Lit(_variables->epsilon(var, dir));
          vec<Lit> rhs;
          for (i = 0, l=e->u.l; l!=NULL; l=l->next, i++) {
            // sprintf(new_var, "%sc%d", var, i)
            char* s = last_new_var;
            *s++ = 'c';
            fast_sprintf(s, i);
            rhs.push(Lit(_variables->epsilon(new_var, dir)));
          }
          generate_classical_and_definition(lhs, rhs);
        }
        return eps;
      }
  } else if (e->type == OR_type) {
    if (e->u.l != NULL && e->u.l->next == NULL) {
      /* unary or - skip */
      return generate_epsilon_for_expression(w, dfs_position, e->u.l->e, var, root, dir);
    } else {
      /* Recurse */
      E_list* l;
      int i;
      bool eps = false;

      char new_var[MAX_VARIABLE_NAME];
      char* last_new_var = new_var;
      char* last_var = var;
      while ((*last_new_var = *last_var)) {
        last_new_var++;
        last_var++;
      }

      vec<Lit> rhs;
      for (i = 0, l = e->u.l; l != NULL; l = l->next, i++) {
        // sprintf(new_var, "%sc%d", var, i)
        char* s = last_new_var;
        *s++ = 'd';
        fast_sprintf(s, i);

        if (generate_epsilon_for_expression(w, dfs_position, l->e, new_var, false, dir) && !root) {
          rhs.push(Lit(_variables->epsilon(new_var, dir)));
          eps = true;
        }
      }

      if (!root && eps) {
        Lit lhs = Lit(_variables->epsilon(var, dir));
        generate_or_definition(lhs, rhs);
      }

      return eps;
    }
  }
  return false;
}

void SATEncoder::power_prune()
{
  generate_epsilon_definitions();

  // on two non-adjacent words, a pair of connectors can be used only
  // if not [both of them are the deepest].

  for (size_t wl = 0; wl < _sent->length - 2; wl++) {
    const std::vector<PositionConnector>& rc = _word_tags[wl].get_right_connectors();
    std::vector<PositionConnector>::const_iterator rci;
    for (rci = rc.begin(); rci != rc.end(); rci++) {
      if (!(*rci).leading_right || (*rci).connector->multi)
        continue;

      const std::vector<PositionConnector*>& matches = rci->matches;
      for (std::vector<PositionConnector*>::const_iterator lci = matches.begin(); lci != matches.end(); lci++) {
        if (!(*lci)->leading_left || (*lci)->connector->multi || (*lci)->word <= wl + 2)
          continue;

        //        printf("LR: .%d. .%d. %s\n", wl, rci->position, rci->connector->string);
        //        printf("LL: .%d. .%d. %s\n", (*lci)->word, (*lci)->position, (*lci)->connector->string);

        vec<Lit> clause;
        for (std::vector<int>::const_iterator i = rci->eps_right.begin(); i != rci->eps_right.end(); i++) {
          clause.push(~Lit(*i));
        }

        for (std::vector<int>::const_iterator i = (*lci)->eps_left.begin(); i != (*lci)->eps_left.end(); i++) {
          clause.push(~Lit(*i));
        }

        add_additional_power_pruning_conditions(clause, wl, (*lci)->word);

        clause.push(~Lit(_variables->link(
               wl, rci->position, rci->connector->string, rci->exp,
               (*lci)->word, (*lci)->position, (*lci)->connector->string, (*lci)->exp)));
        add_clause(clause);
      }
    }
  }

  /*
  // on two adjacent words, a pair of connectors can be used only if
  // they're the deepest ones on their disjuncts
  for (int wl = 0; wl < _sent->length - 2; wl++) {
    const std::vector<PositionConnector>& rc = _word_tags[wl].get_right_connectors();
    std::vector<PositionConnector>::const_iterator rci;
    for (rci = rc.begin(); rci != rc.end(); rci++) {
      if (!(*rci).leading_right)
        continue;

      const std::vector<PositionConnector*>& matches = rci->matches;
      for (std::vector<PositionConnector*>::const_iterator lci = matches.begin(); lci != matches.end(); lci++) {
        if (!(*lci)->leading_left || (*lci)->word != wl + 1)
          continue;
        int link = _variables->link(wl, rci->position, rci->connector->string,
                                    (*lci)->word, (*lci)->position, (*lci)->connector->string);
        std::vector<int> clause(2);
        clause[0] = -link;

        for (std::vector<int>::const_iterator i = rci->eps_right.begin(); i != rci->eps_right.end(); i++) {
          clause[1] = *i;
        }

        for (std::vector<int>::const_iterator i = (*lci)->eps_left.begin(); i != (*lci)->eps_left.end(); i++) {
          clause[1] = *i;
        }

        add_clause(clause);
      }
    }
  }


  // Two deep connectors cannot match (deep means notlast)
  std::vector<std::vector<PositionConnector*> > certainly_deep_left(_sent->length), certainly_deep_right(_sent->length);
  for (int w = 0; w < _sent->length; w++) {
    if (_sent->word[w].x == NULL)
      continue;

    bool join = _sent->word[w].x->next != NULL;
    Exp* exp = join ? join_alternatives(w) : _sent->word[w].x->exp;

    int dfs_position = 0;
    certainly_non_trailing(w, exp, '+', dfs_position, certainly_deep_right[w], false);
    dfs_position = 0;
    certainly_non_trailing(w, exp, '-', dfs_position, certainly_deep_left[w], false);

    if (join)
      free_alternatives(exp);
  }

  for (int w = 0; w < _sent->length; w++) {
    std::vector<PositionConnector*>::const_iterator i;
    for (i = certainly_deep_right[w].begin(); i != certainly_deep_right[w].end(); i++) {
      const std::vector<PositionConnector*>& matches = (*i)->matches;
      std::vector<PositionConnector*>::const_iterator j;
      for (j = matches.begin(); j != matches.end(); j++) {
        if (std::find(certainly_deep_left[(*j)->word].begin(), certainly_deep_left[(*j)->word].end(),
                      *j) != certainly_deep_left[(*j)->word].end()) {
          generate_literal(-_variables->link((*i)->word, (*i)->position, (*i)->connector->string,
                                             (*j)->word, (*j)->position, (*j)->connector->string));
        }
      }
    }
  }
  */
}

/*--------------------------------------------------------------------------*
 *        P O S T P R O C E S S I N G   &  P P     P R U N I N G            *
 *--------------------------------------------------------------------------*/
void SATEncoder::pp_prune()
{
  const std::vector<int>& link_variables = _variables->link_variables();

  if (_sent->dict->base_knowledge == NULL)
    return;

  pp_knowledge * knowledge;
  knowledge = _sent->dict->base_knowledge;

  for (size_t i=0; i<knowledge->n_contains_one_rules; i++)
  {
    pp_rule rule = knowledge->contains_one_rules[i];
    // const char * selector = rule.selector;         /* selector string for this rule */
    pp_linkset * link_set = rule.link_set;            /* the set of criterion links */


    vec<Lit> triggers;
    for (size_t i = 0; i < link_variables.size(); i++) {
      const Variables::LinkVar* var = _variables->link_variable(link_variables[i]);
      if (post_process_match(rule.selector, var->label)) {
        triggers.push(Lit(link_variables[i]));
      }
    }

    if (triggers.size() == 0)
      continue;

    vec<Lit> criterions;
    for (size_t j = 0; j < link_variables.size(); j++) {
      pp_linkset_node *p;
      for (size_t hashval = 0; hashval < link_set->hash_table_size; hashval++) {
        for (p = link_set->hash_table[hashval]; p!=NULL; p=p->next) {
          const Variables::LinkVar* var = _variables->link_variable(link_variables[j]);
          if (post_process_match(p->str, var->label)) {
            criterions.push(Lit(link_variables[j]));
          }
        }
      }
    }

    DEBUG_print("---pp_pruning--");
    for (int k = 0; k < triggers.size(); k++) {
      vec<Lit> clause(criterions.size() + 1);
      for (int i = 0; i < criterions.size(); i++)
        clause[i] = criterions[i];
      clause[criterions.size()] = (~triggers[k]);
      add_clause(clause);
    }
    DEBUG_print("---end pp_pruning--");
  }
}

/*--------------------------------------------------------------------------*
 *                         D E C O D I N G                                  *
 *--------------------------------------------------------------------------*/

/* This is very similar to the api call linkage_create(), except that
 * sat_extract_links is called, instead of normal extract_links().
 * It would be good to refactor this and the other to make them even
 * more similar, because right now, its confusing ...
 */
Linkage SATEncoder::create_linkage()
{
  /* Using exalloc since this is external to the parser itself. */
  Linkage linkage = (Linkage) exalloc(sizeof(struct Linkage_s));
  memset(linkage, 0, sizeof(struct Linkage_s));
  partial_init_linkage(linkage, _sent->length);

  sat_extract_links(linkage);

  return linkage;
}

void SATEncoder::generate_linkage_prohibiting()
{
  vec<Lit> clause;
  const std::vector<int>& link_variables = _variables->link_variables();
  for (std::vector<int>::const_iterator i = link_variables.begin(); i != link_variables.end(); i++) {
    int var = *i;
    if (_solver->model[var] == l_True) {
      clause.push(~Lit(var));
    } else if (_solver->model[var] == l_False) {
      clause.push(Lit(var));
    }
  }
  _solver->addConflictingClause(clause);
}

Linkage SATEncoder::get_next_linkage()
{
  if (l_False == _solver->solve()) return NULL;
  Linkage linkage = create_linkage();
  Linkage lkg = NULL;

  std::vector<int> components;
  bool connected = connectivity_components(components);
  if (connected) {
    // num_connected_linkages++;

    // Expand the linkage array.
    int index = _sent->num_linkages_alloced;
    _sent->num_linkages_alloced++;
    size_t nbytes = _sent->num_linkages_alloced * sizeof(struct Linkage_s);
    _sent->lnkages = (Linkage) realloc(_sent->lnkages, nbytes);

    lkg = &_sent->lnkages[index];
    *lkg = *linkage;  /* copy en-mass */
    exfree(linkage, sizeof(struct Linkage_s));
    linkage = NULL;

    // perform the post-processing
    sane_linkage_morphism(_sent, lkg, _opts);
    linkage_post_process(lkg, _sent->postprocessor, _opts);
    linkage_score(lkg, _opts);

    if (0 == lkg->lifo.N_violations) {
      _sent->num_valid_linkages++;
    }

    // sane_morphism will increment N_violations if its insane...
    if (0 == lkg->lifo.N_violations) {
      cout << "Linkage PP OK" << endl;
    } else {
      cout << "Linkage PP NOT OK" << endl;
    }

    generate_linkage_prohibiting();
  } else {
    cout << "Linkage DISCONNECTED" << endl;
    generate_disconnectivity_prohibiting(components);
    exfree(linkage, sizeof(struct Linkage_s));
  }

  _solver->printStats();
  return lkg;
}

/*******************************************************************************
 *        SAT encoding for sentences that do not contain conjunction.          *
 *******************************************************************************/

void SATEncoderConjunctionFreeSentences::generate_encoding_specific_clauses() {
}

void SATEncoderConjunctionFreeSentences::handle_null_expression(int w) {
  // Formula is unsatisfiable
  vec<Lit> clause;
  add_clause(clause);
}

void SATEncoderConjunctionFreeSentences::determine_satisfaction(int w, char* name)
{
  // All tags must be satisfied
  generate_literal(Lit(_variables->string(name)));
}

void SATEncoderConjunctionFreeSentences::generate_satisfaction_for_connector(
    int wi, int pi, Exp *e, char* var)
{
  const char* Ci = e->u.string;
  char dir = e->dir;
  bool multi = e->multi;
  double cost = e->cost;

  Lit lhs = Lit(_variables->string_cost(var, cost));

#ifdef _DEBUG
  cout << "*** Connector: ." << wi << ". ." << pi << ". " << Ci << dir << endl;
#endif

  // words that can potentially match (wi, pi)
  int low = (dir == '-') ? 0 : wi + 1;
  int hi  = (dir == '-') ? wi : _sent->length;

  std::vector<int> _w_;
  for (int wj = low; wj < hi; wj++) {
    // Eliminate words that cannot be connected to (wi, pi)
    if (!_word_tags[wj].match_possible(wi, pi))
      continue;

    // Now we know that there is a potential link between the
    // connector (wi, pi) and the word wj
    _w_.push_back(wj);

    generate_link_cw_ordinary_definition(wi, pi, e, wj);
  }

  vec<Lit> _link_cw_;
  for (size_t i = 0; i < _w_.size(); i++)
    _link_cw_.push(Lit(_variables->link_cw(_w_[i], wi, pi, Ci)));
  generate_or_definition(lhs, _link_cw_);

  DEBUG_print("--------- multi");
  if (!multi) {
    generate_xor_conditions(_link_cw_);
  }
  DEBUG_print("--------- end multi");

#ifdef _DEBUG
  cout << "*** End Connector: ." << wi << ". ." << pi << ". " <<  Ci << endl;
#endif
}

void SATEncoderConjunctionFreeSentences::generate_linked_definitions()
{
  _linked_possible.resize(_sent->length, 1);

  DEBUG_print("------- linked definitions");
  for (size_t w1 = 0; w1 < _sent->length; w1++) {
    for (size_t w2 = w1 + 1; w2 < _sent->length; w2++) {
      DEBUG_print("---------- ." << w1 << ". ." << w2 << ".");
      Lit lhs = Lit(_variables->linked(w1, w2));

      vec<Lit> rhs;
      const std::vector<PositionConnector>& w1_connectors = _word_tags[w1].get_right_connectors();
      std::vector<PositionConnector>::const_iterator c;
      for (c = w1_connectors.begin(); c != w1_connectors.end(); c++) {
        assert(c->word == w1, "Connector word must match");
        if (_word_tags[w2].match_possible(c->word, c->position)) {
          rhs.push(Lit(_variables->link_cw(w2, c->word, c->position, c->connector->string)));
        }
      }

      _linked_possible.set(w1, w2, rhs.size() > 0);
      generate_or_definition(lhs, rhs);
    }
  }
  DEBUG_print("------- end linked definitions");
}

void SATEncoder::generate_linked_min_max_planarity()
{
  DEBUG_print("---- linked_max");
  for (size_t w1 = 0; w1 < _sent->length; w1++) {
    for (size_t w2 = 0; w2 < _sent->length; w2++) {
      Lit lhs = Lit(_variables->linked_max(w1, w2));
      vec<Lit> rhs;
      if (w2 < _sent->length - 1) {
        rhs.push(Lit(_variables->linked_max(w1, w2 + 1)));
        if (w1 != w2 + 1 && _linked_possible(std::min(w1, w2+1), std::max(w1, w2+1)))
          rhs.push(~Lit(_variables->linked(std::min(w1, w2+1), std::max(w1, w2+1))));
      }
      generate_classical_and_definition(lhs, rhs);
    }
  }
  DEBUG_print("---- end linked_max");


  DEBUG_print("---- linked_min");
  for (size_t w1 = 0; w1 < _sent->length; w1++) {
    for (size_t w2 = 0; w2 < _sent->length; w2++) {
      Lit lhs = Lit(_variables->linked_min(w1, w2));
      vec<Lit> rhs;
      if (w2 > 0) {
        rhs.push(Lit(_variables->linked_min(w1, w2 - 1)));
        if (w1 != w2-1 && _linked_possible(std::min(w1, w2 - 1), std::max(w1, w2 - 1)))
          rhs.push(~Lit(_variables->linked(std::min(w1, w2 - 1), std::max(w1, w2 - 1))));
      }
      generate_classical_and_definition(lhs, rhs);
    }
  }
  DEBUG_print("---- end linked_min");


  vec<Lit> clause;
  for (size_t wi = 3; wi < _sent->length; wi++) {
    for (size_t wj = 1; wj < wi - 1; wj++) {
      for (size_t wl = wj + 1; wl < wi; wl++) {
        clause.growTo(3);
        clause[0] = ~Lit(_variables->linked_min(wi, wj));
        clause[1] = ~Lit(_variables->linked_max(wi, wl - 1));
        clause[2] = Lit(_variables->linked_min(wl, wj));
        add_clause(clause);
      }
    }
  }

  DEBUG_print("------------");

  for (size_t wi = 0; wi < _sent->length - 1; wi++) {
    for (size_t wj = wi + 1; wj < _sent->length - 1; wj++) {
      for (size_t wl = wi+1; wl < wj; wl++) {
        clause.growTo(3);
        clause[0] = ~Lit(_variables->linked_max(wi, wj));
        clause[1] = ~Lit(_variables->linked_min(wi, wl + 1));
        clause[2] = Lit(_variables->linked_max(wl, wj));
        add_clause(clause);
      }
    }
  }

  DEBUG_print("------------");
  clause.clear();
  for (size_t wi = 1; wi < _sent->length; wi++) {
    for (size_t wj = wi + 2; wj < _sent->length - 1; wj++) {
      for (size_t wl = wi + 1; wl < wj; wl++) {
        clause.growTo(2);
        clause[0] = ~Lit(_variables->linked_min(wi, wj));
        clause[1] = Lit(_variables->linked_min(wl, wi));
        add_clause(clause);

        clause.growTo(2);
        clause[0] = ~Lit(_variables->linked_max(wj, wi));
        clause[1] = Lit(_variables->linked_max(wl, wj));
        add_clause(clause);
      }
    }
  }
}

bool SATEncoderConjunctionFreeSentences::sat_extract_links(Linkage lkg)
{

  int current_link = 0;
  const std::vector<int>& link_variables = _variables->link_variables();
  std::vector<int>::const_iterator i;
  for (i = link_variables.begin(); i != link_variables.end(); i++) {
    if (_solver->model[*i] != l_True)
      continue;

    const Variables::LinkVar* var = _variables->link_variable(*i);
    if (_solver->model[_variables->linked(var->left_word, var->right_word)] != l_True)
      continue;

    check_link_size(lkg);
    Link& clink = lkg->link_array[current_link];
    clink.lw = var->left_word;
    clink.rw = var->right_word;

    Connector* connector;

    // XXX Should not alloc a new one, but use the connector from
    // the PositionConnector class ...
    connector = connector_new();
    connector->string = var->left_connector;
    connector->word = var->left_word; // XXX is this needed ?
    clink.lc = connector;

    connector = connector_new();
    connector->string = var->right_connector;
    connector->word = var->right_word; // XXX is this needed ?
    clink.rc = connector;

    // Indicate the disjuncts, too.
    // This is needed so that compute_chosen_word works correctly.

// FIXME -- this is deeply and fundamentally broken. First, we should not
// use unsplit_word below, but instead, get the string from the X_node.
// However, the X_node was discarded previously, up above.  Next, we are
// trying to reconstruct the actual disjunct that was used, but we aren't
// really doing that correctly, because we never recorded the DNF that
// was used.  Maybe .. maybe we could hack around this by looking at the
// actual connectors that were used for each word, and combine those to
// get the disjunct ... so its a mess.
// Its not ever clear to me why chosen_words needs disjuncts .. really
// all we need here are the X_node strings, and nothing else, and so ...
// all teh exp flow through this code could be removed. Arghhh.
    Disjunct *d;

    // XXX FIXME -- the chosen disjunct has probably already been
    // set for this word .. don't set it again.  Oh, and it should
    // be consistent, too ...
    d = build_disjuncts_for_exp(var->left_exp,
                                _sent->word[var->left_word].unsplit_word,
                                UNLIMITED_LEN);
    _sent->word[var->left_word].d = d;
    lkg->chosen_disjuncts[clink.lw] = d;

    d = build_disjuncts_for_exp(var->right_exp,
                                _sent->word[var->right_word].unsplit_word,
                                 UNLIMITED_LEN);
    _sent->word[var->right_word].d = d;
    lkg->chosen_disjuncts[clink.rw] = d;

    current_link++;
  }

  lkg->num_links = current_link;
  compute_link_names(lkg, _sent->string_set);

  DEBUG_print("Total: ." <<  lkg->num_links << "." << endl);
  return false;
}


/****************************************************************************
 *              Main entry point into the SAT parser                        *
 ****************************************************************************/
extern "C" int sat_parse(Sentence sent, Parse_Options  opts)
{
  SATEncoder* encoder = (SATEncoder*) sent->hook;
  if (encoder) delete encoder;

  // Prepare for parsing - extracted for "preparation.c"
  encoder = new SATEncoderConjunctionFreeSentences(sent, opts);
  sent->hook = encoder;
  encoder->encode();

  // XXX this is wrong, we actually don't know yet ...
  // If we don't return some large number here, then the
  // Command-line client will fail to print all of the possible
  // linkages. Work around this by lying ...
  sent->num_valid_linkages = 222;
  sent->num_linkages_post_processed = 222;

  return 0;
}

// XXX Caution -- assumes that k increases by one every time this
// is called ... we should check and assert if this is not the case.
// XXX FIXME
extern "C" Linkage sat_create_linkage(int k, Sentence sent, Parse_Options  opts)
{
  SATEncoder* encoder = (SATEncoder*) sent->hook;
  if (!encoder) return NULL;

  return encoder->get_next_linkage();
}

extern "C" void sat_sentence_delete(Sentence sent)
{
  SATEncoder* encoder = (SATEncoder*) sent->hook;
  if (!encoder) return;
  delete encoder;
}
