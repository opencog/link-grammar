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

Or* Or::flatten() const
{
	size_t sz = _oset.size();
	OutList newset;
	for (int i=0; i<sz; i++)
	{
		Or* ora = dynamic_cast<Or*>(_oset[i]);
		if (!ora)
		{
			/* Copy without change */
			newset.push_back(_oset[i]);
			continue;
		}

		/* Get rid of a level */
		ora = ora->flatten();
		size_t osz = ora->get_arity();
		for (int j=0; j<osz; j++)
			newset.push_back(ora->_oset[j]);
	}

	return new Or(newset);
}

} // namespace viterbi
} // namespace link-grammar
