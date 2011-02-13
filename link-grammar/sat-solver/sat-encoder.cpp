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
#include <link-grammar/api.h>
#include "preparation.h"
}

// Macro DEBUG_print is used to dump to stdout information while debugging
#ifdef _DEBUG
#define DEBUG_print(x) (cout << x << endl)
#else
#define DEBUG_print(x) (0)
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

void SATEncoder::generate_conditional_or_definition(Lit condition, Lit lhs, vec<Lit>& rhs) {
  {
    vec<Lit> clause;
    for (int i = 0; i < rhs.size(); i++) {
      clause.growTo(3);
      clause[0] = ~condition;
      clause[1] = lhs;
      clause[2] = ~rhs[i];
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
void SATEncoder::build_word_tags() {
  for (int w = 0; w < _sent->length; w++) {
    _word_tags.push_back(WordTag(w, _variables, _sent, _opts));
    int dfs_position = 0;

    if (_sent->word[w].x == NULL) {
      DEBUG_print("Word ." << w << ".: " << _sent->word[w].string << " (null)" <<  endl);
      continue;
    }

    bool join = _sent->word[w].x->next != NULL;
    Exp* exp = join ? join_alternatives(w) : _sent->word[w].x->exp;

#ifdef _DEBUG
    cout << "Word ." << w << ".: " << _sent->word[w].string << endl;
    print_expression(exp);
    cout << endl;
#endif
      
    char name[MAX_VARIABLE_NAME];
    sprintf(name, "w%d", w);
    bool leading_right = true;
    bool leading_left = true;
    std::vector<int> eps_right, eps_left;

    _word_tags[w].insert_connectors(exp, dfs_position, leading_right, leading_left, eps_right, eps_left, name, true, 0);

    if (join)
      free_alternatives(exp);
  }

  for (int wl = 0; wl < _sent->length - 1; wl++) {
    for (int wr = wl + 1; wr < _sent->length; wr++) {
      _word_tags[wl].add_matches_with_word(_word_tags[wr]);
    }
  }
}

void SATEncoder::find_all_matches_between_words(int w1, int w2, 
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

void SATEncoder::generate_satisfaction_conditions() {
  for (int w = 0; w < _sent->length; w++) {
    if (_sent->word[w].x == NULL) {
      DEBUG_print("Word: " << _sent->word[w].string << " : " << "(null)" << endl);
      handle_null_expression(w);
      continue;
    }

    bool join = _sent->word[w].x->next != NULL;
    Exp* exp = join ? join_alternatives(w) : _sent->word[w].x->exp;

#ifdef _DEBUG
    cout << "Word: " << _sent->word[w].string << endl;
    print_expression(exp);
    cout << endl;
#endif

    char name[MAX_VARIABLE_NAME];
    sprintf(name, "w%d", w);

    determine_satisfaction(w, name);
    int dfs_position = 0;
    generate_satisfaction_for_expression(w, dfs_position, exp, name, 0);

    if (join)
      free_alternatives(exp);
  }
}


void SATEncoder::generate_satisfaction_for_expression(int w, int& dfs_position, Exp* e, char* var, int parrent_cost) {
  E_list *l;
  int total_cost = parrent_cost + e->cost;

  if (e->type == CONNECTOR_type) {
    dfs_position++;

    generate_satisfaction_for_connector(w, dfs_position, e->u.string, e->dir, e->multi, e->cost, var);
    
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
	while(*last_new_var = *last_var) {
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
	while(*last_new_var = *last_var) {
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

Exp* SATEncoder::join_alternatives(int w) {
  // join all alternatives using and OR_type node
  Exp* exp;
  E_list* or_list = NULL;;
  for (X_node* x = _sent->word[w].x; x != NULL; x = x->next) {
    E_list* new_node = (E_list*)xalloc(sizeof(E_list));
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
  exp = (Exp*)xalloc(sizeof(Exp));
  exp->type = OR_type;
  exp->u.l = or_list;
  exp->cost = 0;

  return exp;
}

void SATEncoder::free_alternatives(Exp* exp) {
  E_list *l = exp->u.l;
  while (l != NULL) {
    E_list* next = l->next;
    xfree(l, sizeof(E_list));
    l = next;
  }
  xfree(exp, sizeof(exp));
}


void SATEncoder::generate_link_cw_ordinary_definition(int wi, int pi, const char* Ci, char dir, int cost, int wj) {
  Lit lhs = Lit(_variables->link_cw(wj, wi, pi, Ci));

  char str[MAX_VARIABLE_NAME];
  sprintf(str, "w%d", wj);
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
      rhs.push(Lit(_variables->link_cost(wi, pi, Ci, 
					 (*i)->word, (*i)->position, (*i)->connector->string, 
					 cost + (*i)->cost)));
    } else if (dir == '-'){
      rhs.push(Lit(_variables->link((*i)->word, (*i)->position, (*i)->connector->string,
				    wi, pi, Ci)));	
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
int SATEncoder::num_connectors(Exp* e) {
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

int SATEncoder::empty_connectors(Exp* e, char dir) {
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

int SATEncoder::non_empty_connectors(Exp* e, char dir) {
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
					     std::vector<PositionConnector*>& connectors) {
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
				     std::vector<PositionConnector*>& connectors) {
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
  for (int other_node = node + 1; other_node < components.size(); other_node++) {
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
  for (int node = 0; node < _sent->length; node++)
    dfs(node, graph, node, components);
  bool connected = true;
  for (int node = 0; node < _sent->length; node++) {
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
void SATEncoder::generate_planarity_conditions() {
  vec<Lit> clause;
  for (int wi1 = 0; wi1 < _sent->length; wi1++) {
    for (int wj1 = wi1+1; wj1 < _sent->length; wj1++) {
      for (int wi2 = wj1+1; wi2 < _sent->length; wi2++) {
	if (!_linked_possible(wi1, wi2))
	  continue;
	for (int wj2 = wi2+1; wj2 < _sent->length; wj2++) {
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

void SATEncoder::generate_epsilon_definitions() {
  for (int w = 0; w < _sent->length; w++) {
    if (_sent->word[w].x == NULL) {
      continue;
    }

    bool join = _sent->word[w].x->next != NULL;
    Exp* exp = join ? join_alternatives(w) : _sent->word[w].x->exp;

    char name[MAX_VARIABLE_NAME];
    sprintf(name, "w%d", w);

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
						 char* var, bool root, char dir) {
  E_list *l;
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
	while(*last_new_var = *last_var) {
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
      while(*last_new_var = *last_var) {
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

void SATEncoder::power_prune() {
  generate_epsilon_definitions();

  // on two non-adjacent words, a pair of connectors can be used only
  // if not [both of them are the deepest].

  for (int wl = 0; wl < _sent->length - 2; wl++) {
    const std::vector<PositionConnector>& rc = _word_tags[wl].get_right_connectors();
    std::vector<PositionConnector>::const_iterator rci;
    for (rci = rc.begin(); rci != rc.end(); rci++) {
      if (!(*rci).leading_right || (*rci).connector->multi)
	continue;

      const std::vector<PositionConnector*>& matches = rci->matches;
      for (std::vector<PositionConnector*>::const_iterator lci = matches.begin(); lci != matches.end(); lci++) {
	if (!(*lci)->leading_left || (*lci)->connector->multi || (*lci)->word <= wl + 2)
	  continue;

	//	printf("LR: .%d. .%d. %s\n", wl, rci->position, rci->connector->string);
	//	printf("LL: .%d. .%d. %s\n", (*lci)->word, (*lci)->position, (*lci)->connector->string);

	vec<Lit> clause;
	for (std::vector<int>::const_iterator i = rci->eps_right.begin(); i != rci->eps_right.end(); i++) {
	  clause.push(~Lit(*i));
	}
	
	for (std::vector<int>::const_iterator i = (*lci)->eps_left.begin(); i != (*lci)->eps_left.end(); i++) {
	  clause.push(~Lit(*i));
	}

	add_additional_power_pruning_conditions(clause, wl, (*lci)->word);

	clause.push(~Lit(_variables->link(wl, rci->position, rci->connector->string, 
					  (*lci)->word, (*lci)->position, (*lci)->connector->string)));
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
void SATEncoder::pp_prune() {
  const std::vector<int>& link_variables = _variables->link_variables();


  if (_sent->dict->postprocessor == NULL) 
    return;

  pp_knowledge * knowledge;
  knowledge = _sent->dict->postprocessor->knowledge;

  for (int i=0; i<knowledge->n_contains_one_rules; i++) {
    pp_rule rule;
    const char * selector;
    pp_linkset * link_set;

    rule = knowledge->contains_one_rules[i]; 
    selector = rule.selector;				/* selector string for this rule */
    link_set = rule.link_set;				/* the set of criterion links */


    vec<Lit> triggers; 
    for (int i = 0; i < link_variables.size(); i++) {
      const Variables::LinkVar* var = _variables->link_variable(link_variables[i]);
      if (post_process_match(rule.selector, var->label)) {
	triggers.push(Lit(link_variables[i]));
      }
    }

    if (triggers.size() == 0)
      continue;

    vec<Lit> criterions; 
    for (int j = 0; j < link_variables.size(); j++) {
      pp_linkset_node *p;
      for (int hashval = 0; hashval < link_set->hash_table_size; hashval++) {
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

/* TODO: replace with analyze_xxx_linkage */
bool SATEncoder::post_process_linkage(Linkage linkage)
{
  if (parse_options_get_use_fat_links(_opts) &&
      set_has_fat_down(_sent)) {
    Linkage_info li = analyze_fat_linkage(_sent, _opts, PP_SECOND_PASS);
    return li.N_violations == 0;
  } else {
    Linkage_info li = analyze_thin_linkage(_sent, _opts, PP_SECOND_PASS);
    return li.N_violations == 0;
  }
  return 1;
}

/*--------------------------------------------------------------------------*
 *                         D E C O D I N G                                  *
 *--------------------------------------------------------------------------*/
Linkage SATEncoder::create_linkage()
{
  /* Using exalloc since this is external to the parser itself. */
  Linkage linkage = (Linkage) exalloc(sizeof(struct Linkage_s));
  memset(linkage, 0, sizeof(struct Linkage_s));

  linkage->num_words = _sent->length;
  linkage->word = (const char **) exalloc(linkage->num_words*sizeof(char *));
  linkage->sent = _sent;
  linkage->opts = _opts;
  //  linkage->info = sent->link_info[k];  

  if (_sent->parse_info) {
    Parse_info pi = _sent->parse_info;
    for (int i=0; i< MAX_LINKS; i++) {
      free_connectors(pi->link_array[i].lc);
      free_connectors(pi->link_array[i].rc);
    }
    free_parse_info(_sent->parse_info);
    _sent->parse_info = NULL;
  }
  Parse_info pi = _sent->parse_info = parse_info_new(_sent->length);
  bool fat = extract_links(pi);
  
  //  compute_chosen_words(sent, linkage);
  /* TODO: this is just a simplified version of the
     compute_chosen_words. */
  for (int i = 0; i < _sent->length; i++) {
    char *s = (char *) exalloc(strlen(_sent->word[i].string)+1);
    strcpy(s, _sent->word[i].string);
    linkage->word[i] = s;
  }
  linkage->num_words = _sent->length;
  pi->N_words = _sent->length;

  if (!fat || !parse_options_get_use_fat_links(_opts))
    extract_thin_linkage(_sent, _opts, linkage);
  else
    extract_fat_linkage(_sent, _opts, linkage);

  linkage_set_current_sublinkage(linkage, 0);
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

  std::vector<int> components;
  bool connected = connectivity_components(components);
  if (connected) {
    // num_connected_linkages++;

    if (post_process_linkage(linkage)) {
      cout << "Linkage PP OK" << endl;
      _sent->num_valid_linkages++;
    } else {
	   cout << "Linkage PP NOT OK" << endl;
    }

    generate_linkage_prohibiting();
  } else {
    cout << "Linkage DISCONNECTED" << endl;
    generate_disconnectivity_prohibiting(components);
  }

  _solver->printStats();
  return linkage;
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

void SATEncoderConjunctionFreeSentences::determine_satisfaction(int w, char* name) {
  // All tags must be satisfied
  generate_literal(Lit(_variables->string(name))); 
}

void SATEncoderConjunctionFreeSentences::generate_satisfaction_for_connector(int wi, int pi, const char* Ci, 
									     char dir, bool multi, int cost, char* var) {
  Lit lhs = Lit(_variables->string_cost(var, cost));

#ifdef _DEBUG
  cout << "*** Connector: ." << wi << ". ." << pi << ". " <<  Ci << dir << endl;
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

    generate_link_cw_ordinary_definition(wi, pi, Ci, dir, cost, wj);
  }
  
  vec<Lit> _link_cw_;
  for (int i = 0; i < _w_.size(); i++) 
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

void SATEncoderConjunctionFreeSentences::generate_linked_definitions() {
  _linked_possible.resize(_sent->length, 1);

  DEBUG_print("------- linked definitions");
  for (int w1 = 0; w1 < _sent->length; w1++) {
    for (int w2 = w1 + 1; w2 < _sent->length; w2++) {
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

void SATEncoder::generate_linked_min_max_planarity() {
  DEBUG_print("---- linked_max");
  for (int w1 = 0; w1 < _sent->length; w1++) {
    for (int w2 = 0; w2 < _sent->length; w2++) {
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
  for (int w1 = 0; w1 < _sent->length; w1++) {
    for (int w2 = 0; w2 < _sent->length; w2++) {
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
  for (int wi = 3; wi < _sent->length; wi++) {
    for (int wj = 1; wj < wi - 1; wj++) {
      for (int wl = wj + 1; wl < wi; wl++) {
	clause.growTo(3);
	clause[0] = ~Lit(_variables->linked_min(wi, wj));
	clause[1] = ~Lit(_variables->linked_max(wi, wl - 1));
	clause[2] = Lit(_variables->linked_min(wl, wj));
	add_clause(clause);
      }
    }
  }

  DEBUG_print("------------");

  for (int wi = 0; wi < _sent->length - 1; wi++) {
    for (int wj = wi + 1; wj < _sent->length - 1; wj++) {
      for (int wl = wi+1; wl < wj; wl++) {
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
  for (int wi = 1; wi < _sent->length; wi++) {
    for (int wj = wi + 2; wj < _sent->length - 1; wj++) {
      for (int wl = wi + 1; wl < wj; wl++) {
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

bool SATEncoderConjunctionFreeSentences::extract_links(Parse_info pi)
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

    pi->link_array[current_link].l = var->left_word;
    pi->link_array[current_link].r = var->right_word;
    //    pi->link_array[j].name = var->label;

    Connector* connector;

    connector = connector_new();
    connector->string = var->left_connector;
    pi->link_array[current_link].lc = connector;

    connector = connector_new();
    connector->string = var->right_connector;
    pi->link_array[current_link].rc = connector;
    
    current_link++;
  }
  
  pi->N_links = current_link;
  DEBUG_print("Total: ." <<  pi->N_links << "." << endl);
  return false;
}


/*******************************************************************************
 *              SAT encoding for sentences that contain conjunction.           *
 *******************************************************************************/

// Check if a connector is andable
static int is_andable(Sentence sent, Connector* c, char dir) {
	/* if no set, then everything is considered andable */
	if (sent->dict->andable_connector_set == NULL) 
	  return TRUE;
	return match_in_connector_set(sent, sent->dict->andable_connector_set, c, dir);
}

void SATEncoderConjunctiveSentences::handle_null_expression(int w)
{
  // Formula is unsatisfiable, but only if w is not a conjunctive
  // word. Conjunctive words can have empty tags, but then they must
  // have fat-links attached.
  if (!isConnectiveOrComma(w)) {
    vec<Lit> clause;
    add_clause(clause);
  } else {
    char str[MAX_VARIABLE_NAME];
    sprintf(str, "fl_d_%d", w);
    generate_literal(Lit(_variables->string(str)));
    sprintf(str, "w%d", w);
    generate_literal(~Lit(_variables->string(str)));
  }
}

void SATEncoderConjunctiveSentences::determine_satisfaction(int w, char* name) {
  if (!parse_options_get_use_fat_links(_opts) ||
      !isConnectiveOrComma(w)) {
    // Non-conjunctive words must have their tags satisfied
    generate_literal(Lit(_variables->string(name))); 
  } else {
    // Tags of conjunctive words are satisfied iff they are not fat-linked
    either_tag_or_fat_link(w, Lit(_variables->string(name)));
  }
}


void SATEncoderConjunctiveSentences::generate_satisfaction_for_connector(int wi, int pi, const char* Ci, 
									 char dir, bool multi, int cost, char* var) {
  Lit lhs = Lit(_variables->string_cost(var, cost));

#ifdef _DEBUG
  cout << "*** Connector: ." << wi << ". ." << pi << ". " <<  Ci << dir << endl;
#endif
  // words that can potentially match (wi, pi)
  int low = (dir == '-') ? 0 : wi + 1;
  int hi  = (dir == '-') ? wi : _sent->length;

  // determine if opposite of (wi, pi) is andable
  Connector conn;
  init_connector(&conn);
  conn.label = NORMAL_LABEL;
  conn.priority = THIN_priority;
  conn.string = Ci;
  bool andable_opposite = is_andable(_sent, &conn, dir == '+' ? '-' : '+');

  std::vector<int> _w_;
  for (int wj = low; wj < hi; wj++) {
    // Eliminate words that cannot be connected to (wi, pi)
    if (!link_cw_possible(wi, pi, Ci, dir, wj, 1, _sent->length-1))
      continue;

    // Now we know that there is a potential link between the
    // connector (wi, pi) and the word w
    _w_.push_back(wj);

    // link_cw down definitions as ordinary words
    if (_word_tags[wj].match_possible(wi, pi) || isConnectiveOrComma(wj)) {
      generate_link_cw_ordinary_definition(wi, pi, Ci, dir, cost, wj);
    }
    
    // link_cw down definitions as special words 
    if (isConnectiveOrComma(wj)) {
      if (!link_cw_possible_with_fld(wi, pi, Ci, dir, wj, 1, _sent->length-1)) {
	generate_link_cw_connective_impossible(wi, pi, Ci, wj);
      } else {
	generate_link_cw_connective_definition(wi, pi, Ci, wj);
      }
    }

    // ------------------------------------------------------------------------
    
    // link_top_cw up defintions
    if (andable_opposite) {
      // connections can be either directly established or inherited from above
      generate_link_top_cw_up_definition(wj, wi, pi, Ci, multi);
    } else {
      // connection is established iff it is directly established
      // i.e., it cannot be inherited from above
      generate_link_top_cw_iff_link_cw(wj, wi, pi, Ci);      
    }

    // Commas cannot be directly connected if they have fat links down
    if (parse_options_get_use_fat_links(_opts) &&
        isComma(_sent, wj)) {
      vec<Lit> clause;
      char str[MAX_VARIABLE_NAME];
      sprintf(str,"fl_d_%d", wj);
      clause.push(~Lit(_variables->string(str)));
      clause.push(~Lit(_variables->link_top_cw(wj, wi, pi, Ci)));
      add_clause(clause);
    }
  }
  
  vec<Lit> _link_cw_;
  for (int i = 0; i < _w_.size(); i++) 
    _link_cw_.push(Lit(_variables->link_cw(_w_[i], wi, pi, Ci)));
  generate_or_definition(lhs, _link_cw_);

  vec<Lit> _link_top_cw_;
  for (int i = 0; i < _w_.size(); i++) 
    _link_top_cw_.push(Lit(_variables->link_top_cw(_w_[i], wi, pi, Ci)));
  // Faster
  generate_or_definition(lhs, _link_top_cw_);

  DEBUG_print("--------- multi");
  if (!multi) {
    generate_xor_conditions(_link_top_cw_);
  }
  DEBUG_print("--------- end multi");

#ifdef _DEBUG
  cout << "*** End Connector: ." << wi << ". ." << pi << ". " <<  Ci << endl;
#endif
}

void SATEncoderConjunctiveSentences::add_additional_power_pruning_conditions(vec<Lit>& clause, int wl, int wr) {
  char str[MAX_VARIABLE_NAME];
  sprintf(str, "fl_ur_%d", wl);
  clause.push(Lit(_variables->string(str)));
  sprintf(str, "fl_ul_%d", wl);
  clause.push(Lit(_variables->string(str)));
  sprintf(str, "fl_ul_%d", wr);
  clause.push(Lit(_variables->string(str)));
  sprintf(str, "fl_ur_%d", wr);
  clause.push(Lit(_variables->string(str)));
}

void SATEncoderConjunctiveSentences::generate_encoding_specific_clauses() {
  generate_label_compatibility();
  generate_fat_link_existence();
  generate_fat_link_up_definitions();
  generate_fat_link_down_definitions();
  generate_fat_link_up_between_down_conditions();
  generate_fat_link_comma_conditions();
  generate_fat_link_crossover_conditions();
  generate_fat_link_Left_Wall_not_inside();
  generate_fat_link_linked_upperside();
  generate_fat_link_neighbor();
}

void SATEncoderConjunctiveSentences::init_connective_words() {
  for (int i = 0; i < _sent->length; i++) {
    if (::isConnectiveOrComma(_sent, i)) {
      _connectives.push_back(i);
      _is_connective_or_comma.push_back(true);
    } else {
      _is_connective_or_comma.push_back(false);
    }
  }

  
}

/* If there is a direct link between a connective word wi and a
   connector (w,p), then there is an indirect link between wi and (w, p).
 */
void SATEncoderConjunctiveSentences::generate_link_top_cw_iff_link_cw(int wi, int wj, int pj, const char* Cj) {
  DEBUG_print("--------- link_top_cw iff link_cw");
  vec<Lit> clause;	

  clause.growTo(2);  
  clause[0] = ~Lit(_variables->link_cw(wi, wj, pj, Cj));
  clause[1] = Lit(_variables->link_top_cw(wi, wj, pj, Cj));
  add_clause(clause);

  clause.growTo(2);  
  clause[0] = ~Lit(_variables->link_top_cw(wi, wj, pj, Cj));
  clause[1] = Lit(_variables->link_cw(wi, wj, pj, Cj));
  add_clause(clause);

  DEBUG_print("--------- end link_top_cw iff link_cw");
}



void SATEncoderConjunctiveSentences::generate_link_top_cw_up_definition(int wj, int wi, int pi, const char* Ci, bool multi) {
  DEBUG_print("--------- link_top_cw definition");
  Lit link_cw_wj = Lit(_variables->link_cw(wj, wi, pi, Ci));
  Lit link_top_cw_wj = Lit(_variables->link_top_cw(wj, wi, pi, Ci));
  char dir = wi < wj ? '+' : '-';

  vec<Lit> clause;

  clause.growTo(2);
  clause[0] = ~link_top_cw_wj;
  clause[1] = link_cw_wj;
  add_clause(clause);


  // Collect connectors up
  std::vector<int> Wk;
  for (int k = 0; k < _connectives.size(); k++) {
    int wk = _connectives[k];
    if (wk != wj && (wj - wi)*(wk - wi) > 0 &&
	link_cw_possible_with_fld(wi, pi, Ci, dir, wk, 1, _sent->length - 1))
      Wk.push_back(wk);
  }


  for (int k = 0; k < Wk.size(); k++) {
    int wk = Wk[k];
    clause.growTo(4);
    clause[0] = link_top_cw_wj;
    clause[1] = ~link_cw_wj;
    clause[2] = ~Lit(_variables->fat_link(wj, wk));
    clause[3] = Lit(_variables->link_cw(wk, wi, pi, Ci));
    add_clause(clause);
  }

  clause.clear();
  for (int k = 0; k < Wk.size(); k++) {
    int wk = Wk[k];
    clause.growTo(3);
    clause[0] = ~link_top_cw_wj;
    clause[1] = ~Lit(_variables->fat_link(wj, wk));
    clause[2] = ~Lit(_variables->link_cw(wk, wi, pi, Ci));
    add_clause(clause);
  } 


  clause.clear();
  clause.push(~link_cw_wj);
  clause.push(link_top_cw_wj);
  for (int k = 0; k < Wk.size(); k++) {
    int wk = Wk[k];
    clause.push(Lit(_variables->fat_link(wj, wk)));
  }
  add_clause(clause);


  if (clause.size() == 2)
    return;

  // Faster
  clause.clear();
  clause.push(~link_cw_wj);
  clause.push(link_top_cw_wj);
  for (int k = 0; k < Wk.size(); k++) {
    int wk = Wk[k];
    clause.push(Lit(_variables->link_cw(wk, wi, pi, Ci)));
  }
  add_clause(clause);


  if (multi) {
    // cannot directly link to both ends of the fat link
    clause.clear();
    for (int k = 0; k < Wk.size(); k++) {
      int wk = Wk[k];
      clause.growTo(3);
      clause[0] = ~Lit(_variables->fat_link(wj, wk));
      clause[1] = ~Lit(_variables->link_top_cw(wj, wi, pi, Ci));
      clause[2] = ~Lit(_variables->link_top_cw(wk, wi, pi, Ci));
      add_clause(clause);
    }
  }

  DEBUG_print("--------- end link_top_cw definition");
}

/* If there is an indirect link from a connective word or a comma wi to a 
   connector (wj, pj), then, there exist two words wk1 and wk2, such that:
     (1)
          (a) wk1 is between wi and wj
          (b) there is a fat-link between wk1 and wi
	  (c) there is an indirect link between wk1 and (wj, pj)
     (2)
          (a) wi is between wk2 and wj
          (b) there is a fat-link between wk2 and wi
	  (c) there is an indirect link between wk2 and (wj, pj)
 */
void SATEncoderConjunctiveSentences::generate_link_cw_connective_definition(int wj, int pj, const char* Cj, int wi) {
  DEBUG_print("--------- link_cw as special");

  char str[MAX_VARIABLE_NAME];
  sprintf(str, "fl_d_%d", wi);
  Lit not_wi_fld = ~Lit(_variables->string(str));

  char link_cw_k1_str[MAX_VARIABLE_NAME];
  sprintf(link_cw_k1_str, "%d_%d_%d__1", wj, pj, wi);
  Lit link_cw_k1(_variables->string(link_cw_k1_str));

  char link_cw_k2_str[MAX_VARIABLE_NAME];
  sprintf(link_cw_k2_str, "%d_%d_%d__2", wj, pj, wi);
  Lit link_cw_k2(_variables->string(link_cw_k2_str));

  Lit not_link_wi_wjpj = ~Lit(_variables->link_cw(wi, wj, pj, Cj));

  char dir = wj < wi ? '+' : '-';

  vec<Lit> clause;
  clause.growTo(3);
  clause[0] = not_wi_fld;
  clause[1] = not_link_wi_wjpj;
  clause[2] = link_cw_k1;
  add_clause(clause);

  clause.growTo(3);
  clause[0] = not_wi_fld;
  clause[1] = not_link_wi_wjpj;
  clause[2] = link_cw_k2;
  add_clause(clause);

  clause.growTo(3);
  clause[0] = ~link_cw_k1;
  clause[1] = ~link_cw_k2;
  clause[2] = ~not_link_wi_wjpj;
  add_clause(clause);
  
  clause.clear();

  // B(wi, w, wj)
  int wl, wr;
  if (wi < wj) {
    wl = wi + 1;
    wr = wj;
  } else {
    wl = wj + 1;
    wr = wi;
  }

  std::vector<bool> link_cw_possible_cache(wr - wl);
  
  for (int wk1 = wl; wk1 < wr; wk1++) {
    if (!(link_cw_possible_cache[wk1 - wl] = link_cw_possible(wj, pj, Cj, dir, wk1, wl, wr))) 
      continue;

    clause.growTo(3);
    clause[0] = ~link_cw_k1;
    clause[1] = ~Lit(_variables->fat_link(wk1, wi));
    clause[2] = Lit(_variables->link_cw(wk1, wj, pj, Cj));
    add_clause(clause);
  }

  clause.clear();
  clause.push(~link_cw_k1);
  for (int wk1 = wl; wk1 < wr; wk1++) {
    if (!link_cw_possible_cache[wk1 - wl])
      continue;

    clause.push(Lit(_variables->fat_link(wk1, wi)));
  }
  add_clause(clause);

  // Faster
  clause.clear();
  clause.push(~link_cw_k1);
  for (int wk1 = wl; wk1 < wr; wk1++) {
    if (!link_cw_possible_cache[wk1 - wl])
      continue;

    clause.push(Lit(_variables->link_cw(wk1, wj, pj, Cj)));
  }
  add_clause(clause);


  clause.clear();
  for (int wk1 = wl; wk1 < wr; wk1++) {
    if (!link_cw_possible_cache[wk1 - wl])
      continue;
    clause.growTo(3);
    clause[0] = ~Lit(_variables->fat_link(wk1, wi));
    clause[1] = ~Lit(_variables->link_cw(wk1, wj, pj, Cj));
    clause[2] = link_cw_k1;
    add_clause(clause);
  }

  DEBUG_print("---");

  // B(wk2, wi, wj)
  if (wi < wj) {
    wl = 1;
    wr = wi;
  } else {
    wl = wi + 1;
    wr = _sent->length - 1;
  }
  link_cw_possible_cache.resize(wr - wl);

  clause.clear();
  for (int wk2 = wl; wk2 < wr; wk2++) {
    if (!(link_cw_possible_cache[wk2 - wl] = link_cw_possible(wj, pj, Cj, dir, wk2, wl, wr)))
      continue;

    clause.growTo(3);
    clause[0] = ~link_cw_k2;
    clause[1] = ~Lit(_variables->fat_link(wk2, wi));
    clause[2] = Lit(_variables->link_cw(wk2, wj, pj, Cj));
    add_clause(clause);    
  }

  clause.clear();
  clause.push(~link_cw_k2);
  for (int wk2 = wl; wk2 < wr; wk2++) {
    if (!link_cw_possible_cache[wk2 - wl])
      continue;

    clause.push(Lit(_variables->fat_link(wk2, wi)));
  }
  add_clause(clause);

  // Faster
  clause.clear();
  clause.push(~link_cw_k2);
  for (int wk2 = wl; wk2 < wr; wk2++) {
    if (!link_cw_possible_cache[wk2 - wl])
      continue;

    clause.push(Lit(_variables->link_cw(wk2, wj, pj, Cj)));
  }
  add_clause(clause);

  clause.clear();
  for (int wk2 = wl; wk2 < wr; wk2++) {
    if (!link_cw_possible_cache[wk2 - wl])
      continue;
    clause.growTo(3);
    clause[0] = ~Lit(_variables->fat_link(wk2, wi));
    clause[1] = ~Lit(_variables->link_cw(wk2, wj, pj, Cj));
    clause[2] = link_cw_k2;
    add_clause(clause);
  }

  DEBUG_print("--------- end link_cw as special");
}

void SATEncoderConjunctiveSentences::generate_link_cw_connective_impossible(int wi, int pi, const char* Ci, int wj) {
  DEBUG_print("--------- link_cw as special down - impossible");
  // If the connective word w has fat-links down it cannot be
  // connected to (wi, pi)
  vec<Lit> clause;
  char str[MAX_VARIABLE_NAME];
  sprintf(str,"fl_d_%d", wj);
  clause.push(~Lit(_variables->string(str)));
  clause.push(~Lit(_variables->link_cw(wj, wi, pi, Ci)));
  add_clause(clause);
  DEBUG_print("--------- end link_cw as special down - impossible");
}


void SATEncoderConjunctiveSentences::generate_fat_link_up_definitions() {
  Lit lhs;
  vec<Lit> rhs;
  char fl_str[MAX_VARIABLE_NAME];
  vec<Lit> clause;

  for (int w = 1; w < _sent->length - 1; w++) {
    DEBUG_print("----fat_link up definition");
    // Fatlink up left definition
    rhs.clear();
    for (std::vector<int>::const_iterator cl = _connectives.begin(); cl != _connectives.end(); cl++) {
      if (*cl < w)
	rhs.push(Lit(_variables->fat_link(w, *cl)));
    }
    sprintf(fl_str, "fl_ul_%d", w);
    lhs = Lit(_variables->string(fl_str));
    generate_or_definition(lhs, rhs);
    generate_xor_conditions(rhs);

    // Fatlink up right definition
    rhs.clear();
    for (std::vector<int>::const_iterator cr = _connectives.begin(); cr != _connectives.end(); cr++) {
      if (*cr > w)
	rhs.push(Lit(_variables->fat_link(w, *cr)));
    }
    sprintf(fl_str, "fl_ur_%d", w);
    lhs = Lit(_variables->string(fl_str));
    generate_or_definition(lhs, rhs);
    generate_xor_conditions(rhs);

    // There can not be two links up
    clause.growTo(2);
    sprintf(fl_str, "fl_ul_%d", w);
    clause[0] = ~Lit(_variables->string(fl_str));
    sprintf(fl_str, "fl_ur_%d", w);
    clause[1] = ~Lit(_variables->string(fl_str));
    add_clause(clause);
    

    DEBUG_print("----end fat_link up definition");
  }  
}


void SATEncoderConjunctiveSentences::generate_fat_link_down_definitions() {
  // Fat links down
  char fl_str[MAX_VARIABLE_NAME];
  std::vector<int>::const_iterator w;
  for (w = _connectives.begin(); w != _connectives.end(); w++) {
    DEBUG_print("----fat_link down definition");
    // Fatlink down left definition
    vec<Lit> rhs;
    for (int wl = 1; wl < *w; wl++) {
      rhs.push(Lit(_variables->fat_link(wl, *w)));
    }
    sprintf(fl_str, "fl_d_%d", *w);
    Lit lhs = Lit(_variables->string(fl_str));
    generate_or_definition(lhs, rhs);
    generate_xor_conditions(rhs);

    // Fatlink down right definition
    rhs.clear();
    for (int wr = *w + 1; wr < _sent->length - 1; wr++) {
      rhs.push(Lit(_variables->fat_link(wr, *w)));
    }
    sprintf(fl_str, "fl_d_%d", *w);
    lhs = Lit(_variables->string(fl_str));
    generate_or_definition(lhs, rhs);
    generate_xor_conditions(rhs);
    DEBUG_print("----end fat_link down definition");
  }
}

void  SATEncoderConjunctiveSentences::generate_fat_link_comma_conditions() {
  char fl_str[MAX_VARIABLE_NAME];
  std::vector<int>::const_iterator w;
  vec<Lit> clause;
  for (w = _connectives.begin(); w != _connectives.end(); w++) {
    if (isComma(_sent, *w)) {
      DEBUG_print("---comma---");

      // If comma has a link down it has to have a link up to the right
      sprintf(fl_str, "fl_d_%d", *w);
      clause.growTo(2);
      clause[0] = ~Lit(_variables->string(fl_str));
      sprintf(fl_str, "fl_ur_%d", *w);
      clause[1] = Lit(_variables->string(fl_str));
      add_clause(clause);


      // If comma has a link down it cannot have a link up to the left
      sprintf(fl_str, "fl_d_%d", *w);
      clause.growTo(2);
      clause[0] = ~Lit(_variables->string(fl_str));
      sprintf(fl_str, "fl_ul_%d", *w);
      clause[1] = ~Lit(_variables->string(fl_str));
      add_clause(clause);      

      DEBUG_print("---end comma---");
    }
  }
}


void  SATEncoderConjunctiveSentences::generate_fat_link_crossover_conditions() {
  vec<Lit> clause;
  // If a word has a fat link up, than it cannot establish links with words behind that connective
  for (int w = 1; w < _sent->length - 1; w++) {
    DEBUG_print("----links cannot crossover the fat-links");
    std::vector<int>::const_iterator c1, c2;
    for (c1 = _connectives.begin(); c1 != _connectives.end(); c1++) {
      if (*c1 > w) {
	for (int w1 = *c1 + 1; w1 < _sent->length; w1++) {
	  if (!_linked_possible(w, w1))
	    continue;

	  clause.growTo(3);
	  clause[0] = ~Lit(_variables->fat_link(w, *c1));
	  clause[1] = Lit(_variables->link_top_ww(*c1, w1));
	  clause[2] = ~Lit(_variables->link_top_ww(w, w1));
	  add_clause(clause);
	}
      }

      if (*c1 < w) {
	for (int w1 = 0; w1 < *c1; w1++) {
	  if (!_linked_possible(w1, w))
	    continue;

	  clause.growTo(3);
	  clause[0] = ~Lit(_variables->fat_link(w, *c1));
	  clause[1] = Lit(_variables->link_top_ww(*c1, w1));
	  clause[2] = ~Lit(_variables->link_top_ww(w, w1));
	  add_clause(clause);
	}
      }
    }
    DEBUG_print("----end links cannot cross over the fat-links");
  }


  // One step structure violation pruning heuristic If a word is
  // directly linked to a fat linked word, than it cannot establish
  // links with words behind that connective
  std::vector<int>::const_iterator c;
  for (c = _connectives.begin(); c != _connectives.end(); c++) {
    DEBUG_print("----links cannot crossover the fat-links heuristic");
    for (int w = 1; w < *c; w++) {
      for (int w1 = 0; w1 < w; w1++) {
	if (!_linked_possible(w1, w))
	  continue;
	for (int w2 = *c; w2 < _sent->length; w2++) {
	  if (!_linked_possible(w1, w2))
	    continue;
	    

	  vec<Lit> clause;
	  clause.push(~Lit(_variables->fat_link(w, *c)));
	  clause.push(~Lit(_variables->linked(w1, w)));
	  clause.push(~Lit(_variables->linked(w1, w2)));
	  add_clause(clause);
	}
      }
    }
    for (int w = *c + 1; w < _sent->length - 1; w++) {
      for (int w1 = w+1; w1 < _sent->length; w1++) {
	if (!_linked_possible(w, w1))
	  continue;
	for (int w2 = 0; w2 <= *c; w2++) {
	  if (!_linked_possible(w2, w1))
	    continue;

	  vec<Lit> clause;
	  clause.push(~Lit(_variables->fat_link(w, *c)));
	  clause.push(~Lit(_variables->linked(w, w1)));
	  clause.push(~Lit(_variables->linked(w2, w1)));
	  add_clause(clause);
	}
      }
    }
    DEBUG_print("----end links cannot cross over the fat-links heuristic");
  }

}

void  SATEncoderConjunctiveSentences::generate_fat_link_up_between_down_conditions() {
  // If a connective wi has a fat-link down to wj, it cannot have
  // links up between wi and wj.
  vec<Lit> clause;
  for (std::vector<int>::const_iterator wi = _connectives.begin(); wi != _connectives.end(); wi++) {
    DEBUG_print("---up between down---");
    for (int wj = 1; wj < _sent->length - 1; wj++) {
      if (*wi == wj)
	continue;
      int l = std::min(*wi, wj) + 1;
      int r = std::max(*wi, wj);
      
      for (int wk = l; wk < r; wk++) {
	int mi = std::min(*wi, wk);
	int ma = std::max(*wi, wk);
	if (!_linked_possible(mi, ma))
	  continue;

	clause.growTo(2);
	clause[0] = ~Lit(_variables->fat_link(wj, *wi));
	clause[1] = ~Lit(_variables->linked(mi, ma));
	add_clause(clause);
      }

      if (isConnectiveOrComma(wj)) {
	clause.growTo(2);
	clause[0] = ~Lit(_variables->fat_link(wj, *wi));
	clause[1] = ~Lit(_variables->fat_link(*wi, wj));
	add_clause(clause);
      }
      

    }
    DEBUG_print("---end up between down---");
  }
}

void  SATEncoderConjunctiveSentences::generate_fat_link_neighbor() {
  // Search guiding heuristic: one fat-link is usually just next to the word
  Lit lhs;
  vec<Lit> rhs;
  char fl_str[MAX_VARIABLE_NAME];
  std::vector<int>::const_iterator w;
  for (w = _connectives.begin(); w != _connectives.end(); w++) {  
    DEBUG_print("----fat-link-neighbor");
    if (*w > 1 && *w < _sent->length - 1) {
      rhs.clear();
      rhs.push(Lit(_variables->fat_link(*w - 1, *w)));
      rhs.push(Lit(_variables->fat_link(*w + 1, *w)));
      lhs = Lit(_variables->neighbor_fat_link(*w));
      sprintf(fl_str, "fl_d_%d", *w);
      Lit fld = Lit(_variables->string(fl_str));
      generate_conditional_or_definition(fld, lhs, rhs);
    }
    DEBUG_print("----end fat-link-neighbor");
  }
}

void SATEncoderConjunctiveSentences::generate_label_compatibility() {
  // Eliminate non-matching indirect links
  DEBUG_print("----label compatibility");
  vec<Lit> clause;
  const std::vector<int>& vars = _variables->link_variables();
  std::vector<int>::const_iterator i, j;
  for (i = vars.begin(); i != vars.end(); i++) {
    const Variables::LinkVar* vari = _variables->link_variable(*i);

    const std::vector<int>& varsjr = _variables->link_variables(vari->right_word, vari->right_position);
    for (j = varsjr.begin(); j != varsjr.end(); j++) {
      const Variables::LinkVar* varj = _variables->link_variable(*j);
      if (!labels_match(vari->label, varj->label)) {
	clause.growTo(2);
	clause[0] = ~Lit(*i);
	clause[1] = ~Lit(*j);
	add_clause(clause);
      }
    }

    const std::vector<int>& varsjl = _variables->link_variables(vari->left_word, vari->left_position);
    for (j = varsjl.begin(); j != varsjl.end(); j++) {
      const Variables::LinkVar* varj = _variables->link_variable(*j);
      if (!labels_match(vari->label, varj->label)) {
	clause.growTo(2);
	clause[0] = ~Lit(*i);
	clause[1] = ~Lit(*j);
	add_clause(clause);
      }
    }
  }
  DEBUG_print("----label compatibility");
}

void SATEncoderConjunctiveSentences::generate_fat_link_existence() {
  // If there is a fat link from wa to wb then there should be 
  // at least one connector in the wa word tag that is indirectly 
  // connected to words between:
  //   if wa < wb:  [0, wa) and [wb+2, n)
  //   if wa > wb:  [0, wb-1) and [wa+1, n)
  DEBUG_print("----fat-link-existence");
  vec<Lit> clause_a, clause_b;
  for (std::vector<int>::const_iterator wb = _connectives.begin(); wb != _connectives.end(); wb++) {
    for (int wa = 1; wa < _sent->length-1; wa++) {
      if (isConnectiveOrComma(wa))
	continue;
      if (wa == *wb)
	continue;
      
      int up  = (wa < *wb) ? wa : *wb - 1;
      int low = (wa < *wb) ? *wb + 2 : wa + 1;

      clause_a.clear();
      clause_b.clear();
      clause_a.push(~Lit(_variables->fat_link(wa, *wb)));
      clause_b.push(~Lit(_variables->fat_link(wa, *wb)));

      std::vector<PositionConnector>::const_iterator ci;
      char dir;
      
      const std::vector<PositionConnector>& lconnectors = _word_tags[wa].get_left_connectors();
      for (ci = lconnectors.begin(); ci != lconnectors.end(); ci++) {
	const std::vector<PositionConnector*>& matches = ci->matches;
	std::vector<PositionConnector*>::const_iterator m;
	for (m = matches.begin(); m != matches.end(); m++) {
	  if ((0 <= (*m)->word && (*m)->word < up) && 
	      link_cw_possible_with_fld((*m)->word, (*m)->position, (*m)->connector->string, 
					'+', *wb, 1, _sent->length - 1)) {
	    clause_a.push(Lit(_variables->link_cw(wa, (*m)->word, (*m)->position, (*m)->connector->string)));	  
	    clause_b.push(Lit(_variables->link_cw(*wb, (*m)->word, (*m)->position, (*m)->connector->string)));	  
	  }
	}
      }

      const std::vector<PositionConnector>& rconnectors = _word_tags[wa].get_right_connectors();
      for (ci = rconnectors.begin(); ci != rconnectors.end(); ci++) {
	const std::vector<PositionConnector*>& matches = ci->matches;
	std::vector<PositionConnector*>::const_iterator m;
	for (m = matches.begin(); m != matches.end(); m++) {
	  if ((low <= (*m)->word && (*m)->word < _sent->length) && 
	      link_cw_possible_with_fld((*m)->word, (*m)->position, (*m)->connector->string, 
					'-', *wb, 1, _sent->length - 1)) {
	    clause_a.push(Lit(_variables->link_cw(wa, (*m)->word, (*m)->position, (*m)->connector->string)));
	    clause_b.push(Lit(_variables->link_cw(*wb, (*m)->word, (*m)->position, (*m)->connector->string)));
	  }
	}
      }

      add_clause(clause_a);
      add_clause(clause_b);
    }
  }
  DEBUG_print("----end fat-link-existence");
}

void SATEncoderConjunctiveSentences::generate_fat_link_Left_Wall_not_inside() {
  // Wall cannot be connected just inside the fat-link tree
  for (int w = 1; w < _sent->length-1; w++) {
    DEBUG_print("---Left Wall cannot be connected inside");
    vec<Lit> clause;
    std::vector<int>::const_iterator c;

    if (!_linked_possible(0, w))
      continue;

    for (c = _connectives.begin(); c != _connectives.end(); c++) {
      if (w == *c)
	continue;
      if (!_linked_possible(0, *c))
	continue;
      
      clause.growTo(3);
      clause[0] = ~Lit(_variables->linked(0, w));
      clause[1] = ~Lit(_variables->fat_link(w, *c));
      clause[2] = Lit(_variables->linked(0, *c));
      add_clause(clause);
    }
    DEBUG_print("---end Left Wall cannot be connected inside");
  }
}

void SATEncoderConjunctiveSentences::generate_fat_link_linked_upperside() {
  // Fat link must be linked on the upper side
  DEBUG_print("--- Fat link must be linked on the upper side");
  vec<Lit> clause;
  std::vector<int>::const_iterator c;
  for (c = _connectives.begin(); c != _connectives.end(); c++) {
    if (isComma(_sent, *c)) 
      continue;

    clause.clear();
    char fl_str[MAX_VARIABLE_NAME];
    sprintf(fl_str, "fl_d_%d", *c);
    clause.push(~Lit(_variables->string(fl_str)));

    for (int wl = 0; wl < *c - 1; wl++) {
      if (_linked_possible(wl, *c)) {
	const std::vector<PositionConnector>& vc = _word_tags[wl].get_right_connectors();
	std::vector<PositionConnector>::const_iterator i;
	for (i = vc.begin(); i != vc.end(); i++) {
	  if (link_cw_possible(wl, i->position, i->connector->string, '+', *c, 1, _sent->length - 1))
	    clause.push(Lit(_variables->link_cw(*c, wl, (*i).position, (*i).connector->string)));
	}	      
      }
    }
    for (int wr = *c+2; wr < _sent->length; wr++) {
      if (_linked_possible(*c, wr)) {
	const std::vector<PositionConnector>& vc = _word_tags[wr].get_left_connectors();
	std::vector<PositionConnector>::const_iterator i;
	for (i = vc.begin(); i != vc.end(); i++) {
	  if (link_cw_possible(wr, i->position, i->connector->string, '-', *c, 1, _sent->length - 1))
	    clause.push(Lit(_variables->link_cw(*c, wr, (*i).position, (*i).connector->string)));
	}	      
      }	    
    }

    std::vector<int>::const_iterator c1;
    for (c1 = _connectives.begin(); c1 != _connectives.end(); c1++) {
      if (c1 != c)
	clause.push(Lit(_variables->fat_link(*c, *c1)));	      
    }
    add_clause(clause);
  }
  DEBUG_print("---end Fat link must be connected on the upper side");
}

void SATEncoderConjunctiveSentences::either_tag_or_fat_link(int w, Lit tag) {
  DEBUG_print("----either tag or fat_link");
  vec<Lit> clause;
  char fl_str[MAX_VARIABLE_NAME];
  sprintf(fl_str, "fl_d_%d", w);

  // Either tag of the word is satisfied or there must be fat-link on
  // this word
  clause.growTo(2);
  clause[0] = tag;
  clause[1] = Lit(_variables->string(fl_str));
  add_clause(clause);

  // There cannot be both fat-links and a satisfied tag
  clause.growTo(2);
  clause[0] = ~tag;
  clause[1] = ~Lit(_variables->string(fl_str));
  add_clause(clause);

  DEBUG_print("----end either tag or fat_link");
}

// Check if there can be a connection between the connector (wi, pi)
// with the word w when the word w has fat links down
bool SATEncoderConjunctiveSentences::link_cw_possible_with_fld(int wi, int pi, const char* Ci, 
							       char dir, int w, int llim, int rlim) {
  // Connective word can match iff the connector is andable and there
  // are words wk1 and wk2 such that B(wi, wk1, w) and B(wi, w, wk1)
  // and there are connectors on wk1 and wk2 that can match (wi, pi)
  Connector conn;
  init_connector(&conn);
  conn.label = NORMAL_LABEL;
  conn.priority = THIN_priority;
  conn.string = Ci;
  bool andable_opposite = is_andable(_sent, &conn, dir == '+' ? '-' : '+');
  
  if (!andable_opposite) {
    return false;
  }

  int low = wi < w ? std::max(llim, wi + 1) : llim;
  int hi = wi < w ? rlim : std::min(wi, rlim);

  return matches_in_interval(wi, pi, low, w) && matches_in_interval(wi, pi, w + 1, hi);
}

// Check if there can be a connection between the connector (wi, pi) with the word w
bool SATEncoderConjunctiveSentences::link_cw_possible(int wi, int pi, const char* Ci, char dir, int w, int llim, int rlim) {
  if (!isConnectiveOrComma(w)) {
    // Ordinary word can match iff it has a connector that can match
    return _word_tags[w].match_possible(wi, pi);
  } else {
    // Connective word can match iff it can match as an ordinary word
    // or the connector is andable and there are words wk1 and wk2
    // such that B(wi, wk1, w) and B(wi, w, wk1) and there are
    // connectors on wk1 and wk2 that can match (wi, pi)
    return _word_tags[w].match_possible(wi, pi) || 
      link_cw_possible_with_fld(wi, pi, Ci, dir, w, llim, rlim);
  }  
}




void SATEncoderConjunctiveSentences::generate_link_top_ww_connective_comma_definition(Lit lhs, int w1, int w2) {
  DEBUG_print("---- link_top_connective_comma");
  
  vec<Lit> clause;
  
  vec<Lit> fatlinks_left, fatlinks_right, link_top_left, link_top_right;
  int low = w1 < w2 ? 1 : w2+1;
  int hi  = w1 < w2 ? w2 : _sent->length - 1;
  for (int w = low; w < w1; w++) {
    Lit fl(_variables->fat_link(w, w1));
    Lit tl(_variables->link_top_ww(w, w2));

    clause.growTo(3);
    clause[0] = ~fl;
    clause[1] = ~lhs;
    clause[2] = tl;
    add_clause(clause);

    fatlinks_left.push(fl);
    link_top_left.push(tl);
  }

  DEBUG_print("---------");

  for (int w = w1 + 1; w < hi; w++) {
    Lit fr(_variables->fat_link(w, w1));
    Lit tr(_variables->link_top_ww(w, w2));

    clause.growTo(3);
    clause[0] = ~fr;
    clause[1] = ~lhs;
    clause[2] = tr;
    add_clause(clause);

    fatlinks_right.push(fr);
    link_top_right.push(tr);
  }

  clause.clear();
  for (int i = 0; i < fatlinks_left.size(); i++) {
    for (int j = 0; j < fatlinks_right.size(); j++) {
      clause.growTo(5);
      clause[0] = ~fatlinks_left[i];
      clause[1] = ~link_top_left[i];
      clause[2] = ~fatlinks_right[j];
      clause[3] = ~link_top_right[j];
      clause[4] = lhs;
      add_clause(clause);
    }
  }

  DEBUG_print("---- end link_top_connective_comma");
}


void SATEncoderConjunctiveSentences::generate_linked_definitions() {
  Matrix<int> _thin_link_possible;
  _thin_link_possible.resize(_sent->length, 1);

  DEBUG_print("------- link_top_ww definitions");
  for (int w1 = 0; w1 < _sent->length; w1++) {
    for (int w2 = 0; w2 < _sent->length; w2++) {
      if (w1 == w2)
	continue;
      DEBUG_print("---------- ." << w1 << ". ." << w2 << ".");
      Lit lhs;
      vec<Lit> rhs;
      const std::vector<PositionConnector>& w1_connectors = 
	(w1 < w2) ? _word_tags[w1].get_right_connectors() : _word_tags[w1].get_left_connectors();
      std::vector<PositionConnector>::const_iterator c;
      for (c = w1_connectors.begin(); c != w1_connectors.end(); c++) {
	if (link_cw_possible(w1, c->position, c->connector->string, c->dir, w2, 1, _sent->length - 1)) {
	  rhs.push(Lit(_variables->link_top_cw(w2, w1, c->position, c->connector->string)));
	}
      }
      lhs = Lit(_variables->link_top_ww(w1, w2));

      if (!isConnectiveOrComma(w1) || abs(w1 - w2) == 1) {
	if (rhs.size() == 0)
	  _thin_link_possible.set(w1, w2, 0);

	generate_or_definition(lhs, rhs);
      } else {
	char w1_str[MAX_VARIABLE_NAME];
	sprintf(w1_str, "w%d", w1);
	generate_conditional_or_definition(Lit(_variables->string(w1_str)), lhs, rhs);
	generate_link_top_ww_connective_comma_definition(lhs, w1, w2);
      }
      
      DEBUG_print("----------");
    }
  }
  DEBUG_print("------- end link_top_ww definitions");

  
  DEBUG_print("------- thin_link definitions");
  Lit lhs;
  vec<Lit> rhs;
  for (int w1 = 0; w1 < _sent->length - 1; w1++) {
    for (int w2 = w1 + 1; w2 < _sent->length; w2++) {
      rhs.clear();
      if (_thin_link_possible(w1, w2)) {
	lhs = Lit(_variables->thin_link(w1, w2));
	rhs.growTo(2);
	rhs[0] = Lit(_variables->link_top_ww(w1, w2));
	rhs[1] = Lit(_variables->link_top_ww(w2, w1));
	generate_classical_and_definition(lhs, rhs);
      }
    }
  }
  DEBUG_print("------- thin_linked definitions");


  DEBUG_print("------- linked definitions");
  _linked_possible.resize(_sent->length, 1);
  for (int w1 = 0; w1 < _sent->length - 1; w1++) {
    for (int w2 = w1 + 1; w2 < _sent->length; w2++) {
      rhs.clear();
      if (_thin_link_possible(w1, w2))
	rhs.push(Lit(_variables->thin_link(w1, w2)));
      if (isConnectiveOrComma(w1) && w2 < _sent->length - 1)
	rhs.push(Lit(_variables->fat_link(w2, w1)));
      if (isConnectiveOrComma(w2) && w1 > 0)
	rhs.push(Lit(_variables->fat_link(w1, w2)));

      if (rhs.size() > 0) {
	lhs = Lit(_variables->linked(w1, w2));
	generate_or_definition(lhs, rhs);
	_linked_possible.set(w1, w2, 1);
      }
      else {
	_linked_possible.set(w1, w2, 0);
      }
    }
  }
  DEBUG_print("------- linked definitions");
  

  DEBUG_print("---Weak connectivity");
  for (int w1 = 0; w1 < _sent->length; w1++) {
    // At least one link should exist (weak connectivity)
    vec<Lit> clause;
    for (int w2 = 0; w2 < w1; w2++) {
      if (_linked_possible(w2, w1))
	clause.push(Lit(_variables->linked(w2, w1)));
    }
    for (int w2 = w1+1; w2 < _sent->length; w2++) {
      if (_linked_possible(w1, w2))
	clause.push(Lit(_variables->linked(w1, w2)));
    }
    add_clause(clause);
  }
  DEBUG_print("---end Weak connectivity");

}


void SATEncoderConjunctiveSentences::get_satisfied_link_top_cw_connectors(int word, int top_word, 
									  std::vector<int>& link_top_cw_vars) {
  static int tab = 0;

  // Check if top_word acts as an ordinary or as a special connective words
  char str[MAX_VARIABLE_NAME];
  sprintf(str, "w%d", top_word);
  bool ordinary = _solver->model[_variables->string(str)] == l_True;
    
  if (ordinary) {
    const std::vector<int>& link_top_cw_variables = _variables->link_top_cw_variables();
    std::vector<int>::const_iterator i;
    for (i = link_top_cw_variables.begin(); i != link_top_cw_variables.end(); i++) {
      if (_solver->model[*i] != l_True)
	continue;

      const Variables::LinkTopCWVar* var = _variables->link_top_cw_variable(*i);
      if (var->top_word != word || var->connector_word != top_word)
	continue;

      link_top_cw_vars.push_back(*i);
    }
    
  } else {
    // Find two words that are fat_linked up to the top word
    for (int w = 1; w < _sent->length - 1; w++) {
      if (w == top_word)
	continue;
      if (_solver->model[_variables->fat_link(w, top_word)] == l_True) {
	get_satisfied_link_top_cw_connectors(word, w, link_top_cw_vars);
      }
    }
  }
}

bool SATEncoderConjunctiveSentences::extract_links(Parse_info pi)
{
  int current_link = 0;
  bool fat = false;

  const std::vector<int>& linked_variables = _variables->linked_variables();
  std::vector<int>::const_iterator i;
  for (i = linked_variables.begin(); i != linked_variables.end(); i++) {
    if (_solver->model[*i] != l_True)
      continue;

    const Variables::LinkedVar* var = _variables->linked_variable(*i);

    // Check if words are connected with a fat-link
    bool fl_lr = 0 < var->left_word && var->right_word < _sent->length - 1 && 
      parse_options_get_use_fat_links(_opts) &&
      isConnectiveOrComma(var->right_word) && 
      _solver->model[_variables->fat_link(var->left_word, var->right_word)] == l_True;
    bool fl_rl =  0 < var->left_word && var->right_word < _sent->length - 1 && 
      parse_options_get_use_fat_links(_opts) &&
      isConnectiveOrComma(var->left_word) && 
      _solver->model[_variables->fat_link(var->right_word, var->left_word)] == l_True;
    if (fl_lr || fl_rl) {
      // a fat link
      pi->link_array[current_link].l = var->left_word;
      pi->link_array[current_link].r = var->right_word;

      Connector* connector;

      connector = connector_new();
      connector->priority = fl_lr ? UP_priority : DOWN_priority;
      connector->string = "fat";
      pi->link_array[current_link].lc = connector;

      connector = connector_new();
      connector->priority = fl_lr ? DOWN_priority : UP_priority;
      connector->string = "fat";
      pi->link_array[current_link].rc = connector;
      
      current_link++;
      fat = true;
    } else {
      // a thin link
      std::vector<int> link_top_cw_vars_right;
      std::vector<int> link_top_cw_vars_left;
      get_satisfied_link_top_cw_connectors(var->left_word, var->right_word, link_top_cw_vars_left);
      get_satisfied_link_top_cw_connectors(var->right_word, var->left_word, link_top_cw_vars_right);

      std::vector<const char*> strings_right;
      std::vector<int>::const_iterator rc, lc;
      for (rc = link_top_cw_vars_right.begin(); rc != link_top_cw_vars_right.end(); rc++) {
	const Variables::LinkTopCWVar* var = _variables->link_top_cw_variable(*rc);
	strings_right.push_back(var->connector);
      }

      std::vector<const char*> strings_left;
      for (lc = link_top_cw_vars_left.begin(); lc != link_top_cw_vars_left.end(); lc++) {
	const Variables::LinkTopCWVar* var = _variables->link_top_cw_variable(*lc);
	strings_left.push_back(var->connector);
      }

      // String unification
      std::vector<const char*>::const_iterator li, ri;
      const char* left_string = strings_left[0];
      for (li = strings_left.begin() + 1; li != strings_left.end(); li++) {
	left_string = intersect_strings(_sent, left_string, *li);
      }

      const char* right_string = strings_right[0];
      for (ri = strings_right.begin() + 1; ri != strings_right.end(); ri++) {
	right_string = intersect_strings(_sent, right_string, *ri);
      }
      
      pi->link_array[current_link].l = var->left_word;
      pi->link_array[current_link].r = var->right_word;

      Connector* connector;

      connector = connector_new();
      connector->string = right_string;
      pi->link_array[current_link].lc = connector;

      connector = connector_new();
      connector->string = left_string;
      pi->link_array[current_link].rc = connector;
      
      current_link++;
    }
  }

  pi->N_links = current_link;
  return fat;
}


/****************************************************************************
 *              Main entry point into the SAT parser                        *
 ****************************************************************************/
extern "C" int sat_parse(Sentence sent, Parse_Options  opts)
{
  SATEncoder* encoder = (SATEncoder*) sent->hook;
  if (encoder) delete encoder;

  // Prepare for parsing - extracted for "preparation.c"
  build_deletable(sent, 0);
  build_effective_dist(sent, 0);
  init_count(sent);
  count_set_effective_distance(sent);

  bool conjunction = FALSE;
  if (parse_options_get_use_fat_links(opts)) {
    conjunction = sentence_contains_conjunction(sent);
  }

  // Create the encoder, encode the formula and solve it
  if (conjunction) {
    DEBUG_print("Conjunctive sentence\n");
    encoder = new SATEncoderConjunctiveSentences(sent, opts);
  } else {
    DEBUG_print("Conjunction free sentence\n");
    encoder = new SATEncoderConjunctionFreeSentences(sent, opts);
  }
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

  // Don't do this parse-info-free stuff, if there's no encoder.
  // That's because it will screw up the regular parser.
  if (sent->parse_info) {
    Parse_info pi = sent->parse_info;
    for (int i=0; i< MAX_LINKS; i++) {
      free_connectors(pi->link_array[i].lc);
      free_connectors(pi->link_array[i].rc);
    }
    free_parse_info(sent->parse_info);
    sent->parse_info = NULL;
  }

}
