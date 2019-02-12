/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this softwares.   */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _LINK_GRAMMAR_DISJUNCT_UTILS_H_
#define _LINK_GRAMMAR_DISJUNCT_UTILS_H_

#include <stdbool.h>

#include "api-types.h"
#include "api-structures.h"             // Sentence

// Can undefine VERIFY_MATCH_LIST when done debugging...
#define VERIFY_MATCH_LIST

struct Disjunct_struct
{
	Disjunct *next;
	Connector *left, *right;
	double cost;
	bool marked;               /* unmarked disjuncts get deleted */

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

bool pack_sentence(Sentence);
void share_disjunct_jets(Sentence, bool);
void free_jet_sharing(Sentence);

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

void save_disjuncts(Sentence, Disjuncts_desc_t *);
void restore_disjuncts(Sentence, Disjuncts_desc_t *);
void free_saved_disjuncts(Disjuncts_desc_t *ddesc);
#endif /* _LINK_GRAMMAR_DISJUNCT_UTILS_H_ */
