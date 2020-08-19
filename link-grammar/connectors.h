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
#include "error.h"
#include "memory-pool.h"
#include "string-set.h"

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
#define MAX_CONNECTOR_LC_LENGTH 9

#define MAX_LINK_NAME_LENGTH 12

typedef uint64_t lc_enc_t;

typedef uint32_t connector_hash_t;

#define CD_HEAD_DEPENDENT    (1<<0) /* Has a leading 'h' or 'd'. */
#define CD_HEAD              (1<<1) /* 0: dependent; 1: head; */
#define CD_PERMANENT         (1<<2) /* For SQL dict: Do not clear (future). */

/* The size of the following struct on a 64-bit machine is 32 bytes.
 * It should be kept at this size. If needed, head_dependent, uc_length
 * and uc_start can be eliminate or moved out. Also, there not enough
 * space here to implement cost per connector length - an index to a
 * cost table should be used instead.*/
struct condesc_struct
{
	lc_enc_t lc_letters;
	lc_enc_t lc_mask;

	const char *string;  /* The connector name w/o the direction mark, e.g. AB */
	// double *cost; /* Array of cost by connector length (cost[0]: default) */
	connector_hash_t uc_num; /* uc part enumeration. */
	uint8_t length_limit; /* If not 0, it gives the limit of the length of the
	                       * link that can be used on this connector type. The
	                       * value UNLIMITED_LEN specifies no limit.
	                       * If 0, short_length (a Parse_Option) is used. If
	                       * all_short==true (a Parse_Option), length_limit
	                       * is clipped to short_length. */
	uint8_t flags;        /* CD_* */

	/* For connector match speedup when sorting the connector table. */
	uint8_t uc_length;   /* uc part length */
	uint8_t uc_start;    /* uc start position */
};
typedef struct condesc_struct condesc_t;

typedef struct length_limit_def
{
	const char *defword;
	const Exp *defexp;
	struct length_limit_def *next;
	int length_limit;
} length_limit_def_t;

typedef struct hdesc
{
	condesc_t *desc;
	connector_hash_t str_hash;
} hdesc_t;

typedef struct
{
	hdesc_t *hdesc;       /* Hashed connector descriptors table */
	condesc_t **sdesc;    /* Alphabetically sorted descriptors */
	size_t size;          /* Allocated size */
	size_t num_con;       /* Number of connector types */
	size_t num_uc;        /* Number of connector types with different UC part */
	Pool_desc *mempool;
	length_limit_def_t *length_limit_def;
	length_limit_def_t **length_limit_def_next;
} ConTable;

/* On a 64-bit machine, this struct should be exactly 4*8=32 bytes long.
 * Lets try to keep it that way. */
struct Connector_struct
{
	union
	{
		uint8_t farthest_word;/* The farthest word to my left (or right)
		                         that this could ever connect to. Computed
		                         from length_limit by setup_connectors(). */
		uint8_t length_limit; /* Same purpose as above but relative to the
		                         current word. This is how it is initially
		                         set. */
	};
	uint8_t nearest_word; /* The nearest word to my left (or right) that
	                         this could ever connect to.  Initialized by
	                         setup_connectors(). Final value is found in
	                         the power pruning. */
	uint8_t prune_pass;   /* Prune pass number (one bit could be enough) */
	bool multi;           /* TRUE if this is a multi-connector */
	int32_t tracon_id;    /* Tracon identifier (see disjunct-utils.c) */
	const condesc_t *desc;
	Connector *next;
	union
	{
		const gword_set *originating_gword; /* Used while and after parsing */
		struct
		{
			int32_t refcount;/* Memory-sharing reference count - for pruning. */
			uint16_t exp_pos; /* The position in the originating expression,
			                   currently used only for debugging dict macros. */
			bool shallow;   /* TRUE if this is a shallow connector.
			                 * A connectors is shallow if it is the first in
			                 * its list on its disjunct. (It is deep if it is
			                 * not the first in its list; it is deepest if it
			                 * is the last on its list.) */
		};
	};
};

void condesc_init(Dictionary, size_t);
void condesc_reset(Dictionary);
void condesc_setup(Dictionary);
bool sort_condesc_by_uc_constring(Dictionary);
condesc_t *condesc_add(ConTable *ct, const char *);
void condesc_delete(Dictionary);
void condesc_reuse(Dictionary);

/* GET accessors for connector attributes.
 * Can be used for experimenting with Connector_struct internals in
 * non-trivial ways without the need to change most of the code that
 * accesses connectors */
static inline const char * connector_string(const Connector *c)
{
	return c->desc->string;
}

static inline unsigned int connector_uc_start(const Connector *c)
{
	return c->desc->uc_start;
}

static inline const condesc_t *connector_desc(const Connector *c)
{
	return c->desc;
}

static inline unsigned int connector_uc_hash(const Connector * c)
{
	return c->desc->uc_num;
}

static inline unsigned int connector_uc_num(const Connector * c)
{
	return c->desc->uc_num;
}


/* Connector utilities ... */
Connector * connector_new(Pool_desc *, const condesc_t *, Parse_Options);
void set_connector_length_limit(Connector *, Parse_Options);
void free_connectors(Connector *);

/**
 * Compare only the uppercase part of two connectors.
 * Return true if they are the same, else false.
 */
static inline bool connector_uc_eq(const Connector *c1, const Connector *c2)
{
	return (connector_uc_num(c1) == connector_uc_num(c2));
}

/*
 * Return the deepest connector in the connector chain starting with \p c.
 * @param c Any connector
 * @return The deepest connector (can be modified if needed).
 */
static inline Connector *connector_deepest(const Connector *c)
{
	for (; c->next != NULL; c = c->next)
		;
	return (Connector *)c; /* Note: Constness removed. */
}

/* Length-limits for how far connectors can reach out. */
#define UNLIMITED_LEN 255

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
static inline bool lc_easy_match(const condesc_t *c1, const condesc_t *c2)
{
	return (((c1->lc_letters ^ c2->lc_letters) & c1->lc_mask & c2->lc_mask) ==
	        (c1->lc_mask & c2->lc_mask & 1));
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

static inline uint32_t string_hash(const char *s)
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

/**
 * Hash function for the classic parser linkage memoization.
 */
static inline unsigned int pair_hash(unsigned int table_size,
                            int lw, int rw,
                            int l_id, const int r_id,
                            unsigned int null_count)
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
	i = null_count;
	i = lw + (i << 6) + (i << 16) - i;
	i = rw + (i << 6) + (i << 16) - i;
	i = l_id + (i << 6) + (i << 16) - i;
	i = r_id + (i << 6) + (i << 16) - i;
#endif

	return i & (table_size-1);
}

/**
 * Get the word number of the given tracon.
 * c is the leading tracon connector. The word number is extracted from
 * the nearest_word of the deepest connector.
 * This function depends on setup_connectors() (which initializes
 * nearest_word). It should not be called after power_prune() (which
 * changes nearest_word).
 *
 * Note: An alternative for getting the word number of a tracon is to keep
 * it in the tracon list table or in a separate array. Both ways add
 * noticeable overhead, maybe due to the added CPU cache footprint.
 * However, if the word number will be needed after power_prune() there
 * will be a need to keep it in an alternative way.
 */
static inline int get_tracon_word_number(Connector *c, int dir)
{
	c = connector_deepest(c);
	return c->nearest_word + ((dir == 0) ? 1 : -1);
}
#endif /* _LINK_GRAMMAR_CONNECTORS_H_ */
