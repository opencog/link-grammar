/*************************************************************************/
/* Copyright (c) 2015 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "histogram.h"

/* A histogram distribution of the parse counts. */

void hist_sum(Count_bin* sum, Count_bin* a, Count_bin* b)
{
	sum->count = a->count + b->count;
}

void hist_prod(Count_bin* prod, Count_bin* a, Count_bin* b)
{
	prod->count = a->count * b->count;
}
