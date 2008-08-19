#ifndef __VARIABLES_HPP__
#define __VARIABLES_HPP__

#include <vector>
#include <map>

extern "C" {
#include <link-grammar/api.h>
}

#include "guiding.hpp"
#include "Solver.h"
#include "trie.hpp"
#include "matrix-ut.hpp"
#include "fast-sprintf.hpp"


#define  MAX_VARIABLE_NAME 256


// #define _DEBUG
// #define _VARS

#ifdef _DEBUG
#define _VARS
#endif



#ifdef _VARS
extern ostream& var_defs_stream;
#endif

static char* construct_link_label(const char* connector1, const char* connector2) {
  char* result = (char*)xalloc((std::max(strlen(connector1), strlen(connector2)) + 1)*
			       sizeof(char));
  char* presult = result;
  while (*connector1 != '\0' && *connector2 != '\0') {
    if (*connector1 == '*')
      *presult++ = *connector2;
    else if (*connector2 == '*')
      *presult++ = *connector1;
    else 
      *presult++ = std::max(*connector1, *connector2);

    connector1++;
    connector2++;
  }
  while(*connector1 != '\0') 
    *presult++ = *connector1++;
  while(*connector2 != '\0') 
    *presult++ = *connector2++;
  *presult = '\0';
  return result;
}

static bool labels_match(const char* label1, const char* label2) {
  //  printf("Compare: %s %s\n", label1, label2);

  if ((label1[0] == 'S' &&  label2[0] == 'S') &&
      (label1[1] == 's' || label1[1] == 'p') &&
      (label2[1] == 's' || label2[1] == 'p')) {
    label1 += 2;
    label2 += 2;
  }

  while(*label1 != '\0' && *label2 != '\0') {
    if (*label1 != '*' && *label2 != '*' &&
	*label1 != *label2)
      return false;

    label1++; label2++;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////
class Variables {
public:
  Variables(Sentence sent) 
    :_var(0)
    ,_sent(sent)
    ,_linked_variable_map(sent->length, -1)
    ,_linked_min_variable_map(sent->length, -1)
    ,_linked_max_variable_map(sent->length, -1)
    ,_link_variable_map(sent->length)
    ,_fat_link_variable_map(sent->length, -1)
    ,_link_top_cw_variable_map(sent->length)
    , _link_cw_variable_map(sent->length)
    ,_link_top_ww_variable_map(sent->length, -1)
    ,_thin_link_variable_map(sent->length, -1)
    ,_guiding(new CostDistanceGuiding(sent))
  {
  }

  ~Variables() {
    std::vector<LinkVar*>::iterator i;
    for (i = _link_variables.begin(); i != _link_variables.end(); i++) {
      if ((*i) != 0) {
	xfree((*i)->label, strlen((*i)->label));
	delete *i;
      }
    }
    delete _guiding;
  }
  

  /* 
   * General purpose variables specified by their names
   */

  // If guiding params are unknown, they are set do default
  int string(const char* name) {
    int var;
    if (!get_var_from_trie(name, var)) {
#ifdef _VARS
      var_defs_stream << name << "\t" << var << endl;
#endif
      _guiding->setStringParameters(var, name);
    }
    assert(var != -1, "Var == -1");
    return var;
  }

  // If the cost is explicitely given, guiding params are calculated
  // using the cost. Any params set earlier are overridden. 
  int string_cost(const char* name, int cost) {
    int var;
    var = string(name);
    _guiding->setStringParameters(var, name, cost);
    assert(var != -1, "Var == -1");
    return var;
  }

  /*
   * Variables that specify that a part of word tag is satisfied
   * without making any connections of the given direction.
   */

  // If guiding params are unknown, they are set do default
  int epsilon(char* v, char dir) {
    char name[MAX_VARIABLE_NAME];
    dir = (dir == '+') ? 'r' : 'l';
    char* s = name;
    *s++ = dir;
    *s++ = 'e';
    s = fast_sprintf(s, v);
    int var;
    if (!get_var_from_trie(name, var)) {
#ifdef _VARS
      var_defs_stream << name << "\t" << var << endl;
#endif
      _guiding->setEpsilonParameters(var);
    }
    assert(var != -1, "Var == -1");
    return var;
  }

  /*
   *             linked(wi, wj)
   * Variables that specify that two words are linked
   */

  // If guiding params are unknown, they are set do default
  int linked(int wi, int wj) {
    assert(wi < wj, "Variables: linked should be ordered");
    int var;
    if (!get_linked_variable(wi, wj, var)) {
#ifdef _VARS
      var_defs_stream << "linked_" << wi << "_" << wj << "\t" << var << endl;
#endif
      add_linked_variable(wi, wj, var);
      _guiding->setLinkedParameters(var, wi, wj);
    }
    assert(var != -1, "Var == -1");
    return var;
  }

  // If guiding params are unknown, they are set do default
  int linked_max(int wi, int wj) {
    int var;
    if (!get_linked_max_variable(wi, wj, var)) {
#ifdef _VARS
      var_defs_stream << "linked_max_" << wi << "_" << wj << "\t" << var << endl;
#endif
      _guiding->setLinkedMinMaxParameters(var, wi, wj);
    }
    assert(var != -1, "Var == -1");
    return var;
  }

  // If guiding params are unknown, they are set do default
  int linked_min(int wi, int wj) {
    int var;
    if (!get_linked_min_variable(wi, wj, var)) {
#ifdef _VARS
      var_defs_stream << "linked_min_" << wi << "_" << wj << "\t" << var << endl;
#endif
      _guiding->setLinkedMinMaxParameters(var, wi, wj);
    }
    assert(var != -1, "Var == -1");
    return var;
  }



  /*
   *                  link(wi, pi, wj, pj)
   * Variables that specify that a direct link has been established
   * between the connectors ci of the word_i at position i and
   * cj of the word_j at position j
   */ 

  // If guiding params are unknown, they are set do default
  int link(int wi, int pi, const char* ci, int wj, int pj, const char* cj) {
    assert(wi < wj, "Variables: link should be ordered");
    int var;
    if (!get_link_variable(wi, pi, wj, pj, var)) {
#ifdef _VARS
      var_defs_stream << "link_" << wi << "_" << pi << "_" << ci << "_" 
		      << wj << "_" << pj << "_" << cj << "\t" << var << endl;
#endif
      add_link_variable(wi, pi, ci, wj, pj, cj, var);
      _guiding->setLinkParameters(var, wi, ci, wj, cj, _link_variables[var]->label);
    }
    assert(var != -1, "Var == -1");
    return var;
  }

  // If the cost is specified, guiding params are calculated 
  // using the cost. Any guiding params that are set earlier are overridden
  int link_cost(int wi, int pi, const char* ci, int wj, int pj, const char* cj, 
		int cost) {
    assert(wi < wj, "Variables: link should be ordered");
    int var = link(wi, pi, ci, wj, pj, cj);
    _guiding->setLinkParameters(var, wi, ci, wj, cj, link_variable(var)->label, cost);
    assert(var != -1, "Var == -1");
    return var;
  }

  /*
   * The following types of variables are used for conjunction handling
   */


  /*
   *                  fat_link(wi, wj)
   * Variables that specify that there is a fat link from the word i
   * up to the word j
   */

  // If guiding params for this variable are not set earlier, they are
  // now set to default
  int fat_link(int wi, int wj) {
    assert(_sent->is_conjunction[wj] || strcmp(_sent->word[wj].string, ",") == 0, 
	   "Fat link can up only to a connective word");
    int var;
    if (!get_fat_link_variable(wi, wj, var)) {
#ifdef _VARS
      var_defs_stream << "fatlink_" << wi << "_" << wj << "\t" << var << endl;
#endif
      add_fatlink_variable(wi, wj, var);
      _guiding->setFatLinkParameters(var, wi, wj);
    }
    assert(var != -1, "Var == -1");
    return var;
  }

  /* 
   *                   fat_link_neighbor(w) 
   * It seems that connective words usually have fat links to at least one
   * neighboring word. This is a definitional variable for that notion and
   * it can positively affect the search guiding. 
   */
  int neighbor_fat_link(int w) {
    assert(_sent->is_conjunction[w] || strcmp(_sent->word[w].string, ",") == 0, 
	   "Only connective words can have fat links down");
    int var;
    char name[MAX_VARIABLE_NAME];
    sprintf(name, "nfl_%d", w);
    if (!get_var_from_trie(name, var)) {
#ifdef _VARS
      var_defs_stream << name << "\t" << var << endl;
#endif
      _guiding->setNeighborFatLinkParameters(var, w);
    }
    assert(var != -1, "Var == -1");
    return var;
  }

  /*
   *             thin_link(wi, wj)
   * Variables that specify that two words are linked by a thin link
   */

  // If guiding params are unknown, they are set do default
  int thin_link(int wi, int wj) {
    assert(wi < wj, "Variables: thin link should be ordered");
    int var;
    if (!get_thin_link_variable(wi, wj, var)) {
#ifdef _VARS
      var_defs_stream << "thin_link_" << wi << "_" << wj << "\t" << var << endl;
#endif
      _guiding->setThinLinkParameters(var, wi, wj);
    }
    assert(var != -1, "Var == -1");
    return var;
  }

  /*
   *                   link_cw(wi, wj, pj)
   * Variables that specify that an indirect link has been established
   * between the word i and connector cj of the word_j at position
   * j.
   */
  
  // If guiding params for this variable are not set earlier, they are
  // now set to default
  int link_cw(int wi, int wj, int pj, const char* cj) {
    int var;
    if (!get_link_cw_variable(wi, wj, pj, var)) {
#ifdef _VARS
      var_defs_stream << "link_cw_"
		      << "(" << wj << "_" << pj << "_" << cj  << ")_"  
		      << wi
		      << "\t" << var << endl;
#endif
      _guiding->setLinkCWParameters(var, wi, wj, cj);
    }
    assert(var != -1, "Var == -1");
    return var;
  }  

  
  /*
   *                 link_top_cw(wi, wj, pj)
   * Variables that specify that a connective word has been directly
   * linked to a connector
   */

  // If guiding params for this variable are not set earlier, they are
  // now set to default
  int link_top_cw(int wi, int wj, int pj, const char* cj) {
    int var;
    if (!get_link_top_cw_variable(wi, wj, pj, var)) {
      add_link_top_cw_variable(wi, wj, pj, cj, var);
      _guiding->setLinkTopCWParameters(var, wi, wj, cj);
#ifdef _VARS
      var_defs_stream << "link_top_cw_"
		      << "(" << wj << "_" << pj << "_" << cj  << ")_" 
		      << wi
		      << "\t" << var << endl;
#endif            
    }
    assert(var != -1, "Var == -1");
    return var;
  }

  /*
   *             link_top_ww(wi, wj)
   * Variables that specify that word wi is linked to the word wj, and
   * wj is a top of the fat-link tree.
   */

  // If guiding params are unknown, they are set do default
  int link_top_ww(int wi, int wj) {
    int var;
    if (!get_link_top_ww_variable(wi, wj, var)) {
#ifdef _VARS
      var_defs_stream << "link_top_ww_" << wi << "_" << wj << "\t" << var << endl;
#endif
      _guiding->setLinkTopWWParameters(var, wi, wj);
    }
    assert(var != -1, "Var == -1");
    return var;
  }



#ifdef _CONNECTIVITY_
  /* The following variables are deprecated */

  // Variables that specify that words i and j are connected
  int con(int i, int j) {
    char name[MAX_VARIABLE_NAME];
    char* s = name;
    *s++ = 'c';
    s = fast_sprintf(s, i);
    *s++ = '_';
    s = fast_sprintf(s, j);
    int var;
    if (!get_var_from_trie(name, var))
      set_variable_sat_params(var, false);
    return var;
  }

  // Auxilary variables used for connectivity encoding
  int l_con(int i, int j, int k) {
    int var;
    if (!get_lcon_variable(i, j, k, var))
      set_variable_sat_params(var, false);
    return var;
  }
#endif

  /* 
   *      link(wi, pi, wj, pj)
   */
  // Returns the indices of all link variables
  const std::vector<int>& link_variables() {
    return _link_variables_indices;
  }

  // Returns the indices of all link_x_x_wj_pj variables
  const std::vector<int>& link_variables(int wj, int pj) {
    std::pair<int, int> p(wj, pj);
    return _link_variable_wp_map[p];
  }

  // Additional info about the link(wi, pi, wj, pj) variable
  struct LinkVar {
    LinkVar(const std::string& _name, char* _label, 
	    int _lw, int _lp, int _rw, int _rp, const char* _lc, const char* _rc) 
      : name(_name), label(_label),
	left_word(_lw), right_word(_rw), left_position(_lp), right_position(_rp),
	left_connector(_lc), right_connector(_rc) {
    }
    
    std::string name;
    char* label;
    const char* left_connector;
    const char* right_connector;
    int left_word;
    int left_position;
    int right_word;
    int right_position;
  };

  // Returns additional info about the given link variable
  const LinkVar* link_variable(int var) {
    return _link_variables[var];
  }

  /* 
   *       linked(wi, wj) 
   */
  // Returns the indices of all linked variables
  const std::vector<int>& linked_variables() {
    return _linked_variables_indices;
  }

  // Additional info about the linked(i, j) variable
  struct LinkedVar {
    LinkedVar(int lw, int rw) 
      : left_word(lw), right_word(rw) {
    }

    int left_word;
    int right_word;
  };  

  // Returns additional info about the given linked variable
  const LinkedVar* linked_variable(int var) {
    return _linked_variables[var];
  }
  


  /* 
   *           link_top_cw((wi, pi), wj)
   */
  
  // Returns indices of all link_top_cw variables
  const std::vector<int>& link_top_cw_variables() {
    return _link_top_cw_variables_indices;
  }

  // Additional info about the link_top_cw(wi, wj, pj) variable
  struct LinkTopCWVar {
    LinkTopCWVar(const std::string& _name, int _tw, int _cw, const char* _c) 
      : name(_name), connector_word(_cw), top_word(_tw), connector(_c) {
    }
    
    std::string name;
    char* label;
    const char* connector;
    int connector_word;
    int top_word;
  };

  // Returns additional info about the given link_top_cw variable
  const LinkTopCWVar* link_top_cw_variable(int var) {
    return _link_top_cw_variables[var];
  }

  /* 
   *           fat_link(wi, wj)
   */
  // Returns indices of all fat-link variables 
  const std::vector<int>& fat_link_variables() {
    return _fat_link_variables_indices;
  }

  // Additional info about the fatlink(wi, wj) variable
  struct FatLinkVar {
    FatLinkVar(int down, int up) 
      : down_word(down), up_word(up) {
    }

    int down_word;
    int up_word;  
  };

  // Return additional info about the given fat-link variable
  const FatLinkVar* fat_link_variable(int var) {
    return _fat_link_variables[var];
  }

  /* Pass SAT search parameters to the MiniSAT solver */
  void setVariableParameters(Solver* solver) {
    _guiding->passParametersToSolver(solver);
  }

private:
  /* Sentence that is being parsed */
  Sentence _sent;

  /* 
   * Information about string variables 
   */

  // What is the number of the variable with the given name?
  Trie<int> _variable_trie;

  /*
   * Information about link(wi, pi, wj, pj) variables 
   */

  // What is the number of the link(wi, pi, wj, pj) variable?
  MatrixUpperTriangle< std::map<std::pair<int, int>, int> > _link_variable_map;


  // What are the numbers of all link(wi, pi, wj, pj) variables?
  std::vector<int>  _link_variables_indices;

  // What are the numbers of all link(x, x, wj, pj) variables?
  std::map< std::pair<int, int>, std::vector<int> > _link_variable_wp_map;


  // Additional info about the link(wi, pi, wj, pj) variable with the given number
  std::vector<LinkVar*> _link_variables;

  // Set this additional info
  void add_link_variable(int i, int pi, const char* ci, int j, int pj, const char* cj, int var) {
    char name[MAX_VARIABLE_NAME];
    char* s = name;
    *s++ = 'l';    *s++ = 'i';     *s++ = 'n';     *s++ = 'k';
    *s++ = '_';
    s = fast_sprintf(s, i);
    *s++ = '_';
    s = fast_sprintf(s, pi);
    *s++ = '_';
    s = fast_sprintf(s, ci);
    *s++ = '_';
    s = fast_sprintf(s, j);
    *s++ = '_';
    s = fast_sprintf(s, pj);
    *s++ = '_';
    s = fast_sprintf(s, cj);
    char* label = construct_link_label(ci, cj);

    if (var >= _link_variables.size()) {
      _link_variables.resize(var + 1, 0);
    }
    _link_variables[var] = new LinkVar(name, label, i, pi, j, pj, ci, cj);
    _link_variables_indices.push_back(var);
  }


  /* 
   * Information about linked(i, j) variables 
   */


  // What is the number of the linked(i, j) variable?
  MatrixUpperTriangle<int> _linked_variable_map;
  
  // What are the numbers of all linked(i, j) variables?
  std::vector<int>  _linked_variables_indices;

  // Additional info about the linked(i, j) variable
  std::vector<LinkedVar*> _linked_variables;

  // Set the additional info
  void add_linked_variable(int i, int j, int var) {
    if (var >= _linked_variables.size()) {
      _linked_variables.resize(var + 1, 0);
    }    
    _linked_variables[var] = new LinkedVar(i, j);
    _linked_variables_indices.push_back(var);
  }

  // What is the number of the linked_min(i, j) variable?
  Matrix<int> _linked_min_variable_map;

  // What is the number of the linked_max(i, j) variable?
  Matrix<int> _linked_max_variable_map;

  /* 
   * Information about the fat_link(i, j) variables 
   */

  // What is the number of the fatlink(wi, wj) variable?
  Matrix<int> _fat_link_variable_map;

  // What are the numbers of all fatlink(wi wj) variables?
  std::vector<int>  _fat_link_variables_indices;

  // Additional info about the fatlink(wi, wj) variable with the given number
  std::vector<FatLinkVar*> _fat_link_variables;

  // Set the additional info
  void add_fatlink_variable(int i, int j, int var) {
    if (var >= _fat_link_variables.size()) {
      _fat_link_variables.resize(var + 1, 0);
    }    
    _fat_link_variables[var] = new FatLinkVar(i, j);
    _fat_link_variables_indices.push_back(var);
  }

  /* 
   * Information about the thin_link(i, j) variables
   */
  // What is the number of the thin_link(i, j) variable?
  MatrixUpperTriangle<int> _thin_link_variable_map;

  /* 
   * Information about the link_top_cw(w, wj, pj) variables 
   */

  // What is the number of the link_top_cw(wi, wj, pj) variable?
  Matrix< std::map<int, int> > _link_top_cw_variable_map;

  // What are the numbers of all link_top_cw(wi, wj, pj) variables?
  std::vector<int>  _link_top_cw_variables_indices;

  // Additional info about the link_top_cw(wi, wj, pj) variable with the given number
  std::vector<LinkTopCWVar*> _link_top_cw_variables;

  // Set this additional info
  void add_link_top_cw_variable(int i, int j, int pj, const char* cj, int var) {
    char name[MAX_VARIABLE_NAME];
    char* s = name;
    *s++ = 'l';    *s++ = 'i';     *s++ = 'n';     *s++ = 'k'; *s++ = '_'; *s++ = 't'; *s++ = 'o'; *s++ = 'p';
    *s++ = '_';
    s = fast_sprintf(s, i);
    *s++ = '_'; *s++ = '(';
    s = fast_sprintf(s, j);
    *s++ = '_';
    s = fast_sprintf(s, pj);
    *s++ = '_';
    s = fast_sprintf(s, cj);
    *s++ = ')';
    *s = '\0';

    if (var >= _link_top_cw_variables.size()) {
      _link_top_cw_variables.resize(var + 1, 0);
    }
    _link_top_cw_variables[var] = new LinkTopCWVar(name, i, j, cj);
    _link_top_cw_variables_indices.push_back(var);
  }

  /* 
   *   Information about the link_cw(w, wj, pj) variables
   */
  // What is the number of the link_cw(wi, wj, pj) variable?
  Matrix< std::map<int, int> > _link_cw_variable_map;

  /* 
   * Information about the link_top_ww(i, j) variables 
   */

  // What is the number of the link_top_ww(i, j) variable?
  Matrix<int> _link_top_ww_variable_map;


#ifdef _CONNECTIVITY_
  std::map<std::pair<std::pair<int, int>,int>, int> _lcon_variables;
#endif

  /* SAT search parameters */
  Guiding* _guiding;

  /* Current free variable number */
  int _var;

  /* Get a variable number that has not been used before */
  int get_fresh_var(void) {
    return _var++;
  }

  /* Helper functions that retrieve variable numbers from appropriate
     data structures. If the variable is not present, it is assigned a
     fresh variable number, and false is returned. Otherwise, the number
     is retrieved and true is returned. */

  bool get_var_from_trie(const char* name, int& var) {
    try {
      int num = _variable_trie.lookup(name);
      if (num != Trie<int>::NOT_FOUND) {
	var = num;
	return true;
      }
      else {
	var = get_fresh_var();
	_variable_trie.insert(name, var);
	return false;
      }
    } catch (const std::string& s) {
      cout << s << endl; 
      exit(EXIT_FAILURE);
    }
  }

  
  bool get_2int_variable(int i, int j, int& var, 
			 Matrix<int>& mp) {
    var = mp(i, j);
    if (var == -1) {
      var = get_fresh_var();
      mp.set(i, j, var);
      return false;
    }
    return true;
  }

  bool get_3int_variable(int i, int j, int pj, int& var, 
			 Matrix< std::map<int, int> >& mp) {
    std::map<int, int>& m = mp(i, j);
    std::map<int, int>::iterator it = m.find(pj);
    if (it == m.end()) {
      var = get_fresh_var();
      m[pj] = var;
      return false;
    } else {
      var = it->second;
      return true;
    }
  }

  bool get_4int_variable(int i, int pi, int j, int pj, int& var, 
			 Matrix< std::map<std::pair<int, int>, int> >& mp) {
    std::map< std::pair<int, int>, int >& m = mp(i, j);
    std::pair<int, int> p(pi, pj);
    std::map< std::pair<int, int>, int >::iterator it = m.find(p);
    if (it == m.end()) {
      var = get_fresh_var();
      m[p] = var;
      return false;
    } else {
      var = it->second;
      return true;
    }    
  }

  bool get_link_variable(int i, int pi, int j, int pj, int& var) {
    bool ret = get_4int_variable(i, pi, j, pj, var, _link_variable_map);
    if (!ret) {
      std::pair<int, int> p(j, pj);
      _link_variable_wp_map[p].push_back(var);
    }
    return ret;
  }


  bool get_linked_variable(int i, int j, int& var) {
    return get_2int_variable(i, j, var, _linked_variable_map);
  }

  bool get_linked_min_variable(int i, int j, int& var) {
    return get_2int_variable(i, j, var, _linked_min_variable_map);
  }

  bool get_linked_max_variable(int i, int j, int& var) {
    return get_2int_variable(i, j, var, _linked_max_variable_map);
  }

  bool get_fat_link_variable(int i, int j, int& var) {
    return get_2int_variable(i, j, var, _fat_link_variable_map);
  }

  bool get_thin_link_variable(int i, int j, int& var) {
    return get_2int_variable(i, j, var, _thin_link_variable_map);
  }

  bool get_link_cw_variable(int i, int j, int pj, int& var) {
    return get_3int_variable(i, j, pj, var, _link_cw_variable_map);
  }

  bool get_link_top_cw_variable(int i, int j, int pj, int& var) {
    return get_3int_variable(i, j, pj, var, _link_top_cw_variable_map);
  }

  bool get_link_top_ww_variable(int i, int j, int& var) {
    return get_2int_variable(i, j, var, _link_top_ww_variable_map);
  }


#ifdef _CONNECTIVITY_
  bool get_lcon_variable(int i, int j, int k, int& var) {
    std::pair<std::pair<int, int>, int> p(std::pair<int, int>(i, j), k);
    std::map<std::pair<std::pair<int, int>, int>, int>::iterator it = _lcon_variables.find(p);
    if (it != _lcon_variables.end()) {
      var = it->second;
      return true;
    } else {
      var = get_fresh_var();
#ifdef _VARS
      var_defs_stream << "lcon_" << i << "_" << j << "_" << k << "\t" << var << endl;
#endif
      _lcon_variables[p] = var;
      return false;
    }
  }
#endif

};

#endif
