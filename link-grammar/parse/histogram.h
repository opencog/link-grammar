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

typedef int64_t w_count_t;          // For overflow detection

/*
 * Count Histogramming is currently not required for anything, and the
 * code runs about 6% faster when it is disabled.
 *
#define PERFORM_COUNT_HISTOGRAMMING 1
 */
#ifdef PERFORM_COUNT_HISTOGRAMMING

typedef int64_t count_t;
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
struct count_t_s
{
	short base;
	count_t total;
	count_t bin[NUM_BINS];
	count_t overrun;
};

typedef struct count_t_s count_t;

count_t hist_zero(void);
count_t hist_one(void);

void hist_accum(count_t* sum, double, const count_t*);
void hist_accumv(count_t* sum, double, const count_t);
void hist_prod(count_t* prod, const count_t*, const count_t*);
void hist_muladd(count_t* prod, const count_t*, double, const count_t*);
void hist_muladdv(count_t* prod, const count_t*, double, const count_t);

static inline count_t hist_total(count_t* tot) { return tot->total; }
count_t hist_cut_total(count_t* tot, int min_total);

double hist_cost_cutoff(count_t*, int count);

#else

typedef int32_t count_t;
#define COUNT_FMT PRId32

typedef count_t count_t;

static inline count_t hist_zero(void) { return 0; }
static inline count_t hist_one(void) { return 1; }

#define hist_accum(sum, cost, a) (*(sum) += *(a))
#define hist_accumv(sum, cost, a) (*(sum) += (a))
#define hist_prod(prod, a, b) (*(prod) = (*a) * (*b))
#define hist_muladd(prod, a, cost, b) (*(prod) += (*a) * (*b))
#define hist_muladdv(prod, a, cost, b) (*(prod) += (*a) * (b))
#define hist_total(tot) (*tot)

#define hist_cut_total(tot, min_total) (*tot)
static inline double hist_cost_cutoff(count_t* tot, int count) { return 1.0e38; }

#endif /* PERFORM_COUNT_HISTOGRAMMING */

#endif /* _HISTOGRAM_H_ */
