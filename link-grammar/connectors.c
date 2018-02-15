/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2013 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/
/*
 * Miscellaneous utilities for dealing with word types.
 */

#include <limits.h>                 // for CHAR_BIT

#include "dict-common/dict-utils.h" // for size_of_expression()
#include "api-structures.h"         // for Parse_Options_s
#include "connectors.h"
#include "link-includes.h"          // for Parse_Options

/**
 * free_connectors() -- free the list of connectors pointed to by e
 * (does not free any strings)
 */
void free_connectors(Connector *e)
{
	Connector * n;
	for (; e != NULL; e = n)
	{
		n = e->next;
		xfree((char *)e, sizeof(Connector));
	}
}

void
set_connector_length_limit(Connector *c, Parse_Options opts)
{
	if (NULL == opts)
	{
		c->length_limit = UNLIMITED_LEN;
		return;
	}

	int short_len = opts->short_length;
	bool all_short = opts->all_short;
	int length_limit = c->desc->length_limit;

	if ((all_short && (length_limit > short_len)) || (0 == length_limit))
		c->length_limit = short_len;
	else
		c->length_limit = length_limit;
}

Connector * connector_new(const condesc_t *desc, Parse_Options opts)
{
	Connector *c = (Connector *) xalloc(sizeof(Connector));

	c->desc = desc;
	c->nearest_word = 0;
	c->multi = false;
	set_connector_length_limit(c, opts);
	//assert(0 != c->length_limit, "Connector_new(): Zero length_limit");

	return c;
}

/* ======================================================== */
/* UNLIMITED-CONNECTORS handling. */

static size_t get_connectors_from_expression(condesc_t **conlist, const Exp *e)
{
	if (e->type == CONNECTOR_type)
	{
		if (NULL != conlist) *conlist = e->u.condesc;
		return 1;
	}

	size_t cl_size = 0;
	for (E_list *l = e->u.l; l != NULL; l = l->next)
	{
		cl_size += get_connectors_from_expression(conlist, l->e);
		if (NULL != conlist) conlist++;
	}

	return cl_size;
}

static int condesc_by_uc_num(const void *a, const void *b)
{
	const condesc_t * const * cda = a;
	const condesc_t * const * cdb = b;

	if ((*cda)->uc_num < (*cdb)->uc_num) return -1;
	if ((*cda)->uc_num > (*cdb)->uc_num) return 1;

	return 0;
}

#define WILD_TYPE '*'

static void set_condesc_length_limit(Dictionary dict, const Exp *e, int length_limit)
{
	size_t exp_num_con;
	ConTable *ct = &dict->contable;
	condesc_t **sdesc = ct->sdesc;
	condesc_t **econlist = NULL;

	if (e)
	{
		/* Create a connector list from the given expression. */
		exp_num_con = get_connectors_from_expression(NULL, e);
		econlist = alloca(exp_num_con * sizeof(*econlist));
		get_connectors_from_expression(econlist, e);
	}

	if ((NULL == econlist) || (NULL == econlist[0])) return;

	qsort(econlist, exp_num_con, sizeof(*econlist), condesc_by_uc_num);

	/* Scan the expression connector list and set length_limit.
	 * restart_cn is needed because several connectors in this list
	 * may match a given uppercase part. */
	size_t restart_cn = 0, cn = 0, en;
	for (en = 0; en < exp_num_con; en++)
	{
		for (cn = restart_cn; cn < ct->num_con; cn++)
			if (sdesc[cn]->uc_num >= econlist[en]->uc_num) break;

		for (; en < exp_num_con; en++)
			if (econlist[en]->uc_num >= sdesc[cn]->uc_num) break;

		if (econlist[en]->uc_num != sdesc[cn]->uc_num) continue;
		restart_cn = cn+1;

		const char *wc_str = econlist[en]->string;
		char *uc_wildcard = strchr(wc_str, WILD_TYPE);

		for (; cn < ct->num_con; cn++)
		{
			if (NULL == uc_wildcard)
			{
				if (econlist[en]->uc_num != sdesc[cn]->uc_num)
					break;
				/* The uppercase parts are equal - match only the lowercase ones. */
				if (!lc_easy_match(econlist[en], sdesc[cn]))
					continue;
			}
			else
			{
				/* The uppercase part is a prefix. */
				if (0 != strncmp(wc_str, sdesc[cn]->string, uc_wildcard - wc_str))
					break;
			}

			sdesc[cn]->length_limit = length_limit;
		}
	}
}

void set_all_condesc_length_limit(Dictionary dict)
{
	ConTable *ct = &dict->contable;
	bool unlimited_len_found = false;

	for (length_limit_def_t *l = ct->length_limit_def; NULL != l; l = l->next)
	{
		set_condesc_length_limit(dict, l->defexp, l->length_limit);
		if (UNLIMITED_LEN == l->length_limit) unlimited_len_found = true;
	}

	if (!unlimited_len_found)
	{
		/* If no connectors are defined as UNLIMITED_LEN, set all the
		 * connectors with no defined length-limit to UNLIMITED_LEN. */
		condesc_t **sdesc = ct->sdesc;

		for (size_t en = 0; en < ct->num_con; en++)
		{
			if (0 == sdesc[en]->length_limit)
				sdesc[en]->length_limit = UNLIMITED_LEN;
		}
	}

	length_limit_def_t *l_next;
	for (length_limit_def_t *l = ct->length_limit_def; NULL != l; l = l_next)
	{
		l_next = l->next;
		free(l);
	}

	if (verbosity_level(D_SPEC+1))
	{
		printf("%5s %-6s %3s\n", "num", "uc_num", "ll");
		for (size_t n = 0; n < ct->num_con; n++)
		{
			printf("%5zu %6d %3d %s\n", n, ct->sdesc[n]->uc_num,
			       ct->sdesc[n]->length_limit, ct->sdesc[n]->string);
		}
	}

}

/* ======================================================== */

static bool connector_encode_lc(const char *lc_string, condesc_t *desc)
{
	lc_enc_t lc_mask = 0;
	lc_enc_t lc_value = 0;
	lc_enc_t wildcard = LC_MASK;
	int lc_pos = 0;

	if ('\0' == *lc_string) return true;
	do
	{
		if (lc_pos > (int)(CHAR_BIT*sizeof(lc_value)/LC_BITS))
		{
			prt_error("Error: Lower-case part '%s' is too long (%d)\n",
			          lc_string, lc_pos);
			return false;
		}
		lc_value |= (lc_enc_t)(*lc_string & LC_MASK) << (lc_pos*LC_BITS);
		if (*lc_string != '*') lc_mask |= wildcard;
		if ('\0' == lc_string[1]) break;
		wildcard <<= LC_BITS;
		lc_pos++;
	} while (lc_string++);

	desc->lc_mask = lc_mask;
	desc->lc_letters = lc_value;

	return true;
}

/**
 * Calculate fixed connector information that only depend on its string.
 * This information is used to speed up the parsing stage.
 * It is calculated during the directory creation and doesn't get
 * changed afterward.
 */
bool calculate_connector_info(condesc_t * c)
{
	const char *s;
	unsigned int i;

	s = c->string;
	if (islower((int) *s)) s++; /* ignore head-dependent indicator */
	c->head_depended = (c->string == s)? '\0' : c->string[0];

	/* For most situations, all three hashes are very nearly equal;
	 * as to which is faster depends on the parsed text.
	 * For both English and Russian, there are about 100 pre-defined
	 * connectors, and another 2K-4K autogen'ed ones (the IDxxx idiom
	 * connectors, and the LLxxx suffix connectors for Russian).
	 * Turns out the cost of setting up the hash table dominates the
	 * cost of collisions. */
#ifdef USE_DJB2
	/* djb2 hash */
	i = 5381;
	while (isupper((int) *s)) /* connector tables cannot contain UTF8, yet */
	{
		i = ((i << 5) + i) + *s;
		s++;
	}
	i += i>>14;
#endif /* USE_DJB2 */

#define USE_JENKINS
#ifdef USE_JENKINS
	/* Jenkins one-at-a-time hash */
	i = 0;
	c->uc_start = s - c->string;
	while (isupper((int) *s)) /* connector tables cannot contain UTF8, yet */
	{
		i += *s;
		i += (i<<10);
		i ^= (i>>6);
		s++;
	}
	i += (i << 3);
	i ^= (i >> 11);
	i += (i << 15);
#endif /* USE_JENKINS */

#ifdef USE_SDBM
	/* sdbm hash */
	i = 0;
	c->uc_start = s - c->string;
	while (isupper((int) *s))
	{
		i = *s + (i << 6) + (i << 16) - i;
		s++;
	}
#endif /* USE_SDBM */

	c->uc_length = s - c->string - c->uc_start;
	c->uc_hash = i;

	return connector_encode_lc(s, c);
}

/* ================= Connector descriptor table. ====================== */

/**
 * Compare connector UC parts, for qsort.
 */
static int condesc_by_uc_constring(const void * a, const void * b)
{
	const condesc_t * const * cda = a;
	const condesc_t * const * cdb = b;

	/* Move the empty slots to the end. */
	if (NULL == *cda) return (NULL != *cdb);
	if (NULL == *cdb) return -1;

	const char *sa = &(*cda)->string[(*cda)->uc_start];
	const char *sb = &(*cdb)->string[(*cdb)->uc_start];

	int la = (*cda)->uc_length;
	int lb = (*cdb)->uc_length;

	if (la == lb)
	{
		//printf("la==lb A=%s b=%s, la=%d lb=%d len=%d\n",sa,sb,la,lb,la);
		return strncmp(sa, sb, la);
	}

	if (la < lb)
	{
		char *uca = strdupa(sa);
		uca[la] = '\0';
		//printf("la<lb A=%s b=%s, la=%d lb=%d len=%d\n",uca,sb,la,lb,lb);
		return strncmp(uca, sb, lb);
	}
	else
	{
		char *ucb = strdupa(sb);
		ucb[lb] = '\0';
		//printf("la>lb A=%s b=%s, la=%d lb=%d len=%d\n",sa,ucb,la,lb,la);
		return strncmp(sa, ucb, la);
	}
}

/**
 * Enumerate the connectors by their UC parts - equal parts get the same number.
 * It replaces the existing connector UC-part hash, and can later serve
 * as table index as if it was a perfect hash.
 */
void sort_condesc_by_uc_constring(Dictionary dict)
{
	condesc_t **sdesc = malloc(dict->contable.size * sizeof(*dict->contable.hdesc));
	memcpy(sdesc, dict->contable.hdesc, dict->contable.size * sizeof(*dict->contable.hdesc));
	qsort(sdesc, dict->contable.size, sizeof(*dict->contable.hdesc),
	      condesc_by_uc_constring);

	/* Find the number of connectors. */
	size_t n;
	for (n = 0; n < dict->contable.size; n++)
		if (NULL == sdesc[n]) break;
	dict->contable.num_con = n;

	if (0 == n)
	{
		prt_error("Error: Dictionary %s: No connectors found.\n", dict->name);
		/* FIXME: Generate a dictionary open error. */
		return;
	}

	/* Enumerate the connectors according to their UC part. */
	int uc_num = 0;
	uint32_t uc_hash = sdesc[0]->uc_hash; /* Will be recomputed */

	sdesc[0]->uc_num = uc_num;
	for (n = 1; n < dict->contable.num_con; n++)
	{
		condesc_t **condesc = &sdesc[n];

//#define DEBUG_UC_HASH_CHANGE
#ifndef DEBUG_UC_HASH_CHANGE /* Use a shortcut - not needed for correctness. */
		if ((condesc[0]->uc_hash != uc_hash) ||
		   (condesc[0]->uc_length != condesc[-1]->uc_length))

		{
			/* We know that the UC part has been changed. */
			uc_num++;
		}
		else
#endif
		{
			const char *uc1 = &condesc[0]->string[condesc[0]->uc_start];
			const char *uc2 = &condesc[-1]->string[condesc[-1]->uc_start];
			if (0 != strncmp(uc1, uc2, condesc[0]->uc_length))
			{
				uc_num++;
			}
		}

		uc_hash = condesc[0]->uc_hash;
		//printf("%5d constring=%s\n", uc_num, condesc[0]->string);
		condesc[0]->uc_hash = uc_num;
	}

	lgdebug(+11, "Dictionary %s: %zu different connectors "
	        "(%d with a different UC part)\n",
	        dict->name, dict->contable.num_con, uc_num+1);

	dict->contable.sdesc = sdesc;
	dict->contable.num_uc = uc_num + 1;
}

void condesc_delete(Dictionary dict)
{
	for (size_t i = 0; i < dict->contable.size; i++)
		free(dict->contable.hdesc[i]);

	free(dict->contable.hdesc);
	free(dict->contable.sdesc);
}

/* ========================= END OF FILE ============================== */
