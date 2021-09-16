/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.   */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _LINK_GRAMMAR_DISJUNCT_UTILS_H_
#define _LINK_GRAMMAR_DISJUNCT_UTILS_H_

#include <stdbool.h>

#include "tracon-set.h"
#include "connectors.h"                 // Connector
#include "api-types.h"
#include "api-structures.h"             // Sentence

// Can undefine VERIFY_MATCH_LIST when done debugging...
#define VERIFY_MATCH_LIST

/* On a 64-bit machine, this struct should be exactly (6+2)*8=64 bytes long.
 * Lets try to keep it that way (for performance). */
struct Disjunct_struct
{
	/* 48 bytes of common stuff. */
	Disjunct *next;
	Connector *left, *right;
	gword_set *originating_gword; /* Set of originating gwords */
	union
	{
		struct
		{
			unsigned int is_category; /* Use the category field */
			float cost;               /* Disjunct cost */
			const char *word_string;  /* Subscripted dictionary word */
		};
		struct
		{
			/* Dictionary category & disjunct cost (for sentence generation). */
			unsigned int num_categories;
			unsigned int num_categories_alloced;
			Category_cost *category;
		};
	};

	/* Shared by different steps. For what | when. */
	union
	{
		Disjunct *dup_table_next; /* Duplicate elimination | before pruning */
		Disjunct *unused1;        /* Unused now | before parsing */
	}; /* 8 bytes */

	/* Shared by different steps. For what | when. */
	union
	{
		uint32_t dup_hash;        /* Duplicate elimination | before pruning */
		int32_t ordinal;          /* Generation mode | after d. elimination */
	}; /* 4 bytes */

	struct
	{
		bool match_left, match_right;
#ifdef VERIFY_MATCH_LIST
		uint16_t match_id;
#endif
	}; /* 4 bytes - during parsing */
};

/* Disjunct utilities ... */
void free_disjuncts(Disjunct *);
void free_sentence_disjuncts(Sentence, bool);
void free_categories(Sentence);
void free_categories_from_disjunct_array(Disjunct *, unsigned int);
unsigned int count_disjuncts(Disjunct *);
Disjunct * catenate_disjuncts(Disjunct *, Disjunct *);
Disjunct * eliminate_duplicate_disjuncts(Disjunct *, bool);
int left_connector_count(Disjunct *);
int right_connector_count(Disjunct *);

Tracon_sharing *pack_sentence_for_pruning(Sentence);
Tracon_sharing *pack_sentence_for_parsing(Sentence);
void free_tracon_sharing(Tracon_sharing *);
void count_disjuncts_and_connectors(Sentence, unsigned int *, unsigned int *);

/* Print disjunct/connector */
char *print_one_disjunct_str(const Disjunct *);
char *print_one_connector_str(const Connector *, const char *);
char *print_connector_list_str(const Connector *, const char *);
void print_one_connector(const Connector *, const char *);
void print_connector_list(const Connector *, const char *);
void print_disjunct_list(const Disjunct *, const char *);
void print_all_disjuncts(Sentence);

/* Save and restore sentence disjuncts */
typedef struct
{
	Pool_desc *Disjunct_pool;
	Pool_desc *Connector_pool;
	Disjunct **disjuncts;
} Disjuncts_desc_t;

/* Trailing connector sequences (aka tracons) are memory-shared for the
 * benefit of the pruning and parsing stages. To that end, unique tracons
 * are identified. For the parsing stage, a unique tracon_id is assigned
 * to the Connector's tracon_id field. For the pruning stage, this field
 * is unused and the Connector's refcount field reflects the number of
 * connectors which are memory-shared.
 * The details of what considered a unique tracon is slightly different
 * for the pruning stage and the parsing stage - see the comment block
 * "Connector encoding, sharing and packing" in disjunct-utils.c.
 *
 * The dimension of the 2-element arrays below is used as follows:
 * [0] - left side; [1] - right side.
 *
 * The array num_cnctrs_per_word holds the number of different tracon
 * IDs of each word; it is used only for sizing the power table in
 * power_prune() since the first connector (only) of each tracon is
 * inserted into the power table.
 */

typedef struct
{
	/* Table of tracons. A 32bit index into the connector array in
	 * memblock (instead of (Connector*)) is used for better use of the
	 * CPU cache on 64-bit CPUs. */
	uint32_t *table[2];         /* Indexed by entry number */
	size_t entries[2];          /* Actual number of entries */
	size_t table_size[2];       /* Allocated number of entries */
} Tracon_list;

struct tracon_sharing_s
{
	union
	{
		void *memblock;          /* Memory block for disjuncts & connectors */
		Disjunct *dblock_base;   /* Start of disjunct block */
	};
	size_t memblock_sz;         /* memblock size */
	Connector *cblock_base;     /* Start of connector block */
	Connector *cblock;          /* Next available memory for connector */
	Disjunct *dblock;           /* Next available memory for disjunct */
	Disjunct **d;               /* The disjuncts (indexed by word number) */
	unsigned int num_connectors;
	unsigned int num_disjuncts;
	Tracon_set *csid[2];        /* For generating unique tracon IDs */
	int next_id[2];             /* Next unique tracon ID */
	uintptr_t last_token;       /* Tracons are the same only per this token */
	int word_offset;            /* Start number for connector tracon_id */
	bool is_pruning;            /* false: Parsing step, true: Pruning step */
	Tracon_list *tracon_list;   /* Used only for pruning */

	/* The number of different uppercase connector parts per side / word,
	 * for sizing the prune power table. */
	uint8_t *uc_seen[2];        /* The last word number in which an
										  * uppercase connector part has been seen,
										  * (hence doesn't need clearing between words).
										  * Indexed by uc_num. */
	unsigned int *num_cnctrs_per_word[2]; /* Indexed by word number */
};

void *save_disjuncts(Sentence, Tracon_sharing *);
void restore_disjuncts(Sentence, void *, Tracon_sharing *);
void free_saved_disjuncts(Sentence);

/** Get tracon by (dir, tracon_id). */
static inline Connector *get_tracon(Tracon_sharing *ts, int dir, int id)
{
	return &ts->cblock_base[ts->tracon_list->table[dir][id]];
}
#endif /* _LINK_GRAMMAR_DISJUNCT_UTILS_H_ */
