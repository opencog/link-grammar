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

#ifndef _MSC_VER
typedef long long s64; /* signed 64-bit integer, even on 32-bit cpus */
#define PARSE_NUM_OVERFLOW (1LL<<24)
#else
/* Microsoft Visual C Version 6 doesn't support long long. */
typedef signed __int64 s64; /* signed 64-bit integer, even on 32-bit cpus */
#define PARSE_NUM_OVERFLOW (((s64)1)<<24)
#endif


/* A histogram distribution of the parse counts. */
#define NUM_BINS 12
struct Count_bin_s
{
	s64 total;
	s64 bin[NUM_BINS];
};

typedef struct Count_bin_s Count_bin;

Count_bin hist_zero(void);
Count_bin hist_one(void);
Count_bin hist_bad(void);

void hist_accum(Count_bin* sum, double, const Count_bin*);
void hist_accumv(Count_bin* sum, double, const Count_bin);
void hist_sum(Count_bin* sum, const Count_bin*, const Count_bin*);
void hist_prod(Count_bin* prod, const Count_bin*, const Count_bin*);
void hist_muladd(Count_bin* prod, const Count_bin*, double, const Count_bin*);
void hist_muladdv(Count_bin* prod, const Count_bin*, double, const Count_bin);

#endif /* _HISTOGRAM_H_ */
