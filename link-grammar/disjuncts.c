/*************************************************************************/
/* Copyright (c) 2008, 2009 Linas Vepstas                                */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/
/*
 * disjuncts.c
 *
 * Miscellaneous utilities for returning the list of disjuncts that
 * were acutally used in a given parse of a sentence.
 */

#include <stdlib.h>
#include <string.h>
#include "api-structures.h"
#include "disjuncts.h"
#include "utilities.h"

/* ========================================================= */

/**
 * lg_compute_disjunct_strings -- Given sentence, compute disjuncts.
 *
 * This routine will compute the string representation of the disjunct
 * used for each word in parsing the given sentence. A string
 * representation of the disjunct is needed for most of the corpus
 * statistics functions: this string, together with the "inflected"
 * word, is used as a key to index the statistics information in the
 * database. 
 *
 * XXX This implementation works, but I don't think its the simplest
 * one. I think that a better implementation would have used
 * sent->parse_info->chosen_disjuncts[w] to get the one that was used,
 * and then print_disjuncts() to print it.
 */
void lg_compute_disjunct_strings(Sentence sent, Linkage_info *lifo)
{
	char djstr[MAX_TOKEN_LENGTH*20]; /* no word will have more than 20 links */
	size_t copied, left;
	int i, w;
	int nwords = sent->length;
	Parse_info pi = sent->parse_info;
	int nlinks = pi->N_links;
	int *djlist, *djloco, *djcount;

	if (lifo->disjunct_list_str) return;
	lifo->nwords = nwords;
	lifo->disjunct_list_str = (char **) malloc(nwords * sizeof(char *));
	memset(lifo->disjunct_list_str, 0, nwords * sizeof(char *));

	djcount = (int *) malloc (sizeof(int) * (nwords + 2*nwords*nlinks));
	djlist = djcount + nwords;
	djloco = djlist + nwords*nlinks;

	/* Decrement nwords, so as to ignore the RIGHT-WALL */
	nwords --;

	for (w=0; w<nwords; w++)
	{
		djcount[w] = 0;
	}

	/* Create a table of disjuncts for each word. */
	for (i=0; i<nlinks; i++)
	{
		int lword = pi->link_array[i].l;
		int rword = pi->link_array[i].r;
		int slot = djcount[lword];

		/* Skip over RW link to the right wall */
		if (nwords <= rword) continue;

		djlist[lword*nlinks + slot] = i;
		djloco[lword*nlinks + slot] = rword;
		djcount[lword] ++;

		slot = djcount[rword];
		djlist[rword*nlinks + slot] = i;
		djloco[rword*nlinks + slot] = lword;
		djcount[rword] ++;

#ifdef DEBUG
		printf("Link: %d is %s--%s--%s\n", i, 
			sent->word[lword].string, pi->link_array[i].name,
			sent->word[rword].string);
#endif
	}

	/* Process each word in the sentence (skipping LEFT-WALL, which is
	 * word 0. */
	for (w=1; w<nwords; w++)
	{
		/* Sort the disjuncts for this word. -- bubble sort */
		int slot = djcount[w];
		for (i=0; i<slot; i++)
		{
			int j;
			for (j=i+1; j<slot; j++)
			{
				if (djloco[w*nlinks + i] > djloco[w*nlinks + j])
				{
					int tmp = djloco[w*nlinks + i];
					djloco[w*nlinks + i] = djloco[w*nlinks + j];
					djloco[w*nlinks + j] = tmp;
					tmp = djlist[w*nlinks + i];
					djlist[w*nlinks + i] = djlist[w*nlinks + j];
					djlist[w*nlinks + j] = tmp;
				}
			}
		}

		/* Create the disjunct string */
		left = sizeof(djstr);
		copied = 0;
		for (i=0; i<slot; i++)
		{
			int dj = djlist[w*nlinks + i];
			copied += lg_strlcpy(djstr+copied, pi->link_array[dj].name, left);
			left = sizeof(djstr) - copied;
			if (djloco[w*nlinks + i] < w)
				copied += lg_strlcpy(djstr+copied, "-", left--);
			else
				copied += lg_strlcpy(djstr+copied, "+", left--);
			copied += lg_strlcpy(djstr+copied, " ", left--);
		}

		lifo->disjunct_list_str[w] = strdup(djstr);
	}

	free (djcount);
}

