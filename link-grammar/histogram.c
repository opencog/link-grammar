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

#include <math.h>
#include "histogram.h"

/* A histogram distribution of the parse counts. */

Count_bin hist_zero(void)
{
	static Count_bin zero = {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
	return zero;
}

Count_bin hist_one(void)
{
	static Count_bin one = {1, {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
	return one;
}

Count_bin hist_bad(void)
{
	static Count_bin bad = {-1};
	return bad;
}

#define BIN_BASE  (-1.0)
#define BIN_WIDTH 0.333

void hist_accum(Count_bin* sum, double cost, const Count_bin* a)
{
	unsigned int i;
	unsigned int start = (int) floor ((cost - BIN_BASE)/BIN_WIDTH);
	sum->total += a->total;
	for (i = start; i < NUM_BINS; i++)
	{
		sum->bin[i] += a->bin[i-start];
	}
}

void hist_accumv(Count_bin* sum, double cost, const Count_bin a)
{
	hist_accum(sum, cost, &a);
}

void hist_sum(Count_bin* sum, const Count_bin* a, const Count_bin* b)
{
	sum->total = a->total + b->total;
}

void hist_prod(Count_bin* prod, const Count_bin* a, const Count_bin* b)
{
	unsigned int i;
	prod->total = a->total * b->total;

	for (i = 0; i < NUM_BINS; i++)
	{
		prod->bin[i] = a->bin[i] * b->bin[i];
	}
}

void hist_muladd(Count_bin* acc, const Count_bin* a, double cost, const Count_bin* b)
{
	unsigned int i;
	unsigned int start = (int) floor ((cost - BIN_BASE)/BIN_WIDTH);

	acc->total += a->total * b->total;
	for (i = start; i < NUM_BINS; i++)
	{
		acc->bin[i] += a->bin[i] * b->bin[i-start];
	}
}

void hist_muladdv(Count_bin* acc, const Count_bin* a, double cost, const Count_bin b)
{
	hist_muladd(acc, a, cost, &b);
}
