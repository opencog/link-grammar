/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
/*                                                                       */
/*************************************************************************/

#ifndef _LG_VITERBI_CONNECT_H
#define _LG_VITERBI_CONNECT_H

#include "atom.h"
#include "compile.h"

namespace link_grammar {
namespace viterbi {

class Connect
{
	public:
		Connect(WordCset*);
		Link* try_connect(WordCset*);

	protected:
		Link* reassemble(Link*, WordCset*, WordCset*);

		Link* conn_connect_ab(WordCset*, Atom*, Atom*);
		Link* conn_connect_a(WordCset*, Atom*, Node*);
		Link* conn_connect_a(WordCset*, Atom*, Link*);
		Link* conn_connect_b(WordCset*, Node*, Atom*);

		Link* conn_connect_nn(WordCset*, Node*, Node*);
		Link* conn_connect_kn(WordCset*, Link*, Node*);
		Link* conn_connect_nk(WordCset*, Node*, Link*);
		Link* conn_connect_kk(WordCset*, Link*, Link*);

		static bool is_optional(Atom *);

	private:
		WordCset* _right_cset;
		Atom* _rcons;
};


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_CONNECT_H
