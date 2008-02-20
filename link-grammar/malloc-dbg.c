/*
 * malloc-dbg.c
 *
 * Debug memory allocation.  Hacky code, used only 
 * for debugging.
 *
 * Copyright (c) 2008 Linas Vepstas <linas@linas.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

/* ======================================================== */
static void *(*old_malloc_hook)(size_t, const void *);
static void (*old_free_hook)(void *, const void *);
static void *(*old_realloc_hook)(void *, size_t, const void *);

static void my_free_hook(void * mem, const void * caller);
static void * my_malloc_hook(size_t n_bytes, const void * caller);

#define TBLSZ 6000
typedef struct {
	void * mem;
	const void * caller;
	int cnt;
	size_t sz;
} loc_t;

static loc_t loc[TBLSZ];
static int mcnt = 0;

static void * my_realloc_hook(void * mem, size_t n_bytes, const void *caller)
{
	__realloc_hook = old_realloc_hook;
	void * nm = realloc(mem, n_bytes);
	old_realloc_hook = __realloc_hook;
	__realloc_hook = my_realloc_hook;

	int i;
	for (i=0; i<TBLSZ; i++)
	{
		if(mem == loc[i].mem)
		{
			loc[i].mem = nm;
			loc[i].caller = (void *) -1;
			loc[i].sz = n_bytes;
			// loc[i].cnt = -1;
			// printf("realloc %d %p\n", i, mem);
			break;
		}
	}
	if (i <= TBLSZ) return nm;
	for (i=0; i<TBLSZ; i++)
	{
		if((void *) -1 == loc[i].mem)
		{
			loc[i].mem = mem;
			loc[i].sz = n_bytes;
			loc[i].caller = caller;
			loc[i].cnt = -2;
			// printf("realloc %d %p\n", i, mem);
			break;
		}
	}
	if (i == TBLSZ) exit(1);
	return nm;
}

static void * my_malloc_hook(size_t n_bytes, const void * caller)
{
	__malloc_hook = old_malloc_hook;
	__free_hook = old_free_hook;
	void * mem = malloc(n_bytes);
	old_malloc_hook = __malloc_hook;
	old_free_hook = __free_hook;

	int i;
	for (i=0; i<TBLSZ; i++)
	{
		if((void *) -1 == loc[i].mem)
		{
			loc[i].mem = mem;
			loc[i].caller = caller;
			// printf("malloc %d %p\n", i, mem);
			mcnt ++;
			loc[i].cnt = mcnt;
			loc[i].sz = n_bytes;
			break;
		}
	}
	if (i == TBLSZ)
	{
		size_t totsz=0;
		for (i=0; i<TBLSZ; i++)
		{
			// size_t off = loc[i].caller - (void *) my_malloc_hook;
			printf("%d caller=%p sz=%x %d\n",
			       i, loc[i].caller, loc[i].sz, loc[i].cnt);
			totsz += loc[i].sz;
		}
		printf ("Total Size=%x\n", totsz);
*((int *)0) = 1;
		exit(1);
	}

	__malloc_hook = my_malloc_hook;
	__free_hook = my_free_hook;
	return mem;
}

static void my_free_hook(void * mem, const void * caller)
{
	__malloc_hook = old_malloc_hook;
	__free_hook = old_free_hook;
	int i;
	for (i=0; i<TBLSZ; i++)
	{
		if(mem == loc[i].mem)
		{
			loc[i].mem = (void *) -1;
			loc[i].caller = 0;
			loc[i].sz = 0;
			loc[i].cnt = -3;
			// printf("free %d %p\n", i, mem);
			break;
		}
	}
	__malloc_hook = my_malloc_hook;

	free(mem);
	old_free_hook = __free_hook;
	__free_hook = my_free_hook;
}

void report_mem_dbg(void)
{
	int i;
	int cnt = 0;
	size_t totsz = 0;
	for (i=0; i<TBLSZ; i++)
	{
		if(loc[i].mem != (void *)-1)
		{
			cnt++;
			totsz += loc[i].sz;
		}
	}
	printf ("tbl slots used = %d alloc mem=%d\n", cnt, totsz);
}

static void my_init_hook(void)
{
	old_malloc_hook = __malloc_hook;
	__malloc_hook = my_malloc_hook;

	old_free_hook = __free_hook;
	__free_hook = my_free_hook;

	old_realloc_hook = __realloc_hook;
	__realloc_hook = my_realloc_hook;

	int i;
	for (i=0; i<TBLSZ; i++)
	{
		loc[i].mem = (void *) -1;
		loc[i].caller = 0;
		loc[i].cnt = 0;
		loc[i].sz = 0;
	}
}

void (*__malloc_initialize_hook)(void) = my_init_hook;
