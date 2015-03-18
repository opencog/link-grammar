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
	static Count_bin zero = {0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0};
	return zero;
}

Count_bin hist_one(void)
{
	static Count_bin one = {1, {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0};
	return one;
}

Count_bin hist_bad(void)
{
	static Count_bin bad = {-1};
	return bad;
}

#define BIN_WIDTH 0.334

void hist_accum(Count_bin* sum, double cost, const Count_bin* a)
{
	unsigned int i;
	unsigned int start;

	// Skip, if nothing to accumulate.
	if (0 == a->total) return;

	start = (unsigned int) floor (cost / BIN_WIDTH);
	sum->total += a->total;
	for (i = start; i < NUM_BINS; i++)
	{
		sum->bin[i] += a->bin[i-start];
	}
	for (i = NUM_BINS-start; i < NUM_BINS; i++)
	{
		sum->overrun += a->bin[i];
	}
	sum->overrun += a->overrun;
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
	unsigned int i, j;
	prod->total = a->total * b->total;

	for (i = 0; i < NUM_BINS; i++) prod->bin[i] = 0;
	for (i = 0; i < NUM_BINS; i++)
	{
		for (j = 0; j < NUM_BINS; j++)
		{
			if (i+j < NUM_BINS)
				prod->bin[i+j] += a->bin[i] * b->bin[j];
			else
				prod->overrun += a->bin[i] * b->bin[j];
		}
	}
}

void hist_muladd(Count_bin* acc, const Count_bin* a, double cost, const Count_bin* b)
{

	{
		Count_bin tmp;
		hist_prod(&tmp, a, b);
		hist_accum(acc, cost, &tmp);
	}
#if 0
	unsigned int i;
	unsigned int start = (unsigned int) floor (cost / BIN_WIDTH);
	acc->total += a->total * b->total;
	for (i = 0; i < NUM_BINS-start; i++)
	{
		s64 subtot = 0;
		for (j=0; j<start; j++)
		{
			subtot += a->bin[i + j] * b->bin[i + j - start];
		}
printf("duude muladd %d %llu %llu\n", i, acc->bin[i], a->bin[i-start]);
		acc->bin[i+start] += subtot;
	}
#endif
}

void hist_muladdv(Count_bin* acc, const Count_bin* a, double cost, const Count_bin b)
{
	hist_muladd(acc, a, cost, &b);
}
