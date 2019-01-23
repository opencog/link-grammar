/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright 2018 Amir Plivatsky                                         */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "string-id.h"
#include "utilities.h"

/**
 * This is an adaptation of the string_set module for generating
 * sequential unique identifiers for strings.
 *
 * It is currently used only for connector "suffix hashing".
 */

static unsigned int hash_string(const char *str, const String_id *ss)
{
	unsigned int accum = 0;
	for (;*str != '\0'; str++)
		accum = (7 * accum) + (unsigned char)*str;
	return accum % (ss->size);
}

static unsigned int stride_hash_string(const char *str, const String_id *ss)
{
	unsigned int accum = 0;
	for (;*str != '\0'; str++)
		accum = (17 * accum) + (unsigned char)*str;
	accum %= ss->size;
	/* This is the stride used, so we have to make sure that
	 * its value is not 0 */
	if (accum == 0) accum = 1;
	return accum;
}

/** Return the next prime up from start. */
static size_t next_prime_up(size_t start)
{
	size_t i;
	start |= 1; /* make it odd */
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

String_id *string_id_create(void)
{
	String_id *ss;
	ss = (String_id *) malloc(sizeof(String_id));
	ss->size = 9973; /* 9973  is a prime number */
	ss->table = (ss_id *) malloc(ss->size * sizeof(ss_id));
	memset(ss->table, 0, ss->size*sizeof(ss_id));
	ss->count = 0;
	return ss;
}

/**
 * lookup the given string in the table.  Return an index
 * to the place it is, or the place where it should be.
 */
static unsigned int find_place(const char * str, String_id *ss)
{
	unsigned int h, s;
	h = hash_string(str, ss);

	if ((ss->table[h].str == NULL) || (strcmp(ss->table[h].str, str) == 0)) return h;
	s = stride_hash_string(str, ss);
	while (true)
	{
		h = h + s;
		if (h >= ss->size) h %= ss->size;
		if ((ss->table[h].str == NULL) || (strcmp(ss->table[h].str, str) == 0)) return h;
	}
}

static void grow_table(String_id *ss)
{
	String_id old;
	size_t i;
	unsigned int p;

	old = *ss;
	ss->size = next_prime_up(3 * old.size);  /* at least triple the size */
	ss->table = (ss_id *)malloc(ss->size * sizeof(ss_id));
	memset(ss->table, 0, ss->size*sizeof(ss_id));
	ss->count = 0;
	for (i=0; i<old.size; i++)
	{
		if (old.table[i].str != NULL)
		{
			p = find_place(old.table[i].str, ss);
			ss->table[p] = old.table[i];
			ss->count++;
		}
	}
	/* printf("growing from %d to %d\n", old.size, ss->size); */
	/* fflush(stdout); */
	free(old.table);
}

int string_id_add(const char *source_string, String_id *ss)
{
	char * str;
	size_t len;
	unsigned int p;

	assert(source_string != NULL, "STRING_SET: Can't insert a null string");

	p = find_place(source_string, ss);
	if (ss->table[p].str != NULL) return ss->table[p].id;

	len = strlen(source_string);
	str = (char *) malloc(len+1);
	strcpy(str, source_string);
	ss->table[p].str = str;
	ss->table[p].id = ss->count;
	ss->count++;

	int keep_id = ss->table[p].id;

	/* We just added it to the table.  If the table got too big,
	 * we grow it.  Too big is defined as being more than 3/8 full.
	 * There's a huge boost from keeping this sparse. */
	if ((8 * ss->count) > (3 * ss->size)) grow_table(ss);

	return keep_id;
}

int string_id_lookup(const char *source_string, String_id *ss)
{
	unsigned int p;

	p = find_place(source_string, ss);
	return (ss->table[p].str == NULL) ? -1 : ss->table[p].id;
}

void string_id_delete(String_id *ss)
{
	size_t i;

	if (ss == NULL) return;
	for (i=0; i<ss->size; i++)
	{
		if (ss->table[i].str != NULL) free((void *)ss->table[i].str);
	}
	free(ss->table);
	free(ss);
}
