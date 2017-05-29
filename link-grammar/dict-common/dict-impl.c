
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

#include "api-structures.h"
#include "dict-structures.h"
#include "string-set.h"
#include "utilities.h"
#include "dict-file/read-dict.h" // XXXX

// WindowsXP workaround - missing GetLocaleInfoEx
#ifdef _WIN32
int callGetLocaleInfoEx(LPCWSTR lpLocaleName, LCTYPE LCType, LPWSTR lpLCData, int cchData)
{
    int rc = -1;

    // Normal call
    int (WINAPI * pfnGetLocaleInfoEx)(LPCWSTR, LCTYPE, LPWSTR, int);
    *(FARPROC*)&pfnGetLocaleInfoEx = GetProcAddress(GetModuleHandleA("Kernel32" ), "GetLocaleInfoEx" );
    if (pfnGetLocaleInfoEx)
    {
        rc = pfnGetLocaleInfoEx(lpLocaleName, LCType, lpLCData, cchData);
    }
    else
    {
        // Workaround for missing GetLocaleInfoEx
        HMODULE module = LoadLibraryA("Mlang");
        HRESULT (WINAPI * pfnRfc1766ToLcidW)(LCID*, LPCWSTR);
        *(FARPROC*)&pfnRfc1766ToLcidW = GetProcAddress(module, "Rfc1766ToLcidW" );
        if  (pfnRfc1766ToLcidW)
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
#endif //_WIN32


/**
 * Format the given locale for use in setlocale().
 * POSIX systems and Windows use different conventions.
 * On Windows, convert to full language and territory names, because the
 * short ones doesn't work for some reason on every system (including MinGW).
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

/**
 * Return a locale for the given dictionary, in the OS format.
 * - If <dictionary-locale> is defined, use it.
 * - Else use the locale from the environment.
 * - On Windows, if no environment locale use the default locale.
 *
 * <dictionary-locale>: LL4cc+;
 * LL is the ISO639 language code in uppercase,
 * cc is the ISO3166 territory code in lowercase.
 * This particular capitalization is needed for the value to be a
 * valid LG connector.
 * For transliterated dictionaries:
 * <dictionary-locale>: C+;
 *
 * @param dict The dictionary for which the locale is needed.
 * @return The locale, in a format suitable for use by setlocale().
 */
const char * linkgrammar_get_dict_locale(Dictionary dict)
{
	if (dict->locale) return dict->locale;

	const char *locale;
	Dict_node *dn = lookup_list(dict, "<dictionary-locale>");

	if (NULL == dn)
	{
		lgdebug(D_USER_FILES, "Debug: Dictionary '%s': Locale is not defined.\n",
		        dict->name);
		goto locale_error;
	}

	if (0 == strcmp(dn->exp->u.string, "C"))
	{
		locale = string_set_add("C", dict->string_set);
	}
	else
	{
		char c;
		char locale_ll[4], locale_cc[3];
		int locale_numelement = sscanf(dn->exp->u.string, "%3[A-Z]4%2[a-z]%c",
										locale_ll, locale_cc, &c);
		if (2 != locale_numelement)
		{
			prt_error("Error: \"<dictionary-locale>: %s\" "
			          "should be in the form LL4cc+\n"
						 "\t(LL: language code; cc: territory code) "
						 "\tor C+ for transliterated dictionaries.\n",
						 dn->exp->u.string);
			goto locale_error;
		}

		locale = format_locale(dict, locale_ll, locale_cc);

		if (!try_locale(locale))
		{
			prt_error("Debug: Dictionary \"%s\": Locale \"%s\" unknown\n",
			          dict->name, locale);
			goto locale_error;
		}
	}

	free_lookup(dn);
	lgdebug(D_USER_FILES, "Debug: Dictionary locale: \"%s\"\n", locale);
	dict->locale = locale;
	return locale;

locale_error:
	{
		free_lookup(dn);

		const char *locale = get_default_locale();
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

const char * linkgrammar_get_version(void)
{
	const char *s = "link-grammar-" LINK_VERSION_STRING;
	return s;
}

const char * linkgrammar_get_dict_version(Dictionary dict)
{
	char * ver;
	char * p;
	Dict_node *dn;
	Exp *e;

	if (dict->version) return dict->version;

	/* The newer dictionaries should contain a macro of the form:
	 * <dictionary-version-number>: V4v6v6+;
	 * which would indicate dictionary version 4.6.6
	 * Older dictionaries contain no version info.
	 */
	dn = lookup_list(dict, "<dictionary-version-number>");
	if (NULL == dn) return "[unknown]";

	e = dn->exp;
	ver = strdup(&e->u.string[1]);
	p = strchr(ver, 'v');
	while (p)
	{
		*p = '.';
		p = strchr(p+1, 'v');
	}

	free_lookup(dn);
	dict->version = string_set_add(ver, dict->string_set);
	free(ver);
	return dict->version;
}

/* ======================================================================= */
