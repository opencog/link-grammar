/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2013 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <stdarg.h>
#include <stdarg.h>
#include <errno.h>
#include "print-util.h"
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

const char * string_value(String *s)
{
	return s->p;
}

char * string_copy(String *s)
{
	char * p = (char *) exalloc(s->eos + 1);
	strcpy(p, s->p);
	return p;
}

static void string_append_l(String *string, const char *a, size_t len)
{
	if (string->allocated <= string->eos + len)
	{
		string->allocated = 2 * string->allocated + len + 1;
		string->p = (char *)realloc(string->p, string->allocated);
	}
	strcpy(string->p + string->eos, a);
	string->eos += len;
	return;
}

/* Note: As in the rest of the LG library, we assume here C99 compliance. */
void vappend_string(String * string, const char *fmt, va_list args)
{
#define TMPLEN 1024 /* Big enough for a possible error message, see below */
	char temp_buffer[TMPLEN];
	char *temp_string = temp_buffer;
	size_t templen;
	va_list copy_args;

	va_copy(copy_args, args);
	templen = vsnprintf(temp_string, TMPLEN, fmt, copy_args);
	va_end(copy_args);

	if ((int)templen < 0) goto error;
	// if (fmt[0] == '(') { errno=2; goto error;} /* Test the error reporting. */

	if (templen >= TMPLEN)
	{
		/* TMPLEN is too small - use a bigger buffer. Couldn't actually
		 * find any example of entering this code with TMPLEN=1024... */
		temp_string = alloca(templen+1);
		templen = vsnprintf(temp_string, templen+1, fmt, args);
		if ((int)templen < 0) goto error;
	}
	va_end(args);

	string_append_l(string, temp_string, templen);
	return;

error:
	{
		/* Some error has occurred */
		const char msg[] = "[vappend_string(): ";
		strcpy(temp_buffer, msg);
		strerror_r(errno, temp_buffer+sizeof(msg)-1, TMPLEN-sizeof(msg));
		strcat(temp_buffer, "]");
		string_append_l(string, temp_string, strlen(temp_buffer));
		return;
	}
}

void append_string(String * string, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	vappend_string(string, fmt, args);
}

size_t append_utf8_char(String * string, const char * mbs)
{
	/* Copy exactly one multi-byte character to buf */
	char buf[10];
	size_t n = utf8_next(mbs);

	assert(n<10, "Multi-byte character is too long!");
	strncpy(buf, mbs, n);
	buf[n] = 0;
	append_string(string, "%s", buf);
	return n;
}
