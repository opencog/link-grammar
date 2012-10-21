/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
/*                                                                       */
/*************************************************************************/

#ifndef _LG_VITERBI_STATE_H
#define _LG_VITERBI_STATE_H

#include <string>

#include "atom.h"
#include "compile.h"

namespace link_grammar {
namespace viterbi {

class State
{
	public:
		State(Set *);

		void stream_word_conset(WordCset*);

		Set* get_alternatives();

	protected:
		void set_clean_state(Set*);

	private:
		Set* _alternatives;
};


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_STATE_H
