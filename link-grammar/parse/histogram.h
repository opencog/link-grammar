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

#ifndef _HISTOGRAM_H_
#define _HISTOGRAM_H_

#include <stdint.h>
#include <inttypes.h>                   // Format of count_t

#define PARSE_NUM_OVERFLOW (1<<24)  // We always assume sizeof(int)>=4

typedef int32_t count_t;
typedef int64_t w_count_t;          // For overflow detection

/*
 * Count Histogramming is currently not required for anything, and the
 * code runs about 6% faster when it is disabled.
 */
#define PERFORM_COUNT_HISTOGRAMMING 0

#if PERFORM_COUNT_HISTOGRAMMING

#define COUNT_FMT PRId64

/**
 * A histogram distribution of the parse counts.
 * The histogram is with respect to the cost of the parse.  Thus, each
 * bin of the histogram contains a count of the number of parses
 * achievable with that cost.  Rather than setting the baseline cost
 * at zero, it is dynamically scaled, so that 'base' is the number of
 * the first bin with a non-zero count in it.  If there are counts that
 * don't fit into the available bins, then they are accumulated into
 * the overrun bin.  It is always the case that
 *     total == sum_i bin[i] + overrun
 */
#define NUM_BINS 12
typedef struct
{
	short base;
	w_count_t total;
	w_count_t bin[NUM_BINS];
	w_count_t overrun;
} Count_bin;

typedef Count_bin w_Count_bin;

Count_bin hist_zero(void);
Count_bin hist_one(void);

void hist_accum(Count_bin* sum, double, const Count_bin*);
void hist_accumv(Count_bin* sum, double, const Count_bin);
void hist_prod(Count_bin* prod, const Count_bin*, const Count_bin*);
void hist_muladd(Count_bin* prod, const Count_bin*, double, const Count_bin*);
void hist_muladdv(Count_bin* prod, const Count_bin*, double, const Count_bin);

static inline w_count_t hist_total(Count_bin* tot) { return tot->total; }
w_count_t hist_cut_total(Count_bin* tot, count_t min_total);

double hist_cost_cutoff(Count_bin*, count_t count);

#else

#define COUNT_FMT PRId32

typedef count_t Count_bin;
typedef w_count_t w_Count_bin;

static inline count_t hist_zero(void) { return 0; }
static inline count_t hist_one(void) { return 1; }

#define hist_accum(sum, cost, a) (*(sum) += *(a))
#define hist_accumv(sum, cost, a) (*(sum) += (a))
#define hist_prod(prod, a, b) (*(prod) = (*a) * (*b))
#define hist_muladd(prod, a, cost, b) (*(prod) += (*a) * (*b))
#define hist_muladdv(prod, a, cost, b) (*(prod) += (*a) * (b))
#define hist_total(tot) (*tot)

#define hist_cut_total(tot, min_total) (*tot)
static inline double hist_cost_cutoff(count_t* tot, count_t count) { return 1.0e38; }

#endif /* PERFORM_COUNT_HISTOGRAMMING */

#endif /* _HISTOGRAM_H_ */
