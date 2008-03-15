/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */ 
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/
#ifndef _UTILITIESH_
#define _UTILITIESH_

#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

static inline int is_utf8_upper(const char *s)
{
	wchar_t c;
	int nbytes = mbtowc(&c, s, 4);
	if (iswupper(c)) return nbytes;
	return 0;
}
static inline int is_utf8_alpha(const char *s)
{
	wchar_t c;
	int nbytes = mbtowc(&c, s, 4);
	if (iswalpha(c)) return nbytes;
	return 0;
}

static inline const char * skip_utf8_upper(const char * s)
{
	int nb = is_utf8_upper(s);
	while (nb)
	{
		s += nb;
		nb = is_utf8_upper(s);
	}
	return s;
}

void safe_strcpy(char *u, const char * v, int usize);
void safe_strcat(char *u, const char *v, int usize);
char *safe_strdup(const char *u);
void xfree(void *, int);
void exfree(void *, int);
void init_randtable(void);
int  next_power_of_two_up(int);
int  upper_case_match(const char *, const char *);
void left_print_string(FILE* fp, const char *, const char *);

void my_random_initialize(int seed);
void my_random_finalize(void);
int  my_random(void);

/* routines for copying basic objects */
void *      xalloc(int);
void *      exalloc(int);

char * get_default_locale(void);
char * join_path(const char * prefix, const char * suffix);

FILE *dictopen(const char *filename, const char *how);
void set_data_dir(const char * path);

#endif
