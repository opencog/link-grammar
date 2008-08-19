#include "util.hpp"

bool isEndingInterpunction(const char* str) {
  return strcmp(str, ".") == 0 ||
    strcmp(str, "?") == 0 ||
    strcmp(str, "!") == 0;
}

bool isConnective(Sentence sent, int w) {
  return sent->is_conjunction[w];
}

bool isComma(Sentence sent, int w) {
  return strcmp(word(sent, w), ",") == 0;
}

bool isConnectiveOrComma(Sentence sent, int w) {
  return isConnective(sent, w) || isComma(sent, w);
}

const char* word(Sentence sent, int w) {
  return sent->word[w].string;
}
