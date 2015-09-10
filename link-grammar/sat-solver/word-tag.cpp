#include "word-tag.hpp"
#include "fast-sprintf.hpp"

extern "C" {
#include "error.h"
#include "utilities.h"
}

#define D_IC 6
void WordTag::insert_connectors(Exp* exp, int& dfs_position,
                                bool& leading_right, bool& leading_left,
                                std::vector<int>& eps_right,
                                std::vector<int>& eps_left,
                                char* var, bool root, double parent_cost,
                                Exp* parent_exp, const X_node *w_xnode)
{
  double cost = parent_cost + exp->cost;
  if (exp->type == CONNECTOR_type) {
    dfs_position++;

    const char* name = exp->u.string;

    Connector* connector = connector_new();
    connector->multi = exp->multi;
    connector->string = name;
    set_connector_length_limit(connector);

    switch (exp->dir) {
    case '+':
      _position.push_back(_right_connectors.size());
      _dir.push_back('+');
      _right_connectors.push_back(
           PositionConnector(parent_exp, connector, '+', _word, dfs_position,
                             exp->cost, cost, leading_right, false,
                             eps_right, eps_left, w_xnode));
      leading_right = false;
      break;
    case '-':
      _position.push_back(_left_connectors.size());
      _dir.push_back('-');
      _left_connectors.push_back(
           PositionConnector(parent_exp, connector, '-', _word, dfs_position,
                             exp->cost, cost, false, leading_left,
                             eps_right, eps_left, w_xnode));
      leading_left = false;
      break;
    default:
      throw std::string("Unknown connector direction: ") + exp->dir;
    }
  } else if (exp->type == AND_type) {
    if (exp->u.l == NULL) {
      /* zeroary and */
    } else
      if (exp->u.l != NULL && exp->u.l->next == NULL) {
        /* unary and - skip */
        insert_connectors(exp->u.l->e, dfs_position, leading_right,
             leading_left, eps_right, eps_left, var, root, cost, parent_exp, w_xnode);
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

        for (i = 0, l = exp->u.l; l != NULL; l = l->next, i++) {
          char* s = last_new_var;
          *s++ = 'c';
          fast_sprintf(s, i);

          insert_connectors(l->e, dfs_position, leading_right, leading_left,
                eps_right, eps_left, new_var, false, cost, parent_exp, w_xnode);

          if (leading_right && var != NULL) {
            eps_right.push_back(_variables->epsilon(new_var, '+'));
          }
          if (leading_left && var != NULL) {
            eps_left.push_back(_variables->epsilon(new_var, '-'));
          }
        }
      }
  } else if (exp->type == OR_type) {
    if (exp->u.l != NULL && exp->u.l->next == NULL) {
      /* unary or - skip */
      insert_connectors(exp->u.l->e, dfs_position, leading_right, leading_left,
          eps_right, eps_left, var, root, cost, exp->u.l->e, w_xnode);
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

      for (i = 0, l = exp->u.l; l != NULL; l = l->next, i++) {
        bool lr = leading_right, ll = leading_left;
        std::vector<int> er = eps_right, el = eps_left;

        char* s = last_new_var;
        *s++ = 'd';
        fast_sprintf(s, i);

        lgdebug(+D_IC, "Word%d: var: %s; exp%d; X_node: %s\n",
                _word, var, i, w_xnode ? w_xnode->word->subword : "NULL X_node");
        assert(w_xnode != NULL, "NULL X_node for var %s", new_var);
        if (root && parent_exp == NULL && l->e != w_xnode->exp) {
          E_list *we = NULL;

          if (w_xnode->exp->type == OR_type) {
            for (we = w_xnode->exp->u.l; we != NULL; we = we-> next)
            {
              if (l->e == we->e)
                break;
            }
          }
          if (we == NULL) {
            lgdebug(+D_IC, "Next w_xnode for word %d is needed\n", _word);
            w_xnode = w_xnode->next;
            assert(w_xnode != NULL, "NULL next X_node for var %s", new_var);
          }
        }
        insert_connectors(l->e, dfs_position, lr, ll, er, el, new_var, false, cost, l->e, w_xnode);

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

void WordTag::find_matches(int w, const char* C, char dir, std::vector<PositionConnector*>& matches)
{
  //  cout << "Look connection on: ." << _word << ". ." << w << ". " << C << dir << endl;
  Connector search_cntr;
  init_connector(&search_cntr);
  search_cntr.string = C;
  set_connector_length_limit(&search_cntr);

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
    if (WordTag::match(w, search_cntr, dir, (*i).word, *((*i).connector))) {
      matches.push_back(&(*i));
    }
  }
}

void WordTag::add_matches_with_word(WordTag& tag)
{
  std::vector<PositionConnector>::iterator i;
  for (i = _right_connectors.begin(); i != _right_connectors.end(); i++) {
    std::vector<PositionConnector*> connector_matches;
    tag.find_matches(_word, (*i).connector->string, '+', connector_matches);
    std::vector<PositionConnector*>::iterator j;
    for (j = connector_matches.begin(); j != connector_matches.end(); j++) {
      i->matches.push_back(*j);
      set_match_possible((*j)->word, (*j)->position);
      (*j)->matches.push_back(&(*i));
      tag.set_match_possible(_word, (*i).position);
    }
  }
}
