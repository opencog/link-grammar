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

void hist_accum(Count_bin* sum, Count_bin* a)
{
	sum->total += a->total;
}

void hist_accumv(Count_bin* sum, const Count_bin a)
{
	sum->total += a.total;
}

void hist_sum(Count_bin* sum, Count_bin* a, Count_bin* b)
{
	sum->total = a->total + b->total;
}

void hist_prod(Count_bin* prod, Count_bin* a, Count_bin* b)
{
	prod->total = a->total * b->total;
}

void hist_muladd(Count_bin* acc, Count_bin* a, Count_bin* b)
{
	acc->total += a->total * b->total;
}

void hist_muladdv(Count_bin* acc, Count_bin* a, const Count_bin b)
{
	acc->total += a->total * b.total;
}
