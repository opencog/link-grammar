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
void my_strdup_hook(const char *, const char *, int, const char *);

#define malloc(X) my_malloc_hook(X, __FILE__, __LINE__, __FUNCTION__)
#define realloc(X,N) my_realloc_hook(X, N, __FILE__, __LINE__, __FUNCTION__)
#define free(X) my_free_hook(X, __FILE__, __LINE__, __FUNCTION__)
#define strdup(X) my_strdup_hook(X, __FILE__, __LINE__, __FUNCTION__)

 *
 * Copyright (c) 2008, 2023 Linas Vepstas <linas@linas.org>
 */

#if !defined(_MSC_VER) && !defined(__MINGW32__)
#include <execinfo.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <threads.h>

#include <sys/time.h>
#include <sys/resource.h>

extern void * my_realloc_hook(void *, size_t, const char *, int, const char *);
extern void * my_malloc_hook(size_t, const char *, int, const char *);
extern void my_free_hook(void *, const char *, int, const char *);
extern char * my_strdup_hook(const char *, const char *, int, const char *);
extern void my_reset_hook(void);
void my_init_hook(void);
/* ======================================================== */

#define NUSERS 436
typedef struct {
	const char * caller;
	const char * file;
	int line;
	int64_t sz;
	int nalloc;
	int nrealloc;
	int nmove;
	int nfree;
	int ndup;
} user_t;
static user_t users[NUSERS];
int nusers = 0;
static volatile size_t mcnt = 0;

mtx_t mutx;

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
	users[callerid].ndup = 0;

	users[callerid].caller = caller;
	if (nusers == NUSERS)
	{
		printf("ran out of user slots\n");
		exit(1);
	}

	return callerid;
}

#define RFREQ 21123123

static int cmpu(const void * a, const void * b)
{
	user_t* ua = (user_t*) a;
	user_t* ub = (user_t*) b;
	return strcmp(ua->file, ub->file);
}

static void report(void)
{
	user_t lusers[NUSERS];
	for (int i=0; i<nusers; i++)
		lusers[i] = users[i];

	qsort(lusers, nusers, sizeof(user_t), cmpu);

	// int64_t totsz = 0;
	__int128_t totsz = 0;
	__int128_t abssz = 0;
	int64_t subsz = 0;
	char * prev = NULL;
	fprintf(fh, "Performed %lu mallocs\n", mcnt);
	for (int i=0; i<nusers; i++)
	{
		char* c = strchr(lusers[i].file, ':');
		char* f = strndup(lusers[i].file, c-lusers[i].file);
		if (prev)
		{
			if (strcmp(prev, f))
			{
				printf("Summary %s use=%ld   runttot=%Ld abstot=%Ld\n\n", prev, subsz,
					(long long int)totsz, (long long int)abssz);
				subsz = 0;
			}
			free(prev);
		}
		prev = f;
		fprintf(fh, "%d caller=%s:%d %s sz=%ld nalloc=%d %d %d %d %d\n",
			i, lusers[i].file, lusers[i].line, lusers[i].caller,
			lusers[i].sz, lusers[i].nalloc, lusers[i].nrealloc,
			lusers[i].nmove, lusers[i].nfree, lusers[i].ndup);
		totsz += lusers[i].sz;
		subsz += lusers[i].sz;
		if (0 < lusers[i].sz) abssz += lusers[i].sz;
	}
	printf("Summary %s use=%ld   runttot=%Ld abstot=%Ld\n\n", prev, subsz,
		(long long int)totsz, (long long int)abssz);

	static int rptnum = 0;
	rptnum++;

	struct rusage rus;
	getrusage(RUSAGE_SELF, &rus);

	__int128_t avg = abssz;
	if (0 < mcnt) avg /= mcnt;

	totsz /= 1024;
	abssz /= 1024*1024;

	fprintf (fh, "%d Use= %Ld KB; Tot= %Ld MB in %lu mlocs; avg= %Ld Bytes; RSS= %ld KB\n\n",
		rptnum,
		(long long int)totsz, (long long int)abssz, mcnt,
		(long long int)avg, rus.ru_maxrss);
}

void * my_realloc_hook(void * mem, size_t n_bytes,
                       const char *file, int line, const char *caller)
{
	mtx_lock(&mutx);
	int callerid = get_callerid(file, line, caller);

	size_t oldsz = malloc_usable_size(mem);
	void * nm = realloc(mem, n_bytes);
	size_t newsz = malloc_usable_size(nm);

	users[callerid].sz += newsz - oldsz;
	users[callerid].nrealloc ++;
	if (mem != nm)
		users[callerid].nmove ++;

	mcnt++;
	if (mcnt%RFREQ == 0) report();
	allprt(fh, "realloc new alloc %d %p\n", i, nm);

	mtx_unlock(&mutx);
	return nm;
}

void * my_malloc_hook(size_t n_bytes,
                      const char *file, int line, const char *caller)
{
	init_io();

	mtx_lock(&mutx);
	void * mem = malloc(n_bytes);
	size_t newsz = malloc_usable_size(mem);

	int callerid = get_callerid(file, line, caller);
	users[callerid].sz += newsz;
	users[callerid].nalloc ++;

	mcnt++;
	if (mcnt%RFREQ == 0) report();

	mtx_unlock(&mutx);
	return mem;
}

char * my_strdup_hook(const char *s,
                      const char *file, int line, const char *caller)
{
	init_io();

	mtx_lock(&mutx);
	char * news = strdup(s);
	size_t newsz = malloc_usable_size(news);

	int callerid = get_callerid(file, line, caller);
	users[callerid].sz += newsz;
	users[callerid].ndup ++;

	// mcnt++;
	// if (mcnt%RFREQ == 0) report();

	mtx_unlock(&mutx);
	return news;
}

void my_free_hook(void * mem,
                  const char *file, int line, const char *caller)
{
	if (0x0 == mem) return;

	mtx_lock(&mutx);
	size_t oldsz = malloc_usable_size(mem);
	int callerid = get_callerid(file, line, caller);
	users[callerid].sz -= oldsz;
	users[callerid].nfree ++;

	free(mem);
	mtx_unlock(&mutx);
}

void my_reset_hook(void)
{
	mtx_lock(&mutx);
	printf("reset malloc trace hook\n");
	report();
	nusers = 0;
	mcnt = 0;
	report();
	mtx_unlock(&mutx);
}

__attribute__((constructor))
void my_init_hook(void)
{
	static bool is_init = false;
	if (is_init) return;
	is_init = true;

	printf("init malloc trace hook\n");
	nusers = 0;
	mcnt = 0;

	mtx_init(&mutx, mtx_plain);
}

#endif /*_MSC_VER && __MINGW32__*/
