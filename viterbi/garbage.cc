/*************************************************************************/
/* Copyright (c) 2013 Linas Vepstas <linasvepstas@gmail.com>             */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the Viterbi parsing system is subject to the terms of the      */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <gc/gc.h>
#include "garbage.h"

using namespace std;

namespace atombase {

void lg_init_gc()
{
	static bool is_inited = false;
	if (is_inited)  // not thread safe.
		return;
	is_inited = true;

	GC_init();

	/* Max heap size of a quarter-gig. */
	GC_set_max_heap_size(256*1024*1024);

}


} // namespace atombase
