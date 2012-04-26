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

#include "api.h"

/**
 * Suppose you have a program that generates strings and keeps pointers to them.
   The program never needs to change these strings once they're generated.
   If it generates the same string again, then it can reuse the one it
   generated before.  This is what this package supports.

   String_set is the object.  The functions are:

   char * string_set_add(char * source_string, String_set * ss);
     This function returns a pointer to a string with the same
     contents as the source_string.  If that string is already
     in the table, then it uses that copy, otherwise it generates
     and inserts a new one.

   char * string_set_lookup(char * source_string, String_set * ss);
     This function returns a pointer to a string with the same
     contents as the source_string.  If that string is not already
     in the table, returns NULL;

   String_set * string_set_create(void);
     Create a new empty String_set.
 
   string_set_delete(String_set *ss);
     Free all the space associated with this string set.

   The implementation uses probed hashing (i.e. not bucket).
 */

static unsigned int hash_string(const char *str, const String_set *ss)
{
	unsigned int accum = 0;
	for (;*str != '\0'; str++)
		accum = ((256 * accum) + ((unsigned char) *str)) % (ss->size);
	return accum;
}

static unsigned int stride_hash_string(const char *str, const String_set *ss)
{
	unsigned int accum = 0;
	for (;*str != '\0'; str++)
		accum = ((17 * accum) + ((unsigned char) *str)) % (ss->size);
	/* This is the stride used, so we have to make sure that
	 * its value is not 0 */
	if (accum == 0) accum = 1;
	return accum;
}

/* return the next prime up from start */
static unsigned int next_prime_up(unsigned int start)
{
	unsigned int i;
	start = start | 1; /* make it odd */
	for (;;) {
		for (i=3; (i <= (start/i)); i += 2) {
			if (start % i == 0) break;
		}
		if (start % i == 0) {
			start += 2;
		} else {
			return start;
		}
	}
}

String_set * string_set_create(void)
{
	String_set *ss;
	ss = (String_set *) xalloc(sizeof(String_set));
	ss->size = next_prime_up(100);
	ss->table = (char **) xalloc(ss->size * sizeof(char *));
	memset(ss->table, 0, ss->size*sizeof(char *));
	ss->count = 0;
	return ss;
}

/**
 * lookup the given string in the table.  Return a pointer
 * to the place it is, or the place where it should be.
 */
static unsigned int find_place(const char * str, String_set *ss)
{
	unsigned int h, s, i;
	h = hash_string(str, ss);
	s = stride_hash_string(str, ss);
	for (i=h; 1; i = (i + s)%(ss->size))
	{
		if ((ss->table[i] == NULL) || (strcmp(ss->table[i], str) == 0)) return i;
	}
}

static void grow_table(String_set *ss)
{
	String_set old;
	unsigned int i, p;
	
	old = *ss;
	ss->size = next_prime_up(2 * old.size);  /* at least double the size */
	ss->table = (char **) xalloc(ss->size * sizeof(char *));
	memset(ss->table, 0, ss->size*sizeof(char *));
	ss->count = 0;
	for (i=0; i<old.size; i++)
	{
		if (old.table[i] != NULL)
		{
			p = find_place(old.table[i], ss);
			ss->table[p] = old.table[i];
			ss->count++;
		}
	}
	/* printf("growing from %d to %d\n", old.size, ss->size); */
	/* fflush(stdout); */
	xfree((char *) old.table, old.size * sizeof(char *));
}

const char * string_set_add(const char * source_string, String_set * ss)
{
	char * str;
	unsigned int len, p;
	
	assert(source_string != NULL, "STRING_SET: Can't insert a null string");

	p = find_place(source_string, ss);
	if (ss->table[p] != NULL) return ss->table[p];
	
	len = strlen(source_string);
	str = (char *) xalloc(len+1);
	strcpy(str, source_string);
	ss->table[p] = str;
	ss->count++;
	
	/* We just added it to the table.
	   If the table got too big, we grow it.
	   Too big is defined as being more than 3/4 full */
	if ((4 * ss->count) > (3 * ss->size)) grow_table(ss);
	
	return str;
}

const char * string_set_lookup(const char * source_string, String_set * ss)
{
	unsigned int p;
	
	p = find_place(source_string, ss);
	return ss->table[p];
}

void string_set_delete(String_set *ss)
{
	unsigned int i;
	
	if (ss == NULL) return;
	for (i=0; i<ss->size; i++)
	{
		if (ss->table[i] != NULL) xfree(ss->table[i], strlen(ss->table[i]) + 1);
	}
	xfree((char *) ss->table, ss->size * sizeof(char *));
	xfree((char *) ss, sizeof(String_set));
}
