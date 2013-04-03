#include "util.hpp"

bool isEndingInterpunction(const char* str) {
  return strcmp(str, ".") == 0 ||
    strcmp(str, "?") == 0 ||
    strcmp(str, "!") == 0;
}

#ifdef USE_FAT_LINKAGES
bool isConnective(Sentence sent, int w) {
  return sent->is_conjunction[w];
}

bool isComma(Sentence sent, int w) {
  return strcmp(word(sent, w), ",") == 0;
}

bool isConnectiveOrComma(Sentence sent, int w) {
  return isConnective(sent, w) || isComma(sent, w);
}
#endif /* USE_FAT_LINKAGES */

const char* word(Sentence sent, int w) {
  // XXX FIXME this is fundamentally wrong, should explore all alternatives!
  return sent->word[w].alternatives[0];
}
