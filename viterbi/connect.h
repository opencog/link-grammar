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
		Set* try_connect(StatePair*);

	protected:
		Set* try_connect(Seq*);
		Set* next_connect(WordCset*);

		Set* reassemble(Set*, WordCset*, WordCset*);
		Ling* reassemble(Ling*, WordCset*, WordCset*);

		Link* conn_connect_aa(Atom*, Atom*);
		Link* conn_connect_na(Connector*, Atom*);
		Set* conn_connect_ka(Link*, Atom*);

		Ling* conn_connect_nn(Connector*, Connector*);
		Link* conn_connect_nk(Connector*, Link*);

		static const OutList& flatten(OutList&);

	private:
		Seq* _left_sequence;
		WordCset* _right_cset;
		Atom* _rcons;
};


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_CONNECT_H
