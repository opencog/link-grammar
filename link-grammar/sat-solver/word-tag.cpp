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

#define D_IC 6
void WordTag::insert_connectors(Exp* exp, int& dfs_position,
                                bool& leading_right, bool& leading_left,
                                std::vector<int>& eps_right,
                                std::vector<int>& eps_left,
                                char* var, bool root, double parent_cost,
                                Exp* parent_exp, const X_node *word_xnode)
{
  double cost = parent_cost + exp->cost;

#ifdef DEBUG
  if (0 && verbosity_level(+D_IC)) { // Extreme debug
    printf("Expression type %d for Word%d, var %s:\n", exp->type, _word, var);
    printf("parent_exp: "); print_expression(parent_exp);
    printf("exp: "); print_expression(exp);
  }
#endif

  if (exp->type == CONNECTOR_type) {
    dfs_position++;

    Connector connector;
    connector.multi = exp->multi;
    connector.desc = exp->u.condesc;
    set_connector_length_limit(&connector, _opts);

    switch (exp->dir) {
    case '+':
      _position.push_back(_right_connectors.size());
      _dir.push_back('+');
      _right_connectors.push_back(
           PositionConnector(parent_exp, &connector, '+', _word, dfs_position,
                             exp->cost, cost, leading_right, false,
                             eps_right, eps_left, word_xnode));
      leading_right = false;
      break;
    case '-':
      _position.push_back(_left_connectors.size());
      _dir.push_back('-');
      _left_connectors.push_back(
           PositionConnector(parent_exp, &connector, '-', _word, dfs_position,
                             exp->cost, cost, false, leading_left,
                             eps_right, eps_left, word_xnode));
      leading_left = false;
      break;
    default:
      throw std::string("Unknown connector direction: ") + exp->dir;
    }
  } else if (exp->type == AND_type) {
    if (exp->u.vtx.left == NULL) {
      /* zeroary and */
    } else
      if (exp->u.vtx.right == NULL) {
        /* unary and - skip */
        insert_connectors(exp->u.vtx.left, dfs_position, leading_right,
             leading_left, eps_right, eps_left, var, root, cost, parent_exp, word_xnode);
      } else {
        char new_var[MAX_VARIABLE_NAME];
        char* last_new_var = new_var;
        char* last_var = var;
        while ((*last_new_var = *last_var)) {
          last_new_var++;
          last_var++;
        }

        char* s = last_new_var;
        *s++ = 'c';
        fast_sprintf(s, 0);

        insert_connectors(exp->u.vtx.left, dfs_position, leading_right, leading_left,
              eps_right, eps_left, new_var, false, cost, parent_exp, word_xnode);

#ifdef POWER_PRUNE_CONNECTORS
        if (leading_right) {
          eps_right.push_back(_variables->epsilon(new_var, '+'));
        }
        if (leading_left) {
          eps_left.push_back(_variables->epsilon(new_var, '-'));
        }
#endif

        s = last_new_var;
        *s++ = 'c';
        fast_sprintf(s, 1);

        insert_connectors(exp->u.vtx.right, dfs_position, leading_right, leading_left,
              eps_right, eps_left, new_var, false, cost, parent_exp, word_xnode);

#ifdef POWER_PRUNE_CONNECTORS
        if (leading_right) {
          eps_right.push_back(_variables->epsilon(new_var, '+'));
        }
        if (leading_left) {
          eps_left.push_back(_variables->epsilon(new_var, '-'));
        }
#endif
      }
  } else if (exp->type == OR_type) {
    if (exp->u.vtx.right == NULL) {
      /* unary or - skip */
      insert_connectors(exp->u.vtx.left, dfs_position, leading_right, leading_left,
          eps_right, eps_left, var, root, cost, exp->u.vtx.left, word_xnode);
    } else {
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

      bool lr = leading_right, ll = leading_left;
      std::vector<int> er = eps_right, el = eps_left;

      char* s = last_new_var;
      *s++ = 'd';
      fast_sprintf(s, 0);

      lgdebug(+D_IC, "Word%d: var: %s; exp%d=%p; X_node: %s\n",
              _word, var, 0, exp->u.vtx.left, word_xnode ? word_xnode->word->subword : "NULL X_node");
      assert(word_xnode != NULL, "NULL X_node for var %s", new_var);
      if (root && parent_exp == NULL && exp->u.vtx.left != word_xnode->exp) {
        Exp *we = NULL;

        if (word_xnode->exp->type == OR_type) {
          we = word_xnode->exp->u.vtx.left;
          if (exp->u.vtx.left != we)
             we = word_xnode->exp->u.vtx.right;
        }
        if (we == NULL && word_xnode->next != NULL) {
          lgdebug(+D_IC, "Next word_xnode for word %d is needed\n", _word);
          word_xnode = word_xnode->next;
        }
      }
      insert_connectors(exp->u.vtx.left, dfs_position, lr, ll, er, el, new_var, false, cost, exp->u.vtx.left, word_xnode);

      if (lr)
        lr_true = true;
      if (ll)
        ll_true = true;

      lr = leading_right;
      ll = leading_left;
      er = eps_right;
      el = eps_left;

      s = last_new_var;
      *s++ = 'd';
      fast_sprintf(s, 1);

      lgdebug(+D_IC, "Word%d: var: %s; exp%d=%p; X_node: %s\n",
              _word, var, 1, exp->u.vtx.right, word_xnode ? word_xnode->word->subword : "NULL X_node");
      assert(word_xnode != NULL, "NULL X_node for var %s", new_var);
      if (root && parent_exp == NULL && exp->u.vtx.right != word_xnode->exp) {
        Exp *we = NULL;

        if (word_xnode->exp->type == OR_type) {
          we = word_xnode->exp->u.vtx.left;
          if (exp->u.vtx.left != we)
            we = word_xnode->exp->u.vtx.right;
        }
        if (we == NULL && word_xnode->next != NULL) {
          lgdebug(+D_IC, "Next word_xnode for word %d is needed\n", _word);
          word_xnode = word_xnode->next;
        }
      }
      insert_connectors(exp->u.vtx.right, dfs_position, lr, ll, er, el, new_var, false, cost, exp->u.vtx.right, word_xnode);

      if (lr)
        lr_true = true;
      if (ll)
        ll_true = true;

      leading_right = lr_true;
      leading_left = ll_true;
    }
  }
}
#undef D_IC

void WordTag::find_matches(int w, const condesc_t* C, char dir, std::vector<PositionConnector*>& matches)
{
  //  cout << "Look connection on: ." << _word << ". ." << w << ". " << C << dir << endl;
  Connector search_cntr;
  search_cntr.desc = C;
  set_connector_length_limit(&search_cntr, _opts);

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
    if (WordTag::match(w, search_cntr, dir, (*i).word, ((*i).connector))) {
      matches.push_back(&(*i));
    }
  }
}

void WordTag::add_matches_with_word(WordTag& tag)
{
  std::vector<PositionConnector>::iterator i;
  for (i = _right_connectors.begin(); i != _right_connectors.end(); i++) {
    std::vector<PositionConnector*> connector_matches;
    tag.find_matches(_word, (*i).connector.desc, '+', connector_matches);
    std::vector<PositionConnector*>::iterator j;
    for (j = connector_matches.begin(); j != connector_matches.end(); j++) {
      i->matches.push_back(*j);
      set_match_possible((*j)->word, (*j)->position);
      (*j)->matches.push_back(&(*i));
      tag.set_match_possible(_word, (*i).position);
    }
  }
}
