extern "C" {
#include <link-grammar/api.h>
}

extern "C" int sat_encode(Sentence sent, Parse_Options  opts);
extern "C" Linkage sat_create_linkage(int k, Sentence sent, Parse_Options  opts);
extern "C" void sat_sentence_delete(Sentence sent);

#include "word-tag.hpp"

/**
 *    Base class for all SAT encodings
 */
class SATEncoder {
public:

  // Construct the encoder based on given sentence
  SATEncoder(Sentence sent, Parse_Options  opts)
    : _sent(sent), _opts(opts), _solver(new Solver()), _variables(new Variables(sent)) {
    // Preprocess word tags of the sentence
    build_word_tags();
  }

  virtual ~SATEncoder()
  {
    delete _variables;
    delete _solver;
  }

  // Create the formula from the sentence
  void encode();

  // Solve the formula, returning the next linkage.
  Linkage get_next_linkage();
 
protected:

  /**
   *  Methods that generate various link-grammar constraints.
   */

  // Top-level method that generates satisfaction conditions for every
  // word in the sentence
  void generate_satisfaction_conditions();

  // Generates satisfaction conditions for the word-tag expression e
  void generate_satisfaction_for_expression(int w, int& dfs_position, Exp* e, char* var, int parrent_cost);

  // Handle the case of NULL expression of a word
  virtual void handle_null_expression(int w) = 0;

  // Determine if this word-tag must be satisfied and generate appropriate clauses
  virtual void determine_satisfaction(int w, char* name) = 0;

  // Generates satisfaction condition for the connector (wi, pi)
  virtual void generate_satisfaction_for_connector(int wi, int pi, const char* Ci, 
						   char dir, bool multi, int cost, char* var) = 0;

  // Definition of link_cw((wi, pi), wj) variables when wj is an ordinary word
  void generate_link_cw_ordinary_definition(int wi, int pi, const char* Ci, char dir, int cost, int wj);

  // Generates order constraints for the elements of a conjunction.
  void generate_conjunct_order_constraints(int w, Exp *e1, Exp* e2, int& dfs_position);

  /**
   *  Methods used for optimizing conjunction ordering constraints
   */

  // Number of connectors in an expression
  int num_connectors(Exp* e);
  
  // This expression can be matched without using any connectors of
  // the given direction
  static int  empty_connectors(Exp* exp, char dir);

  // This expression can be matched while using a connector of the
  // given direction
  static int  non_empty_connectors(Exp* exp, char dir);

  // Trailing connectors of a given direction in the given expression
  void trailing_connectors(int w, Exp* exp, char dir, int& dfs_position, 
			   std::vector<PositionConnector*>& connectors);
  bool trailing_connectors_and_aux(int w, E_list* l, char dir, int& dfs_position, 
				   std::vector<PositionConnector*>& connectors);

  // Connectors of the given direction that cannot be trailing
  // connectors
  void certainly_non_trailing(int w, Exp* exp, char dir, int& dfs_position,
			      std::vector<PositionConnector*>& connectors, bool has_right);

  // Connectors that can act as leading connectors of a given
  // direction in the given direction
  void leading_connectors(int w, Exp* exp, char dir, int& dfs_position, 
			  std::vector<PositionConnector*>& connectors);
  
  /**
   *  Definitions of linked(wi, wj) variables.
   */

  // Define all linked(wi, wj) variables
  virtual void generate_linked_definitions() = 0;

  // In order to reduce the number of clauses, some linked(wi, wj)
  // variables can apriori be eliminated. The information about pairs
  // of words that can be linked is kept in this matrix.
  MatrixUpperTriangle<int> _linked_possible;

  /**
   *  Planarity constraints
   */

  // Generates clauses that forbid link-crossing
  void generate_planarity_conditions();
  // Stronger planarity pruning
  void generate_linked_min_max_planarity();


  /**
   *  Connectivity constraints
   */

#ifdef _CONNECTIVITY_
  // Generate clauses that encode the connectivity requirement of the
  // linkage. Experiments showed that it is better to check the
  // connectivity aposteriori and this method has been excised.
  void generate_connectivity();
#endif


  // Helper method for connectivity_components
  static void dfs(int node, const MatrixUpperTriangle<int>& graph, int component, std::vector<int>& components);

  // Extract connectivity components of a linkage. Return true iff the linkage is connected.
  bool connectivity_components(std::vector<int>& components);

  // Generate clauses that prohibit all disconnected linkages that
  // have the specified connectivity components.
  void generate_disconnectivity_prohibiting(std::vector<int> components);


  /**
   *   Encoding specific clauses - override to add clauses that are
   *   specific to a certain encoding
   */
  virtual void generate_encoding_specific_clauses() {}


  /**
   *   Post-processing - PP pruning
   */

  // Generates PP pruning clauses.
  void pp_prune();
  bool post_process_linkage(Linkage linkage);


  /**
   *   Power pruning
   */
  // Generate definition of epsilon variables that are used for power
  // pruning
  void generate_epsilon_definitions();
  bool generate_epsilon_for_expression(int w, int& dfs_position, Exp* e, char* var, bool root, char dir);


  // Power pruning
  void power_prune();
  // auxiliary method that extends power pruning clauses with additional literals 
  // (e.g., link should not be power-prunned if there words are fat-linked)
  virtual void add_additional_power_pruning_conditions(vec<Lit>& clause, int wl, int wr) 
  {}



  /**
   *   Cost cutoff
   */
  
  // Cost cutoff treshold value. Nodes of the expression tree are
  // pruned if their cost exceeds this value. Cost cutoff is performed
  // during satisfaction condition generating.
  static const int _cost_cutoff = 2;


  /**
   *   Creating clauses and passing them to the MiniSAT solver
   */

  // Add the specified clause to the solver
  void add_clause(vec<Lit>& clause) {
#ifdef _DEBUG
    print_clause(clause);
#endif
    for (int i = 0; i < clause.size(); i++) {
      while (var(clause[i]) >= _solver->nVars()) {
	_solver->newVar();
      }
    }
    _solver->addClause(clause);
  }
  

  // Print clause literals to standard output
  static void print_clause(const vec<Lit>& clause) {
    static int num = 1;

    cout << "Clause: ." << num++ << ". ";
    for (int i = 0; i < clause.size(); i++)
      cout << (sign(clause[i]) ? '-' : '+') << var(clause[i]) << " ";
    cout << endl;  
  }



  /**
   *   Conversion of various formula types to CNF. Clauses obtained
   *   are automatically passed to the SAT Solver.
   */
  void generate_literal(Lit l);
  void generate_and_definition(Lit lhs, vec<Lit>& rhs);
  void generate_or_definition(Lit lhs, vec<Lit>& rhs);
  void generate_xor_definition(Lit lhs, vec<Lit>& rhs);
  void generate_equivalence_definition(Lit l1, Lit l2);
  void generate_classical_and_definition(Lit lhs, vec<Lit>& rhs);
  void generate_and(vec<Lit>& vect);
  void generate_or(vec<Lit>& vect);
  void generate_xor_conditions(vec<Lit>& vect);
  void generate_conditional_or_definition(Lit condition, Lit lhs, vec<Lit>& rhs);
  void generate_conditional_lr_implication_or_definition(Lit condition, Lit lhs, vec<Lit>& rhs);
  void generate_conditional_lr_implication_or_definition(Lit condition1, Lit condition2, Lit lhs, vec<Lit>& rhs);

  /*
   *   Word tags of the words in a sentence kept in a preprocessed
   *   form. This enables users to get information about the
   *   connectors in a very eficient way.
   */
  // Word tags
  std::vector<WordTag> _word_tags;

  // Initializes _word_tags array
  void build_word_tags();


  // Find all matching connectors between two words
  void find_all_matches_between_words(int w1, int w2, 
				      std::vector<std::pair<const PositionConnector*, const PositionConnector*> >& matches);

  // Check if the connector (wi, pi) can match any word in [l, r)
  bool matches_in_interval(int wi, int pi, int l, int r);  
  
  
  // Join several expressions corresponding to different dictionary
  // entries of a word into a single expression.
  Exp* join_alternatives(int w); 

  // Erase auxiliary expression tree nodes obtained by joining several
  // expressions into one.
  void free_alternatives(Exp* e);


  /**
   * Decoding
   */

  // Convert propositional model to a parse info structure
  virtual bool extract_links(Parse_info pi) = 0;

  // Create linkage from a propositional model
  Linkage create_linkage();

  // Generate clause that prohibits the current model
  void generate_linkage_prohibiting();


  // Object that contains all information about the variable
  // encoding.
  Variables* _variables;

  // The MiniSAT solver instance. The solver keeps the set of clauses.
  Solver* _solver;
  
  // Sentence that is being parsed.
  Sentence _sent;

  // Parse options.
  Parse_Options  _opts;

};


/*******************************************************************************
 *       SAT encoding for sentences that do not contain conjunction.           *
 *******************************************************************************/
class SATEncoderConjunctionFreeSentences : public SATEncoder {
public:
  SATEncoderConjunctionFreeSentences(Sentence sent, Parse_Options  opts)
    : SATEncoder(sent, opts) {
  }

  virtual void handle_null_expression(int w);
  virtual void determine_satisfaction(int w, char* name);
  virtual void generate_satisfaction_for_connector(int wi, int pi, const char* Ci, 
						   char dir, bool multi, int cost, char* var);


  virtual void generate_linked_definitions();
  virtual bool extract_links(Parse_info pi);

  virtual void generate_encoding_specific_clauses();
};

/*******************************************************************************
 *              SAT encoding for sentences that contain conjunction.           *
 *******************************************************************************/
class SATEncoderConjunctiveSentences : public SATEncoder {
public:
  SATEncoderConjunctiveSentences(Sentence sent, Parse_Options  opts)
    : SATEncoder(sent, opts) {
    init_connective_words();
  }  

private:
  virtual void handle_null_expression(int w);
  virtual void determine_satisfaction(int w, char* name);

  virtual void generate_satisfaction_for_connector(int wi, int pi, const char* Ci, 
						   char dir, bool multi, int cost, char* var);

  virtual void add_additional_power_pruning_conditions(vec<Lit>& clause, int wl, int wr);

  virtual void generate_encoding_specific_clauses();

  // various fat-link conditions	
  void either_tag_or_fat_link(int w, Lit tag);
  void generate_fat_link_up_definitions();
  void generate_fat_link_down_definitions();
  void generate_fat_link_up_between_down_conditions();
  void generate_fat_link_comma_conditions();
  void generate_fat_link_crossover_conditions();
  void generate_fat_link_Left_Wall_not_inside();
  void generate_fat_link_linked_upperside();
  void generate_fat_link_existence();
  void generate_fat_link_neighbor();
  void generate_label_compatibility();

  // link_cw variables

  bool link_cw_possible(int wi, int pi, const char* Ci, char dir, int w, int llim, int rlim);
  bool link_cw_possible_with_fld(int wi, int pi, const char* Ci, char dir, int w, int llim, int rlim);

  void generate_link_cw_connective_impossible         (int wi, int pi, const char* Ci, int wj);
  void generate_link_cw_connective_definition         (int wi, int pi, const char* Cj, int wj);

  // link_top_cw variables
  void generate_link_top_cw_up_definition                  (int wi, 
							    int wj, int pj, const char* Cj, bool multi);
  void generate_link_top_cw_iff_link_cw                    (int wi, 
							    int wj, int pj, const char* Cj);  
  // link_top_ww
  void generate_link_top_ww_connective_comma_definition    (Lit lhs, int wi, int wj);

  // link_ww
  virtual void generate_linked_definitions();

  // Linkage extraction from the model
  void get_satisfied_link_top_cw_connectors(int word, int top_word, std::vector<int>& link_top_cw_vars);
  virtual bool extract_links(Parse_info pi);

  // Initialize the vector of connective words of this sentence
  void init_connective_words();

  // Words that are connectives or commas
  std::vector<int> _connectives;

  // Cache isConnectiveOrComma results for faster checking
  std::vector<bool> _is_connective_or_comma;

  bool isConnectiveOrComma(int w) {
    return _is_connective_or_comma[w];
  }
};
