/*************************************************************************/
/* Copyright (c) 2013 Linas Vepstas <linasvepstas@gmail.com>             */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the Viterbi parsing system is subject to the terms of the      */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "compress.h"

namespace link_grammar {
namespace viterbi {

/// Try to shrink down a set of alternatives by collapsing repeated
/// states into disjuncts.  Best explained by example:  Suppose that
/// the input is 
/// SET :
///   STATE_PAIR :
///     SEQ :
///       WORD_CSET :
///         WORD : this
///         CONNECTOR : Ss*b+
///       WORD_CSET :
///         WORD : LEFT-WALL
///         CONNECTOR : Wd+
///     SEQ :
///   STATE_PAIR :
///     SEQ :
///       WORD_CSET :
///         WORD : this
///         CONNECTOR : Ss*b+
///       WORD_CSET :
///         WORD : LEFT-WALL
///         CONNECTOR : Wi+
///     SEQ :
/// 
/// Then the compressed output will be:
/// SET :
///   STATE_PAIR :
///     SEQ :
///       WORD_CSET :
///         WORD : this
///         CONNECTOR : Ss*b+
///       WORD_CSET :
///         WORD : LEFT-WALL
///         OR :
///           CONNECTOR : Wd+
///           CONNECTOR : Wi+
///     SEQ :
///
/// Note how two alternative state pairs were collapsed down into a
/// single word cset.  The goal of this compression is to shrink down
/// the state to make the output more tractable, slightly less
/// combinatoric-explosiony.
//

Set* compress_alternatives(Set* state_alternatives)
{
	Seq* prev_out = NULL;

	size_t sz = state_alternatives->get_arity();
	for (size_t i = 0; i < sz; i++)
	{
		Atom* a = state_alternatives->get_outgoing_atom(i);
		StatePair* sp = dynamic_cast<StatePair*>(a);
		Seq* out = sp->get_output();
		if (out != prev_out)
		{
			prev_out = out;
			continue;
		}
	}
}

} // namespace viterbi
} // namespace link-grammar

