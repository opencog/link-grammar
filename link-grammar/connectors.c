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

#include <limits.h>                     // CHAR_BIT

#include "dict-common/dict-utils.h"     // size_of_expression
#include "api-structures.h"             // Parse_Options_s
#include "connectors.h"
#include "link-includes.h"              // Parse_Options

#define WILD_TYPE '*'
#define LENGTH_LINIT_WILD_TYPE WILD_TYPE

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
		free(e);
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

Connector * connector_new(Pool_desc *connector_pool, const condesc_t *desc,
                          Parse_Options opts)
{
	Connector *c;

	if (NULL == connector_pool) /* For the SAT-parser, until fixed. */
	{
		c = malloc(sizeof(Connector));
		memset(c, 0, sizeof(Connector));
	}
	else
		c = pool_alloc(connector_pool); /* Memory-pool has zero_out attribute.*/

	c->desc = desc;
	set_connector_length_limit(c, opts);
	//assert(0 != c->length_limit, "Connector_new(): Zero length_limit");

	return c;
}

/* ======================================================== */
/* Connector length limit handling - UNLIMITED-CONNECTORS and LENGTH-LIMIT-n. */

static size_t get_connectors_from_expression(condesc_t **conlist, const Exp *e)
{
	if (e->type == CONNECTOR_type)
	{
		if (NULL != conlist) *conlist = e->condesc;
		return 1;
	}

	size_t cl_size = 0;
	for (Exp *opd = e->operand_first; opd != NULL; opd = opd->operand_next)
	{
		cl_size += get_connectors_from_expression(conlist, opd);
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

/**
 * Set the length limit of all the connectors that match those in e.
 * XXX A connector in e that doesn't match any other connector cannot
 * be detected, because it has been inserted into the connector table and
 * hence matches at least itself.
 */
static void set_condesc_length_limit(Dictionary dict, const Exp *e, int length_limit)
{
	size_t exp_num_con;
	ConTable *ct = &dict->contable;
	condesc_t **sdesc = ct->sdesc;
	condesc_t **econlist;

	/* Create a connector list from the given expression. */
	exp_num_con = get_connectors_from_expression(NULL, e);
	if (0 == exp_num_con) return; /* Empty connector list. */
	econlist = alloca(exp_num_con * sizeof(*econlist));
	get_connectors_from_expression(econlist, e);

	qsort(econlist, exp_num_con, sizeof(*econlist), condesc_by_uc_num);

	/* Scan the expression connector list and set length_limit.
	 * restart_cn is needed because several connectors in this list
	 * may match a given uppercase part. */
	size_t restart_cn = 0, cn, en;
	for (en = 0; en < exp_num_con; en++)
	{
		for (cn = restart_cn; cn < ct->num_con; cn++)
			if (sdesc[cn]->uc_num >= econlist[en]->uc_num) break;

		for (; en < exp_num_con; en++)
			if (econlist[en]->uc_num >= sdesc[cn]->uc_num) break;
		if (en == exp_num_con) break;

		if (econlist[en]->uc_num != sdesc[cn]->uc_num) continue;
		restart_cn = cn;

		const char *wc_str = econlist[en]->string;
		char *uc_wildcard = strchr(wc_str, LENGTH_LINIT_WILD_TYPE);

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

static void condesc_length_limit_def_delete(ConTable *ct)
{
	length_limit_def_t *l_next;

	for (length_limit_def_t *l = ct->length_limit_def; NULL != l; l = l_next)
	{
		l_next = l->next;
		free(l);
	}
	ct->length_limit_def = NULL;
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

	condesc_length_limit_def_delete(&dict->contable);

	if (verbosity_level(D_SPEC+1))
	{
		prt_error("Debug:\n%5s %-6s %3s\n\\", "num", "uc_num", "ll");
		for (size_t n = 0; n < ct->num_con; n++)
		{
			prt_error("%5zu %6u %3d %s\n\\", n, ct->sdesc[n]->uc_num,
			       ct->sdesc[n]->length_limit, ct->sdesc[n]->string);
		}
		prt_error("\n");
	}
}

/* ======================================================== */

/**
 * Pack the LC part of a connector into 64 bits, and compute a wild-card mask.
 * Up to 9 characters can be so packed.
 *
 * Because we pack by shifts, we can do it using 7-bit per original
 * character at the same overhead needed for 8-bit packing.
 *
 * Note: The LC part may consist of chars in the range [a-z0-9]
 * (total 36) so a 6-bit packing is possible (by abs(value-60) on each
 * character value).
 */
static void connector_encode_lc(const char *lc_string, condesc_t *desc)
{
	lc_enc_t lc_mask = 0;
	lc_enc_t lc_value = 0;
	lc_enc_t wildcard = LC_MASK;
	const char *s;

	for (s = lc_string; '\0' != *s; s++)
	{
		if (*s != WILD_TYPE)
		{
			lc_value |= (lc_enc_t)(*s & LC_MASK) << ((s-lc_string)*LC_BITS);
			lc_mask |= wildcard;
		}
		wildcard <<= LC_BITS;
	};

	/* FIXME: Check on dict read. */
	if ((size_t)(s-lc_string) > (sizeof(lc_value)/LC_BITS)*CHAR_BIT)
	{
		prt_error("Warning: Lower-case part '%s' is too long (%d>%d)\n",
					 lc_string, (int)(s-lc_string), MAX_CONNECTOR_LC_LENGTH);
	}

	desc->lc_mask = (lc_mask << 1) + !!(desc->flags & CD_HEAD_DEPENDENT);
	desc->lc_letters = (lc_value << 1) + !!(desc->flags & CD_HEAD);
}

/**
 * Calculate fixed connector information that only depend on its string.
 * This information is used to speed up the parsing stage. It is
 * calculated during the directory creation and doesn't change afterward.
 */
static void calculate_connector_info(condesc_t * c)
{
	const char *s;

	s = c->string;
	if (islower(*s)) s++; /* Ignore head-dependent indicator. */
	if ((c->string[0] == 'h') || (c->string[0] == 'd'))
		c->flags |= CD_HEAD_DEPENDENT;
	if ((c->flags & CD_HEAD_DEPENDENT) && (c->string[0] == 'h'))
		c->flags |= CD_HEAD;

	c->uc_start = (uint8_t)(s - c->string);
	while (isupper(*++s)) {} /* Skip the uppercase part. */
	c->uc_length = (uint8_t)(s - c->string - c->uc_start);

	connector_encode_lc(s, c);
}

/* ================= Connector descriptor table. ====================== */

static uint32_t connector_str_hash(const char *s)
{
	/* From an old-code comment:
	 * For most situations, all three hashes are very nearly equal.
	 * For both English and Russian, there are about 100 pre-defined
	 * connectors, and another 2K-4K autogen'ed ones (the IDxxx idiom
	 * connectors, and the LLxxx suffix connectors for Russian). */
#ifdef USE_DJB2
	/* djb2 hash. */
	uint32_t i = 5381;
	while (isupper(*s)) /* Connector tables cannot contain UTF8. */
	{
		i = ((i << 5) + i) + *s;
		s++;
	}
	i += i>>14;
#endif /* USE_DJB2 */

#define USE_JENKINS
#ifdef USE_JENKINS
	/* Jenkins one-at-a-time hash. */
	uint32_t i = 0;
	while (isupper(*s)) /* Connector tables cannot contain UTF8. */
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
	/* sdbm hash. */
	uint32_t i = 0;
	c->uc_start = s - c->string;
	while (isupper(*s))
	{
		i = *s + (i << 6) + (i << 16) - i;
		s++;
	}
#endif /* USE_SDBM */

	return i;
}

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
 * It can later serve as a table index, as if it was a perfect hash.
 */
bool sort_condesc_by_uc_constring(Dictionary dict)
{
	if ((0 == dict->contable.num_con) && !IS_DB_DICT(dict))
	{
		prt_error("Error: Dictionary %s: No connectors found.\n", dict->name);
		return false;
	}

	/* An SQL dict without <UNKNOWN-WORD> may have 0 connectors here. */
	if (0 == dict->contable.num_con)
		return true;

	condesc_t **sdesc = malloc(dict->contable.num_con * sizeof(condesc_t *));
	size_t i = 0;
	for (size_t n = 0; n < dict->contable.size; n++)
	{
		condesc_t *condesc = dict->contable.hdesc[n].desc;

		if (NULL == condesc) continue;
		calculate_connector_info(condesc);
		sdesc[i++] = dict->contable.hdesc[n].desc;
	}

	qsort(sdesc, dict->contable.num_con, sizeof(*dict->contable.sdesc),
	      condesc_by_uc_constring);

	/* Enumerate the connectors according to their UC part. */
	int uc_num = 0;

	sdesc[0]->uc_num = uc_num;
	for (size_t n = 1; n < dict->contable.num_con; n++)
	{
		condesc_t **condesc = &sdesc[n];

		if (condesc[0]->uc_length != condesc[-1]->uc_length)

		{
			/* We know that the UC part has been changed. */
			uc_num++;
		}
		else
		{
			const char *uc1 = &condesc[0]->string[condesc[0]->uc_start];
			const char *uc2 = &condesc[-1]->string[condesc[-1]->uc_start];
			if (0 != strncmp(uc1, uc2, condesc[0]->uc_length))
			{
				uc_num++;
			}
		}

		//printf("%5d constring=%s\n", uc_num, condesc[0]->string);
		condesc[0]->uc_num = uc_num;
	}

	lgdebug(+11, "Dictionary %s: %zu different connectors "
	        "(%d with a different UC part)\n",
	        dict->name, dict->contable.num_con, uc_num+1);

	dict->contable.sdesc = sdesc;
	dict->contable.num_uc = uc_num + 1;

	/* hdesc is not freed here because it is needed for finding ZZZ.
	 * It could be freed here if we have ZZZ cached in the dict structure. */
	return true;
}

void condesc_delete(Dictionary dict)
{
	ConTable *ct = &dict->contable;

	free(ct->hdesc);
	pool_delete(ct->mempool);
	condesc_length_limit_def_delete(ct);
}

void condesc_reuse(Dictionary dict)
{
	ConTable *ct = &dict->contable;

	ct->num_con = 0;
	ct->num_uc = 0;
	memset(ct->hdesc, 0, ct->size * sizeof(hdesc_t));
	pool_reuse(ct->mempool);
}

static hdesc_t *condesc_find(ConTable *ct, const char *constring, uint32_t hash)
{
	uint32_t i = hash & (ct->size-1);

	while ((NULL != ct->hdesc[i].desc) &&
	       !string_set_cmp(constring, ct->hdesc[i].desc->string))
	{
		i = (i + 1) & (ct->size-1);
	}

	return &ct->hdesc[i];
}

static void condesc_table_alloc(ConTable *ct, size_t size)
{
	ct->hdesc = malloc(size * sizeof(hdesc_t));
	memset(ct->hdesc, 0, size * sizeof(hdesc_t));
	ct->size = size;
}

#define CONDESC_TABLE_GROWTH_FACTOR 2

static bool condesc_grow(ConTable *ct)
{
	size_t old_size = ct->size;
	hdesc_t *old_hdesc = ct->hdesc;

	lgdebug(+11, "Growing ConTable from %zu\n", old_size);
	condesc_table_alloc(ct, ct->size * CONDESC_TABLE_GROWTH_FACTOR);

	for (size_t i = 0; i < old_size; i++)
	{
		hdesc_t *old_h = &old_hdesc[i];
		if (NULL == old_h->desc) continue;
		hdesc_t *new_h = condesc_find(ct, old_h->desc->string, old_h->str_hash);

		if (NULL != new_h->desc)
		{
			prt_error("Fatal Error: condesc_grow(): Internal error\n");
			free(old_hdesc);
			return false;
		}
		*new_h = *old_h;
	}

	free(old_hdesc);
	return true;
}

condesc_t *condesc_add(ConTable *ct, const char *constring)
{
	uint32_t hash = (connector_hash_t)connector_str_hash(constring);
	hdesc_t *h = condesc_find(ct, constring, hash);

	if (NULL == h->desc)
	{
		assert(0 == ct->num_uc, "Trying to add a connector (%s) "
				 "after reading the dict.\n", constring);
		lgdebug(+11, "Creating connector '%s' (%zu)\n", constring, ct->num_con);
		h->desc = pool_alloc(ct->mempool);
		h->desc->string = constring;
		h->str_hash = hash;
		ct->num_con++;

		if ((8 * ct->num_con) > (3 * ct->size))
		{
			if (!condesc_grow(ct)) return NULL;
			h = condesc_find(ct, constring, hash);
		}
	}

	return h->desc;
}

void condesc_init(Dictionary dict, size_t num_con)
{
	ConTable *ct = &dict->contable;

	condesc_table_alloc(ct, num_con);
	ct->mempool = pool_new(__func__, "ConTable",
								  /*num_elements*/1024, sizeof(condesc_t),
								  /*zero_out*/true, /*align*/true, /*exact*/false);

	ct->length_limit_def = NULL;
	ct->length_limit_def_next = &ct->length_limit_def;
}

void condesc_setup(Dictionary dict)
{
	sort_condesc_by_uc_constring(dict);
	set_all_condesc_length_limit(dict);
	free(dict->contable.sdesc);
}
/* ========================= END OF FILE ============================== */
