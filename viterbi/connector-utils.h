/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
/*                                                                       */
/*************************************************************************/

#ifndef _LG_CONNECTOR_UTILS_H
#define _LG_CONNECTOR_UTILS_H

#include <string>

#include "atom.h"
#include "compile.h"

namespace link_grammar {
namespace viterbi {

bool conn_match(const std::string&, const std::string&);
std::string conn_merge(const std::string&, const std::string&);
bool is_optional(Atom *);

WordCset* cset_trim_left_pointers(WordCset*);

} // namespace viterbi
} // namespace link-grammar

#endif // _LG_CONNECTOR_UTILS_H
