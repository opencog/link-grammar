/*************************************************************************/
/* Copyright (c) 2018 Amir Plivatsky                                     */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _MEMORY_POOL_H
#define _MEMORY_POOL_H

#include "link-grammar/link-includes.h"
#include "link-grammar/utilities.h"     // GNUC_MALLOC (XXX separate include?)

#define D_MEMPOOL (D_SPEC+4)
#define MIN_ALIGNMENT sizeof(void *)    // Minimum element alignment.
#define MAX_ALIGNMENT 64                // Maximum element alignment.

typedef struct Pool_desc_s Pool_desc;

/* See below the definition of pool_new(). */
Pool_desc *pool_new(const char *, const char *, size_t, size_t, bool, bool, bool);
void *pool_alloc(Pool_desc *) GNUC_MALLOC;
void pool_reuse(Pool_desc *);
void pool_delete(Pool_desc *);

/* Pool allocator debug facility:
 * If configured with "CFLAGS=-DPOOL_ALLOCATOR=0", a fake pool allocator
 * that uses malloc() for each allocation is defined, in order that ASAN
 * or valgrind can be used to find memory usage bugs.
 */

#ifndef POOL_ALLOCATOR
#define POOL_ALLOCATOR 1
#endif

#define FLDSIZE_NEXT sizeof(char *) // "next block" field size
#define POOL_NEXT_BLOCK(blk, offset_next) (*(char **)((blk)+(offset_next)))

struct  Pool_desc_s
{
	/* Used only by the real pool allocator. */
	char *chain;                // Allocated blocks. */
	char *ring;                 // Current area for allocation.
	char *alloc_next;           // Next element to be allocated.
	size_t block_size;          // Block size for pool extension.
	size_t data_size;           // Size of data inside block_size.
	size_t alignment;           // Alignment of element allocation.

	/* Common to the real and fake pool allocators. */
	size_t element_size;        // Allocated memory per element.
	const char *name;           // Pool name.
	const char *func;           // Invoker of pool_new().

	/* For debug and stats. */
	size_t num_elements;
	size_t curr_elements;

	/* Flags that are used by pool_alloc(). */
	bool zero_out;              // Zero out allocated elements.
	bool exact;                 // Abort if more than num_elements are needed.
};
#endif // _MEMORY_POOL_H
