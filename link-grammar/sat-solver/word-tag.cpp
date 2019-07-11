#include "word-tag.hpp"
#include "fast-sprintf.hpp"

extern "C" {
#ifdef DEBUG
#include "prepare/build-disjuncts.h"    // prt_exp_mem()
#include "dict-common/dict-api.h"       // print_expression()
#endif
#include "error.h"
#include "tokenize/tok-structures.h"
#include "utilities.h"
}

#define D_IC 8
void WordTag::insert_connectors(Exp* exp, int& dfs_position,
                                bool& leading_right, bool& leading_left,
                                std::vector<int>& eps_right,
                                std::vector<int>& eps_left,
                                char* var, bool root, double parent_cost,
                                Exp* parent_exp, const X_node *word_xnode)
{
  double cost = parent_cost + exp->cost;

#ifdef DEBUG
  if (0 && verbosity_level(+D_IC)) // Extreme debug
  //if (_word == 2)
  {
    const char*type =
      ((const char *[]) {"OR_type", "AND_type", "CONNECTOR_type"}) [exp->type-1];
    printf("Expression type %s for Word%d, var %s:\n", type, _word, var);
    //printf("parent_exp: "); print_expression(parent_exp);
    printf("exp(%s) e=%.2f pc=%.2f ", word_xnode->string,exp->cost, parent_cost);
    print_expression(exp);
    if (exp->cost > 0 || root) prt_exp_mem(exp, 0);
  }
#endif

  if (exp->type == CONNECTOR_type) {
    dfs_position++;

    switch (exp->dir) {
    case '+':
      _position.push_back(_right_connectors.size());
      _dir.push_back('+');
      _right_connectors.push_back(
           PositionConnector(parent_exp, exp, '+', _word, dfs_position,
                             cost, parent_cost, leading_right, false,
                             eps_right, eps_left, word_xnode, _opts));
      leading_right = false;
      break;
    case '-':
      _position.push_back(_left_connectors.size());
      _dir.push_back('-');
      _left_connectors.push_back(
           PositionConnector(parent_exp, exp, '-', _word, dfs_position,
                             cost, parent_cost, false, leading_left,
                             eps_right, eps_left, word_xnode, _opts));
      leading_left = false;
      break;
    default:
      throw std::string("Unknown connector direction: ") + exp->dir;
    }
  } else if (exp->type == AND_type) {
    if (exp->operand_first == NULL) {
      /* zeroary and */
      if (cost != 0)
      {
        lgdebug(+D_IC, "EmptyConnector var=%s(%d) cost %.2f pcost %.2f\n",
                var, _variables->string(var), cost, parent_cost);
        _empty_connectors.push_back(EmptyConnector(_variables->string(var),cost));
      }
    } else
      if (exp->operand_first->next == NULL) {
        /* unary and - skip */
        insert_connectors(exp->operand_first->e, dfs_position, leading_right,
             leading_left, eps_right, eps_left, var, root, cost, parent_exp, word_xnode);
      } else {
        int i;
        E_list* l;

        char new_var[MAX_VARIABLE_NAME];
        char* last_new_var = new_var;
        char* last_var = var;
        while ((*last_new_var = *last_var)) {
          last_new_var++;
          last_var++;
        }

        for (i = 0, l = exp->operand_first; l != NULL; l = l->next, i++) {
          char* s = last_new_var;
          *s++ = 'c';
          fast_sprintf(s, i);

          double and_cost = (i == 0) ? cost : 0;
          insert_connectors(l->e, dfs_position, leading_right, leading_left,
                eps_right, eps_left, new_var, false, and_cost, parent_exp, word_xnode);

          if (leading_right) {
            eps_right.push_back(_variables->epsilon(new_var, '+'));
          }
          if (leading_left) {
            eps_left.push_back(_variables->epsilon(new_var, '-'));
          }
        }
      }
  } else if (exp->type == OR_type) {
    if (exp->operand_first != NULL && exp->operand_first->next == NULL) {
      /* unary or - skip */
      insert_connectors(exp->operand_first->e, dfs_position, leading_right, leading_left,
          eps_right, eps_left, var, root, cost, exp->operand_first->e, word_xnode);
    } else {
      int i;
      E_list* l;
      bool ll_true = false;
      bool lr_true = false;

      char new_var[MAX_VARIABLE_NAME];
      char* last_new_var = new_var;
      char* last_var = var;
      while ((*last_new_var = *last_var)) {
        last_new_var++;
        last_var++;
      }

#ifdef DEBUG
      if (0 && verbosity_level(+D_IC)) { // Extreme debug
        printf("Word%d, var %s OR_type:\n", _word, var);
        printf("exp mem: "); prt_exp_mem(exp, 0);
      }
#endif

      for (i = 0, l = exp->operand_first; l != NULL; l = l->next, i++) {
        bool lr = leading_right, ll = leading_left;
        std::vector<int> er = eps_right, el = eps_left;

        char* s = last_new_var;
        *s++ = 'd';
        fast_sprintf(s, i);

        lgdebug(+D_IC, "Word%d: var: %s; exp%d=%p; X_node: %s\n",
                _word, var, i, l, word_xnode ? word_xnode->word->subword : "NULL X_node");
        assert(word_xnode != NULL, "NULL X_node for var %s", new_var);
        if ((NULL != word_xnode->next) && (l->e == word_xnode->next->exp))
          word_xnode = word_xnode->next;

        insert_connectors(l->e, dfs_position, lr, ll, er, el, new_var, false, cost, l->e, word_xnode);

        if (lr)
          lr_true = true;
        if (ll)
          ll_true = true;
      }
      leading_right = lr_true;
      leading_left = ll_true;
    }
  }
}
#undef D_IC

void WordTag::find_matches(int w, Connector* search_cntr, char dir, std::vector<PositionConnector*>& matches)
{
  // cout << "Look connection on: ." << _word << ". ." << w << ". " << search_cntr << dir << endl;

  std::vector<PositionConnector>* connectors;
  switch(dir) {
  case '+':
    connectors = &_left_connectors;
    break;
  case '-':
    connectors = &_right_connectors;
    break;
  default:
    throw std::string("Unknown connector direction: ") + dir;
  }

  std::vector<PositionConnector>::iterator i;
  for (i = connectors->begin(); i != connectors->end(); i++) {
    if (WordTag::match(w, *search_cntr, dir, (*i).word, ((*i).connector))) {
      matches.push_back(&(*i));
    }
  }
}

void WordTag::add_matches_with_word(WordTag& tag)
{
  std::vector<PositionConnector>::iterator i;
  for (i = _right_connectors.begin(); i != _right_connectors.end(); i++) {
    std::vector<PositionConnector*> connector_matches;
    tag.find_matches(_word, &(*i).connector, '+', connector_matches);
    std::vector<PositionConnector*>::iterator j;
    for (j = connector_matches.begin(); j != connector_matches.end(); j++) {
      i->matches.push_back(*j);
      set_match_possible((*j)->word, (*j)->position);
      (*j)->matches.push_back(&(*i));
      tag.set_match_possible(_word, (*i).position);
    }
  }
}
