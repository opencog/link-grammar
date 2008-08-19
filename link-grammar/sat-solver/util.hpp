#ifndef __UTIL_HPP__
#define __UTIL_HPP__

extern "C" {
#include <link-grammar/api.h>
}

bool isEndingInterpunction(const char* str);
bool isConnective(Sentence sent, int w);
bool isComma(Sentence sent, int w);
bool isConnectiveOrComma(Sentence sent, int w);
const char* word(Sentence sent, int w);

#endif
