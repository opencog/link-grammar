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
		void stream_word_conset(Link*);

		Link* find_matches(Atom*, Atom*);

		Link* word_consets(const std::string& word);

	protected:
		void initialize_state();
		Link* get_state_set();

		Atom* lg_exp_to_atom(Exp*);
		static bool conn_match(const std::string&, const std::string&);
		static std::string conn_merge(const std::string&, const std::string&);
		static Link* conn_connect(Node*, Node*);
		static Link* conn_connect(Atom*, Atom*);

		Dictionary _dict;
	private:
		Link *_state;
};


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_PARSER_H
