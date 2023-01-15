/*
 * malloc-dbg2.c
 *
 * Debug memory allocation.  Hacky code, used only
 * for debugging. Tracks how much memory different malloc users ask for.
 *
 * To use:
 * Add following to a header file and recompile:

void * my_realloc_hook(void *, size_t, const char *, int, const char *);
void * my_malloc_hook(size_t, const char *, int, const char *);
void my_free_hook(void *, const char *, int, const char *);
#define malloc(X) my_malloc_hook(X, __FILE__, __LINE__, __FUNCTION__)
#define realloc(X,N) my_realloc_hook(X, N, __FILE__, __LINE__, __FUNCTION__)
#define free(X) my_free_hook(X, __FILE__, __LINE__, __FUNCTION__)

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

#define NUSERS 4136
typedef struct {
	const char * caller;
	const char * file;
	int line;
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

static int get_callerid(const char *file, int line, const char *caller)
{
	for (int i=0; i<nusers; i++)
	{
		if (caller == users[i].caller)
			return i;
	}

	int callerid = nusers;
	nusers++;
	users[callerid].file = file;
	users[callerid].line = line;
	users[callerid].caller = caller;
	users[callerid].sz = 0;
	users[callerid].nalloc = 0;
	users[callerid].nmove = 0;
	users[callerid].nrealloc = 0;
	users[callerid].nfree = 0;

	users[callerid].caller = caller;
	if (nusers == NUSERS)
	{
		printf("ran out of user slots\n");
		exit(1);
	}

	return callerid;
}

static void report(void)
{
	size_t totsz = 0;
	fprintf(fh, "Performed to %lu mallos\n", mcnt);
	for (int i=0; i<nusers; i++)
	{
		fprintf(fh, "%d caller=%s:%d %s sz=%ld nalloc=%d %d %d %d\n",
			i, users[i].file, users[i].line, users[i].caller,
			users[i].sz, users[i].nalloc,
			users[i].nrealloc, users[i].nmove, users[i].nfree);
		totsz += users[i].sz;
	}
	fprintf (fh, "Total Size=%zu\n", totsz);
}

void * my_realloc_hook(void * mem, size_t n_bytes,
                       const char *file, int line, const char *caller)
{
	int callerid = get_callerid(file, line, caller);

	size_t oldsz = malloc_usable_size(mem);
	void * nm = realloc(mem, n_bytes);
	size_t newsz = malloc_usable_size(nm);

	users[callerid].sz += newsz - oldsz;
	users[callerid].nrealloc ++;
	if (mem != nm)
		users[callerid].nmove ++;

	mcnt++;
	if (mcnt%1000000 == 0) report();
	allprt(fh, "realloc new alloc %d %p\n", i, nm);
	return nm;
}

void * my_malloc_hook(size_t n_bytes,
                      const char *file, int line, const char *caller)
{
	init_io();

	void * mem = malloc(n_bytes);
	size_t newsz = malloc_usable_size(mem);

	int callerid = get_callerid(file, line, caller);
	users[callerid].sz += newsz;
	users[callerid].nalloc ++;

	mcnt++;
	if (mcnt%1000000 == 0) report();

	return mem;
}

void my_free_hook(void * mem,
                  const char *file, int line, const char *caller)
{
	if (0x0 == mem) return;

	size_t oldsz = malloc_usable_size(mem);
	int callerid = get_callerid(file, line, caller);
	users[callerid].sz -= oldsz;
	users[callerid].nfree ++;

	free(mem);
}

// void my_init_hook(void);

__attribute__((constructor))
void my_init_hook(void)
{
	static bool is_init = false;
	if (is_init) return;
	is_init = true;

	printf("init malloc trace hook\n");
	nusers = 0;
}

#endif /*_MSC_VER && __MINGW32__*/
