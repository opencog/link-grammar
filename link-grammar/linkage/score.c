/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2012, 2014 Linas Vepstas                                */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <stdarg.h>
#include "api-structures.h" // Needed for Parse_Options
#include "connectors.h"
#include "disjunct-utils.h" // Needed for Disjunct
#include "linkage.h"
#include "score.h"

/**
 * This function defines the cost of a link as a function of its length.
 */
static inline int cost_for_length(int length)
{
	return length-1;
}

/**
 * Computes the cost of the current parse of the current sentence,
 * due to the length of the links.
 */
static size_t compute_link_cost(Linkage lkg)
{
	size_t lcost, i;
	lcost =  0;
	for (i = 0; i < lkg->num_links; i++)
	{
		lcost += cost_for_length(lkg->link_array[i].rw - lkg->link_array[i].lw);
	}
	return lcost;
}

static int unused_word_cost(Linkage lkg)
{
	int lcost;
	size_t i;
	lcost =  0;
	for (i = 0; i < lkg->num_words; i++)
		lcost += (lkg->chosen_disjuncts[i] == NULL);
	return lcost;
}

/**
 * Computes the cost of the current parse of the current sentence
 * due to the cost of the chosen disjuncts.
 */
static float compute_disjunct_cost(Linkage lkg)
{
	size_t i;
	float lcost;
	lcost =  0.0;
	for (i = 0; i < lkg->num_words; i++)
	{
		Disjunct * dj = lkg->chosen_disjuncts[i];
		if (dj != NULL)
			lcost += dj->is_category ? dj->category[0].cost : dj->cost;
	}
	return lcost;
}

/// Compute the extra cost of a multi-connector being used multiple
/// times. Any cost on that multi-connector needs to be added for each
/// usage of it.
static float compute_multi_cost(Linkage lkg)
{
	size_t lword = 0;
	float mcost = 0.0;
	for (size_t i = 0; i < lkg->num_words; i++)
	{
		Disjunct * dj = lkg->chosen_disjuncts[i];
		if (NULL == dj) continue;

		// Look at the right-going connectors.
		for (Connector* rcon = dj->right; rcon; rcon=rcon->next)
		{
			// No-op if not a multi-connector.
			if (false == rcon->multi) continue;

			// Find the links using this connector.
			for (size_t l = 0; l < lkg->num_links; l++)
			{
				// The link is not even for this word. Ignore it.
				if (lword != lkg->link_array[l].lw) continue;

				// Find the links that are using our multi-connector.
				for (Connector* wrc = lkg->link_array[l].rc; wrc; wrc=wrc->next)
				{
					// Skip if no match.
					if (strcmp(rcon->desc->string, wrc->desc->string)) continue;
dj->cost += 0.0001;
	 mcost += 1.0;
				}
			}
		}
		lword++; // increment only if disjunct is non-null.
	}
	return mcost;
}

/** Assign parse score (cost) to linkage, used for parse ranking. */
void linkage_score(Linkage lkg, Parse_Options opts)
{
float mcost = compute_multi_cost(lkg);

	lkg->lifo.unused_word_cost = unused_word_cost(lkg);
	lkg->lifo.disjunct_cost = compute_disjunct_cost(lkg);
	lkg->lifo.link_cost = compute_link_cost(lkg);

// printf("duuude mcost=%f\n", mcost);
// lkg->lifo.disjunct_cost += 0.0001 * mcost;
}
