/*
 * malloc-dbg2.c
 *
 * Debug memory allocation.  Hacky code, used only
 * for debugging. Tracks how much memory different malloc users ask for.
 *
 * To use: just compile and link into the executable.
 *
 * Copyright (c) 2008, 2023 Linas Vepstas <linas@linas.org>
 */

#if !defined(_MSC_VER) && !defined(__MINGW32__)
#include <execinfo.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

/* ======================================================== */
void report_mem_dbg(void);

static void *(*old_malloc_hook)(size_t, const void *);
static void (*old_free_hook)(void *, const void *);
static void *(*old_realloc_hook)(void *, size_t, const void *);

static void my_free_hook(void * mem, const void * caller);
static void * my_malloc_hook(size_t n_bytes, const void * caller);

#define NUSERS 4136
typedef struct {
	const void * caller;
	int64_t sz;
	int nalloc;
	int nrealloc;
	int nmove;
	int nfree;
} user_t;
static user_t users[NUSERS];
int nusers = 0;
static volatile size_t mcnt = 0;

static FILE *fh = NULL;

static void init_io(void)
{
	if (fh) return;
	/* fh = fopen("/tmp/m", "w"); */
	fh = stdout;
}

// #define allprt(f, varargs...) fprintf(f, ##varargs)
#define allprt(f, varargs...)

static int get_callerid(const void *caller)
{
	for (int i=0; i<nusers; i++)
	{
		if (caller == users[i].caller)
			return i;
	}

	int callerid = nusers;
	nusers++;
	users[callerid].caller = caller;
	users[callerid].sz = 0;
	users[callerid].nalloc = 0;
	users[callerid].nmove = 0;
	users[callerid].nrealloc = 0;
	users[callerid].nfree = 0;

	if (nusers == NUSERS)
	{
		printf("ran out of user slots\n");
		exit(1);
	}

	return callerid;
}

static void report()
{
	size_t totsz = 0;
	fprintf(fh, "Performed to %lu mallos\n", mcnt);
	for (int i=0; i<nusers; i++)
	{
		fprintf(fh, "%d caller=%p sz=%ld nalloc=%d %d %d %d\n",
			i, users[i].caller, users[i].sz, users[i].nalloc,
			users[i].nrealloc, users[i].nmove, users[i].nfree);
		totsz += users[i].sz;
	}
	fprintf (fh, "Total Size=%zu\n", totsz);
}

static void * my_realloc_hook(void * mem, size_t n_bytes, const void *caller)
{
	int callerid = get_callerid(caller);

	__realloc_hook = old_realloc_hook;
	size_t oldsz = malloc_usable_size(mem);
	void * nm = realloc(mem, n_bytes);
	size_t newsz = malloc_usable_size(nm);
	old_realloc_hook = __realloc_hook;
	__realloc_hook = my_realloc_hook;

	users[callerid].sz += newsz - oldsz;
	users[callerid].nrealloc ++;
	if (mem != nm)
		users[callerid].nmove ++;

	mcnt++;
	if (mcnt%1000000 == 0) report();
	allprt(fh, "realloc new alloc %d %p\n", i, nm);
	return nm;
}

static void * my_malloc_hook(size_t n_bytes, const void * caller)
{
	init_io();

	__malloc_hook = old_malloc_hook;
	__free_hook = old_free_hook;
	void * mem = malloc(n_bytes);
	size_t newsz = malloc_usable_size(mem);
	old_malloc_hook = __malloc_hook;
	old_free_hook = __free_hook;

	int callerid = get_callerid(caller);
	users[callerid].sz += newsz;
	users[callerid].nalloc ++;

	mcnt++;
	if (mcnt%1000000 == 0) report();

	__malloc_hook = my_malloc_hook;
	__free_hook = my_free_hook;
	return mem;
}

static void my_free_hook(void * mem, const void * caller)
{
	if (0x0 == mem) return;
	__malloc_hook = old_malloc_hook;
	__free_hook = old_free_hook;

	size_t oldsz = malloc_usable_size(mem);
	int callerid = get_callerid(caller);
	users[callerid].sz -= oldsz;
	users[callerid].nfree ++;

	__malloc_hook = my_malloc_hook;

	free(mem);
	old_free_hook = __free_hook;
	__free_hook = my_free_hook;
}

void my_init_hook(void);
void my_init_hook(void)
{
	static bool is_init = false;
	if (is_init) return;
	is_init = true;

	printf("init malloc trace hook\n");
	nusers = 0;

	old_malloc_hook = __malloc_hook;
	__malloc_hook = my_malloc_hook;

	old_free_hook = __free_hook;
	__free_hook = my_free_hook;

	old_realloc_hook = __realloc_hook;
	__realloc_hook = my_realloc_hook;
}

void (*__MALLOC_HOOK_VOLATILE __malloc_initialize_hook)(void) = my_init_hook;
#endif /*_MSC_VER && __MINGW32__*/
