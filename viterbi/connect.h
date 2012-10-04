/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
/*                                                                       */
/*************************************************************************/

#ifndef _LG_VITERBI_CONNECT_H
#define _LG_VITERBI_CONNECT_H

#include "atom.h"

namespace link_grammar {
namespace viterbi {

class Connect
{
	public:
		Connect(Link*);
		Link* try_connect(Link*);

	protected:
		Link* reassemble(Link*, Link*, Link*);

		Link* conn_connect_ab(Link*, Atom*, Atom*);
		Link* conn_connect_a(Link*, Atom*, Node*);
		Link* conn_connect_a(Link*, Atom*, Link*);
		Link* conn_connect_b(Link*, Node*, Atom*);

		Link* conn_connect_nn(Link*, Node*, Node*);
		Link* conn_connect_kn(Link*, Link*, Node*);
		Link* conn_connect_nk(Link*, Node*, Link*);
		Link* conn_connect_kk(Link*, Link*, Link*);

		static bool is_optional(Atom *);

	private:
		Link* _right_cset;
		Atom* _rcons;
};


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_CONNECT_H
