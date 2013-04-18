/*************************************************************************/
/* Copyright (c) 2012 Linas Vepstas <linasvepstas@gmail.com>             */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the Viterbi parsing system is subject to the terms of the      */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _LG_VITERBI_STATE_H
#define _LG_VITERBI_STATE_H

#include <string>

#include "atom.h"
#include "compile.h"
#include "garbage.h"

namespace link_grammar {
namespace viterbi {

class State : public gc
{
	public:
		State(Set *);

		Set* stream_word_conset(WordCset*);

	protected:
		void set_clean_state(Set*);

	private:
		Set* _alternatives;
};


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_STATE_H
