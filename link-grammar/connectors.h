/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2009, 2013 Linas Vepstas                                */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _LINK_GRAMMAR_CONNECTORS_H_
#define _LINK_GRAMMAR_CONNECTORS_H_

#include <ctype.h>   // for islower()
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>  // for uint8_t

#include "api-types.h"
#include "lg_assert.h"
#include "string-set.h"

/* MAX_SENTENCE cannot be more than 254, because word MAX_SENTENCE+1 is
 * BAD_WORD -- it is used to indicate that nothing can connect to this
 * connector, and this should fit in one byte (because the word field
 * of a connector is an uint8_t, see below).
 */
#define MAX_SENTENCE 254        /* Maximum number of words in a sentence */

typedef struct ConDesc
{
	const char *string;  /* The connector name w/o the direction mark, e.g. AB */
	uint32_t str_hash;
	union
	{
		uint32_t uc_hash;
		uint32_t uc_num;
	};
	uint8_t length_limit;
	                      /* If not 0, it gives the limit of the length of the
	                       * link that can be used on this connector type. The
	                       * value UNLIMITED_LEN specifies no limit.
	                       * If 0, short_length (a Parse_Option) is used. If
	                       * all_short==true (a Parse_Option), length_limit
	                       * is clipped to short_length. */

	/* The following are used for connector match speedup */
	uint8_t lc_start;    /* lc start position (or 0) */
	uint8_t uc_length;   /* uc part length */
	uint8_t uc_start;    /* uc start position */

	Connector *next;
} condesc_t;

typedef struct ConTable
{
	condesc_t **hdesc;    /* Hashed connector descriptors table */
	condesc_t **sdesc;    /* Alphabetically sorted descriptors */
	size_t size;          /* Allocated size */
	size_t num_con;       /* Number of connector types */
	size_t num_uc;        /* Number of connector types with different UC part */
} ConTable;

/* On a 64-bit machine, this struct should be exactly 4*8=32 bytes long.
 * Lets try to keep it that way.
 */
struct Connector_struct
{
	uint8_t length_limit; /* Can be different than in the descriptor */
	uint8_t nearest_word;
	                      /* The nearest word to my left (or right) that
	                         this could ever connect to.  Computed by
	                         setup_connectors() */
	bool multi;           /* TRUE if this is a multi-connector */
	const condesc_t *desc;
	Connector *next;

	/* Hash table next pointer, used only during pruning. */
	union
	{
		Connector * tableNext;
		const gword_set *originating_gword;
	};
};

void sort_condesc_by_uc_constring(Dictionary);
void condesc_delete(Dictionary);

static inline const char * connector_get_string(Connector *c)
{
	return c->desc->string;
}

/* Connector utilities ... */
Connector * connector_new(const condesc_t *, Parse_Options);
void free_connectors(Connector *);

/* Length-limits for how far connectors can reach out. */
#define UNLIMITED_LEN 255

#if 0
static inline Connector * init_connector(Connector *c)
{
	c->uc_hash = -1;
	c->str_hash = -1;
	c->length_limit = UNLIMITED_LEN;
	return c;
}
#endif

#if 0
/* Connector-set utilities ... */
void connector_set_create(Dictionary, Exp *e);
#endif
void connector_set_delete(Connector_set * conset);
bool match_in_connector_set(Connector_set*, Connector*);

void set_condesc_unlimited_length(Dictionary, Exp *);

/**
 * Returns TRUE if s and t match according to the connector matching
 * rules.  The connector strings must be properly formed, starting with
 * zero or one lower case letters, followed by one or more upper case
 * letters, followed by some other letters.
 *
 * The algorithm is symmetric with respect to a and b.
 *
 * Connectors starting with lower-case letters match ONLY if the initial
 * letters are DIFFERENT.  Otherwise, connectors only match if the
 * upper-case letters are the same, and the trailing lower case letters
 * are the same (or have wildcards).
 *
 * The initial lower-case letters allow an initial 'h' (denoting 'head
 * word') to match an initial 'd' (denoting 'dependent word'), while
 * rejecting a match 'h' to 'h' or 'd' to 'd'.  This allows the parser
 * to work with catena, instead of just links.
 */
static inline bool easy_match(const char * s, const char * t)
{
	char is = 0, it = 0;
	if (islower((int) *s)) { is = *s; s++; }
	if (islower((int) *t)) { it = *t; t++; }

	if (is != 0 && it != 0 && is == it) return false;

	while (isupper((int)*s) || isupper((int)*t))
	{
		if (*s != *t) return false;
		s++;
		t++;
	}

	while ((*s!='\0') && (*t!='\0'))
	{
		if ((*s == '*') || (*t == '*') || (*s == *t))
		{
			s++;
			t++;
		}
		else
			return false;
	}
	return true;
}


static inline int string_hash(const char *s)
{
	unsigned int i;

	/* djb2 hash */
	i = 5381;
	while (*s)
	{
		i = ((i << 5) + i) + *s;
		s++;
	}
	return i;
}

void calculate_connector_info(condesc_t *);

static inline int connector_uc_hash(Connector * c)
{
	return c->desc->uc_hash;
}

static inline uint32_t connector_str_hash(const char *s)
{
	uint32_t i;

	/* For most situations, all three hashes are very nearly equal;
	 * as to which is faster depends on the parsed text.
	 * For both English and Russian, there are about 100 pre-defined
	 * connectors, and another 2K-4K autogen'ed ones (the IDxxx idiom
	 * connectors, and the LLxxx suffix connectors for Russian).
	 * Turns out the cost of setting up the hash table dominates the
	 * cost of collisions. */
#ifdef USE_DJB2
	/* djb2 hash */
	i = 5381;
	while (*s)
	{
		i = ((i << 5) + i) + *s;
		s++;
	}
	i += i>>14;
#endif /* USE_DJB2 */

#define USE_JENKINS
#ifdef USE_JENKINS
	/* Jenkins one-at-a-time hash */
	i = 0;
	while (*s)
	{
		i += *s;
		i += (i<<10);
		i ^= (i>>6);
		s++;
	}
	i += (i << 3);
	i ^= (i >> 11);
	i += (i << 15);
#endif /* USE_JENKINS */

	return i;
}

/**
 * hash function. Based on some tests, this seems to be an almost
 * "perfect" hash, in that almost all hash buckets have the same size!
 */
static inline unsigned int pair_hash(unsigned int table_size,
                            int lw, int rw,
                            const Connector *le, const Connector *re,
                            unsigned int cost)
{
	unsigned int i;

#if 0
	/* hash function. Based on some tests, this seems to be
	 * an almost "perfect" hash, in that almost all hash buckets
	 * have the same size! */
	i = 1 << cost;
	i += 1 << (lw % (log2_table_size-1));
	i += 1 << (rw % (log2_table_size-1));
	i += ((unsigned int) le) >> 2;
	i += ((unsigned int) le) >> log2_table_size;
	i += ((unsigned int) re) >> 2;
	i += ((unsigned int) re) >> log2_table_size;
	i += i >> log2_table_size;
#else
	/* sdbm-based hash */
	i = cost;
	i = lw + (i << 6) + (i << 16) - i;
	i = rw + (i << 6) + (i << 16) - i;
	i = ((int)(intptr_t)le) + (i << 6) + (i << 16) - i;
	i = ((int)(intptr_t)re) + (i << 6) + (i << 16) - i;
#endif

	return i & (table_size-1);
}

static inline condesc_t *condesc_add(ConTable *ct, const char *constring)
{
	uint32_t i, s;
	uint32_t hash = connector_str_hash(constring);

	s = i = hash & (ct->size-1);

	while ((NULL != ct->hdesc[i]) &&
	       !string_set_cmp(constring, ct->hdesc[i]->string))
	{
		i = (i + 1) & (ct->size-1);
		if (i == s)
		{
			prt_error("Error: condesc(): Table size (%zu) overflow\n", ct->size);
			return NULL;
		}
	}

	if (NULL == ct->hdesc[i])
	{
		lgdebug(+11, "Creating connector '%s'\n", constring);
		ct->hdesc[i] = (condesc_t *)malloc(sizeof(condesc_t));
		memset(ct->hdesc[i], 0, sizeof(condesc_t));
		ct->hdesc[i]->str_hash = hash;
		ct->hdesc[i]->string = constring;
		calculate_connector_info(ct->hdesc[i]);
	}

	return ct->hdesc[i];
}

#endif /* _LINK_GRAMMAR_CONNECTORS_H_ */
