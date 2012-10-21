/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
/*                                                                       */
/*************************************************************************/

#ifndef _LG_VITERBI_PARSER_H
#define _LG_VITERBI_PARSER_H

#include <string>

#include "atom.h"
#include "compile.h"

// link-grammar include files, needed for Exp, Dict
#include "api-types.h"
#include "structures.h"

namespace link_grammar {
namespace viterbi {

class Parser
{
	public:
		Parser(Dictionary dict);

		void streamin(const std::string&);
		void stream_word(const std::string&);
		void stream_word_conset(WordCset*);

		Set* word_consets(const std::string& word);

		Set* get_alternatives();
	protected:
		void initialize_state();
		Atom* lg_exp_to_atom(Exp*);

		Dictionary _dict;
	private:
		Set* _alternatives;
};


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_PARSER_H
