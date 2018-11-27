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
#include "dict-common/dict-common.h"    // Dictionary_s
#include "disjunct-utils.h"
#include "externs.h"
#include "preparation.h"
#include "print/print.h"
#include "prune.h"
#include "resources.h"
#include "string-set.h"
#include "string-id.h"
#include "utilities.h"
#include "tokenize/word-structures.h"   // Word_struct
#include "tokenize/tok-structures.h"    // gword_set

#define D_PREP 5 // Debug level for this module.

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
		printf("%s%s", e->multi?"@":"", connector_string(e));
		if (e->next != NULL) printf(" ");
	}
	printf("\n");
}
#endif

/**
 * Convert integer to ASCII. Use base 64 for compactness.
 * The following characters shouldn't appear in the result:
 * CONSEP, ",", and ".", because they are reserved for separators.
 */
#define ITOA_BASE 64
static char* itoa_compact(char* buffer, size_t num)
{
	 do
	 {
		*buffer++ = '0' + (num % ITOA_BASE);
		num /= ITOA_BASE;
	 }
	 while (num > 0);

	*buffer = '\0';

	return buffer;
}

/**
 * Set a hash identifier per connector according to the trailing sequence
 * it starts. Ensure it is unique per word, alternative and disjunct cost.
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
 * The idea that is implemented here is based on the fact that the number
 * of linkages using a connector-pair is governed only by these connectors
 * and the connectors after them (since cross links are not permitted).
 * This allows us to share the memoizing table hash value between
 * connectors that have the same "connector sequence suffix" (trailing
 * connectors) on their disjuncts. As a result, long sentences have
 * significantly less different connector hash values than their total
 * number of connectors.
 *
 * Algorithm:
 * The string-set code has been adapted (see string-id.c) to generate
 * a unique identifier per string.
 * All the connector sequence suffixes of each disjunct are generated here
 * as strings, which are used for getting a unique identifier for the
 * starting connector of each such sequence.
 * In order to save the need to cache the endpoint word numbers the
 * connector identifiers should not be shared between words.
 * We also should consider the case of alternatives - trailing connector
 * sequences that belong to disjuncts of different alternatives may have
 * different linkage counts because some alternatives-connectivity checks
 * are done in the fast-matcher.
 * Prepending the gword numbers solve both of these requirements.
 */
#define WORD_OFFSET 256 /* Reserved for null connectors. */
static void set_connector_hash(Sentence sent)
{
	/* FIXME: For short sentences, setting the optimized connector hashing
	 * has a slight overhead. If this overhead is improved, maybe this
	 * limit can be set lower. */
	size_t min_sent_len_trailing_hash = 36;
	const char *len_trailing_hash = test_enabled("len-trailing-hash");
	if (NULL != len_trailing_hash)
		min_sent_len_trailing_hash = atoi(len_trailing_hash+1);

	if (sent->length < min_sent_len_trailing_hash)
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
		lgdebug(D_PREP, "Debug: Using trailing hash (Sentence length %zu)\n",
		    sent->length);
#define CONSEP '&'      /* Connector string separator in the suffix sequence .*/
#define MAX_LINK_NAME_LENGTH 10 // XXX Use a global definition.
#define MAX_GWORD_ENCODING 16 /* Up to 64^15 ... */
		char cstr[(MAX_LINK_NAME_LENGTH + MAX_GWORD_ENCODING) * 20];

		String_id *ssid = string_id_create();
		int cnum[2] = { 0 }; /* Connector counts stats for debug. */

		for (size_t w = 0; w < sent->length; w++)
		{
			//printf("WORD %zu\n", w);

			for (Disjunct *d = sent->word[w].d; d != NULL; d = d->next)
			{
				int l;
				char *s;

				/* Generate a string with the disjunct Gword number(s). It
				 * makes unique trailing connector sequences of different
				 * alternatives, so they will get their own suffix_id. */
				l = 0;
				char gword_num[MAX_GWORD_ENCODING];
#define MAX_DIFFERENT_GWORDS 6 /* More than 3 have not been seen yet. */
				char gword_nums[MAX_GWORD_ENCODING * MAX_DIFFERENT_GWORDS];
				for (const gword_set *g = d->originating_gword; NULL != g; g = g->next)
				{
					itoa_compact(gword_num, g->o_gword->node_num);
					l += lg_strlcpy(gword_nums+l, gword_num, sizeof(gword_nums)-l);
					cstr[l++] = ',';
				}
				if (l > (int)sizeof(gword_nums)-2)
				{
					/* Overflow. Never observed, maybe cannot happen. Tag it with
					 * a unique identifier so it will get its own suffix_id. */
#ifdef DEBUG
					prt_error("Warning: set_connector_hash(): "
					          "Token %s: Gword overflow: %s\n",
					          d->word_string, gword_nums);
#endif
					sprintf(gword_nums, "Gword overflow(%p)\n", d);
				}
				//printf("w=%zu, token=%s: GWORD_NUMS: %s\n",
				//       w, d->word_string, gword_nums);

				for (int dir = 0; dir < 2; dir ++)
				{
					Connector *first_c = (0 == dir) ? d->left : d->right;

					l = 0;
					for (Connector *c = first_c; NULL != c; c = c->next)
					{
						cnum[dir]++;
						l += lg_strlcpy(cstr+l, gword_num, sizeof(cstr)-l);
						cstr[l++] = ',';
						if (c->multi) cstr[l++] = '@'; /* May have different linkages. */
						l += lg_strlcpy(cstr+l, connector_string(c), sizeof(cstr)-l);
						cstr[l++] = CONSEP;
					}
					/* XXX Check overflow. */
					cstr[l] = '\0';

					s = cstr;
					//print_connector_list(d->word_string, dir?"RIGHT":"LEFT", first_c);
					for (Connector *c = first_c; NULL != c; c = c->next)
					{
						int id = string_id_add(s, ssid) + WORD_OFFSET;
						c->suffix_id = id;
						//printf("ID %d trail=%s\n", id, s);
						s = memchr(s, CONSEP, sizeof(cstr));
						s++;
					}
				}
			}
		}

		if (verbosity_level(D_PREP))
		{
			int maxid = string_id_add("MAXID", ssid) + WORD_OFFSET - 1;
			prt_error("Debug: suffix_id %d, %d (%d+,%d-) connectors\n",
			          maxid, cnum[1]+cnum[0], cnum[1], cnum[0]);
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
	if (verbosity_level(D_PREP))
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

	if (verbosity_level(D_PREP))
	{
		prt_error("Debug: After expression pruning and duplicate elimination:\n");
		print_disjunct_counts(sent);
	}

	gword_record_in_connector(sent);
	setup_connectors(sent);

	set_connector_hash(sent);
}
