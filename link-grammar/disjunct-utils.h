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

struct Disjunct_struct
{
	Disjunct *next;
	Disjunct *dup_table_next;
	Connector *left, *right;
	double cost;

	/* match_left, right used only during parsing, for the match list. */
	bool match_left, match_right;

	union
	{
		unsigned int match_id;     /* verify the match list integrity */
		unsigned int dup_hash;     /* hash value for duplicate elimination */
	};
	gword_set *originating_gword; /* Set of originating gwords */
	const char * word_string;     /* subscripted dictionary word */
};

/* Disjunct utilities ... */
void free_disjuncts(Disjunct *);
void free_sentence_disjuncts(Sentence);
unsigned int count_disjuncts(Disjunct *);
Disjunct * catenate_disjuncts(Disjunct *, Disjunct *);
Disjunct * eliminate_duplicate_disjuncts(Disjunct *);
char * print_one_disjunct(Disjunct *);
void word_record_in_disjunct(const Gword *, Disjunct *);
int left_connector_count(Disjunct *);
int right_connector_count(Disjunct *);

Tracon_sharing *pack_sentence_for_pruning(Sentence);
Tracon_sharing *pack_sentence_for_parsing(Sentence);
void free_tracon_sharing(Tracon_sharing *);

void print_one_connector(Connector *, int, int);
void print_connector_list(Connector *);
void print_disjunct_list(Disjunct *);
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
 * remains 0 (later used for pruning pass_count - see below) and instead
 * the Connector's refcount filed reflects the number of connectors which
 * are memory-shared.
 * In the pruning stage, on each pass the tracon_id of "good"
 * connectors is assigned the pass number so they will not be checked
 * again on the same pass. The pruning stage also uses the connector
 * refcount field as a reference count - the number of times this
 * connector is memory-shared.
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
	unsigned int *num_cnctrs_per_word[2]; /* Indexed by word number */
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
	Tracon_list *tracon_list;   /* Used only for pruning */
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
