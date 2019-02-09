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

	sent->Disjunct_pool = pool_new(__func__, "Disjunct",
	                   /*num_elements*/2048, sizeof(Disjunct),
	                   /*zero_out*/false, /*align*/false, /*exact*/false);
	sent->Connector_pool = pool_new(__func__, "Connector",
	                   /*num_elements*/8192, sizeof(Connector),
	                   /*zero_out*/false, /*align*/false, /*exact*/false);

	for (w = 0; w < sent->length; w++)
	{
		d = NULL;
		for (x = sent->word[w].x; x != NULL; x = x->next)
		{
			Disjunct *dx = build_disjuncts_for_exp(sent, x->exp, x->string, cost_cutoff, opts);
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
 * Convert integer to ASCII into the given buffer.
 * Use base 64 for compactness.
 * The following characters shouldn't appear in the result:
 * CONSEP, ",", and ".", because they are reserved for separators.
 * Return the number of characters in the buffer (not including the '\0').
 */
#define PTOA_BASE 64
static unsigned int ptoa_compact(char* buffer, uintptr_t ptr)
{
	char *p = buffer;
	do
	{
	  *p++ = '0' + (ptr % PTOA_BASE);
	  ptr /= PTOA_BASE;
	}
	 while (ptr > 0);

	*p = '\0';

	return (unsigned int)(p - buffer);
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
 * Prepending the gword_set encoding solve both of these requirements.
 */
#define WORD_OFFSET 256 /* Reserved for null connectors. */
bool set_connector_hash(Sentence sent)
{
	/* FIXME: For short sentences, setting the optimized connector hashing
	 * has a slight overhead. If this overhead is improved, maybe this
	 * limit can be set lower. */
	size_t min_sent_len_trailing_hash = 20;
	const char *len_trailing_hash = test_enabled("len-trailing-hash");
	if (NULL != len_trailing_hash)
		min_sent_len_trailing_hash = atoi(len_trailing_hash+1);

	if (sent->length < min_sent_len_trailing_hash)
	{
		/* Set dummy suffix_id's and return. */
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

		sent->num_suffix_id = id + 1;
		return false;
	}

#define CONSEP "&"      /* Connector string separator in the suffix sequence. */
#define MAX_LINK_NAME_LENGTH 10 // XXX Use a global definition
#define MAX_LINKS 20            // XXX Use a global definition
#define MAX_GWORD_ENCODING 32   /* Actually up to 12 characters. */
	char cstr[((MAX_LINK_NAME_LENGTH + 3) * MAX_LINKS) + MAX_GWORD_ENCODING];
	String_id *csid;

	if (NULL == sent->connector_suffix_id)
		sent->connector_suffix_id = string_id_create();
	csid = sent->connector_suffix_id;

	for (size_t w = 0; w < sent->length; w++)
	{
		for (Disjunct *d = sent->word[w].d; d != NULL; d = d->next)
		{
			/* Generate a string with a disjunct Gword encoding. It makes
			 * unique trailing connector sequences of different words and
			 * alternatives of a word, so they will get their own suffix_id.
			 */
			size_t lg = ptoa_compact(cstr, (uintptr_t)d->originating_gword);
			cstr[lg++] = ',';

			for (int dir = 0; dir < 2; dir ++)
			{
				//printf("Word %zu d=%p dir %s\n", w, d, dir?"RIGHT":"LEFT");

				Connector *first_c = (0 == dir) ? d->left : d->right;
				if (NULL == first_c) continue;

				Connector *cstack[MAX_LINKS];
				size_t ci = 0;
				size_t l = lg;

				//print_connector_list(d->word_string, dir?"RIGHT":"LEFT", first_c);
				for (Connector *c = first_c; NULL != c; c = c->next)
					cstack[ci++] = c;

				for (Connector **cp = &cstack[--ci]; cp >= &cstack[0]; cp--)
				{
					if ((*cp)->multi)
						cstr[l++] = '@'; /* May have different linkages. */
					l += lg_strlcpy(cstr+l, connector_string((*cp)), sizeof(cstr)-l);

					if (l > sizeof(cstr)-2)
					{
						/* This is improbable, given the big cstr buffer. */
						prt_error("Warning: set_connector_hash(): Buffer overflow.\n"
						          "Parsing may be wrong.\n");
					}

					int id = string_id_add(cstr, csid) + WORD_OFFSET;
					(*cp)->suffix_id = id;
					//printf("ID %d trail=%s\n", id, cstr);

					l += lg_strlcpy(cstr+l, CONSEP, sizeof(cstr)-l);
				}
			}
		}
	}

	/* Support incremental suffix_id generation (only one time is needed). */
	const char *snumid[] = { "NUMID", "NUMID1" };

	int t = string_id_lookup(snumid[0], csid);
	int numid = string_id_add(snumid[(int)(t > 0)], csid) + WORD_OFFSET;
	lgdebug(D_PREP, "Debug: Using trailing hash (length %zu): suffix_id %d\n",
	        sent->length, numid);
	sent->num_suffix_id = numid;

	return true;
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
}
