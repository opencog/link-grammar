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
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>  // for uint8_t

#include "api-types.h"
#include "lg_assert.h"
#include "memory-pool.h"
#include "string-set.h"
#include "dict-common/dict-structures.h"

/* MAX_SENTENCE cannot be more than 254, because word MAX_SENTENCE+1 is
 * BAD_WORD -- it is used to indicate that nothing can connect to this
 * connector, and this should fit in one byte (because the word field
 * of a connector is an uint8_t, see below).
 */
#define MAX_SENTENCE 254        /* Maximum number of words in a sentence */

/* For faster comparisons, the connector lc part is encoded into a number
 * and a mask. Each letter is encoded using LC_BITS bits. With 7 bits, it
 * is possible to encode up to 9 letters in an uint64_t. */
#define LC_BITS 7
#define LC_MASK ((1<<LC_BITS)-1)
typedef uint64_t lc_enc_t;

typedef uint16_t connector_hash_size; /* Change to uint32_t if needed. */

/* When connector_hash_size is uint16_t, the size of the following
 * struct on a 64-bit machine is 32 bytes.
 * FIXME: Make it 16 bytes by separating the info that is not needed
 * by do_count() into another structure (and some other minor changes). */
struct condesc_struct
{
	lc_enc_t lc_letters;
	lc_enc_t lc_mask;

	const char *string;  /* The connector name w/o the direction mark, e.g. AB */
	// double *cost; /* Array of cost by length_limit (cost[0]: default) */
	connector_hash_size str_hash;
	union
	{
		connector_hash_size uc_hash;
		connector_hash_size uc_num;
	};
	uint8_t length_limit;
	                      /* If not 0, it gives the limit of the length of the
	                       * link that can be used on this connector type. The
	                       * value UNLIMITED_LEN specifies no limit.
	                       * If 0, short_length (a Parse_Option) is used. If
	                       * all_short==true (a Parse_Option), length_limit
	                       * is clipped to short_length. */
	char head_dependent;   /* 'h' for head, 'd' for dependent, or '\0' if none */

	/* For connector match speedup when sorting the connector table. */
	uint8_t uc_length;   /* uc part length */
	uint8_t uc_start;    /* uc start position */
};

typedef struct length_limit_def
{
	const char *defword;
	const Exp *defexp;
	struct length_limit_def *next;
	int length_limit;
} length_limit_def_t;

typedef struct
{
	condesc_t **hdesc;    /* Hashed connector descriptors table */
	condesc_t **sdesc;    /* Alphabetically sorted descriptors */
	size_t size;          /* Allocated size */
	size_t num_con;       /* Number of connector types */
	size_t num_uc;        /* Number of connector types with different UC part */
	Pool_desc *mempool;
	length_limit_def_t *length_limit_def;
	length_limit_def_t **length_limit_def_next;
} ConTable;

/* On a 64-bit machine, this struct should be exactly 4*8=32 bytes long.
 * Lets try to keep it that way.
 */
struct Connector_struct
{
	Conn conn;
	uint8_t length_limit; /* Can be different than in the descriptor */
	uint8_t nearest_word;
	                      /* The nearest word to my left (or right) that
	                         this could ever connect to.  Computed by
	                         setup_connectors() */
	Connector *next;
	const gword_set *originating_gword;
};

void sort_condesc_by_uc_constring(Dictionary);
condesc_t *condesc_add(ConTable *ct, const char *);
void condesc_delete(Dictionary);

/* GET accessors for connector attributes.
 * Can be used for experimenting with Connector_struct internals in
 * non-trivial ways without the need to change most of the code that
 * accesses connectors */
static inline const char * connector_string(const Connector *c)
{
	return c->conn.desc->string;
}

static inline char connector_head(const Connector *c)
{
	return c->conn.desc->head_dependent;
}

/*
static inline int connector_uc_start(const Connector *c)
{
	return c->conn.desc->uc_start;
}
*/

static inline const condesc_t *connector_desc(const Connector *c)
{
	return c->conn.desc;
}

static inline int connector_uc_hash(const Connector * c)
{
	return c->conn.desc->uc_hash;
}

static inline int connector_uc_num(const Connector * c)
{
	return c->conn.desc->uc_num;
}


/* Connector utilities ... */
Connector * connector_new(const condesc_t *, Parse_Options);
void set_connector_length_limit(Connector *, Parse_Options);
void free_connectors(Connector *);

/* Length-limits for how far connectors can reach out. */
#define UNLIMITED_LEN 255

void set_all_condesc_length_limit(Dictionary);

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

/**
 * Compare the lower-case and head/dependent parts of two connector descriptors.
 * When this function is called, it is assumed that the upper-case
 * parts are equal, and thus do not need to be checked again.
 */
static bool lc_easy_match(const condesc_t *c1, const condesc_t *c2)
{
	if ((c1->lc_letters ^ c2->lc_letters) & c1->lc_mask & c2->lc_mask)
		return false;
	if (('\0' != c1->head_dependent) && (c1->head_dependent == c2->head_dependent))
		return false;

	return true;
}

/**
 * This function is like easy_match(), but with connector descriptors.
 * It uses a shortcut comparison of the upper-case parts.
 */
static inline bool easy_match_desc(const condesc_t *c1, const condesc_t *c2)
{
	if (c1->uc_num != c2->uc_num) return false;
	return lc_easy_match(c1, c2);
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

bool calculate_connector_info(condesc_t *);

static inline int connector_str_hash(const char *s)
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
#endif /* _LINK_GRAMMAR_CONNECTORS_H_ */
