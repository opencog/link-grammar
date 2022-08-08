/*************************************************************************/
/* Copyright (c) 2005  Sampo Pyysalo                                     */
/* Copyright (c) 2009 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

/**
 * Support for the regular-expression based token matching system
 * using standard POSIX regex, PCRE2 or C++.
 */

#if HAVE_REGEX_H
#if HAVE_PCRE2_H || USE_CXXREGEX
#error HAVE_REGEX_H: Another regex implementation is already defined..
#endif
/* On MS Windows, regex.h fails to pull in size_t, so work around this by
 * including <stddef.h> before <regex.h> (<sys/types.h> is not enough) */
#include <stddef.h>
#include <regex.h>
#elif HAVE_PCRE2_H
#if USE_CXXREGEX
#error HAVE_REGEX_H: USE_CXXREGEX is already defined.
#endif
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#elif HAVE_TRE_REGEX_H
#include <tre/regex.h>
#define HAVE_REGEX_H 1 // Use the same code below.
#elif USE_CXXREGEX
#include <regex>
#else
#error Undefined regex implementation.
#endif

#include "link-includes.h"
LINK_BEGIN_DECLS
#include "api-structures.h"
#include "error.h"                      // lgdebug prt_error
#include "dict-common/dict-api.h"
#include "dict-common/dict-common.h"
#include "dict-common/regex-morph.h"
LINK_END_DECLS

/* ovector     - returned start and end offsets.
 * re_code     - compiled regex.
 * re_md       - match data.
 */

#if HAVE_PCRE2_H
typedef struct {
	pcre2_code *re_code;
	pcre2_match_data* re_md;
} reg_info;
#endif

#if HAVE_REGEX_H
typedef struct {
	regex_t re_code;
	unsigned int Nre_md;
	regmatch_t re_md[];
} reg_info;
#endif

#if USE_CXXREGEX
typedef struct {
	std::regex *re_code;
	std::cmatch re_md;
} reg_info;
#endif

#define ERRBUFFLEN 120

/* Define reg_comp(), reg_match() and reg_free(), and reg_span()
 * for the selected regex implementation. */

/* ========================================================================= */

#if HAVE_REGEX_H
/**
 * Find an upper limit to the number of capture groups in the pattern.
 */
static unsigned int max_capture_groups(const Regex_node *rn)
{
	if (rn->capture_group < 0) return 0;

	int Nov = 1;
	for (const char *p = rn->pattern; *p != '\0'; p++)
		if (*p == '(') Nov++;

	return Nov;
}

/**
 * Compile the given regex..
 * Return \c true on success, \c false otherwise.
 */
static bool reg_comp(Regex_node *rn)
{
#ifndef REG_ENHANCED  // In macOS it supports \w etc.
#define REG_ENHANCED 0
#endif
#ifndef REG_GNU       // In NetBSD it supports \w etc.
#define REG_GNU 0
#endif

	const bool nosub = (rn->capture_group < 0);
	const unsigned int Novector = nosub ? 0 : (max_capture_groups(rn) + 1);
	int options = REG_EXTENDED|REG_ENHANCED|REG_GNU|(nosub ? REG_NOSUB : 0);

	reg_info *re = malloc(sizeof(reg_info) + sizeof(regmatch_t) * 2 * Novector);
	memset(re, 0, sizeof(reg_info)); /* No need to initialize re_md. */

	re->Nre_md = Novector;

	int rc = regcomp(&re->re_code, rn->pattern, options);

	if (rc != 0)
	{
		char errbuf[ERRBUFFLEN];
		regerror(rc, &re->re_code, errbuf, ERRBUFFLEN);
		prt_error("Error: Failed to compile regex: \"%s\" (pattern \"%s\"): %s "
		          "(code %d)\n", rn->name, rn->pattern, errbuf, rc);
		free(re);
		return false;
	}
	rn->re = re;

	return true;
}

static bool reg_match(const char *s, const Regex_node *rn)
{
	reg_info *reinfo = (reg_info *)rn->re;
	int nmatch = (rn->capture_group < 0) ? 0 : reinfo->Nre_md;
	int rc = regexec(&reinfo->re_code, s, nmatch, reinfo->re_md, /*eflags*/0);

	if (rc == REG_NOMATCH) return false;
	if (rc == 0) return true;

	/* We have an error. */
	char errbuf[ERRBUFFLEN];
	regerror(rc, &reinfo->re_code, errbuf, ERRBUFFLEN);
	prt_error("Error: Regex matching error: \"%s\" (pattern \"%s\"): %s "
	          "(code %d)\n", rn->name, rn->pattern, errbuf, rc);
	return false;
}

static void reg_span(Regex_node *rn)
{
	int cgn = rn->capture_group;
	reg_info *reinfo = rn->re;

	if (unlikely(cgn > rn->capture_group))
	{
		rn->ovector[0] = rn->ovector[1] = -1;
	}
	else
	{
		rn->ovector[0] = reinfo->re_md[cgn].rm_so;
		rn->ovector[1] = reinfo->re_md[cgn].rm_eo;
	}
}

static void reg_free(Regex_node *rn)
{
	regex_t *re_code = &((reg_info *)rn->re)->re_code;
	regfree(re_code);
	free(rn->re);
}
#endif // HAVE_REGEX_H

/* ========================================================================= */
#if HAVE_PCRE2_H
static bool reg_comp(Regex_node *rn)
{
	reg_info *re = rn->re = malloc(sizeof(reg_info));
	PCRE2_SIZE erroffset;
	int rc;

	const bool nosub = (rn->capture_group < 0);
	uint32_t options = PCRE2_UTF|PCRE2_UCP| (nosub ? PCRE2_NO_AUTO_CAPTURE : 0);

	re->re_code = pcre2_compile((PCRE2_SPTR)rn->pattern, PCRE2_ZERO_TERMINATED,
	                            options, &rc, &erroffset, NULL);
	if (re->re_code != NULL)
	{
		re->re_md = pcre2_match_data_create_from_pattern(re->re_code, NULL);
		if (re->re_md == NULL)
		{
			prt_error("Error: pcre2_match_data_create failed\n");
			free(re);
			return false;
		}
		return true;
	}

	/* We have an error. */
	PCRE2_UCHAR errbuf[ERRBUFFLEN];
	pcre2_get_error_message(rc, errbuf, ERRBUFFLEN);
	prt_error("Error: Failed to compile regex: \"%s\" (pattern \"%s\": %s "
	          "(code %d) at %d\n",
	          rn->name, rn->pattern, errbuf, rc, (int)erroffset);
	free(re);
	return false;
}

static int reg_match(const char *s, const Regex_node *rn)
{
	reg_info *reinfo = rn->re;

	int rc = pcre2_match(reinfo->re_code, (PCRE2_SPTR)s,
	                     PCRE2_ZERO_TERMINATED, /*startoffset*/0,
	                     PCRE2_NO_UTF_CHECK, reinfo->re_md, NULL);
	if (rc == PCRE2_ERROR_NOMATCH) return false;
	if (rc >= 0) return true;

	/* We have an error. */
	PCRE2_UCHAR errbuf[ERRBUFFLEN];
	pcre2_get_error_message(rc, errbuf, ERRBUFFLEN);
	prt_error("Error: Regex matching error: \"%s\" (pattern \"%s\"): %s "
	          "(code %d)\n", rn->name, rn->pattern, errbuf, rc);
	return false;
}

static void reg_span(Regex_node *rn)
{
	int cgn = rn->capture_group;
	reg_info *reinfo = rn->re;
	PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(reinfo->re_md);

	if (unlikely(cgn >= (int)pcre2_get_ovector_count(reinfo->re_md)))
	{
		rn->ovector[0] = rn->ovector[1] = -1;
	}
	else
	{
		rn->ovector[0] = ovector[2*cgn];
		rn->ovector[1] = ovector[2*cgn + 1];
	}
}

static void reg_free(Regex_node *rn)
{
	reg_info *re = rn->re;

	pcre2_match_data_free(re->re_md);
	pcre2_code_free(re->re_code);
	free(re);
}
#endif // HAVE_PCRE2_H

/* ========================================================================= */
#if USE_CXXREGEX
static bool reg_comp(Regex_node *rn)
{
	rn->re = new reg_info;

	try
	{
		((reg_info *)rn->re)->re_code = new std::regex(rn->pattern);
	}
	catch (const std::regex_error& e)
	{
		prt_error("Error: Failed to compile regex \"%s\" (pattern \"%s\"): %s "
		          "(code %d)\n", rn->pattern, rn->name, e.what(), e.code());
		return false;
	}

	return true;
}

static bool reg_match(const char *s, const Regex_node *rn)
{
	/* "nosub" not used, as no time difference found w/o using reinfo->re_md. */
	bool match;
	reg_info *reinfo = (reg_info *)rn->re;

	try
	{
		match = std::regex_search(s, reinfo->re_md, *reinfo->re_code);
	}
	catch (const std::regex_error& e)
	{
		prt_error("Error: Regex matching error \"%s\" (pattern \"%s\"): %s "
		          "(code %d)\n", rn->pattern, rn->name, e.what(), e.code());
		match = false;
	}

	return match;
}

static void reg_span(Regex_node *rn)
{
	int cgn = rn->capture_group;
	std::cmatch re_md = ((reg_info *)rn->re)->re_md;

	if (unlikely(cgn >= (int)re_md.size()))
	{
		rn->ovector[0] = rn->ovector[1] = -1;
	}
	else
	{
		rn->ovector[0] = re_md.position(cgn);
		rn->ovector[1] = rn->ovector[0] + re_md.length(cgn);
	}
}

static void reg_free(Regex_node *rn)
{
	delete ((reg_info *)rn->re)->re_code;
	delete (reg_info *)rn->re;
}
#endif // USE_CXXREGEX

/* ============================== Internal API ============================= */

/**
 * Compile all the given regexs.
 * @param rn Regex list
 * @param dict Validate that the pattern is in the given dict.
 * @retrun \c true on success, else \c false.
 */
bool compile_regexs(Regex_node *rn, Dictionary dict)
{
	while (rn != NULL)
	{
		/* If rn->re non-null, assume compiled already. */
		if(rn->re == NULL)
		{
			if (!reg_comp(rn))
			{
				rn->re = NULL;
				return false;
			}

			/* Check that the regex name is defined in the dictionary. */
			if ((dict != NULL) && !dict_has_word(dict, rn->name))
			{
				/* TODO: better error handing. Maybe remove the regex? */
				prt_error("Error: Regex name \"%s\" not found in dictionary!\n",
				          rn->name);
			}
		}
		rn = rn->next;
	}
	return true;
}

/**
 * Try to match each regex in turn to word s.
 * On match, return the name of the first matching regex.
 * If no match is found, return NULL.
 */
#define D_MRE 6
const char *match_regex(const Regex_node *rn, const char *s)
{
	while (rn != NULL)
	{
		if (rn->re == NULL) continue; // Make sure the regex has been compiled.

		if (reg_match(s, rn))
		{
			lgdebug(+D_MRE, "%s%s %s\n", &"!"[!rn->neg], rn->name, s);
			if (!rn->neg)
				return rn->name; /* Match found - return--no multiple matches. */

			/* Negative match - skip this regex name. */
			for (const char *nre_name = rn->name; rn->next != NULL; rn = rn->next)
			{
				if (!string_set_cmp(nre_name, rn->next->name)) break;
			}
		}

		rn = rn->next;
	}

	return NULL; /* No matches. */
}
#undef D_MRE

/**
 * Delete associated regex storage.
 */
void free_regexs(Regex_node *rn)
{
	while (rn != NULL)
	{
		Regex_node *next = rn->next;

		if (rn->re != NULL)
		{
			reg_free(rn);
			rn->re = NULL;
		}
		free(rn->pattern);
		free(rn);

		rn = next;
	}
}
