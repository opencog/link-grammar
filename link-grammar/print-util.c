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

#include <stdarg.h>
#include "api.h"
#include "utilities.h"

/* This is a "safe" append function, used here to build up a link diagram
   incrementally.  Because the diagram is built up a few characters at
   a time, we keep around a pointer to the end of string to prevent
   the algorithm from being quadratic. */

struct String_s
{
	size_t allocated;  /* Unsigned so VC++ doesn't complain about comparisons */
	size_t eos; /* offset to end of string */
	char * p;
};

String * string_new(void)
{
#define INITSZ 30
	String * string;
	string = (String *) exalloc(sizeof(String));
	string->allocated = INITSZ;
	string->p = (char *) exalloc(INITSZ*sizeof(char));
	string->p[0] = '\0';
	string->eos = 0;
	return string;
}

void string_delete(String *s)
{
	exfree(s->p, s->allocated*sizeof(char));
	exfree(s, sizeof(String));
}

char * string_copy(String *s)
{
	char * p = (char *) exalloc(s->eos + 1);
	strcpy(p, s->p);
	return p;
}

void append_string(String * string, const char *fmt, ...)
{
#define TMPLEN 1024
	char temp_string[TMPLEN];
	size_t templen;
	char * p;
	size_t new_size;
	va_list args;

	va_start(args, fmt);
	templen = vsnprintf(temp_string, TMPLEN, fmt, args);
	va_end(args);

	if (string->allocated <= string->eos + templen)
	{
		new_size = 2 * string->allocated + templen + 1;
		p = (char *) exalloc(sizeof(char)*new_size);
		strcpy(p, string->p);
		strcpy(p + string->eos, temp_string);

		exfree(string->p, sizeof(char)*string->allocated);

		string->p = p;
		string->allocated = new_size;
		string->eos += templen;
	}
	else
	{
		strcpy(string->p + string->eos, temp_string);
		string->eos += templen;
	}
}

