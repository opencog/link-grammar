#include "util.hpp"

extern "C" {
#include "api-structures.h"
#include "structures.h"
};

bool isEndingInterpunction(const char* str)
{
  return strcmp(str, ".") == 0 ||
    strcmp(str, "?") == 0 ||
    strcmp(str, "!") == 0;
}

const char* word(Sentence sent, int w)
{
  // XXX FIXME this is fundamentally wrong, should explore all alternatives!
  return sent->word[w].alternatives[0];
}
