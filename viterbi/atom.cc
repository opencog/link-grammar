/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
/*                                                                       */
/*************************************************************************/

#include <iostream>
#include "atom.h"

namespace link_grammar {
namespace viterbi {


std::ostream& operator<<(std::ostream& out, const Atom*)
{
	out <<"whazzup";
	return out;
}


} // namespace viterbi
} // namespace link-grammar
