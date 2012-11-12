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

#include "compile.h"

namespace link_grammar {
namespace viterbi {


/// Flatten a set.  That is, remove the nested, recursive
/// structure of the set, and return all elements as just
/// one single set. For example, the flattened version of
/// {a, {b,c}} is {a, b, c}
///
// Note that this algo, as currently implemented, is order-preserving.
// This is important for the Seq class, and the And class (since the
// link-grammar AND must be an ordered sequence, to preserve planarity
// of the parses.
OutList Set::flatset() const
{
	size_t sz = _oset.size();
	OutList newset;
	for (int i=0; i<sz; i++)
	{
		/* Copy without change, if types differ. */
		if (_type != _oset[i]->get_type())
		{
			newset.push_back(_oset[i]);
			continue;
		}

		/* Get rid of a level */
		Set* ora = dynamic_cast<Set*>(_oset[i]);
		OutList fora = ora->flatset();
		size_t osz = fora.size();
		for (int j=0; j<osz; j++)
			newset.push_back(fora[j]);
	}

	return newset;
}

} // namespace viterbi
} // namespace link-grammar
