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
		Set* try_connect(WordCset*);

	protected:
		Set* reassemble(Set*, WordCset*, WordCset*);
		Ling* reassemble(Ling*, WordCset*, WordCset*);

		Link* conn_connect_aa(WordCset*, Atom*, Atom*);
		Link* conn_connect_na(WordCset*, Node*, Atom*);
		Set* conn_connect_ka(WordCset*, Link*, Atom*);

		Ling* conn_connect_nn(WordCset*, Node*, Node*);
		Link* conn_connect_nk(WordCset*, Node*, Link*);

		static const OutList& flatten(OutList&);

	private:
		WordCset* _right_cset;
		Atom* _rcons;
};


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_CONNECT_H
