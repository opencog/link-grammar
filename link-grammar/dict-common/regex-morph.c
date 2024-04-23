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

#if HAVE_THREADS_H && !__EMSCRIPTEN__
#include <threads.h>
#endif // HAVE_THREADS_H && !__EMSCRIPTEN__

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

/*
 * re_code     - compiled regex.
 * re_md       - match data.
 */

#if HAVE_PCRE2_H
typedef struct {
	pcre2_code *re_code;
} reg_info;
#endif

#if HAVE_REGEX_H
typedef struct {
	regex_t re_code;
} reg_info;
#endif

#if USE_CXXREGEX
typedef struct {
	std::regex *re_code;
} reg_info;
#endif

#define ERRBUFFLEN 120

#define MAX_CAPTURE_GROUPS 10

/* Define reg_comp(), reg_match() and reg_free(), and reg_span()
 * for the selected regex implementation. */

/* ========================================================================= */

#if HAVE_REGEX_H

#define ALLOCTE_MATCH_DATA(var) \
	regmatch_t mem_##var[MAX_CAPTURE_GROUPS]; regmatch_t *var = mem_##var

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
	int options = REG_EXTENDED|REG_ENHANCED|REG_GNU|(nosub ? REG_NOSUB : 0);
	reg_info *re = malloc(sizeof(reg_info));
	memset(re, 0, sizeof(reg_info));

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

static bool reg_match(const char *s, const Regex_node *rn, regmatch_t *re_md)
{
	reg_info *re = rn->re;
	int rc = regexec(&re->re_code, s, MAX_CAPTURE_GROUPS, re_md, /*eflags*/0);

	if (rc == REG_NOMATCH) return false;
	if (rc == 0) return true;

	/* We have an error. */
	char errbuf[ERRBUFFLEN];
	regerror(rc, &re->re_code, errbuf, ERRBUFFLEN);
	prt_error("Error: Regex matching error: \"%s\" (pattern \"%s\"): %s "
	          "(code %d)\n", rn->name, rn->pattern, errbuf, rc);
	return false;
}

static void reg_span(Regex_node *rn, int *start, int *end, regmatch_t *re_md)
{
	int cgn = rn->capture_group;

	if (unlikely(cgn > MAX_CAPTURE_GROUPS))
	{
		*start = *end = -1;
	}
	else
	{
		*start = re_md[cgn].rm_so;
		*end = re_md[cgn].rm_eo;
	}
}

static void reg_free(Regex_node *rn)
{
	reg_info *re = rn->re;

	regfree(&re->re_code);
	free(re);
	rn->re = NULL;
}
#endif // HAVE_REGEX_H

/* ========================================================================= */
#if HAVE_PCRE2_H

#if HAVE_THREADS_H && !__EMSCRIPTEN__
static once_flag call_once_flag = ONCE_FLAG_INIT;
static tss_t re_md_key;
#endif // HAVE_THREADS_H && !__EMSCRIPTEN__

static void alloc_key(void)
{
	int rc = tss_create(&re_md_key, (tss_dtor_t) pcre2_match_data_free);
	if (thrd_success == rc) return;
	prt_error("Error: pcre2 alloc per-thread key failed\n");
}

static pcre2_match_data* alloc_match_data(void)
{
#if HAVE_THREADS_H && !__EMSCRIPTEN__
	call_once(&call_once_flag, alloc_key);

	pcre2_match_data *pmd = (pcre2_match_data *) tss_get(re_md_key);
	if (pmd) return pmd;

	pmd = pcre2_match_data_create(MAX_CAPTURE_GROUPS, NULL);
	tss_set(re_md_key, pmd);
	if (pmd) return pmd;
#else

	// No threads. Malloc once, forever, and we are done.
	static pcre2_match_data *pmd = NULL;
	if (pmd) return pmd;
	pmd = pcre2_match_data_create(MAX_CAPTURE_GROUPS, NULL);
	if (pmd) return pmd;

#endif // HAVE_THREADS_H && !__EMSCRIPTEN__
	prt_error("Error: pcre2_match_data_create() failed\n");
	return NULL;
}

#define ALLOCTE_MATCH_DATA(var) pcre2_match_data *var = alloc_match_data();

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
		// JIT is not used due to a significant slowdown. (Strings too short?)
		//pcre2_jit_compile(re->re_code, PCRE2_JIT_COMPLETE);
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

static int reg_match(const char *s, const Regex_node *rn, pcre2_match_data *re_md)
{
	reg_info *re = rn->re;

	int rc = pcre2_match(re->re_code, (PCRE2_SPTR)s,
	                     PCRE2_ZERO_TERMINATED, /*startoffset*/0,
	                     PCRE2_NO_UTF_CHECK, re_md, NULL);
	if (rc == PCRE2_ERROR_NOMATCH) return false;
	if (rc >= 0) return true;

	/* We have an error. */
	PCRE2_UCHAR errbuf[ERRBUFFLEN];
	pcre2_get_error_message(rc, errbuf, ERRBUFFLEN);
	prt_error("Error: Regex matching error: \"%s\" (pattern \"%s\"): %s "
	          "(code %d)\n", rn->name, rn->pattern, errbuf, rc);
	return false;
}

static void reg_span(Regex_node *rn, int *start, int *end, pcre2_match_data *re_md)
{
	int cgn = rn->capture_group;

	if (unlikely(cgn >= MAX_CAPTURE_GROUPS))
	{
		*start = *end = -1;
	}
	else
	{
		PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(re_md);
		*start = (int)ovector[2*cgn];
		*end = (int)ovector[2*cgn + 1];
	}
}

static void reg_free(Regex_node *rn)
{
	reg_info *re = rn->re;
	pcre2_code_free(re->re_code);
	free(re);
	rn->re = NULL;
}
#endif // HAVE_PCRE2_H

/* ========================================================================= */
#if USE_CXXREGEX

#define ALLOCTE_MATCH_DATA(var) \
	std::cmatch mem_##var; std::cmatch *var = &mem_##var

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
		delete (reg_info *)rn->re;
		return false;
	}

	return true;
}

static bool reg_match(const char *s, const Regex_node *rn, std::cmatch *re_md)
{
	/* "nosub" not used, as no time difference found w/o using re_md. */
	bool match = false;
	reg_info *re = (reg_info *)rn->re;

	try
	{
		match = std::regex_search(s, *re_md, *re->re_code);
	}
	catch (const std::regex_error& e)
	{
		prt_error("Error: Regex matching error \"%s\" (pattern \"%s\"): %s "
		          "(code %d)\n", rn->pattern, rn->name, e.what(), e.code());
		match = false;
	}

	return match;
}

static void reg_span(Regex_node *rn, int *start, int *end, std::cmatch *re_md)
{
	int cgn = rn->capture_group;

	if (unlikely(cgn >= (int)re_md->size()))
	{
		*start = *end = -1;
	}
	else
	{
		*start = (int)re_md->position(cgn);
		*end = *start + (int)re_md->length(cgn);
	}
}

static void reg_free(Regex_node *rn)
{
	reg_info *re = (reg_info *)rn->re;

	delete re->re_code;
	delete re;
	rn->re = NULL;
}
#endif // USE_CXXREGEX

/**
 * Check the specified capture group of the pattern (if any).
 * Return true if no capture group specified or if it is valid,
 * and -1 on error.
 *
 * Algo: Append the specified capture group specification to the pattern
 * and compile it. If it doesn't exist in the pattern, an error will
 * occur.
 */
static bool check_capture_group(const Regex_node *rn)
{
	if (rn->capture_group <= 0) return true;
	assert(rn->capture_group < MAX_CAPTURE_GROUPS,
		"Bogus capture group %d", rn->capture_group);

	Regex_node check_cg = *rn;
	const size_t pattern_len = strlen(rn->pattern);

	check_cg.pattern = (char *)alloca(pattern_len + 2/* \N */ + 1/* \0 */);
	strcpy(check_cg.pattern, rn->pattern);

	check_cg.pattern[pattern_len] = '\\';
	check_cg.pattern[pattern_len + 1] = '0' + (char)rn->capture_group;
	check_cg.pattern[pattern_len + 2] = '\0';

	bool rc = reg_comp(&check_cg);
	if (rc) reg_free(&check_cg);

	return rc;
}

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
			if (!check_capture_group(rn)) return false;

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
	ALLOCTE_MATCH_DATA(re_md);

	while (rn != NULL)
	{
		if (rn->re == NULL) continue; // Make sure the regex has been compiled.

		if (reg_match(s, rn, re_md))
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

/**
 * Like match_regex(), but also return the match offsets.
 * If there is no match, \p start and \c end remain the same.
 */
const char *matchspan_regex(Regex_node *rn, const char *s,
                            int *start, int *end)
{
	assert(rn->capture_group >= 0, "No capture");
	ALLOCTE_MATCH_DATA(re_md);

	while (rn != NULL)
	{
		if (rn->re == NULL) continue; // Make sure the regex has been compiled.

		if (reg_match(s, rn, re_md))
		{
			lgdebug(+D_MRE, "%s%s %s\n", &"!"[!rn->neg], rn->name, s);
			if (!rn->neg)
			{
				reg_span(rn, start, end, re_md);
				lgdebug(+D_MRE, " [%d, %d)\n", *start, *end);

				if (unlikely(*start == -1))
				{
					lgdebug(+D_USER_INFO, "Regex \"%s\": "
					        "Specified capture group %d didn't match \"%s\"\n",
					        rn->name, rn->capture_group, s);
					return NULL;
				}
				return rn->name; /* Match found - return--no multiple matches. */
			}

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
