/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2009, 2012 Linas Vepstas                                */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _WIN32
#include <langinfo.h>
#endif
#include <limits.h>

#include "build-disjuncts.h"
#include "error.h"
#include "externs.h"
#include "read-dict.h"
#include "regex-morph.h"
#include "spellcheck.h"
#include "string-set.h"
#include "structures.h"
#include "tokenize.h"
#include "utilities.h"
#include "word-utils.h"

#define MAX_STRIP 10

/* These are no longer in use, but are read from the 4.0.affix file */
/* I've left these here, as an axample of what to expect. */
/*static char * strip_left[] = {"(", "$", "``", NULL}; */
/*static char * strip_right[] = {")", "%", ",", ".", ":", ";", "?", "!", "''", "'", "'s", NULL};*/

#define ENTITY_MARKER   "<marker-entity>"
#define COMMON_ENTITY_MARKER   "<marker-common-entity>"

/**
 * is_common_entity - Return true if word is a common noun or adjective
 * Common nouns and adjectives are typically used in corporate entity
 * names -- e.g. "Sun State Bank" -- "sun", "state" and "bank" are all 
 * common nouns.
 */
static Boolean is_common_entity(Dictionary dict, const char * str)
{
	if (word_contains(dict, str, COMMON_ENTITY_MARKER) == 1)
		return TRUE;
	return FALSE;
}

static Boolean is_entity(Dictionary dict, const char * str)
{
	const char * regex_name;
	if (word_contains(dict, str, ENTITY_MARKER) == 1)
		return TRUE;
	regex_name = match_regex(dict, str);
	if (NULL == regex_name) return FALSE;
	return word_contains(dict, regex_name, ENTITY_MARKER);
}


#if defined HAVE_HUNSPELL || defined HAVE_ASPELL
/**
 * Return TRUE if word is a proper name.
 * XXX This is a cheap hack that works only in English, and is 
 * broken for German!  We need to replace this with something
 * language-specific. 
 *
 * Basically, if word starts with upper-case latter, we assume
 * its a proper name, and that's that.
 */
static Boolean is_proper_name(const char * word)
{
	return is_utf8_upper(word);
}
#endif /* defined HAVE_HUNSPELL || defined HAVE_ASPELL */

/* Create a string containing anything that can be construed to
 * be a quotation mark. This works, because link-grammar is more
 * or less ignorant of quotes at this time.
 */
static const wchar_t* list_of_quotes(void) 
{
#ifdef _MSC_VER
	/* Microsoft Visual Studio understands these unicode chars as
	 * wide chars, so just return them as-is.  At least, this is what
	 * Alexander Tkachuk reports, circa 17 April 2013.
	 */
	static const wchar_t* wqs = "\"«»《》【】『』`„“";
#else
	/* Unix (Linux) and GCC understand the embedded unicode chars
	 * as utf8, so convert them to wide chars before use.
	 */
#define QUSZ 50
	static wchar_t wqs[QUSZ];
	mbstate_t mbs;
	/* Single-quotes are used for abbreviations, don't mess with them */
	/* const char * qs = "\"\'«»《》【】『』‘’`„“"; */
	const char* qs = "\"«»《》【】『』`„“";

	const char* pqs = qs;
	memset(&mbs, 0, sizeof(mbs));
	mbsrtowcs(wqs, &pqs, QUSZ, &mbs);
#endif /* _MSC_VER */
	return wqs;
}

/**
 * Return TRUE if the character is a quotation character.
 */
static Boolean is_quote(wchar_t wc)
{
	static const wchar_t *quotes = NULL;
	if (NULL == quotes) quotes = list_of_quotes();

	if (NULL !=  wcschr(quotes, wc)) return TRUE;
	return FALSE;
}

/**
 * Returns true if the word can be interpreted as a number.
 * The ":" is included here so we allow "10:30" to be a number.
 * We also allow U+00A0 "no-break space"
 */
static Boolean is_number(const char * s)
{
	mbstate_t mbs;
	int nb = 1;
	wchar_t c;
	if (!is_utf8_digit(s)) return FALSE;

	memset(&mbs, 0, sizeof(mbs));
	while ((*s != 0) && (0 < nb))
	{
		nb = mbrtowc(&c, s, MB_CUR_MAX, &mbs);
		if (iswdigit(c)) { s += nb; }

		/* U+00A0 no break space */
		else if (0xa0 == c) { s += nb; }

		else if ((*s == '.') || (*s == ',') || (*s == ':')) { s++; }
		else return FALSE;
	}
	return TRUE;
}

/**
 * Returns true if the word contains digits. 
 */
static Boolean contains_digits(const char * s)
{
	mbstate_t mbs;
	int nb = 1;
	wchar_t c;

	memset(&mbs, 0, sizeof(mbs));
	while ((*s != 0) && (0 < nb))
	{
		nb = mbrtowc(&c, s, MB_CUR_MAX, &mbs);
		if (iswdigit(c)) return TRUE;
		s += nb;
	}
	return FALSE;
}

/** 
 * Make the string 's' be the next word of the sentence. 
 * That is, it looks like 's' is a word we can handle, so record it
 * as a bona-fide word in the sentence.  Increment the sentence length
 * when done.
 *
 * Do not issue the empty string.  
 */
static void issue_sentence_word(Sentence sent, const char * s, Boolean quote_found)
{
	size_t len = sent->length;
	if (*s == '\0') return;

	sent->post_quote = realloc(sent->post_quote, (len+1) * sizeof(Boolean));
	sent->post_quote[len] = quote_found;

	sent->word = realloc(sent->word, (len+1) * sizeof(Word));
	sent->word[len].x = NULL;
	sent->word[len].d = NULL;

	sent->word[len].alternatives = (const char **) malloc(2*sizeof(const char *));
	sent->word[len].alternatives[0] = string_set_add(s, sent->string_set);
	sent->word[len].alternatives[1] = NULL;

	/* Now we record whether the first character of the word is upper-case.
	   (The first character may be made lower-case
	   later, but we may want to get at the original version) */
	if (is_utf8_upper(s)) sent->word[len].firstupper = TRUE;
	else sent->word[len].firstupper = FALSE;

	sent->length++;
}

static size_t altlen(const char **arr)
{
	size_t len = 0;
	if (arr)
		while (arr[len] != NULL) len++;
	return len;
}

static const char ** resize_alts(const char **arr, size_t len)
{
	arr = realloc(arr, (len+2) * sizeof(const char *));
	arr[len+1] = NULL;
	return arr;
}

/**
 * Accumulate different word-stemming possibilities 
 */
static void add_suffix_alternatives(Tokenizer * tok,
                                   const char * stem, const char * suffix)
{
	char buff[MAX_WORD+1];
	size_t sz;
	
	size_t stemlen = altlen(tok->stem_alternatives);
	size_t sufflen = altlen(tok->suff_alternatives);

	tok->stem_alternatives = resize_alts(tok->stem_alternatives, stemlen);
	tok->stem_alternatives[stemlen] = string_set_add(stem, tok->string_set);

	if (NULL == suffix)
		return;

	tok->suff_alternatives = resize_alts(tok->suff_alternatives, sufflen);

	/* Add an equals-sign to the suffix.  This is needed to distinguish
	 * suffixes that were stripped off from ordinary words that just
	 * happen to be the same as the suffix. Kind-of a weird hack,
	 * but I'm not sure what else to do... */
	sz = MIN(strlen(suffix) + 2, MAX_WORD);
	buff[0] = '=';
	strncpy(&buff[1], suffix, sz);
	buff[sz] = 0;

	tok->suff_alternatives[sufflen] = string_set_add(buff, tok->string_set);
}

static void add_presuff_alternatives(Tokenizer * tok, const char * prefix,
                                    const char * stem, const char * suffix)
{
	char buff[MAX_WORD+1];
	size_t sz;

	size_t preflen = altlen(tok->pref_alternatives);
	size_t stemlen = altlen(tok->stem_alternatives);
	size_t sufflen = altlen(tok->suff_alternatives);

	tok->pref_alternatives = resize_alts(tok->pref_alternatives, preflen);
	tok->pref_alternatives[preflen] = string_set_add(prefix, tok->string_set);

	tok->stem_alternatives = resize_alts(tok->stem_alternatives, stemlen);
	tok->stem_alternatives[stemlen] = string_set_add(stem, tok->string_set);

	if (NULL == suffix)
		return;

	tok->suff_alternatives = resize_alts(tok->suff_alternatives, sufflen);

	/* Add an equals-sign to the suffix.  This is needed to distinguish
	 * suffixes that were stripped off from ordinary words that just
	 * happen to be the same as the suffix. Kind-of a weird hack,
	 * but I'm not sure what else to do... */
	sz = MIN(strlen(suffix) + 2, MAX_WORD);
	buff[0] = '=';
	strncpy(&buff[1], suffix, sz);
	buff[sz] = 0;

	tok->suff_alternatives[sufflen] = string_set_add(buff, tok->string_set);
}

/**
 * Same as issue_sentence_word, except that here, we issue multiple
 * alternative prefix-stem-suffix possibilities. Up to three "words"
 * may be issued.
 */
static Boolean issue_alternatives(Sentence sent, Boolean quote_found)
{
	Boolean issued = FALSE;
	Tokenizer *tokenizer = &sent->tokenizer;
	size_t len = sent->length;
	size_t preflen = altlen(tokenizer->pref_alternatives);
	size_t stemlen = altlen(tokenizer->stem_alternatives);
	size_t sufflen = altlen(tokenizer->suff_alternatives);

	if (preflen)
	{
		size_t i;

		sent->post_quote = realloc(sent->post_quote, (len+1) * sizeof(Boolean));
		sent->post_quote[len] = quote_found;

		sent->word = realloc(sent->word, (len+1) * sizeof(Word));
		sent->word[len].x = NULL;
		sent->word[len].d = NULL;

		sent->word[len].alternatives = tokenizer->pref_alternatives;
		tokenizer->pref_alternatives = NULL;

		sent->word[len].firstupper = FALSE;
		for (i=0; NULL != tokenizer->pref_alternatives[i]; i++) {
			const char *s = tokenizer->pref_alternatives[i];
		   if (is_utf8_upper(s)) sent->word[len].firstupper = TRUE;
		}

		len++;
		issued = TRUE;
	}

	if (stemlen)
	{
		sent->post_quote = realloc(sent->post_quote, (len+1) * sizeof(Boolean));
		sent->post_quote[len] = preflen ? FALSE : quote_found;

		sent->word = realloc(sent->word, (len+1) * sizeof(Word));
		sent->word[len].x = NULL;
		sent->word[len].d = NULL;

		sent->word[len].alternatives = tokenizer->stem_alternatives;

		sent->word[len].firstupper = FALSE;
		if (0 == preflen)
		{
			size_t i;
			for (i = 0; NULL != tokenizer->stem_alternatives[i]; i++) {
				const char *s = tokenizer->stem_alternatives[i];
		   	if (is_utf8_upper(s)) sent->word[len].firstupper = TRUE;
			}
		}

		tokenizer->stem_alternatives = NULL;
		len++;
		issued = TRUE;
	}

	if (sufflen)
	{
		sent->post_quote = realloc(sent->post_quote, (len+1) * sizeof(Boolean));
		sent->post_quote[len] = FALSE;

		sent->word = realloc(sent->word, (len+1) * sizeof(Word));
		sent->word[len].x = NULL;
		sent->word[len].d = NULL;

		sent->word[len].alternatives = tokenizer->suff_alternatives;
		tokenizer->suff_alternatives = NULL;

		sent->word[len].firstupper = FALSE;
		len++;
		issued = TRUE;
	}

	sent->length = len;
	return issued;
}

/*
	Here's a summary of how subscripts are handled:

	Reading the dictionary:

	  If the last "." in a string is followed by a non-digit character,
	  then the "." and everything after it is considered to be the subscript
	  of the word.

	  The dictionary reader does not allow you to have two words that
	  match according to the criterion below.  (so you can't have
	  "dog.n" and "dog")

	  Quote marks are used to allow you to define words in the dictionary
	  which would otherwise be considered part of the dictionary, as in

	   ";": {@Xca-} & Xx- & (W+ or Qd+) & {Xx+};
	   "%" : (ND- & {DD-} & <noun-sub-x> &
		   (<noun-main-x> or B*x+)) or (ND- & (OD- or AN+));

	Rules for chopping words from the input sentence:

	   First the prefix chars are stripped off of the word.  These
	   characters are "(" and "$" (and now "``")

	   Now, repeat the following as long as necessary:

		   Look up the word in the dictionary.
		   If it's there, the process terminates.

		   If it's not there and it ends in one of the right strippable
		   strings (see "strip_right") then remove the strippable string
		   and make it into a separate word.

		   If there is no strippable string, then the process terminates.

	Rule for defining subscripts in input words:

	   The subscript rule is followed just as when reading the dictionary.

	When does a word in the sentence match a word in the dictionary?

	   Matching is done as follows: Two words with subscripts must match
	   exactly.  If neither has a subscript they must match exactly.  If one
	   does and one doesn't then they must match when the subscript is
	   removed.  Notice that this is symmetric.

	So, under this system, the dictonary could have the words "Ill" and
	also the word "Ill."  It could also have the word "i.e.", which could be
	used in a sentence.
*/

#undef		MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

/**
 * Split word into prefix, stem and suffix. 
 * Record all of the alternative splittings in the tokenizer
 * alternatives arrays.
 */
static Boolean suffix_split(Tokenizer *tokenizer, Dictionary dict,
                            const char *w, const char *wend)
{
	int i, j, len;
	int p_strippable, s_strippable;
	const char **prefix, **suffix;
	char newword[MAX_WORD+1];
	Boolean word_is_in_dict = FALSE;

	/* Set up affix tables.  */
	Dictionary afdict = dict->affix_table;
	if (NULL == afdict) return FALSE;

	p_strippable = afdict->p_strippable;
	s_strippable = afdict->s_strippable;
	prefix = afdict->prefix;
	suffix = afdict->suffix;

	j = 0;
	for (i=0; i <= s_strippable; i++)
	{
		Boolean s_ok = FALSE;
		/* Go through once for each suffix; then go through one 
		 * final time for the no-suffix case (i.e. to look for
		 * prefixes only, without suffixes.) */
		if (i < s_strippable)
		{
			len = strlen(suffix[i]);

			/* The remaining w is too short for a possible match */
			if ((wend-w) < len) continue;

			/* Always match the zero-lenght suffix */
			if (0 == len) s_ok = TRUE;
			else if (strncmp(wend-len, suffix[i], len) == 0) s_ok = TRUE;
		}
		else
			len = 0;

		if (s_ok || i == s_strippable)
		{
			size_t sz = MIN((wend-len)-w, MAX_WORD);
			strncpy(newword, w, sz);
			newword[sz] = '\0';

			/* Check if the remainder is in the dictionary;
			 * for the no-suffix case, it won't be */
			if ((i < s_strippable) &&
			    find_word_in_dict(dict, newword))
			{
				if (1 < verbosity)
					printf("Splitting word into two: %s-%s\n", newword, suffix[i]);

				add_suffix_alternatives(tokenizer, newword, suffix[i]);
				word_is_in_dict = TRUE;
			}

			/* If the remainder isn't in the dictionary, 
			 * try stripping off prefixes */
			else
			{
				for (j=0; j<p_strippable; j++)
				{
					if (strncmp(w, prefix[j], strlen(prefix[j])) == 0)
					{
						int sz = MIN((wend - len) - (w + strlen(prefix[j])), MAX_WORD);
						strncpy(newword, w+strlen(prefix[j]), sz);
						newword[sz] = '\0';
						if (find_word_in_dict(dict, newword))
						{
							if (verbosity>1)
							{
								if (i < s_strippable)
									printf("Splitting word into three: %s-%s-%s\n",
										prefix[j], newword, suffix[i]);
								else
									printf("Splitting off a prefix: %s-%s\n",
										prefix[j], newword);
							}
							word_is_in_dict = TRUE;
							if (i < s_strippable)
								add_presuff_alternatives(tokenizer, prefix[j], newword, suffix[i]);
							else
								add_presuff_alternatives(tokenizer, prefix[j], newword, NULL);
						}
					}
				}
			}
			if (j != p_strippable) break;
		}
	}

	return word_is_in_dict;
}

/**
 * Split word into prefix, stem and suffix. 
 * Record all of the alternative splittings in the tokenizer
 * alternatives arrays.
 */
Boolean split_word(Tokenizer *tokenizer, Dictionary dict, const char *word)
{
	return suffix_split(tokenizer, dict, word, word + strlen(word));
}

/**
 * w points to a string, wend points to the char one after the end.  The
 * "word" w contains no blanks.  This function splits up the word if
 * necessary, and calls "issue_sentence_word()" on each of the resulting
 * parts.  The process is described above.  Returns TRUE if OK, FALSE if
 * too many punctuation marks or other separation error.
 */
static void separate_word(Sentence sent, Parse_Options opts,
                         const char *w, const char *wend,
                         Boolean quote_found)
{
	size_t sz;
	int i, len;
	int r_strippable=0, l_strippable=0, u_strippable=0;
	int  n_r_stripped = 0;
	Boolean word_is_in_dict;
	Boolean issued = FALSE;
	Boolean have_empty_suffix = FALSE;

	int found_number = 0;
	int n_r_stripped_save;
	const char * wend_save;

	const char ** strip_left = NULL;
	const char ** strip_right = NULL;
	const char ** strip_units = NULL;
	char word[MAX_WORD+1];

	const char *r_stripped[MAX_STRIP];  /* these were stripped from the right */

	Dictionary dict = sent->dict;

	if (dict->affix_table != NULL)
	{
		have_empty_suffix = dict->affix_table->have_empty_suffix;
	}

	/* First, see if we can already recognize the word as-is. If
	 * so, then we are done. Else we'll try stripping prefixes, suffixes.
	 */
	sz = MIN(wend-w, MAX_WORD);
	strncpy(word, w, sz);
	word[sz] = '\0';

	/* The simplest case: no affixes, suffixes, etc. */
	word_is_in_dict = find_word_in_dict (dict, word);

	/* ... unless its a lang like Russian, which allows empty
	 * suffixes, which have a real morphological linkage.
	 * In which case, we have to try them out.
	 * XXX FIXME but the current russian dict does not have empty suffixes ... */
	if (word_is_in_dict && !have_empty_suffix)
	{
		issue_sentence_word(sent, word, quote_found);
		return;
	}

	/* Set up affix tables.  */
	if (dict->affix_table != NULL)
	{
		Dictionary afdict = dict->affix_table;
		r_strippable = afdict->r_strippable;
		l_strippable = afdict->l_strippable;
		u_strippable = afdict->u_strippable;

		strip_left = afdict->strip_left;
		strip_right = afdict->strip_right;
		strip_units = afdict->strip_units;

		/* If we found the word in the dict, but it also can have an
		 * empty suffix, then just skip right over to suffix processing
		 */
		if (word_is_in_dict && have_empty_suffix)
			goto do_suffix_processing;

		/* Strip off punctuation, etc. on the left-hand side. */
		/* XXX FIXME: this fails in certain cases: e.g.
		 * "By the '50s, he was very prosperous."
		 * where the leading quote is striped, and then "50s," cannot be
		 * found in the dict.  Next, the comma is removed, and "50s" is still
		 * not in the dict ... the trick was that the comma should be 
		 * right-stripped first, then the possible quotes. 
		 * More generally, the current implementation of the link-parser algo
		 * does not support multiple alternative tokenizations; the viterbi
		 * parser, under development, should be able to do better.
		 */
		for (;;)
		{
			for (i=0; i<l_strippable; i++)
			{
				/* This is UTF8-safe, I beleive ... */
				sz = strlen(strip_left[i]);
				if (strncmp(w, strip_left[i], sz) == 0)
				{
					issue_sentence_word(sent, strip_left[i], quote_found);
					w += sz;
					break;
				}
			}
			if (i == l_strippable) break;
		}

		/* Its possible that the token consisted entirely of
		 * left-punctuation, in which case, it has all been issued.
		 * So -- we're done, return.
		 */
		if (w >= wend) return;

		/* Now w points to the string starting just to the right of
		 * any left-stripped characters.
		 * stripped[] is an array of numbers, indicating the index
		 * numbers (in the strip_right array) of any strings stripped off;
		 * stripped[0] is the number of the first string stripped off, etc.
		 * When it breaks out of this loop, n_stripped will be the number
		 * of strings stripped off.
		 */
		for (n_r_stripped = 0; n_r_stripped < MAX_STRIP; n_r_stripped++) 
		{
			sz = MIN(wend-w, MAX_WORD);
			strncpy(word, w, sz);
			word[sz] = '\0';
			if (wend == w) break;  /* it will work without this */

			if (find_word_in_dict(dict, word))
			{
				word_is_in_dict = TRUE;
				break;
			}

			for (i=0; i < r_strippable; i++)
			{
				len = strlen(strip_right[i]);

				/* the remaining w is too short for a possible match */
				if ((wend-w) < len) continue;
				if (strncmp(wend-len, strip_right[i], len) == 0)
				{
					r_stripped[n_r_stripped] = strip_right[i];
					wend -= len;
					break;
				}
			}
			if (i == r_strippable) break;
		}

		/* Is there a number in the word? If so, then search for
		 * trailing units suffixes.
		 */
		if ((FALSE == word_is_in_dict) && contains_digits(word))
		{
			/* Same as above, but with a twist: the only thing that can
			 * preceed a units suffix is a number. This is so that we can
			 * split up things like "12ft" (twelve feet) but not split up
			 * things like "Delft blue". Multiple passes allow for
			 * constructions such as 12sq.ft.
			 */
			n_r_stripped_save = n_r_stripped;
			wend_save = wend;
			for (; n_r_stripped < MAX_STRIP; n_r_stripped++) 
			{
				size_t sz = MIN(wend-w, MAX_WORD);
				strncpy(word, w, sz);
				word[sz] = '\0';
				if (wend == w) break;  /* it will work without this */

				/* Number */
				if (is_number(word))
				{
					found_number = 1;
					break;
				}

				for (i=0; i < u_strippable; i++)
				{
					len = strlen(strip_units[i]);

					/* the remaining w is too short for a possible match */
					if ((wend-w) < len) continue;
					if (strncmp(wend-len, strip_units[i], len) == 0)
					{
						r_stripped[n_r_stripped] = strip_units[i];
						wend -= len;
						break;
					}
				}
				if (i == u_strippable) break;
			}

			/* The root *must* be a number! */
			if (0 == found_number)
			{
				wend = wend_save;
				n_r_stripped = n_r_stripped_save;
			}
		}
	}

	/* Umm, double-check, if need be ... !??  Why? */
	if (FALSE == word_is_in_dict)
	{
		/* w points to the remaining word, 
i		 * "wend" to the end of the word. */
		sz = MIN(wend-w, MAX_WORD);
		strncpy(word, w, sz);
		word[sz] = '\0';

		word_is_in_dict = find_word_in_dict(dict, word);
	}

do_suffix_processing:
	/* OK, now try to strip suffixes. */
	if (!word_is_in_dict || have_empty_suffix)
	{
		Tokenizer *tokenizer = &sent->tokenizer;

		tokenizer->string_set = dict->string_set;
		tokenizer->pref_alternatives = NULL;
		tokenizer->stem_alternatives = NULL;
		tokenizer->suff_alternatives = NULL;

		word_is_in_dict = suffix_split(tokenizer, dict, w, wend);
	}

	/* word is now what remains after all the stripping has been done */
	issued = FALSE;

	/* If n_r_stripped exceed max, the "word" is most likely a long
	 * sequence of periods.  Just accept it as an unknown "word", 
	 * and move on.
	 */
	if (n_r_stripped >= MAX_STRIP)
	{
		n_r_stripped = 0;
		word_is_in_dict = TRUE;
	}

#if defined HAVE_HUNSPELL || defined HAVE_ASPELL
	/* If the word is still not being found, then it might be 
	 * a run-on of two words. Ask the spell-checker to split
	 * the word in two, if possible. Do this only if the word
	 * is not a proper name, and if spell-checking is enabled.
	 */
	if ((FALSE == word_is_in_dict) && 
	    TRUE == opts->use_spell_guess &&
	    dict->spell_checker &&
	    (FALSE == is_proper_name(word)))
	{
		char **alternates = NULL;
		char *sp = NULL;
		char *wp;
		int j, n;
		n = spellcheck_suggest(dict->spell_checker, &alternates, word);
		for (j=0; j<n; j++)
		{
			/* Uhh, XXX this is not utf8 safe! ?? */
			sp = strchr(alternates[j], ' ');
			if (sp) break;
		}

		if (sp) issued = TRUE;

		/* It may be three run-on words or more. Loop over all */
		wp = alternates[j];
		while (sp)
		{
			*sp = 0x0;
			issue_sentence_word(sent, wp, quote_found);
			wp = sp+1;
			sp = strchr(wp, ' ');
			if (NULL == sp)
			{
				issue_sentence_word(sent, wp, quote_found);
			}
		}
		if (alternates) spellcheck_free_suggest(alternates, n);
	}
#endif /* HAVE_HUNSPELL */

	if (FALSE == issued)
		issued = issue_alternatives(sent, quote_found);

	if (FALSE == issued)
		issue_sentence_word(sent, word, quote_found);

	for (i = n_r_stripped - 1; i >= 0; i--)
	{
		issue_sentence_word(sent, r_stripped[i], FALSE);
	}
}

/**
 * The string s has just been read in from standard input.
 * This function breaks it up into words and stores these words in
 * the sent->word[] array.  Returns TRUE if all is well, FALSE otherwise.
 * Quote marks are treated just like blanks.
 */
Boolean separate_sentence(Sentence sent, Parse_Options opts)
{
	const char *t;
	Boolean quote_found;
	Dictionary dict = sent->dict;
	mbstate_t mbs;
	const char * s = sent->orig_sentence;

	sent->length = 0;

	if (dict->left_wall_defined)
		issue_sentence_word(sent, LEFT_WALL_WORD, FALSE);

	/* Reset the multibyte shift state to the initial state */
	memset(&mbs, 0, sizeof(mbs));

	for(;;) 
	{
		int isq;
		wchar_t c;
		int nb = mbrtowc(&c, s, MB_CUR_MAX, &mbs);
		quote_found = FALSE;

		if (0 > nb) goto failure;

		/* Skip all whitespace. Also, ignore *all* quotation marks.
		 * XXX This is sort-of a hack, but that is because LG does
		 * not have any intelligent support for quoted character
		 * strings at this time.
		 */
		isq = is_quote (c);
		if (isq) quote_found = TRUE;
		while (iswspace(c) || isq)
		{
			s += nb;
			nb = mbrtowc(&c, s, MB_CUR_MAX, &mbs);
			if (0 == nb) break;
			if (0 > nb) goto failure;
			isq = is_quote (c);
			if (isq) quote_found = TRUE;
		}

		if (*s == '\0') break;

		t = s;
		nb = mbrtowc(&c, t, MB_CUR_MAX, &mbs);
		if (0 > nb) goto failure;
		while (!iswspace(c) && !is_quote(c) && (c != 0) && (nb != 0))
		{
			t += nb;
			nb = mbrtowc(&c, t, MB_CUR_MAX, &mbs);
			if (0 > nb) goto failure;
		}

		separate_word(sent, opts, s, t, quote_found);
		s = t;
		if (*s == '\0') break;
	}

	if (dict->right_wall_defined)
		issue_sentence_word(sent, RIGHT_WALL_WORD, FALSE);

	return (sent->length > (dict->left_wall_defined ? 1 : 0) +
	       (dict->right_wall_defined ? 1 : 0));

failure:
	prt_error("Unable to process UTF8 input string in current locale %s\n",
		nl_langinfo(CODESET));
	return FALSE;
}

/**
 * Build the word expressions, and add a tag to the word to indicate
 * that it was guessed by means of regular-expression matching.
 * Also, add a subscript to the resulting word to indicate the
 * rule origin.
 */
static X_node * build_regex_expressions(Sentence sent, int i,
                             const char * word_type, const char * word)
{
	X_node *e, *we;

	we = build_word_expressions(sent->dict, word_type);
	for (e = we; e != NULL; e = e->next)
	{
		char str[MAX_WORD+1];
		char * t;
		t = strchr(e->string, '.');
		if (NULL != t)
		{
			snprintf(str, MAX_WORD, "%.50s[!].%.5s", word, t+1);
		}
		else
		{
			snprintf(str, MAX_WORD, "%.50s[!]", word);
		}
		e->string = string_set_add(str, sent->string_set);
	}
	return we;
}

/**
 * Puts into word[i].x the expression for the unknown word 
 * the parameter s is the word that was not in the dictionary 
 * it massages the names to have the corresponding subscripts 
 * to those of the unknown words
 * so "grok" becomes "grok[?].v" 
 */
static X_node * handle_unknown_word(Sentence sent, int i, const char * s)
{
	X_node *d, *we;

	we = build_word_expressions(sent->dict, UNKNOWN_WORD);
	assert(we, "UNKNOWN_WORD must be defined in the dictionary!");

	for (d = we; d != NULL; d = d->next)
	{
		char str[MAX_WORD+1];
		char *t = strchr(d->string, '.');
		if (t != NULL)
		{
			snprintf(str, MAX_WORD, "%.50s[?].%.5s", s, t+1);
		}
		else
		{
			snprintf(str, MAX_WORD, "%.50s[?]", s);
		}
		d->string = string_set_add(str, sent->string_set);
	}
	return we;
}

/**
 * If a word appears to be mis-spelled, then add alternate
 * spellings. Maybe one of those will do ... 
 */
static X_node * guess_misspelled_word(Sentence sent, int i, const char * s)
{
	Boolean spelling_ok;
	Dictionary dict = sent->dict;
	X_node *d, *head = NULL;
	int j, n;
	char **alternates = NULL;

	/* Spell-guessing is disabled if no spell-checker is speficified */
	if (NULL == dict->spell_checker)
	{
		return handle_unknown_word(sent, i, s);
	}

	/* If the spell-checker knows about this word, and we don't ... 
	 * Dang. We should fix it someday. Accept it as such. */
	spelling_ok = spellcheck_test(dict->spell_checker, s);
	if (spelling_ok)
	{
		return handle_unknown_word(sent, i, s);
	}

	/* Else, ask the spell-checker for alternate spellings
	 * and see if these are in the dict. */
	n = spellcheck_suggest(dict->spell_checker, &alternates, s);
	for (j=0; j<n; j++)
	{
		if (find_word_in_dict(sent->dict, alternates[j]))
		{
			X_node *x = build_word_expressions(sent->dict, alternates[j]);
			head = catenate_X_nodes(x, head);
		}
	}
	if (alternates) spellcheck_free_suggest(alternates, n);

	/* Add a [~] to the output to signify that its the result of
	 * guessing. */
	for (d = head; d != NULL; d = d->next)
	{
		char str[MAX_WORD+1];
		const char * t = strchr(d->string, '.');
		if (t != NULL)
		{
			size_t off = t - d->string;
			off = MIN(off, MAX_WORD-3); /* make room for [~] */
			strncpy(str, d->string, off);
			str[off] = 0;
			strcat(str, "[~]");
			strcat(str, t);
		}
		else
		{
			snprintf(str, MAX_WORD, "%.50s[~]", s);
		}
		d->string = string_set_add(str, sent->string_set);
	}

	/* If nothing found at all... */
	if (NULL == head)
	{
		return handle_unknown_word(sent, i, s);
	}
	return head;
}

/**
 * Corrects case of first word, fills in other proper nouns, and
 * builds the expression lists for the resulting words.
 *
 * Algorithm:
 * Apply the following step to all words w:
 * If w is in the dictionary, use it.
 * Else if w is identified by regex matching, use the
 * appropriately matched disjunct collection.
 *
 * Now, we correct the first word, w.
 * If w is upper case, let w' be the lower case version of w.
 * If both w and w' are in the dict, concatenate these disjncts.
 * Else if just w' is in dict, use disjuncts of w', together with
 * the CAPITALIZED-WORDS rule.
 * Else leave the disjuncts alone.
 *
 * XXX There is a fundamental algorithmic failure here, with regards to
 * spell guessing.  If the mispelled word "dont" shows up, the
 * spell-guesser will offer both "done" and "don't" as alternatives.
 * The second alternative should really be affix-stripped, and issued
 * as two words, not one. But the former only issues as one word.  So,
 * should we issue two word or one?  ... and so we can't correctly
 * build sentence expressions for this, since we don't have the correct
 * word count bythis point.  The viterbi incremental parser won't have
 * this bug.
 */
void build_sentence_expressions(Sentence sent, Parse_Options opts)
{
	int i, first_word;  /* the index of the first word after the wall */
	const char *s;
	const char * regex_name;
	X_node * e;
	Dictionary dict = sent->dict;

	if (dict->left_wall_defined) {
		first_word = 1;
	} else {
		first_word = 0;
	}

	/* The following loop treats all words the same
	 * (nothing special for 1st word) */
	for (i=0; i<sent->length; i++)
	{
		size_t ialt;
		for (ialt=0; NULL != sent->word[i].alternatives[ialt]; ialt++)
		{
			X_node *we;

			s = sent->word[i].alternatives[ialt];
			if (boolean_dictionary_lookup(sent->dict, s))
			{
				we = build_word_expressions(sent->dict, s);
			}
			else if ((NULL != (regex_name = match_regex(sent->dict, s))) &&
			         boolean_dictionary_lookup(sent->dict, regex_name))
			{
				we = build_regex_expressions(sent, i, regex_name, s);
			}
			else if (dict->unknown_word_defined && dict->use_unknown_word)
			{
				if (opts->use_spell_guess)
				{
					we = guess_misspelled_word(sent, i, s);
				}
				else
				{
					we = handle_unknown_word(sent, i, s);
				}
			}
			else 
			{
				/* The reason I can assert this is that the word
				 * should have been looked up already if we get here.
				 */
				assert(FALSE, "I should have found that word.");
			}

			/* Under certain cases--if it's the first word of the sentence,
			 * or if it follows a colon or a quotation mark--a word that's 
			 * capitalized has to be looked up as an uncapitalized word
			 * (possibly also as well as a capitalized word).
			 *
			 * XXX This rule is English-language-oriented, and should be
			 * abstracted.
			 */
			if ((i == first_word ||
			     (i > 0 && strcmp(":", sent->word[i-1].alternatives[0])==0) || 
			     sent->post_quote[i]) &&
			   (is_utf8_upper(s)))
			{

				/* If the lower-case version of this word is in the dictionary, 
				 * then add the disjuncts for the lower-case version. The upper
				 * case version disjuncts had previously come from matching the 
				 * CAPITALIZED-WORDS regex.
				 *
				 * Err .. add the lower-case version only if the lower-case word
				 * is a common noun or adjective; otherwise, *replace* the
				 * upper-case word with the lower-case one.  This allows common
				 * nouns and adjectives to be used for entity names: e.g.
				 * "Great Southern Union declares bankruptcy", allowing Great
				 * to be capitalized, while preventing an upper-case "She" being
				 * used as a proper name in "She declared bankruptcy".
				 *
				 * Arghh. This is still messed up. The capitalized-regex runs
				 * too early, I think. We need to *add* Sue.f (female name Sue)
				 * even though sue.v (the verb "to sue") is in the dict. So 
				 * test for capitalized entity names. Glurg. Too much complexity
				 * here, it seems to me.
				 *
				 * This is actually a great example of a combo of an algorithm
				 * together with a list of words used to determine grammatical
				 * function.
				 */
				char temp_word[MAX_WORD+1];
				const char * lc;

				downcase_utf8_str(temp_word, s, MAX_WORD);
				lc = string_set_add(temp_word, sent->string_set);
	
				/* The lower-case dict lookup might trigger regex 
				 * matches in the dictionary. We want to avoid these.
				 * e.g. "Cornwallis" triggers both PL-CAPITALIZED_WORDS
				 * and S-WORDS. Since its not an entity, the regex 
				 * matches will erroneously discard the upper-case version.
				 */
				if (boolean_dictionary_lookup(sent->dict, lc))
				{
					if (is_entity(sent->dict,s) ||
					    is_common_entity(sent->dict,lc))
					{
						if (1 < verbosity)
						{
							printf ("Info: First word: %s entity=%d common=%d\n", 
								s, is_entity(sent->dict,s), 
								is_common_entity(sent->dict,lc));
						}
	
						/* If we are here, then we want both upper and lower case
						 * expressions. The upper-case ones were build above, so now
						 * append the lower-case ones. */
						e = build_word_expressions(sent->dict, lc);
						we = catenate_X_nodes(we, e);
					}
					else
					{
						if (1 < verbosity)
						{
							printf("Info: First word: %s downcase only\n", lc);
						}
	
						/* If we are here, then we want the lower-case 
						 * expressions only.  Erase the upper-case ones, built
						 * previously up above. */
						sent->word[i].alternatives[ialt] = lc;  /* lc already in sent->string-set */
						e = build_word_expressions(sent->dict, lc);
						free_X_nodes(we);
						we = e;
					}
				}
			}

			/* At last .. concatentate the word expressions we build for
			 * this alternative. */
			sent->word[i].x = catenate_X_nodes(sent->word[i].x, we);
		}
	}
}


/**
 * This just looks up all the words in the sentence, and builds
 * up an appropriate error message in case some are not there.
 * It has no side effect on the sentence.  Returns TRUE if all
 * went well.
 *
 * This code is called only is the 'unkown-words' flag is set.
 */
Boolean sentence_in_dictionary(Sentence sent)
{
	Boolean ok_so_far;
	int w;
	const char * s;
	Dictionary dict = sent->dict;
	char temp[1024];

	ok_so_far = TRUE;
	for (w=0; w<sent->length; w++)
	{
		size_t ialt;
		for (ialt=0; NULL != sent->word[w].alternatives[ialt]; ialt++)
		{
			s = sent->word[w].alternatives[ialt];
			if (!find_word_in_dict(dict, s))
			{
				if (ok_so_far)
				{
					safe_strcpy(temp, "The following words are not in the dictionary:", sizeof(temp));
					ok_so_far = FALSE;
				}
				safe_strcat(temp, " \"", sizeof(temp));
				safe_strcat(temp, s, sizeof(temp));
				safe_strcat(temp, "\"", sizeof(temp));
			}
		}
	}
	if (!ok_so_far)
	{
		err_ctxt ec;
		ec.sent = sent;
		err_msg(&ec, Error, "Error: Sentence not in dictionary\n%s\n", temp);
	}
	return ok_so_far;
}
