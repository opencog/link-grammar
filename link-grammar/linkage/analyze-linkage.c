/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2012, 2014 Linas Vepstas                                */
/* All rights reserved                                                   */
/* Copyright (c) 2019 Amir Plivatsky                                     */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <ctype.h>
#include <string.h>

#include "analyze-linkage.h"
#include "connectors.h" // Needed for connector_string
#include "linkage.h"
#include "string-set.h"

/**
 * This returns a string that is the GCD of the two given strings.
 * If the GCD is equal to one of them, a pointer to it is returned.
 * Otherwise a new string for the GCD is put in the string set.
 *
 * Note: The head and dependent indicators (lower-case h and d) are
 * ignored, as the intersection cannot include them.
 */
static const char *intersect_strings(String_set *sset, const Connector *c1,
                                     const Connector *c2)
{
	const condesc_t *d1 = c1->desc;
	const condesc_t *d2 = c2->desc;

	/* Ignore the head/dependent encoding at bit 0. */
	lc_enc_t lc1_letters = d1->lc_letters >> 1;
	lc_enc_t lc2_letters = d2->lc_letters >> 1;

	lc_enc_t lc_label = lc1_letters | lc2_letters;

	/* This catches ~95% of the cases (it would work without this). */
	if (lc_label == lc1_letters) return &connector_string(c1)[d1->uc_start];
	if (lc_label == lc2_letters) return &connector_string(c2)[d2->uc_start];

	char *l = alloca(d1->uc_length + MAX_CONNECTOR_LC_LENGTH + 1);
	memcpy(l, &connector_string(c1)[d1->uc_start], d1->uc_length);

	for (size_t i = d1->uc_length; /* see note below */; i++)
	{
		l[i] = lc_label & LC_MASK;
		if (l[i] == '\0') l[i] = '*';

		lc_label >>= LC_BITS;
		if (lc_label == 0)
		{
			l[i+1] = '\0';
			break;
		}
	}
	/* Note: LC_BITS * MAX_CONNECTOR_LC_LENGTH < sizeof(lc_enc_t).
	 * So after MAX_CONNECTOR_LC_LENGTH shifts lc_label must be 0. */

	return string_set_add(l, sset);
}

/**
 * The name of the link is set to be the GCD of the names of
 * its two endpoints. Must be called after each extract_links(),
 * etc. since that call issues a brand-new set of links into
 * parse_info.
 */
void compute_link_names(Linkage lkg, String_set *sset)
{
	size_t i;
	for (i = 0; i < lkg->num_links; i++)
	{
		lkg->link_array[i].link_name = intersect_strings(sset,
			lkg->link_array[i].lc, lkg->link_array[i].rc);
	}
}
