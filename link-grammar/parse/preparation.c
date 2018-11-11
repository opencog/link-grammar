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

#include "api-structures.h"
#include "prepare/build-disjuncts.h"
#include "connectors.h"
#include "dict-common/dict-common.h" // For Dictionary_s
#include "disjunct-utils.h"
#include "externs.h"
#include "preparation.h"
#include "print/print.h"
#include "prune.h"
#include "resources.h"
#include "string-set.h"
#include "string-id.h"
#include "utilities.h"
#include "tokenize/word-structures.h" // for Word_struct

#define D_PREP 5 // Debug level for this module

/**
 * Set c->nearest_word to the nearest word that this connector could
 * possibly connect to.  The connector *might*, in the end,
 * connect to something more distant, but this is the nearest
 * one that could be connected.
 */
static int set_dist_fields(Connector * c, size_t w, int delta)
{
	int i;
	if (c == NULL) return (int) w;
	i = set_dist_fields(c->next, w, delta) + delta;
	c->nearest_word = i;
	return i;
}

/**
 * Initialize the word fields of the connectors, and
 * eliminate those disjuncts that are so long, that they
 * would need to connect past the end of the sentence.
 */
static void setup_connectors(Sentence sent)
{
	size_t w;
	Disjunct * d, * xd, * head;
	for (w=0; w<sent->length; w++)
	{
		head = NULL;
		for (d=sent->word[w].d; d!=NULL; d=xd)
		{
			xd = d->next;
			if ((set_dist_fields(d->left, w, -1) < 0) ||
			    (set_dist_fields(d->right, w, 1) >= (int) sent->length))
			{
				d->next = NULL;
				free_disjuncts(d);
			}
			else
			{
				d->next = head;
				head = d;
			}
		}
		sent->word[w].d = head;
	}
}

/**
 * Record the wordgraph word in each of its connectors.
 * It is used for checking alternatives consistency.
 */
static void gword_record_in_connector(Sentence sent)
{
	for (size_t w = 0; w < sent->length; w++)
	{
		for (Disjunct *d = sent->word[w].d; d != NULL; d = d->next)
		{
			for (Connector *c = d->right; NULL != c; c = c->next)
				c->originating_gword = d->originating_gword;
			for (Connector *c = d->left; NULL != c; c = c->next)
				c->originating_gword = d->originating_gword;
		}
	}
}

/**
 * Turn sentence expressions into disjuncts.
 * Sentence expressions must have been built, before calling this routine.
 */
static void build_sentence_disjuncts(Sentence sent, double cost_cutoff,
                                     Parse_Options opts)
{
	Disjunct * d;
	X_node * x;
	size_t w;
	for (w = 0; w < sent->length; w++)
	{
		d = NULL;
		for (x = sent->word[w].x; x != NULL; x = x->next)
		{
			Disjunct *dx = build_disjuncts_for_exp(x->exp, x->string, cost_cutoff, opts);
			word_record_in_disjunct(x->word, dx);
			d = catenate_disjuncts(dx, d);
		}
		sent->word[w].d = d;
	}
}

#if 0
static void print_connector_list(const char *s, const char *t, Connector * e)
{
	printf("%s %s: ", s, t);
	for (;e != NULL; e=e->next)
	{
		printf("%s", connector_string(e));
		if (e->next != NULL) printf(" ");
	}
	printf("\n");
}
#endif

/**
 * Set a unique identifier per connector, to be used in the memoizing
 * table of the classic parser.
 *
 * Originally, the classic parser memoized the number of possible linkages
 * per a given connector-pair using the connector addresses. Since an
 * exhaustive search is done, such an approach has two main problems for
 * long sentences:
 * 1. A very big hash table (Table_connector in count.c) is used due to
 * the huge number of connectors (100Ks) in long sentences, a thing that
 * causes a severe memory cache trash (to the degree that absolutely most
 * of the memory accesses are L3 misses).
 * 2. Repeated linkage detailed calculation for what seemed identical
 * connectors. A hint for this solution was the use of 0 hash values for
 * NULL connectors.
 *
 * The idea that is implemented here is based of the fact that the number
 * of linkages using a connector-pair is governed only by these connectors
 * and the connectors after them (since cross links are not permitted).
 * This allows us to share the memoizing table hash value between
 * connectors that have the same "connector sequence suffix" (trailing
 * connectors) on their disjuncts. As a result, long sentences have
 * relatively few different connector hash values relative to their total
 * number of connectors.
 *
 * Algorithm:
 * The string-set code has been adapted (see string-id.c) to generate
 * a unique identifier per string.
 * All the connector sequence suffixes of each disjunct are generated here
 * as strings, which are used for getting a unique identifier for the
 * starting connector of each such sequence.
 * In order to save the need to cache the endpoint word numbers the
 * connector identifiers are not shared between words. To that end the
 * word number is prepended to the said strings. It is done using 2 bytes
 * for convenient (mainly to easily avoid the CONSEP value).
 */
#define WORD_OFFSET 256 /* Reserved for null connectors. */
static void set_connector_hash(Sentence sent)
{
	/* FIXME: For short sentences, setting the optimized connector hashing
	 * has a slight overhead. If this overhead is improved, maybe this
	 * limit can be set lower. */
	if (sent->length < 36)
	{
		int id = WORD_OFFSET;
		for (size_t w = 0; w < sent->length; w++)
		{
			for (Disjunct *d = sent->word[w].d; d != NULL; d = d->next)
			{
				for (Connector *c = d->left; NULL != c; c = c->next)
				{
						c->suffix_id = id++;
				}
				for (Connector *c = d->right; NULL != c; c = c->next)
				{
						c->suffix_id = id++;
				}
			}
		}
	}
	else
	{
#define CONSEP '&'      /* Connector string separator in the suffix sequence .*/
#define WORDENC_ADD 'A'
#define MAX_LINK_NAME_LENGTH 10 // XXX Use a global definition.
		char cstr[MAX_LINK_NAME_LENGTH * 20];

		String_id *ssid = string_id_create();
		int lcnum = 0;
		int rcnum = 0;

		for (size_t w = 0; w < sent->length; w++)
		{
			//printf("WORD %zu\n", w);
			cstr[0] = (char)(w & 0xFF) + WORDENC_ADD;
			cstr[1] = (char)((w>>4) & 0xFF) + WORDENC_ADD;
			const int wpreflen = 2;

			for (Disjunct *d = sent->word[w].d; d != NULL; d = d->next)
			{
				int l;
				char *s;

				l = wpreflen;
				for (Connector *c = d->left; NULL != c; c = c->next)
				{
					lcnum++; /* stat */
					if (c->multi) cstr[l++] = '@'; /* May have different linkages. */
					l += lg_strlcpy(cstr+l, connector_string(c), sizeof(cstr)-l);
					cstr[l++] = CONSEP;
				}
				/* XXX Check overflow. */
				cstr[l] = '\0';

				s = cstr;
				//print_connector_list(d->word_string, "LEFT", d->left);
				for (Connector *c = d->left; NULL != c; c = c->next)
				{
					s++;
					int id = string_id_add(s, ssid) + WORD_OFFSET;
					c->suffix_id = id;
					//printf("ID %d pref=%s\n", id, s);
					s = memchr(s, CONSEP, sizeof(cstr));
				}

				l = wpreflen;
				for (Connector *c = d->right; NULL != c; c = c->next)
				{
					rcnum++; /* stat */
					l += lg_strlcpy(cstr+l, connector_string(c), sizeof(cstr)-l);
					cstr[l++] = CONSEP;
				}
				/* XXX Check overflow. */
				cstr[l] = '\0';

				s = cstr;
				//print_connector_list(d->word_string, "RIGHT", d->right);
				for (Connector *c = d->right; NULL != c; c = c->next)
				{
					s++;
					int id = string_id_add(s, ssid) + WORD_OFFSET;
					c->suffix_id = id;
					//printf("ID %d pref=%s\n", id, s);
					s = memchr(s, CONSEP, sizeof(cstr));
				}
			}
		}

		if (verbosity_level(D_PREP))
		{
			int maxid = string_id_add("MAXID", ssid) - 1;
			prt_error("Debug: lcnum %d rcnum %d suffix_id %d\n",
			          lcnum, rcnum, maxid);
		}

		string_id_delete(ssid);
	}
}

/**
 * Assumes that the sentence expression lists have been generated.
 */
void prepare_to_parse(Sentence sent, Parse_Options opts)
{
	size_t i;

	build_sentence_disjuncts(sent, opts->disjunct_cost, opts);
	if (verbosity_level(5))
	{
		prt_error("Debug: After expanding expressions into disjuncts:\n");
		print_disjunct_counts(sent);
	}
	print_time(opts, "Built disjuncts");

	for (i=0; i<sent->length; i++)
	{
		sent->word[i].d = eliminate_duplicate_disjuncts(sent->word[i].d);

		/* Some long Russian sentences can really blow up, here. */
		if (resources_exhausted(opts->resources))
			return;
	}
	print_time(opts, "Eliminated duplicate disjuncts");

	if (verbosity_level(5))
	{
		prt_error("Debug: After expression pruning and duplicate elimination:\n");
		print_disjunct_counts(sent);
	}

	gword_record_in_connector(sent);
	setup_connectors(sent);

	set_connector_hash(sent);
}
