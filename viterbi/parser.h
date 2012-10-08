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

		Set* get_state();
		Set* get_output_set();

	protected:
		void initialize_state();
		Atom* lg_exp_to_atom(Exp*);

		void set_state(Set*);

		static WordCset* cset_trim_left_pointers(WordCset*);
		static Atom* trim_left_pointers(Atom*);

		static bool cset_has_mandatory_left_pointer(WordCset*);
		static bool has_mandatory_left_pointer(Atom*);

		Dictionary _dict;
	private:
		Set* _state;
		Set* _output;
};


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_PARSER_H
