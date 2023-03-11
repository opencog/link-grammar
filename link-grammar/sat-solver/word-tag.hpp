#ifndef __WORD_TAG_HPP__
#define __WORD_TAG_HPP__

#include <vector>
#include <map>
#include <set>

#include "variables.hpp"

extern "C" {
#include "connectors.h"
#include "connectors.h"
#include "error.h"                      // assert()
#include "dict-common/dict-common.h"
#include "tokenize/tok-structures.h"    // gword_set
#include "tokenize/word-structures.h"   // X_node
#include "tokenize/wordgraph.h"         // in_same_alternative()
};

struct PositionConnector
{
  PositionConnector(Exp* pe, Exp* e, char d, int w, int p,
                    double cst, double pcst, bool lr, bool ll,
                    const std::vector<int>& er, const std::vector<int>& el, const X_node *w_xnode, Parse_Options opts)
    : exp(pe), dir(d), word(w), position(p),
      cost(cst), parent_cost(pcst),
      leading_right(lr), leading_left(ll),
      eps_right(er), eps_left(el), word_xnode(w_xnode)
  {
    if (word_xnode == NULL) {
       cerr << "Internal error: Word" << w << ": " << "; connector: '" << e->condesc->string << "'; X_node: " << (word_xnode?word_xnode->string: "(null)") << endl;
    }

    // Initialize some fields in the connector struct.
    connector.desc = e->condesc;
    connector.multi = e->multi;
    connector.farthest_word = e->farthest_word;
    connector.originating_gword = &w_xnode->word->gword_set_head;

    /*
    cout << c->string << " : ." << w << ". : ." << p << ". ";
    if (leading_right) {
      cout << "lr: ";
      copy(er.begin(), er.end(), ostream_iterator<int>(cout, " "));
    }
    if (leading_left) {
      cout << "ll: ";
      copy(el.begin(), el.end(), ostream_iterator<int>(cout, " "));
    }
    cout << endl;
    */
  }

  // Added only to suppress the warning:
  // warning: inlining failed in call to ‘PositionConnector::~PositionConnector() noexcept’: call is unlikely and code size would grow [-Winline]
  // See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70328
  // Can be removed when this GCC problem is fixed.
  ~PositionConnector() {};

  // Original expression that this connector came from
  Exp* exp;

  // Connector itself
  Connector connector;
  // Direction
  char dir;
  // word in a sentence that this connector belongs to
  size_t word;
  // position in the word tag
  int position;
  // cost of the connector
  double cost;
  // parent cost
  double parent_cost;

  bool leading_right;
  bool leading_left;
  std::vector<int> eps_right;
  std::vector<int> eps_left;


  // The corresponding X_node - chosen-disjuncts[]
  const X_node *word_xnode;

  // Matches with other words
  std::vector<PositionConnector*> matches;

};

/*
 * Record the SAT variable and cost of costly-null expressions.
 * Their cost is recovered (in sat_extract_links()) if
 * they happen to reside on a participating disjunct.
 */
struct EmptyConnector {
  EmptyConnector(int var, double cst)
    : ec_var(var), ec_cost(cst)
  {
  }
  int ec_var;
  double ec_cost;
};

// XXX TODO: Hash connectors for faster matching

class WordTag
{
private:
  std::vector<PositionConnector> _left_connectors;
  std::vector<PositionConnector> _right_connectors;
  std::vector<EmptyConnector> _empty_connectors;

  std::vector<char> _dir;
  std::vector<int> _position;

  int _word;
  Variables* _variables;

  Sentence _sent;
  Parse_Options _opts;

  // Could this word tag match a connector (wi, pi)?
  // For each word wi I keep a set of positions pi that can be matched
  std::vector< std::set<int> > _match_possible;
  void set_match_possible(int wj, int pj) {
    _match_possible[wj].insert(pj);
  }

public:
  WordTag(int word, Variables* variables, Sentence sent, Parse_Options opts)
    : _word(word), _variables(variables), _sent(sent), _opts(opts) {
    _match_possible.resize(_sent->length);

    verbosity = opts->verbosity;
    debug = opts->debug;
    test = opts->test;
  }

  // Added only to suppress the warning:
  // warning: inlining failed in call to ‘WordTag::~WordTag() noexcept’: call is unlikely and code size would grow [-Winline]
  // See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70328
  // Can be removed when this GCC problem is fixed.
  ~WordTag() {};

  const std::vector<PositionConnector>& get_left_connectors() const {
    return _left_connectors;
  }

  const std::vector<PositionConnector>& get_right_connectors() const {
    return _right_connectors;
  }

  const std::vector<EmptyConnector>& get_empty_connectors() const {
    return _empty_connectors;
  }

  PositionConnector* get(int dfs_position)
  {
    switch (_dir[dfs_position - 1]) {
    case '+':
      return &_right_connectors[_position[dfs_position - 1]];
    case '-':
      return &_left_connectors[_position[dfs_position - 1]];
    }
    return NULL;
  }

#define OPTIMIZE_EN
  static bool alt_connectivity_possible(Connector& c1, Connector & c2)
  {
#ifdef OPTIMIZE_EN
  /* Try a shortcut first. */
  if ((c2.originating_gword->o_gword->hier_depth == 0) ||
     (c1.originating_gword->o_gword->hier_depth == 0)) return true;
#endif // OPTIMIZE_EN

    return in_same_alternative(c1.originating_gword->o_gword, c2.originating_gword->o_gword);
  }

  bool match(int w1, Connector& cntr1, char dir, int w2, Connector& cntr2)
  {
      assert(w1 < w2, "match() did not receive words in the natural order.");
      if (cntr1.farthest_word < w2 || cntr2.farthest_word > w1) return false;
      if (!alt_connectivity_possible(cntr1, cntr2)) return false;
      return easy_match_desc(cntr1.desc, cntr2.desc);
  }

  void insert_connectors(Exp* exp, int& dfs_position,
                         bool& leading_right, bool& leading_left,
                         std::vector<int>& eps_right,
                         std::vector<int>& eps_left,
                         char* var, bool root, double parent_cost,
                         Exp* parent, const X_node *word_xnode);

  // Caches information about the found matches to the _matches vector, and also
  // updates the _matches vector of all connectors in the given tag.
  // In order to have all possible matches correctly cached, the function assumes that it is
  // iteratively called for all words in the sentence, where the tag is on the right side of
  // this word
  void add_matches_with_word(WordTag& tag);

  // Find matches in this word tag with the connector (name, dir).
  void find_matches(int w, Connector* C, char dir,  std::vector<PositionConnector*>& matches);

  // A simpler function: Can any connector in this word match a connector wi, pi?
  // It is assumed that
  bool match_possible(int wi, int pi)
  {
    return _match_possible[wi].find(pi) != _match_possible[wi].end();
  }

private:
  int verbosity;
  const char *debug;
  const char *test;
};

#endif
