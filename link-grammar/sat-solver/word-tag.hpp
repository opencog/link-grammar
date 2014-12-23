#ifndef __WORD_TAG_HPP__
#define __WORD_TAG_HPP__

#include <vector>
#include <map>
#include <set>

#include "link-includes.h"

extern "C" {
#include "count.h"
#include "prune.h"
#include "word-utils.h"
};

#include "variables.hpp"


struct PositionConnector
{
  PositionConnector(Exp* e, Connector* c, char d, int w, int p, 
                    double cst, double pcst, bool lr, bool ll,
                    const std::vector<int>& er, const std::vector<int>& el)
    : exp(e), connector(c), dir(d), word(w), position(p),
      cost(cst), parent_cost(pcst),
      leading_right(lr), leading_left(ll),
      eps_right(er), eps_left(el)
  {
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

  // Original expression that this connector came from
  Exp* exp;

  // Connector itself
  Connector* connector;
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

  // Matches with other words
  std::vector<PositionConnector*> matches;

};


// XXX TODO: Hash connectors for faster matching

class WordTag
{
private:
  std::vector<PositionConnector> _left_connectors;
  std::vector<PositionConnector> _right_connectors;

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
  }

  const std::vector<PositionConnector>& get_left_connectors() const {
    return _left_connectors;
  }

  const std::vector<PositionConnector>& get_right_connectors() const {
    return _right_connectors;
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

  void set_connector_length_limit(Connector* c)
  {
    int short_len = _opts->short_length;
    if (short_len > UNLIMITED_LEN)
      short_len = UNLIMITED_LEN;

    Connector_set *conset = _sent->dict->unlimited_connector_set;
    if (parse_options_get_all_short_connectors(_opts)) {
      c->length_limit = short_len;
    }
    else if (conset == NULL || match_in_connector_set(conset, c, '+')) {
      c->length_limit = UNLIMITED_LEN;
    } else {
      c->length_limit = short_len;
    }
  }

  bool match(int w1, Connector& cntr1, char dir, int w2, Connector& cntr2)
  {
      int dist = w2 - w1;
      assert(0 < dist, "match() did not receive words in the natural order.");
      if (dist > cntr1.length_limit || dist > cntr2.length_limit) return false;
      return easy_match(cntr1.string, cntr2.string);
  }

  void insert_connectors(Exp* exp, int& dfs_position,
                         bool& leading_right, bool& leading_left,
                         std::vector<int>& eps_right,
                         std::vector<int>& eps_left,
                         char* var, bool root, double parent_cost,
                         Exp* parent);

  // Caches information about the found matches to the _matches vector, and also
  // updates the _matches vector of all connectors in the given tag.
  // In order to have all possible matches correctly cached, the function assumes that it is
  // iteratively called for all words in the sentence, where the tag is on the right side of
  // this word
  void add_matches_with_word(WordTag& tag);

  // Find matches in this word tag with the connector (name, dir).
  void find_matches(int w, const char* C, char dir,  std::vector<PositionConnector*>& matches);

  // A simpler function: Can any connector in this word match a connector wi, pi?
  // It is assumed that
  bool match_possible(int wi, int pi)
  {
    return _match_possible[wi].find(pi) != _match_possible[wi].end();
  }

};

#endif
