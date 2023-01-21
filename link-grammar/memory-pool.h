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

#include <stddef.h>                     // max_align_t

#if HAVE_THREADS_H
#include <threads.h>                    // mtx_t
#endif

#include "link-includes.h"
#include "error.h"
#include "utilities.h"                  // GNUC_MALLOC (XXX separate include?)

#ifndef D_MEMPOOL                       // Allow redefining for debug.
#define D_MEMPOOL (D_SPEC+4)
#endif
#define MIN_ALIGNMENT sizeof(void *)    // Minimum element alignment.
#define MAX_ALIGNMENT 64                // Maximum element alignment.
//#define POOL_FREE                       // Allow to reuse individual elements.
/*#define POOL_EXACT // Not used for now and hence left undefined. */

typedef struct Pool_desc_s Pool_desc;

Pool_desc *pool_new(const char *, const char *, size_t, size_t, bool, bool, bool);
void *pool_alloc_vec(Pool_desc *, size_t) GNUC_MALLOC;

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
 * If configured with "CPPFLAGS=-DPOOL_ALLOCATOR=0", a fake pool allocator
 * that uses malloc() for each allocation is defined, in order that ASAN
 * or valgrind can be used to find memory usage bugs.
 */

#ifndef POOL_ALLOCATOR
#define POOL_ALLOCATOR 1
#endif

#if !POOL_ALLOCATOR
typedef union
{
	struct{
		char *next;     /* Next allocation. */
		size_t size;    /* Allocation payload size. */
	};
	max_align_t dummy; /* Align the payload properly for all payload sizes. */
} alloc_attr;
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
	size_t num_elements;        // Number of elements per block.

	/* Common to the real and fake pool allocators. */
	char *chain;                // Allocated blocks.
	size_t element_size;        // Allocated memory per element.
	const char *name;           // Pool name.
	const char *func;           // Invoker of pool_new().
	/* num_elements is also used by the fake allocator if the POOL_EXACT
	 * feature is used (it is not used for now). */

	size_t curr_elements;       // Originally for debug. Now used by pool_next().

	/* Flags that are used by pool_alloc(). */
	bool zero_out;              // Zero out allocated elements.
#ifdef POOL_EXACT
	bool exact;                 // Abort if more than num_elements are needed.
#endif /* POOL_EXACT */

#if HAVE_THREADS_H
	mtx_t mutex;                // Optional pool thread-safety lock.
#endif
};

typedef struct
{
	char *current_element;
	char *block_end;
	size_t element_number;
} Pool_location;

/**
 * Return the number of allocated elements in the given pool.
 */
static inline size_t pool_num_elements_alloced(Pool_desc *mp)
{
	return mp->curr_elements;
}

// Lock-free version, when every thread has it's own pool.
static inline void *pool_alloc(Pool_desc *mp)
{
	return pool_alloc_vec(mp, 1);
}

// Locking version, when multiple threads share a single pool.
static inline void *pool_alloc_concurrent(Pool_desc *mp)
{
#if HAVE_THREADS_H
	mtx_lock(&mp->mutex);
	void* rv = pool_alloc_vec(mp, 1);
	mtx_unlock(&mp->mutex);
	return rv;
#else
	return pool_alloc_vec(mp, 1);
#endif
}

/**
 * Return the next element in the pool, starting with the first one.
 * @param l Iteration state. \c *l should be initialized to
 * (Pool_location)0 before starting the iteration.
 * @return The next element on each call, \c NULL when there are no
 * more.
 *
 * FIXME: When used on a pool with vector allocations, it may return
 * unallocated elements at the end of pool blocks. So it shouldn't be
 * used on pools with vector allocations (unless this is for stats only
 * and skewed results are tolerable, as currently done in
 * free_table_lrcnt()).
 */
static inline void *pool_next(Pool_desc *mp, Pool_location *l)
{
#ifdef POOL_FREE
	assert(mp->free_list == NULL, "Cannot be called after pool_free()");
#endif

	if (l->element_number == mp->curr_elements) return NULL;

	if (l->element_number == 0)
	{
		/* This is an initial request for pool traversal.
		 * Initialize the location descriptor and return the first element. */
		l->element_number = 1;
#if POOL_ALLOCATOR
		l->current_element = mp->chain;
		l->block_end = mp->chain + mp->data_size;
#else
		l->current_element = mp->chain + sizeof(alloc_attr);
#endif

		return l->current_element;
	}

#if POOL_ALLOCATOR
	l->current_element += mp->element_size;
	if (l->current_element == l->block_end)
	{
		l->current_element = *(char **)l->block_end;
		dassert(l->current_element != NULL, "Truncated memory pool");
		l->block_end = l->current_element + mp->data_size;
	}
#else
	alloc_attr *at = (alloc_attr *)(l->current_element - sizeof(alloc_attr));
	l->current_element = at->next + sizeof(alloc_attr);
#endif

	l->element_number++;
	return l->current_element;
}

#if 0 /* For planned memory management. */
static size_t pool_size(Pool_desc *mp)
{
	return mp->current_elements;
}
#endif

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
