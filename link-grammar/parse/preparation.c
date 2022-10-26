/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
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
#include "utilities.h"
#include "tokenize/word-structures.h"   // Word_struct
#include "tokenize/tokenize.h"          // word0_set
#include "tokenize/tok-structures.h"    // gword_set

#define D_PREP 5 // Debug level for this module.

/**
 * Set c->nearest_word to the nearest word that this connector could
 * possibly connect to.
 * The connector *might*, in the end, connect to something more distant,
 * but this is the nearest one that could be connected.
 */
static int set_dist_fields(Connector * c, size_t w, int delta)
{
	if (c == NULL) return (int) w;
	c->nearest_word = set_dist_fields(c->next, w, delta) + delta;
	return c->nearest_word;
}

/**
 * Initialize the word fields of the connectors,
 * eliminate those disjuncts that are so long, that they
 * would need to connect past the end of the sentence.
 */
static void setup_connectors(Sentence sent)
{
	for (WordIdx w = 0; w < sent->length; w++)
	{
		Disjunct *head = NULL;
		Disjunct *xd;

		for (Disjunct *d = sent->word[w].d; d != NULL; d = xd)
		{
			xd = d->next;
			if ((set_dist_fields(d->left, w, -1) < 0) ||
			    (set_dist_fields(d->right, w, 1) >= (int)sent->length))
			{
				if (d->is_category != 0) free(d->category);
				/* Skip this disjunct. */
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
void gword_record_in_connector(Sentence sent)
{
	for (Disjunct *d = sent->dc_memblock;
	     d < &((Disjunct *)sent->dc_memblock)[sent->num_disjuncts]; d++)
	{
		for (Connector *c = d->right; NULL != c; c = c->next)
			c->originating_gword = d->originating_gword;
		for (Connector *c = d->left; NULL != c; c = c->next)
			c->originating_gword = d->originating_gword;
	}
}

/**
 * Turn sentence expressions into disjuncts.
 * Sentence expressions must have been built, before calling this routine.
 */
static void build_sentence_disjuncts(Sentence sent, float cost_cutoff,
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
	                   /*zero_out*/true, /*align*/false, /*exact*/false);

#ifdef DEBUG
	size_t num_con_alloced = pool_num_elements_alloced(sent->Connector_pool);
#endif

	for (w = 0; w < sent->length; w++)
	{
		d = NULL;
		for (x = sent->word[w].x; x != NULL; x = x->next)
		{
			Disjunct *dx = build_disjuncts_for_exp(sent, x->exp, x->string,
				&x->word->gword_set_head, cost_cutoff, opts);
			d = catenate_disjuncts(dx, d);
		}
		sent->word[w].d = d;
	}

#ifdef DEBUG
	unsigned int dcnt, ccnt;
	count_disjuncts_and_connectors(sent, &dcnt, &ccnt);
	lgdebug(+D_PREP, "%u disjucts, %u connectors (%zu allocated)\n",
	        dcnt, ccnt,
	        pool_num_elements_alloced(sent->Connector_pool) - num_con_alloced);
#endif

	/* Delete the memory pools created in build_disjuncts_for_exp(). */
	pool_delete(sent->Clause_pool);
	pool_delete(sent->Tconnector_pool);
	sent->Clause_pool = NULL;
}


static void create_wildcard_word_disjunct_list(Sentence sent,
                                               Parse_Options opts)
{
	if (opts->verbosity >= D_USER_TIMES)
		prt_error("#### Creating a wild-card word disjunct list\n");

	int spell_option = parse_options_get_spell_guess(opts);
	parse_options_set_spell_guess(opts, 0);
	Sentence wc_word_list = sentence_create(WILDCARD_WORD, sent->dict);
	if (0 != sentence_split(wc_word_list, opts)) goto error;

	/* The result sentence may already consist of a wild-card word only,
	 * or may include walls. In the later case - discard the walls.
	 * FIXME? Use build_word_expressions() instead. */
	WordIdx w = 1; /* Check RIGHT-WALL here. */
	if (0 == strcmp(wc_word_list->word[0].unsplit_word, LEFT_WALL_WORD))
	{
		Word tmp = wc_word_list->word[0];
		wc_word_list->word[0] = wc_word_list->word[1];
		wc_word_list->word[1] = tmp;
		wc_word_list->word[1].x = NULL;  /* Don't generate disjuncts. */
		w = 2;
	}
	if ((wc_word_list->length == w + 1) &&
	    (0 == strcmp(wc_word_list->word[w].unsplit_word, RIGHT_WALL_WORD)))
	{
		wc_word_list->word[w].x = NULL;  /* Don't generate disjuncts. */
	}

	build_sentence_disjuncts(wc_word_list, opts->disjunct_cost, opts);

	Word *word0 = &wc_word_list->word[0];
	word0->d = eliminate_duplicate_disjuncts(word0->d, false);
	word0->d = eliminate_duplicate_disjuncts(word0->d, true);

	wc_word_list->min_len_encoding = 2; /* Don't share/encode. */
	Tracon_sharing *t = pack_sentence_for_pruning(wc_word_list);

	for (unsigned int n = 0; n < t->num_disjuncts; n++)
		t->dblock_base[n].ordinal = (int)n;

	sent->wildcard_word_dc_memblock = t->memblock;
	sent->wildcard_word_dc_memblock_sz = t->memblock_sz;
	sent->wildcard_word_num_disjuncts = t->num_disjuncts;

	if (opts->verbosity >= D_USER_TIMES)
		print_time(opts, "Finished creating list: %u disjuncts", t->num_disjuncts);

	t->memblock = NULL;
	free_tracon_sharing(t);

error:
	parse_options_set_spell_guess(opts, spell_option);
	sentence_delete(wc_word_list);
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
		prt_error("Debug: After expanding expressions into disjuncts:\n\\");
		print_disjunct_counts(sent);
	}
	print_time(opts, "Built disjuncts");

	int nord = 0;
	for (i=0; i<sent->length; i++)
	{
		sent->word[i].d = eliminate_duplicate_disjuncts(sent->word[i].d, false);
		if (IS_GENERATION(sent->dict))
		{
			if ((sent->word[i].d != NULL) && (sent->word[i].d->is_category != 0))
			{
				/* Also with different word_string. */
				sent->word[i].d = eliminate_duplicate_disjuncts(sent->word[i].d, true);

				for (Disjunct *d = sent->word[i].d; d != NULL; d = d->next)
					d->ordinal = nord++;
			}
			else
			{
				for (Disjunct *d = sent->word[i].d; d != NULL; d = d->next)
					d->ordinal = -1;
			}
		}
#if 0
		/* eliminate_duplicate_disjuncts() is now very efficient and doesn't
		 * take a significant time even for millions of disjuncts. If a very
		 * large number of disjuncts per word or very large number of words
		 * per sentence will ever be a problem, then a "checktimer"
		 * counter can be used there. Old comment and code are retained
		 * below for documentation. */

		/* Some long Russian sentences can really blow up, here. */
		if (resources_exhausted(opts->resources))
			return;
#endif
	}

	if (IS_GENERATION(sent->dict))
		create_wildcard_word_disjunct_list(sent, opts);

	print_time(opts, "Eliminated duplicate disjuncts");

	if (verbosity_level(D_PREP))
	{
		prt_error("Debug: After duplicate elimination:\n");
		print_disjunct_counts(sent);
	}

	setup_connectors(sent);

	if (verbosity_level(D_PREP))
	{
		prt_error("Debug: After setting connectors:\n");
		print_disjunct_counts(sent);
	}

	if (verbosity_level(D_SPEC+2))
	{
		printf("prepare_to_parse:\n");
		print_all_disjuncts(sent);
	}
}
