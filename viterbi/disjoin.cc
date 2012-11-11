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

#include "atom.h"

namespace link_grammar {
namespace viterbi {

/**
 * Convert dictionary-normal form into disjunctive normal form.
 * That is, convert the mixed-form dictionary entries into a disjunction
 * of a list of conjoined connectors.  The goal of this conversion is to
 * simplify the parsing algorithm.
 */

Atom *disjoin(Atom *dict_form)
{
	AtomType intype = dict_form->getType();
	if (CONNECTOR == intype)
		return dict_form;


	return NULL;
}


} // namespace viterbi
} // namespace link-grammar

