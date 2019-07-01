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

#include <errno.h>                      // errno

#include "error.h"
#include "memory-pool.h"
#include "utilities.h"                  // MIN/MAX, aligned alloc, lg_strerror_r

/* TODO: Add valgrind descriptions. See:
 * http://valgrind.org/docs/manual/mc-manual.html#mc-manual.mempools */

/**
 * Align given size to the nearest upper power of 2
 * for size<MAX_ALIGNMENT, else to MIN_ALIGNMENT.
 */
static size_t align_size(size_t element_size)
{
	if (element_size < MAX_ALIGNMENT)
	{
		size_t s = next_power_of_two_up(element_size);
		if (s != element_size)
			element_size = ALIGN(element_size, s);
	}
	else
	{
		element_size = ALIGN(element_size, MIN_ALIGNMENT);
	}

	return element_size;
}

/* The address of the next allocated block is at the end of the block. */

/**
 * Create a memory pool descriptor.
 * 1. If required, set the allocation size to a power of 2 of the element size.
 * 2. Save the given parameters in the pool descriptor, to be used by
 *    pool_alloc();
 * 3. Chain the pool descriptor to the given pool_list, so it can be
 *    automatically freed.
 */
Pool_desc *pool_new(const char *func, const char *name,
                    size_t num_elements, size_t element_size,
                    bool zero_out, bool align, bool exact)
{
	Pool_desc *mp = malloc(sizeof(Pool_desc));

	mp->func = func;
	mp->name = name;

	if (align)
	{
		mp->element_size = align_size(element_size);
		mp->alignment = MAX(MIN_ALIGNMENT, mp->element_size);
		mp->alignment = MIN(MAX_ALIGNMENT, mp->alignment);
		mp->data_size = num_elements * mp->element_size;
		mp->block_size = ALIGN(mp->data_size + FLDSIZE_NEXT, mp->alignment);
	}
	else
	{
		mp->element_size = element_size;
		mp->alignment = MIN_ALIGNMENT;
		mp->data_size = num_elements * mp->element_size;
		mp->block_size = mp->data_size + FLDSIZE_NEXT;
	}

	mp->zero_out = zero_out;
#ifdef POOL_EXACT
	mp->exact = exact;
#endif /* POOL_EXACT */
	mp->alloc_next = NULL;
	mp->chain = NULL;
	mp->ring = NULL;
#ifdef POOL_FREE
	mp->free_list = NULL;
#endif // POOL_FREE
	mp->curr_elements = 0;
	mp->num_elements = num_elements;

	lgdebug(+D_MEMPOOL, "%sElement size %zu, alignment %zu (pool '%s' created in %s())\n",
	        POOL_ALLOCATOR?"":"(Fake pool allocator) ",
	        mp->element_size, mp->alignment, mp->name, mp->func);
	return mp;
}

/**
 * Delete the given memory pool.
 */
#undef pool_delete
#ifndef DEBUG
void pool_delete (Pool_desc *mp)
#else
void pool_delete (const char *func, Pool_desc *mp)
#endif
{
	if (NULL == mp) return;
	const char *from_func = "";
#ifdef DEBUG /* Macro-added first argument. */
	from_func = func;
#endif
	lgdebug(+D_MEMPOOL, "Used %zu (%zu) elements (%s deleted pool '%s' created in %s())\n",
	        mp->curr_elements, mp->num_elements, from_func, mp->name, mp->func);

	/* Free its chained memory blocks. */
	char *c_next;
	size_t alloc_size;
#if POOL_ALLOCATOR
	alloc_size = mp->data_size;
#else
	alloc_size = mp->element_size;
#endif
	for (char *c = mp->chain; c != NULL; c = c_next)
	{
#if !POOL_ALLOCATOR
		ASAN_UNPOISON_MEMORY_REGION(c, mp->element_size + FLDSIZE_NEXT);
#endif
		c_next = POOL_NEXT_BLOCK(c, alloc_size);
#if POOL_ALLOCATOR
		aligned_free(c);
#else
		free(c);
#endif
	}
	free(mp);
}

#if POOL_ALLOCATOR
/**
 * Allocate an element from the requested pool.
 * This function uses the feature that pointers to void and char are
 * interchangeable.
 * 1. If no current block or current block exhausted - obtain another one
 *    and chain it the block chain. Else reuse an LRU unused block;
 *    The element pointer is aligned to the required alignment.
 * 2. Zero the block if required;
 * 3. Return element pointer.
 */
void *pool_alloc(Pool_desc *mp)
{
	mp->curr_elements++; /* For stats. */

#ifdef POOL_FREE
	if (NULL != mp->free_list)
	{
		void *alloc_next = mp->free_list;
		ASAN_UNPOISON_MEMORY_REGION(alloc_next, mp->element_size);
		mp->free_list = *(char **)mp->free_list;
		if (mp->zero_out) memset(alloc_next, 0, mp->element_size);
		return alloc_next;
	}
#endif // POOL_FREE

	if ((NULL == mp->alloc_next) || (mp->alloc_next == mp->ring + mp->data_size))
	{
#ifdef POOL_EXACT
		assert(!mp->exact || (NULL == mp->alloc_next),
				 "Too many elements %zu>%zu (pool '%s' created in %s())",
				 mp->curr_elements, mp->num_elements, mp->name, mp->func);
#endif /* POOL_EXACT */

		/* No current block or current block exhausted - obtain another one. */
		char *prev = mp->ring; /* Remember current block for possible chaining. */
		if (NULL != mp->ring)
		{
			/* Next block already exists. */
			mp->ring = POOL_NEXT_BLOCK(mp->ring, mp->data_size);
		}

		if (NULL == mp->ring)
		{
			/* Allocate a new block and chain it. */
			mp->ring = aligned_alloc(mp->alignment, mp->block_size);
			if (NULL == mp->ring)
			{
				/* aligned_alloc() has strict requirements. */
				char errbuf[64];
				lg_strerror_r(errno, errbuf, sizeof(errbuf));
				assert(NULL != mp->ring, "Block/element sizes %zu/%zu: %s",
				       mp->block_size, mp->element_size, errbuf);
			}
			if (NULL == mp->alloc_next)
				mp->chain = mp->ring; /* This is the start of the chain. */
			else
				POOL_NEXT_BLOCK(prev, mp->data_size) = mp->ring;
			POOL_NEXT_BLOCK(mp->ring, mp->data_size) = NULL;
			//printf("New ring %p next %p\n", mp->ring,
			       //POOL_NEXT_BLOCK(mp->ring, mp->data_size));
		} /* Else reuse existing block. */

		if (mp->zero_out) memset(mp->ring, 0, mp->data_size);
		mp->alloc_next = mp->ring;
	}

	/* Grab a new element. */
	void *alloc_next = mp->alloc_next;
	mp->alloc_next += mp->element_size;
	return alloc_next;
}

/**
 * Reuse the given memory pool.
 * Reset the pool pointers without freeing its memory.
 * pool_alloc() will then reuse the existing pool blocks before allocating
 * new blocks.
 */
void pool_reuse(Pool_desc *mp)
{
	lgdebug(+D_MEMPOOL, "Reuse %zu elements (pool '%s' created in %s())\n",
	        mp->curr_elements, mp->name, mp->func);
	mp->ring = mp->chain;
	mp->alloc_next = mp->ring;
	mp->curr_elements = 0;
#ifdef POOL_FREE
	mp->free_list = NULL;
#endif // POOL_FREE
}

#ifdef POOL_FREE
/**
 * Allow to reuse individual elements. They are added to a free list that is
 * used by pool_alloc() before it allocates from memory blocks.
 */
void pool_free(Pool_desc *mp, void *e)
{
	assert(mp->element_size >= FLDSIZE_NEXT);
	if (NULL == e) return;
	mp->curr_elements--;

	char *next = mp->free_list;
	mp->free_list = e;
	*(char **)e = next;
	ASAN_POISON_MEMORY_REGION(e, mp->element_size);
}
#endif // POOL_FREE

#else // !POOL_ALLOCATOR

/* A dummy pool allocator - for debugging (see the comment in memory-pool.h).
 * Note: No Doxygen headers because these function replace functions with
 * the same name defined above. */

/*
 * Allocate an element by using malloc() directly.
 */
void *pool_alloc(Pool_desc *mp)
{
	mp->curr_elements++;
#ifdef POOL_EXACT
	assert(!mp->exact || mp->curr_elements <= mp->num_elements,
	       "Too many elements (%zu>%zu) (pool '%s' created in %s())",
	       mp->curr_elements, mp->num_elements, mp->name, mp->func);
#endif /* POOL_EXACT */

	/* Allocate a new element and chain it. */
	char *next = mp->chain;
	mp->chain = malloc(mp->element_size + FLDSIZE_NEXT);
	POOL_NEXT_BLOCK(mp->chain, mp->element_size) = next;

	void *alloc_next = mp->chain;
	if (mp->zero_out) memset(alloc_next, 0, mp->element_size);

	return alloc_next;
}

/*
 * Reuse the given fake memory pool by freeing its memory.
 */
void pool_reuse(Pool_desc *mp)
{
	if (NULL == mp) return;
	lgdebug(+D_MEMPOOL, "Reuse %zu elements (pool '%s' created in %s())\n",
	        mp->curr_elements, mp->name, mp->func);

	/* Free its chained memory blocks. */
	char *c_next;
	for (char *c = mp->chain; c != NULL; c = c_next)
	{
		ASAN_UNPOISON_MEMORY_REGION(c, mp->element_size + FLDSIZE_NEXT);
		c_next = POOL_NEXT_BLOCK(c, mp->element_size);
		free(c);
	}

	mp->chain = NULL;
	mp->curr_elements = 0;
}

#ifdef POOL_FREE
void pool_free(Pool_desc *mp, void *e)
{
	if (ASAN_ADDRESS_IS_POISONED(e))
	{
		prt_error("Fatal error: Double pool free of %p\n", e);
		exit(1);
	}
	mp->curr_elements--;
	ASAN_POISON_MEMORY_REGION(e, mp->element_size + FLDSIZE_NEXT);
}
#endif // POOL_FREE
#endif // POOL_ALLOCATOR
