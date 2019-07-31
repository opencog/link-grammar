/*************************************************************************/
/* Copyright 2018 Amir Plivatsky                                         */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/
#ifndef _CONST_PRIME_H_
#define _CONST_PRIME_H_

#include <stddef.h>

/* Optimizing compilers use a multiplication for modulo constant prime.
 * The code here adds support for that. */

static const size_t s_prime[] =
{
	419, 1259, 3779, 11317, 33939, 101817, 305471, 916361, 2749067,
	8247199, 24741547, 74224603, 222673783, 668021359, 2004064039,
};
#define MAX_S_PRIMES (sizeof(s_prime) / sizeof(s_prime[0]))

#define FPNAME(n) fprime##n
#define PFUNC(p) \
	static inline unsigned int FPNAME(p)(size_t h)\
	{ return (unsigned int)h % s_prime[p]; }

PFUNC(0)
PFUNC(1)
PFUNC(2)
PFUNC(3)
PFUNC(4)
PFUNC(5)
PFUNC(6)
PFUNC(7)
PFUNC(8)
PFUNC(9)
PFUNC(10)
PFUNC(11)
PFUNC(12)
PFUNC(13)
PFUNC(14)

typedef unsigned int (*prime_mod_func_t)(size_t);

static const prime_mod_func_t prime_mod_func[] =
{
	FPNAME(0),
	FPNAME(1),
	FPNAME(2),
	FPNAME(3),
	FPNAME(4),
	FPNAME(5),
	FPNAME(6),
	FPNAME(7),
	FPNAME(8),
	FPNAME(9),
	FPNAME(10),
	FPNAME(11),
	FPNAME(12),
	FPNAME(13),
	FPNAME(14),
};
#endif /* _CONST_PRIME_H_ */
