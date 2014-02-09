#define ALTOLD 0
/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2009, 2012, 2013 Linas Vepstas                          */
/* Copyright (c) 2014 Amir Plivatsky                                     */
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
#include "dict-api.h"
#include "error.h"
#include "externs.h"
#include "regex-morph.h"
#include "spellcheck.h"
#include "string-set.h"
#include "structures.h"
#include "tokenize.h"
#include "utilities.h"
#include "word-utils.h"
#include "stdarg.h"

#define MAX_STRIP 10

/* These are no longer in use, but are read from the 4.0.affix file */
/* I've left these here, as an axample of what to expect. */
/*static char * strip_left[] = {"(", "$", "``", NULL}; */
/*static char * strip_right[] = {")", "%", ",", ".", ":", ";", "?", "!", "''", "'", "'s", NULL};*/

#define ENTITY_MARKER   "<marker-entity>"
#define COMMON_ENTITY_MARKER   "<marker-common-entity>"
#define INFIX_MARK '='

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
 * Return TRUE if the character is white-space
 */
static Boolean is_space(wchar_t wc)
{
	if (iswspace(wc)) return TRUE;

	/* 0xc2 0xa0 is U+00A0, c2 a0, NO-BREAK SPACE */
	/* For some reason, iswspace doesn't get this */
	if (0xa0 == wc) return TRUE;

	/* iswspace seems to use somewhat different rules than what we want,
	 * so over-ride special cases in the U+2000 to U+206F range.
	 * Caution: this potentially screws with arabic, and right-to-left
	 * languages.
	 */
/***  later, not now ..
	if (0x2000 <= wc && wc <= 0x200f) return TRUE;
	if (0x2028 <= wc && wc <= 0x202f) return TRUE;
	if (0x205f <= wc && wc <= 0x206f) return TRUE;
***/

	return FALSE;
}

/**
 * Returns true if the word can be interpreted as a number.
 * The ":" is included here so we allow "10:30" to be a number.
 * The "." and "," allow numbers in both US and European notation:
 * e.g. American million: 1,000,000.00  Euro million: 1.000.000,00
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
		if (nb < 0) return FALSE;
		if (iswdigit(c)) return TRUE;
		s += nb;
	}
	return FALSE;
}

static void add_alternative(Sentence, int, const char **, int, const char **, int, const char **);
static Boolean issue_alternatives1(Sentence, const char *, Boolean);
static void print_sentence_word_alternatives(Sentence);
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
if (ALTOLD)
{
	size_t len = sent->length;

	if (*s == '\0') return;

	sent->post_quote = (Boolean *)realloc(sent->post_quote, (len+1) * sizeof(Boolean));
	sent->post_quote[len] = quote_found;

	sent->word = (Word *)realloc(sent->word, (len+1) * sizeof(Word));
	sent->word[len].x = NULL;
	sent->word[len].d = NULL;

	sent->word[len].alternatives = (const char **) malloc(2*sizeof(const char *));
	sent->word[len].alternatives[0] = string_set_add(s, sent->string_set);
	sent->word[len].alternatives[1] = NULL;

	/* Record the original form, as well. */
	sent->word[len].unsplit_word = sent->word[len].alternatives[0];

	/* Now we record whether the first character of the word is upper-case.
	   (The first character may be made lower-case
	   later, but we may want to get at the original version) */
	if (is_utf8_upper(s)) sent->word[len].firstupper = TRUE;
	else sent->word[len].firstupper = FALSE;

	sent->length++;
}
else
{
		add_alternative(sent, 0,0, 1,&s, 0,0);
		(void) issue_alternatives1(sent, s, quote_found);
}
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
	arr = (const char **)realloc(arr, (len+2) * sizeof(const char *));
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
	buff[0] = INFIX_MARK;
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

		sent->post_quote = (Boolean *)realloc(sent->post_quote, (len+1) * sizeof(Boolean));
		sent->post_quote[len] = quote_found;

		sent->word = (Word *)realloc(sent->word, (len+1) * sizeof(Word));
		sent->word[len].x = NULL;
		sent->word[len].d = NULL;
		sent->word[len].unsplit_word = NULL;

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
		sent->post_quote = (Boolean *)realloc(sent->post_quote, (len+1) * sizeof(Boolean));
		sent->post_quote[len] = preflen ? FALSE : quote_found;

		sent->word = (Word *)realloc(sent->word, (len+1) * sizeof(Word));
		sent->word[len].x = NULL;
		sent->word[len].d = NULL;
		sent->word[len].unsplit_word = tokenizer->unsplit_word;

		sent->word[len].alternatives = tokenizer->stem_alternatives;

		/* Perform a check for captialization only if no prefixes. */
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
		sent->post_quote = (Boolean *)realloc(sent->post_quote, (len+1) * sizeof(Boolean));
		sent->post_quote[len] = FALSE;

		sent->word = (Word *)realloc(sent->word, (len+1) * sizeof(Word));
		sent->word[len].x = NULL;
		sent->word[len].d = NULL;
		sent->word[len].unsplit_word = NULL;

		sent->word[len].alternatives = tokenizer->suff_alternatives;
		tokenizer->suff_alternatives = NULL;

		sent->word[len].firstupper = FALSE;
		len++;
		issued = TRUE;
	}

	sent->length = len;
	return issued;
}

static void altappend(Sentence sent, int word_index, const char *w)
{
	const char ***a = &sent->word[word_index].alternatives;
	int n = altlen(*a);

	*a = resize_alts(*a, n);
	(*a)[n] = string_set_add(w, sent->string_set);
}

/**
 * Add to the sentence prefnum elements from prefix, stemnum elements from stem, and suffnum elements from suffix.
 * Mark the prefixes and suffixes.
 * Balance all alternatives using empty words.
 * Do not add an empty string [as stem] (Can it happen? I couldn't find such a case. [ap])
 */
static void add_alternative(Sentence sent, int prefnum, const char **prefix, int stemnum, const char **stem, int suffnum, const char **suffix)
{
	int t_start = sent->t_start;	/* position of word starting the current token sequence */
	int t_count = sent->t_count;	/* number of words in the current token sequence */
	int len;			/* word index */
	int ai = 0;			/* affix index */
	const char **affix;		/* affix list pointer */
	const char **affixlist[] = { prefix, stem, suffix, NULL };
	int numlist[] = { prefnum, stemnum, suffnum };
	enum affixtype { PREFIX, STEM, SUFFIX, END };
	enum affixtype at;
	char buff[MAX_WORD+1];

	if (3 < verbosity) { /* DEBUG */
		printf(">>>add_alternative: ");
		if (prefix) printf("prefix '%s' ", *prefix);
		printf("stem '%s' ", *stem);
		if (suffix) printf("suffix '%s'", *suffix);
		printf("\n");
	}

	for (at = PREFIX; at != END; at++)
	{
		int affixnum = numlist[at];
		for (affix = affixlist[at]; affixnum-- > 0; affix++, ai++)
		{
			assert(ai <= t_count, "add_alternative: word index > t_count");
			if (ai == 0 && *affix[0] == '\0')	/* can it happen? */
			{
				if (1 < verbosity) /* DEBUG */
					printf("add_alternative(): skipping given empty string (type %d, argnum %d/%d/%d)\n",
						at, prefnum, stemnum, suffnum);
				return;
			}

			if (ai == t_count)	/* need to add a word */
			{
				len = t_start + t_count;

				/* allocate new word */
				sent->word = (Word *)realloc(sent->word, (len+1) * sizeof(Word));
				sent->word[len].x= NULL;
				sent->word[len].d= NULL;
				sent->word[len].alternatives = NULL;
				sent->word[len].unsplit_word = NULL;
				sent->word[len].firstupper = FALSE;

				t_count++;
				if (t_count > 1) /* not first added word */
				{
					/* BALANCING: complete alternative total number at the added new word */
					int i;

					int numalt = altlen(sent->word[t_start].alternatives)-1;
					const char **alt = (const char **)malloc((numalt+1) * sizeof(const char *));
					for (i = 0; i <= numalt; i++) alt[i] = NULL;
					sent->word[len].alternatives = alt;
				}
			}
			/* Add the token as an alternative */
			/* Capitalization marking:
			 * Original algorithm:
			 * - Mark prefix word if any of its alternative is capitalize
			 * - Similarly for stem if no prefix
			 * - No handling for suffix
			 * Algorithm here (still not generally good [ap]):
			 * - Mark first word of an alternative sequence if any prefix or stem is capitalized.
			 * This is intended to be the same as actually done now for en and ru (prefix is not used).
			 */
			switch (at)
			{
				size_t sz;

				case PREFIX:	/* word= */
					sz = MIN(strlen(*affix), MAX_WORD);
					strncpy(buff, *affix, sz);
					buff[sz] = INFIX_MARK;
					buff[sz+1] = 0;
					break;
				case STEM:	/* word.= */	/* TEMPORARY NOTE: a corrected dict_order_user() is needed! */
					strcpy(buff, *affix);
					if (suffnum && *suffix[0] != '\0') /* there is a suffix, so we must dictionary-fetch only a stem */
						strcat(buff, ".=");	/* XXX should use SUFFIX_WORD but it is not yet defined globally */
					break;
				case SUFFIX:	/* =word */
					sz = MIN(strlen(*affix) + 2, MAX_WORD);
					buff[0] = INFIX_MARK;
					strncpy(&buff[1], *affix, sz);
					buff[sz] = 0;
					break;
				case END:
					assert(TRUE, "affixtype END reached");
			}
			if (is_utf8_upper(buff)) sent->word[t_start].firstupper = TRUE; /* suffixes start with INFIX_MARK... */
			altappend(sent, t_start + ai, buff);
		}
	}

	/* BALANCING: add an empty alternative to the rest of words */
	for (len = t_start + ai; len < t_start + t_count; len++)
	{
		char infix_string[] = { INFIX_MARK, '\0' };
		altappend(sent, len, infix_string);
	}
	sent->t_count = t_count;
}

static Boolean issue_alternatives1(Sentence sent, const char *word, Boolean quote_found)
{
		int t_start = sent->t_start;
		int t_count = sent->t_count;

		if (0 == t_count) return FALSE;

		sent->word[t_start].unsplit_word = string_set_add(word, sent->string_set);
		sent->post_quote = (Boolean *)realloc(sent->post_quote, (t_start+1) * sizeof(Boolean));
		sent->post_quote[t_start] = quote_found;

		sent->length += t_count;
		sent->t_start = sent->length;
		sent->t_count = 0;

		if (3 < verbosity) {
			printf("issue_alternatives1:\n");
			print_sentence_word_alternatives(sent);
		}

		return TRUE;
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

	/* Record the original word */
	len = MIN(wend-w, MAX_WORD);
	strncpy(newword, w, len);
	newword[len] = '\0';
	tokenizer->unsplit_word = string_set_add(newword, tokenizer->string_set);

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

			/* Always match the zero-length suffix */
			/* XXX there is no suffux with len 0 (and should not be) [ap] */
			if (0 == len) { printf("Suffix %d of %d - len 0\n", i, s_strippable); s_ok = TRUE; }
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
				if (2 < verbosity)
					printf("Splitting word into two: %s-%s\n", newword, suffix[i]);

				if (ALTOLD) {
				add_suffix_alternatives(tokenizer, newword, suffix[i]);
				}
				else
				{ /* We still use here the original suffix_split() that uses the Tokenizer struct as first argument,
				   * but we need struct sent for add_alternative().
				   * So for the purpose of this test code, compute the location of struct sent from the value of tokenizer.
				   */
				#include <stddef.h>
				const char *wp = suffix[i], *nw = newword;
				Sentence sent;
				sent = (Sentence)((size_t)tokenizer - offsetof(struct Sentence_s, tokenizer));
				add_alternative(sent, 0,0, 1,&nw, 1,&wp);
				}
				word_is_in_dict = TRUE;
			}

			/* If the remainder isn't in the dictionary, 
			 * try stripping off prefixes */
			else
			{
if (2 < verbosity && find_word_in_dict(dict, newword)) {
	printf("->>>>suffix_split: no suffix and word '%s' found!\n", newword); /* contradicts the comment above? */
}
				for (j=0; j<p_strippable; j++)
				{
					if (strncmp(w, prefix[j], strlen(prefix[j])) == 0)
					{
						int sz = MIN((wend - len) - (w + strlen(prefix[j])), MAX_WORD);
						strncpy(newword, w+strlen(prefix[j]), sz);
						newword[sz] = '\0';
						if (find_word_in_dict(dict, newword))
						{
							if (2 < verbosity)
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

static int revcmplen(const void *a, const void *b)
{
           return strlen(*(char * const *)b) - strlen(*(char * const *)a);
}

#define HEB_PRENUM_MAX	5	/* no more than 5 prefix "subwords" */
#define HEB_MPREFIX_MAX	16	/* >mp_strippable (13) */
#define HEB_UTF8_BYTES 2	/* Hebrew UTF8 characters are always 2-byte */
#define HEB_CHAREQ(s, c) (strncmp(s, c, HEB_UTF8_BYTES) == 0)
/**
 * Handle "formative letters"  ב, ה, ו, כ, ל, מ, ש.
 * Split word into multiple prefix "subwords" (1-3 characters each) and an unprefixed word
 * (which must be in the dictionary or be null) in all possible ways
 * (even when the prefix combination is not valid, the LG rulse will resolve that).
 * Add all the alternatives.
 * If the whole word (i.e. including the prefixes is in the dictionary, the word will be added in separate_word().
 *
 * These assumptions are used:
 * - every prefix "subword" is unique ('ככ' is considered here as one "subword")
 * - words with length <= 2 don't have a prefix
 * - longer prefixes have priority over shorter ones
 * - less than 16 prefix types - 13 are actually defined (for Boolean array limit)
 * - the given word contains only Hebrew characters (mixed words are to be split beforehand)
 * - the letter "ו" can only be the first prefix "subword"
 * - if the last "subword" is not "ו" and the word (lenght>2) starts with 2 "ו",
 *   the acatual word to be looked up starts with one "ו" (see TBD there)
 *
 * These algorithm is most probably very Hebrew-specific.
 * Much later LG overhead can be saved by including in the dictionary the list of all valid prefix "subword"
 * combinations (maybe several hundreds ) using m4. In that case the original suffix_split() could be used instead
 * of this funcion, with a small modification (for handling "ו").
 * Even more LG overheadi can be saved by allowing allowing only the prefixes that are appropriate according to
 * part of speach of the word.
 * No support for that here, for now.
 *
 * To implement this function in a way which is appropriate for more languages,
 * Hunspell-like definitions (but more general) are needed.
 */
static Boolean mprefix_split(Sentence sent, Dictionary dict, char *word)
{
	static int mprefix_sorted = 0;			/* need longer prefixes first */
	int i;
	int mp_strippable;
	const char **mprefix;
	const char *newword;
	const char *w;
	int sz;
	Boolean word_is_in_dict;
	int split_prefix_i = 0;				/* split prefix index */
	const char *split_prefix[HEB_PRENUM_MAX];	/* all prefix  */
	/* pseen is a simple prefix combination filter */
	Boolean pseen[HEB_MPREFIX_MAX];			/* prefix "subword" seen, don't allow again */
	Dictionary afdict;
	int wordlen = strlen(word);			/* guaranteed < MAX_WORD by separate_word() */

	if (2 * HEB_UTF8_BYTES >= wordlen) return FALSE; /* no prefix in words with total length <= 2 */

	/* set up affix table  */
	afdict = dict->affix_table;
	if (NULL == afdict) return FALSE;

	mp_strippable = afdict->mp_strippable;
	if (!mprefix_sorted)
	{
#define s_xstr(s) s_str(s)
#define s_str(s) #s
		assert(mp_strippable <= HEB_MPREFIX_MAX, "mp_strippable>" s_xstr(HEB_MPREFIX_MAX));
#undef s_xstr
#undef s_str
		qsort(afdict->mprefix, mp_strippable, sizeof(char *), revcmplen); /* revers-sort by length */
		mprefix_sorted = 1;
	}
	mprefix = afdict->mprefix;

	memset(pseen, 0, sizeof(pseen));	/* assuming zeroed-out bytes will be interpreted as FALSE */
	word_is_in_dict = FALSE;
	w = word;
	do
	{
		for (i=0; i<mp_strippable; i++)
		{
			int wlen;
			int plen;

			if (pseen[i] || ((split_prefix_i > 0) &&
			   (HEB_CHAREQ(mprefix[i], "ו")) && (HEB_CHAREQ(word, "ו")))) break;

			plen = strlen(mprefix[i]);
			wlen = strlen(w);
			sz = wlen - plen;
			if (2 * HEB_UTF8_BYTES > sz) break;	/* no one-letter words with prefix*/
			if (strncmp(w, mprefix[i], plen) == 0)
			{
				pseen[i] = TRUE;
				split_prefix[split_prefix_i++] = mprefix[i];
				newword = w + plen;
				if (!HEB_CHAREQ(mprefix[i], "ו") &&
				   (HEB_CHAREQ(w, "ו") && HEB_CHAREQ(w+HEB_UTF8_BYTES, "ו") && w[HEB_UTF8_BYTES+1]))
				{
					newword++; /* skip one 'ו' (TBD: check also without skipping) */
					sz--;
				}
				if (find_word_in_dict(dict, newword))
				{
					word_is_in_dict = TRUE;
					if (0 == sz)
					{
						if (2 < verbosity)
							printf("Whole-word prefix: %s\n", word);

						add_alternative(sent, split_prefix_i,split_prefix, 0,0, 0,0);
						break;
					}
					if (2 < verbosity)
						printf("Splitting off a prefix: %.*s-%s\n", wordlen-sz, word, newword);

					add_alternative(sent, split_prefix_i,split_prefix, 1,&newword, 0,0);
				}
				w = newword;
			}
		}
	} while ((1 * HEB_UTF8_BYTES < sz) && (i < mp_strippable) && (split_prefix_i < HEB_PRENUM_MAX));

	return word_is_in_dict;
}

/* Return TRUE if the word might be capitalized by convention:
 * -- if its the first word of a sentencorde
 * -- if its the first word following a colon
 * -- if its the first word of a quote
 *
 * XXX FIXME: These rules are rather English-centric.  Someone should
 * do something about this someday.
 */
static Boolean is_capitalizable(Sentence sent, int curr_word)
{
	int first_word; /* the index of the first word after the wall */
	Dictionary dict = sent->dict;

	if (dict->left_wall_defined) {
		first_word = 1;
	} else {
		first_word = 0;
	}

	/* Words at the start of sentences are capitalizable */
	if (curr_word == first_word) return TRUE;

	/* Words following colons are capitalizable */
	if (curr_word > 0 && strcmp(":", sent->word[curr_word-1].alternatives[0])==0)
		return TRUE;

	/* First word after a quote mark can be capitalized */
	if (sent->post_quote[curr_word])
		return TRUE;

	return FALSE;
}

/**
 * w points to a string, wend points to the char one after the end.  The
 * "word" w contains no blanks.  This function splits up the word if
 * necessary, and calls "issue_sentence_word()" on each of the resulting
 * parts.  The process is described above.  Returns TRUE if OK, FALSE if
 * too many punctuation marks or other separation error.
 *
 * This is used to split Russian words into stem+suffix, issueing a
 * separate "word" for each.  In addition, there are many English
 * constructions that need splitting:
 *
 * 86mm  -> 86 + mm (millimeters, measureument)
 * $10   ->  $ + 10 (dollar sign plus a number)
 * Surprise!  -> surprise + !  (pry the punctuation off the end of the word)
 * you've   -> you + 've  (undo contraction, treat 've as synonym for 'have')
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
	Boolean have_suffix = FALSE;
	Boolean have_mprefix = FALSE;

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
		have_suffix = (0 < dict->affix_table->s_strippable) ||
		              (0 < dict->affix_table->p_strippable);
		have_mprefix = (0 < dict->affix_table->mp_strippable);
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
	 */
	/* XXX FIXME: currently, we are doing a check for capitalized first
	 * words in the build_sentence_expressions() routine, but really, 
	 * we should be doing it here, and building two alternatives, one
	 * capitalized, and one not.  This is particularly true for the
	 * Russian dictionaries, where captialization and suffix splitting
	 * only 'accidentally' work, due to CAPITALIZED_WORDS in the regex
	 * file, which matches capitalized stems (for all the wrong reasons...)
	 * Now that we have alternatives, we should use them wisely.
	 *
	 * Well, actually, easier said-than-done. The problem is that
	 * captilized words will match regex's during build_expressions,
	 * and we sometimes need to wipe those out...
	 */
#if 0
	if (is_capitalizable(sent, sent->length) && is_utf8_upper(word)) {}
#endif

	if (word_is_in_dict && !have_suffix && !have_mprefix)
	{
		issue_sentence_word(sent, word, quote_found);
		return;
	}


	/* Ugly hack: "let's" is in the english dict, but "Let's" is not.
	 * Thus, without this fix, sentences beginning with upper-case Let's
	 * fail, while lower-case pass! So we work around this, since let's
	 * can also split into two ...
	 */
	if (is_capitalizable(sent, sent->length) && is_utf8_upper(word))
	{
		char temp_word[MAX_WORD+1];
		downcase_utf8_str(temp_word, word, MAX_WORD);
		word_is_in_dict = find_word_in_dict (dict, temp_word);
		if (word_is_in_dict)
		{
			const char *lc = string_set_add(temp_word, sent->string_set);
			issue_sentence_word(sent, lc, quote_found);
			return;
		}
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
		 * empty suffix, then just skip right over to suffix processing.
		 */
		if (word_is_in_dict && have_suffix)
			goto do_suffix_processing;
		if (word_is_in_dict && have_mprefix)
			goto do_mprefix_processing;

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
	if (!word_is_in_dict || have_suffix)
	{
		Tokenizer *tokenizer = &sent->tokenizer;

		tokenizer->string_set = dict->string_set;
		tokenizer->pref_alternatives = NULL;
		tokenizer->stem_alternatives = NULL;
		tokenizer->suff_alternatives = NULL;

		/* If the word is in the dict and can also be split into two,
		 * then we need to insert a two-word version, with the second
		 * word being the empty word.  This is a crazy hack to always
		 * keep a uniform word-count */
if (ALTOLD) {
		if (word_is_in_dict) {
			if (2 < verbosity)
				printf("Info: Adding %s + empty-word\n", word);
			add_suffix_alternatives(tokenizer, word, "");
		}

		word_is_in_dict |= suffix_split(tokenizer, dict, w, wend);
} else {
		if (suffix_split(tokenizer, dict, w, wend)) {
			if (word_is_in_dict) {
				if (2 < verbosity)
					printf("Info: Adding %s + empty-word\n", word);
				if (ALTOLD)
				{
				add_suffix_alternatives(tokenizer, word, "");
				}
				else
				{
					const char *wp = word;
					const char *empty_word = "";
					add_alternative(sent, 0,0, 1,&wp, 1,&empty_word);
				}
			}
			word_is_in_dict = TRUE;
		} else if (word_is_in_dict) {
			if (2 < verbosity)
				printf("Info: Cannot split %s - will be added w/o alternatives\n", word);
		}
	}
}

do_mprefix_processing:
	if (!word_is_in_dict || have_mprefix)
	{
		if (word_is_in_dict)
		{
			const char *wp = word;
			add_alternative(sent, 0,0, 1,&wp, 0,0);
		}
		if (mprefix_split(sent, dict, word))
		{
			word_is_in_dict = TRUE;
		}
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
		if (3 < verbosity)
		{
			printf("Info: separate_word() spellcheck_suggest for %s:%s\n", word, n == 0 ? " (nothing)" : "");
			for (j=0; j<n; j++) {
				printf("- %s\n", alternates[j]);
			}
		}
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
	{
		if (ALTOLD)
		issued = issue_alternatives(sent, quote_found);
		else
		issued = issue_alternatives1(sent, word, quote_found);
	}

	if (FALSE == issued)
		issue_sentence_word(sent, word, quote_found);

	for (i = n_r_stripped - 1; i >= 0; i--)
	{
		issue_sentence_word(sent, r_stripped[i], FALSE);
	}

	if (3 < verbosity)
		print_sentence_word_alternatives(sent);
}

/**
 * The string s has just been read in from standard input.
 * This function breaks it up into words and stores these words in
 * the sent->word[] array.  Returns TRUE if all is well, FALSE otherwise.
 * Quote marks are treated just like blanks.
 */
Boolean separate_sentence(Sentence sent, Parse_Options opts)
{
	const char * word_end;
	Boolean quote_found;
	Dictionary dict = sent->dict;
	mbstate_t mbs;
	const char * word_start = sent->orig_sentence;

	sent->length = 0;

	if (dict->left_wall_defined)
		issue_sentence_word(sent, LEFT_WALL_WORD, FALSE);

	/* Reset the multibyte shift state to the initial state */
	memset(&mbs, 0, sizeof(mbs));

	for(;;) 
	{
		int isq;
		wchar_t c;
		int nb = mbrtowc(&c, word_start, MB_CUR_MAX, &mbs);
		quote_found = FALSE;

		if (0 > nb) goto failure;

		/* Skip all whitespace. Also, ignore *all* quotation marks.
		 * XXX This is sort-of a hack, but that is because LG does
		 * not have any intelligent support for quoted character
		 * strings at this time.
		 */
		isq = is_quote (c);
		if (isq) quote_found = TRUE;
		while (is_space(c) || isq)
		{
			word_start += nb;
			nb = mbrtowc(&c, word_start, MB_CUR_MAX, &mbs);
			if (0 == nb) break;
			if (0 > nb) goto failure;
			isq = is_quote (c);
			if (isq) quote_found = TRUE;
		}

		if ('\0' == *word_start) break;

      /* Loop over non-blank characters until word-end is found. */
		word_end = word_start;
		nb = mbrtowc(&c, word_end, MB_CUR_MAX, &mbs);
		if (0 > nb) goto failure;
		while (!is_space(c) && !is_quote(c) && (c != 0) && (0 < nb))
		{
			word_end += nb;
			nb = mbrtowc(&c, word_end, MB_CUR_MAX, &mbs);
			if (0 > nb) goto failure;
		}

      /* Perform prefix, suffix splitting, if needed */
		separate_word(sent, opts, word_start, word_end, quote_found);
		word_start = word_end;
		if ('\0' == *word_start) break;
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
	if (3 < verbosity)
	{
		printf("Info: guess_misspelled_word() spellcheck_suggest for %s:%s\n", s, n == 0 ? " (nothing)" : "");
		for (j=0; j<n; j++) {
			printf("- %s\n", alternates[j]);
		}
	}
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
 * should we issue two words, or one?  ... and so we can't correctly
 * build sentence expressions for this, since we don't have the correct
 * word count by this point.  The viterbi incremental parser won't have
 * this bug.
 */
void build_sentence_expressions(Sentence sent, Parse_Options opts)
{
	int i;
	const char *s;
	const char * regex_name;
	X_node * e;
	Dictionary dict = sent->dict;

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
			 * XXX For the first-word case, we should be handling capitalization
			 * as an alternative, when doing separate_word(), and not here.
			 * separate_word() should build capitalized and non-capitalized
			 * altenatives.  This is especially true for Russian, where we
			 * need to deal with capitalized stems; this is not really the
			 * right place to do it, and this works 'by accident' only because
			 * there is a CAPITALIZED_WORDS regex match for Russian that matches
			 * stems. Baaaddd.
			 */
			if (is_capitalizable(sent, i) && is_utf8_upper(s))
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
					if (2 < verbosity)
						printf ("Info: First word: %s is_entity=%d is_common=%d\n", 
						        s, is_entity(sent->dict,s),
						        is_common_entity(sent->dict,lc));

					if (is_entity(sent->dict,s) ||
					    is_common_entity(sent->dict,lc))
					{
						/* If we are here, then we want both upper and lower case
						 * expressions. The upper-case ones were built above, so now
						 * append the lower-case ones. */
						e = build_word_expressions(sent->dict, lc);
						we = catenate_X_nodes(we, e);
					}
					else
					{
						if (2 < verbosity)
							printf("Info: First word: %s downcase only\n", lc);
	
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

/* DEBUG */
static void print_sentence_word_alternatives(Sentence sent)
{
	int wi;
	int ai;
	printf("Words and alternatives:\n");
	for (wi=0; wi<sent->length; wi++) {
		Word w = sent->word[wi];
		printf("  word%d: %s\n  ", wi, w.unsplit_word);
		if (w.alternatives && w.alternatives[0]) {
			int w_start = wi;
			for (ai=0; ;  ai++) {
				Boolean alt_exists = w.alternatives[ai] != NULL;
				if (alt_exists)
					printf("   alt%d:", ai);
				for (wi=w_start; wi==w_start || (wi<sent->length && !sent->word[wi].unsplit_word); wi++) {
					if (w.alternatives[ai] == NULL && sent->word[wi].alternatives[ai]) {
						char errmsg[128];
						sprintf(errmsg, "  Internal error: extra alt %d for word %d\n", wi, ai);
						assert(0, errmsg);
					}
					if (alt_exists)
						printf(" %s", sent->word[wi].alternatives[ai]);
				}
				if (!alt_exists)
					break;
			}
			wi--;
			printf("\n");
		}
	}
	printf("\n");
}
