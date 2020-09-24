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

#include "link-includes.h"
#include "utilities.h"                  // GNUC_MALLOC (XXX separate include?)

#define D_MEMPOOL (D_SPEC+4)
#define MIN_ALIGNMENT sizeof(void *)    // Minimum element alignment.
#define MAX_ALIGNMENT 64                // Maximum element alignment.
//#define POOL_FREE                       // Allow to reuse individual elements.
/*#define POOL_EXACT // Not used for now and hence left undefined. */

typedef struct Pool_desc_s Pool_desc;

/* See below the definition of pool_new(). */
Pool_desc *pool_new(const char *, const char *, size_t, size_t, bool, bool, bool);
void *pool_alloc(Pool_desc *) GNUC_MALLOC;
void pool_reuse(Pool_desc *);
#ifndef DEBUG
void pool_delete(Pool_desc *);
#else
void pool_delete(const char *func, Pool_desc *);
#define pool_delete(...) pool_delete (__func__, __VA_ARGS__)
#endif
#ifdef POOL_FREE
void pool_free(Pool_desc *, void *e);
#endif // POOL_FREE

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
	char *ring;                 // Current area for allocation.
	char *alloc_next;           // Next element to be allocated.
#ifdef POOL_FREE
	char *free_list;            // Allocations that got freed.
#endif // POOL_FREE
	size_t block_size;          // Block size for pool extension.
	size_t data_size;           // Size of data inside block_size.
	size_t alignment;           // Alignment of element allocation.

	/* Common to the real and fake pool allocators. */
	char *chain;                // Allocated blocks.
	size_t element_size;        // Allocated memory per element.
	const char *name;           // Pool name.
	const char *func;           // Invoker of pool_new().

	/* For debug and stats. */
	size_t num_elements;
	size_t curr_elements;

	/* Flags that are used by pool_alloc(). */
	bool zero_out;              // Zero out allocated elements.
#ifdef POOL_EXACT
	bool exact;                 // Abort if more than num_elements are needed.
#endif /* POOL_EXACT */
};

// Macros for our memory-pool usage debugging.
// https://github.com/google/sanitizers/wiki/AddressSanitizerManualPoisoning
#if !defined(__has_feature)
#define __has_feature(x) 0
#endif
#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#include <sanitizer/asan_interface.h>
#define ASAN_POISON_MEMORY_REGION(addr, size) \
	__asan_poison_memory_region((addr), (size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) \
	__asan_unpoison_memory_region((addr), (size))
#define ASAN_ADDRESS_IS_POISONED(addr) \
	__asan_address_is_poisoned(addr)
#else
#define ASAN_POISON_MEMORY_REGION(addr, size) \
	((void)(addr), (void)(size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) \
	((void)(addr), (void)(size))
#define ASAN_ADDRESS_IS_POISONED(addr) \
	((void)(addr), false)
#endif // __SANITIZE_ADDRESS__

#endif // _MEMORY_POOL_H
