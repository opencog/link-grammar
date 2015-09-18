#ifndef __UTIL_HPP__
#define __UTIL_HPP__

extern "C" {
#include "link-includes.h"
#include "api-structures.h"
#include "structures.h"
#include "disjunct-utils.h"
}

bool isEndingInterpunction(const char* str);
const char* word(Sentence sent, int w);
void free_linkage(Linkage);
Exp* null_exp();
void add_anded_exp(Exp*&, Exp*);

#endif
