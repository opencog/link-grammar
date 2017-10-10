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

#include <math.h>

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

static void
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
	if (short_len > UNLIMITED_LEN) short_len = UNLIMITED_LEN;

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

static void get_connectors_from_expression(condesc_t **conlist, size_t *cl_size,
                                           Exp *e)
{
	E_list * l;

	if (e->type == CONNECTOR_type)
	{
		if (NULL != conlist)
		{
			conlist[*cl_size] = e->u.condesc;
		}
		(*cl_size)++;
	} else {
		for (l=e->u.l; l!=NULL; l=l->next) {
			get_connectors_from_expression(conlist, cl_size, l->e);
		}
	}
}

static int condesc_by_uc_num(const void *a, const void *b)
{
	const condesc_t * const * cda = a;
	const condesc_t * const * cdb = b;

	if ((*cda)->uc_num < (*cdb)->uc_num) return -1;
	if ((*cda)->uc_num > (*cdb)->uc_num) return 1;

	return 0;
}

void set_condesc_unlimited_length(Dictionary dict, Exp *e)
{
	size_t exp_num_con;
	ConTable *ct = &dict->contable;
	condesc_t **sdesc = ct->sdesc;
	condesc_t **econlist = NULL;

	if (e)
	{
		/* Create a connector list from the given expression. */
		get_connectors_from_expression(NULL, (exp_num_con = 0, &exp_num_con), e);
		econlist = alloca(exp_num_con * sizeof(*econlist));
		get_connectors_from_expression(econlist, (exp_num_con = 0, &exp_num_con), e);
	}

	if ((NULL == econlist) || (NULL == econlist[0]))
	{
		/* No connectors are marked as UNLIMITED-CONNECTORS.
		 * All the connectors are set as unlimited. */
		for (size_t en = 0; en < ct->num_con; en++)
			sdesc[en]->length_limit = UNLIMITED_LEN;

		return;
	}

	qsort(econlist, exp_num_con, sizeof(*econlist), condesc_by_uc_num);

	/* Scan the expression connector list and set UNLIMITED_LEN.
	 * restart_cn is needed because several connectors in this list
	 * may match a given UC sequence. */
	size_t restart_cn = 0, cn = 0, en;
	for (en = 0; en < exp_num_con; en++)
	{
		for (cn = restart_cn; cn < ct->num_con; cn++)
			if (sdesc[cn]->uc_num >= econlist[en]->uc_num) break;

		for (; en < exp_num_con; en++)
			if (econlist[en]->uc_num >= sdesc[cn]->uc_num) break;

		if (econlist[en]->uc_num != sdesc[cn]->uc_num) continue;
		restart_cn = cn+1;

		for (; cn < ct->num_con; cn++)
		{
			if (econlist[en]->uc_num != sdesc[cn]->uc_num) break;

			if (easy_match(econlist[en]->string, sdesc[cn]->string))
				sdesc[cn]->length_limit = UNLIMITED_LEN;
		}
	}
}

/* ======================================================== */

/**
 * Calculate fixed connector information that only depend on its string.
 * This information is used to speed up the parsing stage.
 * It is calculated during the directory creation and doesn't get
 * changed afterward.
 */
void calculate_connector_info(condesc_t * c)
{
	const char *s;
	unsigned int i;

	c->str_hash = connector_str_hash(c->string);

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
	s = c->string;
	if (islower((int) *s)) s++; /* ignore head-dependent indicator */
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
	s = c->string;
	if (islower((int) *s)) s++; /* ignore head-dependent indicator */
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
	s = c->string;
	if (islower((int) *s)) s++; /* ignore head-dependent indicator */
	c->uc_start = s - c->string;
	while (isupper((int) *s))
	{
		i = *s + (i << 6) + (i << 16) - i;
		s++;
	}
#endif /* USE_SDBM */

	c->lc_start = ('\0' == *s) ? 0 : s - c->string;
	c->uc_length = s - c->string - c->uc_start;
	c->uc_hash = i;
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

	if (verbosity_level(D_SPEC+1))
	{
		for (n = 0; n < dict->contable.num_con; n++)
		{
			printf("%5zu %5d constring=%s\n",
			       n, sdesc[n]->uc_num, sdesc[n]->string);
		}
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
