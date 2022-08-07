/*************************************************************************/
/* Copyright 2013, 2014 Linas Vepstas                                    */
/* Copyright 2014, 2015 Amir Plivatsky                                   */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <string.h>

#include "api-types.h"
#include "connectors.h"
#include "dict-affix.h"
#include "dict-api.h"
#include "dict-defines.h"
#include "dict-impl.h"
#include "print/print-util.h"           // patch_subscript_mark
#include "regex-morph.h"
#include "dict-structures.h"
#include "string-set.h"
#include "string-id.h"
#include "utilities.h"

/* ======================================================================= */

// WindowsXP workaround - missing GetLocaleInfoEx
#if _WINVER == 0x501 // XP
int callGetLocaleInfoEx(LPCWSTR lpLocaleName, LCTYPE LCType, LPWSTR lpLCData, int cchData)
{
	int rc = -1;

	// Normal call
	int (WINAPI * pfnGetLocaleInfoEx)(LPCWSTR, LCTYPE, LPWSTR, int);
	*(FARPROC*)&pfnGetLocaleInfoEx = GetProcAddress(GetModuleHandleA("Kernel32"), "GetLocaleInfoEx");
	if (pfnGetLocaleInfoEx)
	{
		rc = pfnGetLocaleInfoEx(lpLocaleName, LCType, lpLCData, cchData);
	}
	else
	{
		// Workaround for missing GetLocaleInfoEx
		HMODULE module = LoadLibraryA("Mlang");
		HRESULT (WINAPI * pfnRfc1766ToLcidW)(LCID*, LPCWSTR);
		*(FARPROC*)&pfnRfc1766ToLcidW = GetProcAddress(module, "Rfc1766ToLcidW");
		if (pfnRfc1766ToLcidW)
		{
			 LCID lcid;
			 if (SUCCEEDED(pfnRfc1766ToLcidW(&lcid, lpLocaleName)))
			 {
				rc = GetLocaleInfoW(lcid, LCType, lpLCData, cchData);
			 }
		}
		FreeLibrary(module);
	}

	return rc;
}
#else
#define callGetLocaleInfoEx GetLocaleInfoEx
#endif // _WINVER == 0x501

/* ======================================================================= */

const char *linkgrammar_get_dict_define(Dictionary dict, const char *name)
{
	unsigned int id = string_id_lookup(name, dict->dfine.set);
	if (id == 0) return NULL;
	return dict->dfine.value[id - 1];
}

/* ======================================================================= */

/**
 * Format the given locale for use in setlocale().
 * POSIX systems and Windows use different conventions.
 * On Windows, convert to full language and territory names, because the
 * short ones don't work for some reason on every system (including MinGW).
 * @param dict Used for putting the returned value in a string-set.
 * @param ll Locale 2-letter language code.
 * @param cc Locale 2-letter territory code.
 * @return The formatted locale, directly usable in setlocale().
 */
static const char * format_locale(Dictionary dict,
                                  const char *ll, const char *cc)
{
	unsigned char *locale_ll = (unsigned char *)strdupa(ll);
	unsigned char *locale_cc = (unsigned char *)strdupa(cc);

	for (unsigned char *p = locale_ll; '\0' != *p; p++) *p = tolower(*p);
	for (unsigned char *p = locale_cc; '\0' != *p; p++) *p = toupper(*p);

#ifdef _WIN32
	const int locale_size = strlen(ll) + 1 + strlen(cc) + 1;
	char *locale = alloca(locale_size);
	snprintf(locale, locale_size, "%s-%s", locale_ll, locale_cc);

	wchar_t wlocale[LOCALE_NAME_MAX_LENGTH];
	wchar_t wtmpbuf[LOCALE_NAME_MAX_LENGTH];
	char tmpbuf[LOCALE_NAME_MAX_LENGTH];
	char locale_buf[LOCALE_NAME_MAX_LENGTH];
	size_t r;

	r = mbstowcs(wlocale, locale, LOCALE_NAME_MAX_LENGTH);
	if ((size_t)-1 == r)
	{
		prt_error("Error: Error converting %s to wide character.\n", locale);
		return NULL;
	}
	wlocale[LOCALE_NAME_MAX_LENGTH-1] = L'\0';

	if (0 >= callGetLocaleInfoEx(wlocale, LOCALE_SENGLISHLANGUAGENAME,
	                         wtmpbuf, LOCALE_NAME_MAX_LENGTH))
	{
		prt_error("Error: GetLocaleInfoEx LOCALE_SENGLISHLANGUAGENAME Locale=%s: \n"
		          "Error %d", locale, (int)GetLastError());
		return NULL;
	}
	r = wcstombs(tmpbuf, wtmpbuf, LOCALE_NAME_MAX_LENGTH);
	if ((size_t)-1 == r)
	{
		prt_error("Error: Error converting locale language from wide character.\n");
		return NULL;
	}
	tmpbuf[LOCALE_NAME_MAX_LENGTH-1] = '\0';
	if (0 == strncmp(tmpbuf, "Unknown", 7))
	{
		prt_error("Error: Unknown territory code in locale \"%s\"\n", locale);
		return NULL;
	}
	strcpy(locale_buf, tmpbuf);
	strcat(locale_buf, "_");

	if (0 >= callGetLocaleInfoEx(wlocale, LOCALE_SENGLISHCOUNTRYNAME,
	                         wtmpbuf, LOCALE_NAME_MAX_LENGTH))
	{
		prt_error("Error: GetLocaleInfoEx LOCALE_SENGLISHCOUNTRYNAME Locale=%s: \n"
		          "Error %d", locale, (int)GetLastError());
		return NULL;
	}
	r = wcstombs(tmpbuf, wtmpbuf, LOCALE_NAME_MAX_LENGTH);
	if ((size_t)-1 == r)
	{
		prt_error("Error: Error converting locale territory from wide character.\n");
		return NULL;
	}
	tmpbuf[LOCALE_NAME_MAX_LENGTH-1] = '\0';
	if (0 == strncmp(tmpbuf, "Unknown", 7))
	{
		prt_error("Error: Unknown territory code in locale \"%s\"\n", locale);
		return NULL;
	}
	locale = strcat(locale_buf, tmpbuf);
#else /* Assuming POSIX */
	const int locale_size = strlen(ll) + 1 + strlen(cc) + sizeof(".UTF-8");
	char *locale = alloca(locale_size);
	snprintf(locale, locale_size, "%s_%s.UTF-8", locale_ll, locale_cc);
#endif

	return string_set_add(locale, dict->string_set);
}

/* ======================================================================= */

/**
 * Return a locale for the given dictionary, in the OS format.
 * - If <dictionary-locale> is defined, use it.
 * - Else use the locale from the environment.
 * - On Windows, if no environment locale use the default locale.
 *
 * Old style dictionary domain definition:
 * <dictionary-locale>: LL4cc+;
 * LL is the ISO639 language code in uppercase,
 * cc is the ISO3166 territory code in lowercase.
 * This particular capitalization is needed for the value to be a
 * valid LG connector.
 * For transliterated dictionaries:
 * <dictionary-locale>: C+;
 *
 * New style dictionary domain definition:
 * #define dictionary-locale ll-CC;
 * #define dictionary-locale C;
 *
 * @param dict The dictionary for which the locale is needed.
 * @return The locale, in a format suitable for use by setlocale().
 */
const char * linkgrammar_get_dict_locale(Dictionary dict)
{
	if (dict->locale) return dict->locale;

	Dict_node *dn = NULL;
	const char *locale =
		linkgrammar_get_dict_define(dict, LG_DICTIONARY_LOCALE);

	if (NULL == locale)
	{
		dn = dict->lookup_list(dict, "<"LG_DICTIONARY_LOCALE">");
		if (NULL == dn)
		{
			lgdebug(D_USER_FILES, "Debug: Dictionary '%s': Locale is not defined.\n",
			        dict->name);
			goto locale_error;
		}
		else
		{
			locale = dn->exp->condesc->string;
		}
	}

	if (0 == strcmp(locale, "C"))
	{
		locale = string_set_add("C", dict->string_set);
	}
	else
	{
		char locale_ll[4], locale_cc[3], c;

		if (NULL == dn)
		{
			int locale_numelement = sscanf(locale, "%3[a-z]_%2[A-Z].UTF-8%c",
			                               locale_ll, locale_cc, &c);
			if (2 != locale_numelement)
			{
				prt_error("Error: "LG_DICTIONARY_LOCALE": \"%s\" "
				          "should be in the form ll_CC.UTF-8\n"
				          "\t(ll: language code; CC: territory code) "
				          "or \"C\" for transliterated dictionaries.\n",
				          locale);
				goto locale_error;
			}
		}
		else
		{
			int locale_numelement = sscanf(locale, "%3[A-Z]4%2[a-z]%c",
			                               locale_ll, locale_cc, &c);
			if (2 != locale_numelement)
			{
				prt_error("Error: <"LG_DICTIONARY_LOCALE">: \"%s\" "
				          "should be in the form LL4cc+\n"
				          "\t(LL: language code; cc: territory code) "
				          "or \"C\" for transliterated dictionaries.\n",
				          locale);
				goto locale_error;
			}
		}

		locale = format_locale(dict, locale_ll, locale_cc);

		if (!try_locale(locale))
		{
			prt_error("Debug: Dictionary \"%s\": Locale \"%s\" unknown\n",
			          dict->name, locale);
			goto locale_error;
		}
	}


	if (NULL != dn) dict->free_lookup(dict, dn);
	lgdebug(D_USER_FILES, "Debug: Dictionary locale: \"%s\"\n", locale);
	dict->locale = locale;
	return locale;

locale_error:
	{
		dict->free_lookup(dict, dn);

		locale = get_default_locale();
		if (NULL == locale) return NULL;
		const char *sslocale = string_set_add(locale, dict->string_set);
		free((void *)locale);
		prt_error("Info: Dictionary '%s': No locale definition - "
		          "\"%s\" will be used.\n", dict->name, sslocale);
		if (!try_locale(sslocale))
		{
			lgdebug(D_USER_FILES, "Debug: Unknown locale \"%s\"...\n", sslocale);
			return NULL;
		}
		return sslocale;
	}
}

/* ======================================================================= */

const char * linkgrammar_get_version(void)
{
	const char *s = "link-grammar-" LINK_VERSION_STRING;
	return s;
}

/* ======================================================================= */

const char * linkgrammar_get_dict_version(Dictionary dict)
{
	if (dict->version) return dict->version;

	const char *version =
		linkgrammar_get_dict_define(dict, LG_DICTIONARY_VERSION_NUMBER);
	if (NULL != version)
	{
		dict->version = version;
		return dict->version;
	}

	/* Original code is left for backward compatibility. */

	char * ver;
	char * p;
	Dict_node *dn;
	Exp *e;

	/* The newer dictionaries should contain a macro of the form:
	 * <dictionary-version-number>: V4v6v6+;
	 * which would indicate dictionary version 4.6.6
	 * Older dictionaries contain no version info.
	 */
	dn = dict->lookup_list(dict, "<"LG_DICTIONARY_VERSION_NUMBER">");
	if (NULL == dn) return "[unknown]";

	e = dn->exp;
	ver = strdup(&e->condesc->string[1]);
	p = strchr(ver, 'v');
	while (p)
	{
		*p = '.';
		p = strchr(p+1, 'v');
	}

	dict->free_lookup(dict, dn);
	dict->version = string_set_add(ver, dict->string_set);
	free(ver);
	return dict->version;
}

double linkgrammar_get_dict_max_disjunct_cost(Dictionary dict)
{
	return dict->default_max_disjunct_cost;
}

/* ======================================================================= */

void dictionary_setup_locale(Dictionary dict)
{
	/* Get the locale for the dictionary. The first one of the
	 * following which exists, is used:
	 * 1. The locale which is defined in the dictionary.
	 * 2. The locale from the environment.
	 * 3. On Windows - the user's default locale.
	 * NULL is returned if the locale is not valid.
	 * Note:
	 * If we don't have locale_t, as a side effect of checking the locale
	 * it is set as the program's locale (as desired).  However, in that
	 * case if it is not valid and this is the first dictionary which is
	 * opened, the program's locale may remain the initial one, i.e. "C"
	 * (unless the API user changed it). */
	dict->locale = linkgrammar_get_dict_locale(dict);

	/* If the program's locale doesn't have a UTF-8 codeset (e.g. it is
	 * "C", or because the API user has set it incorrectly) set it to one
	 * that has it. */
	set_utf8_program_locale();

	/* If the dictionary locale couldn't be established - then set
	 * dict->locale so that it is consistent with the current program's
	 * locale.  It will be used as the intended locale of this
	 * dictionary, and the locale of the compiled regexs. */
	if (NULL == dict->locale)
	{
		dict->locale = setlocale(LC_CTYPE, NULL);
		prt_error("Warning: Couldn't set dictionary locale! "
		          "Using current program locale \"%s\"\n", dict->locale);
	}

	/* setlocale() returns a string owned by the system. Copy it. */
	dict->locale = string_set_add(dict->locale, dict->string_set);

#ifdef HAVE_LOCALE_T
	/* Since linkgrammar_get_dict_locale() (which is called above)
	 * validates the locale, the following call is guaranteed to succeed. */
	dict->lctype = newlocale_LC_CTYPE(dict->locale);

	/* If dict->locale is still not set, there is a bug.
	 * Without this assert(), the program may SEGFAULT when it
	 * uses the isw*() functions. */
	assert((locale_t) 0 != dict->lctype, "Dictionary locale is not set.");
#else
	dict->lctype = 0;
#endif /* HAVE_LOCALE_T */

	/* setlocale() returns a string owned by the system. Copy it. */
	dict->locale = string_set_add(dict->locale, dict->string_set);
}

static bool dictionary_setup_max_disjunct_cost(Dictionary dict)
{
	const char *disjunct_cost_str =
		linkgrammar_get_dict_define(dict, LG_DISJUNCT_COST);
	if (NULL == disjunct_cost_str)
	{
		dict->default_max_disjunct_cost = DEFAULT_MAX_DISJUNCT_COST;
	}
	else
	{
		float disjunct_cost_value;
		if (!strtodC(disjunct_cost_str, &disjunct_cost_value))
		{
			prt_error("Error: %s: Invalid cost \"%s\"", LG_DISJUNCT_COST,
			          disjunct_cost_str);
			return false;
		}
		dict->default_max_disjunct_cost = disjunct_cost_value;
	}

	return true;
}

/**
 * Perform initializations according to definitions in the dictionary.
 * There are 3 kind of definitions:
 * 1. Special expressions.
 * 2. #define name value;
 * 3. Currently not in the dictionary (FIXME).
 *
 * @return \c true on success, \c false on failure.
 */
bool dictionary_setup_defines(Dictionary dict)
{
	dict->left_wall_defined  = dict_has_word(dict, LEFT_WALL_WORD);
	dict->right_wall_defined = dict_has_word(dict, RIGHT_WALL_WORD);

	dict->unknown_word_defined = dict_has_word(dict, UNKNOWN_WORD);
	dict->use_unknown_word = true;

	/* In version 5.5.0 UNKNOWN_WORD has been replaced by <UNKNOWN-WORD>. */
	if (!dict->unknown_word_defined &&
	    dict_has_word(dict, "UNKNOWN-WORD"))
	{
		prt_error("Warning: Old name \"UNKNOWN-WORD\" is defined in the "
		          "dictionary. Please use \"<UNKNOWN-WORD>\" instead.\n");
	}

	dict->shuffle_linkages = false;

	dictionary_setup_locale(dict);

	if (!dictionary_setup_max_disjunct_cost(dict)) return false;

	return true;
}

/* ======================================================================= */

/* The affix dictionary is represented as a dynamically allocated array with
 * an element for each class (connector type) in the affix file. Each element
 * has a pointer to an array of strings which are the punctuation/affix
 * names. */

/** initialize the affix class table */
void afclass_init(Dictionary dict)
{
	const size_t sz = sizeof(*dict->afdict_class) * AFDICT_NUM_ENTRIES;

	dict->afdict_class = malloc(sz);
	memset(dict->afdict_class, 0, sz);
}

/**
 * Find the affix table entry for given connector name.
 * If the connector name is not in the table, return NULL.
 */
Afdict_class * afdict_find(Dictionary afdict, const char * con, bool notify_err)
{
	const char * const * ac;

	for (ac = afdict_classname;
	     ac < &afdict_classname[ARRAY_SIZE(afdict_classname)]; ac++)
	{
		if (0 == strcmp(*ac, con))
			return &afdict->afdict_class[ac - afdict_classname];
	}
	if (notify_err) {
		prt_error("Warning: Unknown class name %s found near line %d of %s.\n"
		          "\tThis class name will be ignored.\n",
		          con, afdict->line_number, afdict->name);
	}
	return NULL;
}

#define AFFIX_COUNT_MEM_INCREMENT 64

void affix_list_add(Dictionary afdict, Afdict_class * ac,
		const char * affix)
{
	if (NULL == ac)  return; /* ignore unknown class name */
	if (ac->mem_elems <= ac->length)
	{
		size_t new_sz;
		ac->mem_elems += AFFIX_COUNT_MEM_INCREMENT;
		new_sz = ac->mem_elems * sizeof(const char *);
		ac->string = (char const **) realloc((void *)ac->string, new_sz);
	}
	ac->string[ac->length] = string_set_add(affix, afdict->string_set);
	ac->length++;
}

#ifdef AFDICT_ORDER_NOT_PRESERVED
static int revcmplen(const void *a, const void *b)
{
	return strlen(*(char * const *)b) - strlen(*(char * const *)a);
}
#endif /* AFDICT_ORDER_NOT_PRESERVED */

/**
 * Traverse the main dict in dictionary order, and extract all the suffixes
 * and prefixes - every time we see a new suffix/prefix (the previous one is
 * remembered by w_last), we save it in the corresponding affix-class list.
 * The saved affixes don't include the infix mark.
 */
static void get_dict_affixes(Dictionary dict, Dict_node * dn,
                             char infix_mark,
                             const char** plast, size_t *lastlen)
{
	const char *w;         /* current dict word */
	const char *w_sm;      /* SUBSCRIPT_MARK position in the dict word */
	size_t w_len;          /* length of the dict word */
	Dictionary afdict = dict->affix_table;

	if (dn == NULL) return;
	get_dict_affixes(dict, dn->right, infix_mark, plast, lastlen);

	w = dn->string;
	w_sm = get_word_subscript(w);
	w_len = (NULL == w_sm) ? strlen(w) : (size_t)(w_sm - w);

	if (*lastlen != w_len || 0 != strncmp(*plast, w, w_len))
	{
		char* wtrunc = strdupa(w);
		wtrunc[w_len] = '\0';

		if (infix_mark == w[0])
		{
			affix_list_add(afdict, &afdict->afdict_class[AFDICT_SUF], wtrunc+1);
		}
		else if (infix_mark == w[w_len-1])
		{
			wtrunc[w_len-1] = '\0';
			affix_list_add(afdict, &afdict->afdict_class[AFDICT_PRE], wtrunc);
		}

		*plast = w;
		*lastlen = w_len;
	}

	get_dict_affixes(dict, dn->left, infix_mark, plast, lastlen);
}

/**
 * Concatenate the definitions for the given affix class.
 * This allows specifying the characters in different definitions
 * instead in a one long string, e.g. instead of:
 * ""«»《》【】『』`„": QUOTES+;
 * One can specify (note the added spaces):
 * """  «»  《》 【】 『』  ` „: QUOTES+;
 * Or even:
 * """: QUOTES+;
 * «» : QUOTES+;
 * etc.
 * Note that if there are no definitions or only one definition, there is
 * nothing to do.
 * The result is written to the first entry.
 * @param classno The given affix class.
 */
static void concat_class(Dictionary afdict, int classno)
{
	Afdict_class * ac;
	size_t i;
	dyn_str * qs;

	ac = AFCLASS(afdict, classno);
	if (1 >= ac->length) return;

	qs = dyn_str_new();
	for (i = 0; i < ac->length; i++)
		dyn_strcat(qs, ac->string[i]);

	ac->string[0] = string_set_add(qs->str, afdict->string_set);
	dyn_str_delete(qs);
}

/**
 * Compare lengths of strings, for affix class qsort.
 * Sort order:
 * 1. Longest base words first.
 * 2. Equal base words one after the other.
 * 3. Regexes last, in file order.
 */
static int split_order(const void *a, const void *b)
{
	const char * const *sa = a;
	const char * const *sb = b;

	size_t len_a = strcspn(*sa, subscript_mark_str());
	size_t len_b = strcspn(*sb, subscript_mark_str());
	bool is_regex_a = (get_affix_regex_cg(*sa) >= 0);
	bool is_regex_b = (get_affix_regex_cg(*sb) >= 0);

	if (is_regex_a && is_regex_b) return 0;
	if (is_regex_a) return 1;
	if (is_regex_b) return -1;

	int len_order = (int)(len_b - len_a);
	if (0 == len_order) return strncmp(*sa, *sb, len_a);

	return len_order;
}

/**
 * Initialize several classes.
 * In case of a future dynamic change of the affix table, this function needs to
 * be invoked again after the affix table is re-constructed (changes may be
 * needed - especially to first free memory and initialize the affix dict
 * structure.).
 */
#define D_AI (D_DICT+1)
bool afdict_init(Dictionary dict)
{
	Afdict_class * ac;
	Dictionary afdict = dict->affix_table;
	bool error_found = false;

	/* FIXME: read_entry() builds word lists in reverse order (can we
	 * just create the list top-down without breaking anything?). Unless
	 * it is fixed to preserve the order, reverse here the word list for
	 * each affix class. */
	for (ac = afdict->afdict_class;
		  ac < &afdict->afdict_class[ARRAY_SIZE(afdict_classname)]; ac++)
	{
		int i;
		int l = ac->length - 1;
		const char * t;

		for (i = 0;  i < l; i++, l--)
		{
			t = ac->string[i];
			ac->string[i] = ac->string[l];
			ac->string[l] = t;
		}
	}

	/* Create the affix lists */
	ac = AFCLASS(afdict, AFDICT_INFIXMARK);
	if ((1 < ac->length) || ((1 == ac->length) && (1 != strlen(ac->string[0]))))
	{
		prt_error("Error: afdict_init: Invalid value for class %s in file %s"
		          " (should have been one ASCII punctuation - ignored)\n",
		          afdict_classname[AFDICT_INFIXMARK], afdict->name);
		free((void *)ac->string);
		ac->length = 0;
		ac->mem_elems = 0;
		ac->string = NULL;
	}
	/* XXX For now there is a possibility to use predefined SUF and PRE lists.
	 * So if SUF or PRE are defined, don't extract any of them from the dict. */
	if (1 == ac->length)
	{
		if ((0 == AFCLASS(afdict, AFDICT_PRE)->length) &&
		    (0 == AFCLASS(afdict, AFDICT_SUF)->length))
		{
			const char *last = 0x0;
			size_t len = 0;
			get_dict_affixes(dict, dict->root, ac->string[0][0], &last, &len);
		}
		else
		{
			afdict->pre_suf_class_exists = true;
		}
	}
	else
	{
		/* No INFIX_MARK - create a dummy one that always mismatches */
		affix_list_add(afdict, &afdict->afdict_class[AFDICT_INFIXMARK], "");
	}

	/* Store the SANEMORPHISM regex in the unused (up to now)
	 * regex_root element of the affix dictionary, and precompile it */
	assert(NULL == afdict->regex_root, "SM regex is already assigned");
	ac = AFCLASS(afdict, AFDICT_SANEMORPHISM);
	if (0 != ac->length)
	{
		Regex_node *sm_re = malloc(sizeof(*sm_re));
		dyn_str *rebuf = dyn_str_new();

		/* The regex used to be converted to: ^((original-regex)b)+$
		 * In the initial wordgraph version word boundaries are not supported,
		 * so instead it is converted to: ^(original-regex)+$ */
#ifdef WORD_BOUNDARIES
		dyn_strcat(rebuf, "^((");
#else
		dyn_strcat(rebuf, "^(");
#endif
		dyn_strcat(rebuf, ac->string[0]);
#ifdef WORD_BOUNDARIES
		dyn_strcat(rebuf, ")b)+$");
#else
		dyn_strcat(rebuf, ")+$");
#endif
		sm_re->pattern = strdup(rebuf->str);
		dyn_str_delete(rebuf);

		afdict->regex_root = sm_re;
		sm_re->name = string_set_add(afdict_classname[AFDICT_SANEMORPHISM],
		                             afdict->string_set);
		sm_re->re = NULL;
		sm_re->next = NULL;
		sm_re->neg = false;
		if (!compile_regexs(afdict->regex_root, afdict))
		{
			prt_error("Error: afdict_init: Failed to compile "
			          "regex /%s/ in file \"%s\"\n",
			          afdict_classname[AFDICT_SANEMORPHISM], afdict->name);
			error_found = true;
		}
		else
		{
			lgdebug(+D_AI, "%s regex %s\n",
			        afdict_classname[AFDICT_SANEMORPHISM], sm_re->pattern);
		}
	}

	if (!IS_DYNAMIC_DICT(dict))
	{
		/* Validate that the strippable tokens are in the dict.
		 * UNITS are assumed to be from the dict only.
		 * Possible FIXME: Allow also tokens that match a regex (a tokenizer
		 * change is needed to recognize them). */
		for (size_t i = 0; i < ARRAY_SIZE(affix_strippable); i++)
		{
			if (AFDICT_UNITS != affix_strippable[i])
			{
				ac = AFCLASS(afdict, affix_strippable[i]);
				bool not_in_dict = false;

				for (size_t n = 0;  n < ac->length; n++)
				{
					if (!dict_has_word(dict, ac->string[n]))
					{
						if (!not_in_dict)
						{
							not_in_dict = true;
							prt_error("Warning: afdict_init: Class %s in file %s: "
							          "Token(s) not in the dictionary:",
							          afdict_classname[affix_strippable[i]],
							          afdict->name);
						}

						char *s = strdupa(ac->string[n]);
						patch_subscript_mark(s);
						prt_error(" \"%s\"", s);
					}
				}
				if (not_in_dict) prt_error("\n");
			}
		}
	}

	/* Sort the affix-classes of tokens to be stripped. */
	/* Longer unit names must get split off before shorter ones.
	 * This prevents single-letter splits from screwing things
	 * up. e.g. split 7gram before 7am before 7m.
	 * Another example: The ellipsis "..." must appear before the dot ".".
	 */
	for (size_t i = 0; i < ARRAY_SIZE(affix_strippable); i++)
	{
		ac = AFCLASS(afdict, affix_strippable[i]);
		if (0 < ac->length)
		{
			qsort(ac->string, ac->length, sizeof(char *), split_order);
		}
	}

#ifdef AFDICT_ORDER_NOT_PRESERVED
	/* pre-sort the MPRE list */
	ac = AFCLASS(afdict, AFDICT_MPRE);
	if (0 < ac->length)
	{
		/* Longer subwords have priority over shorter ones,
		 * reverse-sort by length.
		 * XXX mprefix_split() for Hebrew depends on that. */
		qsort(ac->string, ac->length, sizeof(char *), revcmplen);
	}
#endif /* AFDICT_ORDER_NOT_PRESERVED */

	concat_class(afdict, AFDICT_QUOTES);
	concat_class(afdict, AFDICT_BULLETS);

	if (verbosity_level(D_AI))
	{
		size_t l;

		for (ac = afdict->afdict_class;
		     ac < &afdict->afdict_class[ARRAY_SIZE(afdict_classname)]; ac++)
		{
			if (0 == ac->length) continue;
			lgdebug(+0, "Class %s, %zu items:",
			        afdict_classname[ac-afdict->afdict_class], ac->length);
			for (l = 0; l < ac->length; l++)
				lgdebug(0, " '%s'", ac->string[l]);
			lgdebug(0, "\n\\");
		}
		lg_error_flush();
	}

	return !error_found;
}
#undef D_AI
