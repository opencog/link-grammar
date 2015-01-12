/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2009, 2012-2014 Linas Vepstas                           */
/* Copyright (c) 2014 Amir Plivatsky                                     */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _WIN32
#include <langinfo.h>
#endif
#include <limits.h>

#ifdef USE_ANYSPLIT
#include "anysplit.h"
#endif
#include "build-disjuncts.h"
#include "dict-api.h"
#include "dict-common.h"
#include "error.h"
#include "externs.h"
#include "print.h"
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
/* I've left these here, as an example of what to expect. */
/*static char * strip_left[] = {"(", "$", "``", NULL}; */
/*static char * strip_right[] = {")", "%", ",", ".", ":", ";", "?", "!", "''", "'", "'s", NULL};*/
/* Single-quotes are used for abbreviations, don't mess with them */
/*//const char * qs = "\"\'«»《》【】『』‘’`„“"; */
/*const char* qs = "\"«»《》【】『』`„“"; */

#define ENTITY_MARKER   "<marker-entity>"
#define COMMON_ENTITY_MARKER   "<marker-common-entity>"

/**
 * is_common_entity - Return true if word is a common noun or adjective
 * Common nouns and adjectives are typically used in corporate entity
 * names -- e.g. "Sun State Bank" -- "sun", "state" and "bank" are all
 * common nouns.
 */
static bool is_common_entity(Dictionary dict, const char * str)
{
	if (word_contains(dict, str, COMMON_ENTITY_MARKER) == 1)
		return true;
	return false;
}

static bool is_entity(Dictionary dict, const char * str)
{
	const char * regex_name;
	if (word_contains(dict, str, ENTITY_MARKER) == 1)
		return true;
	regex_name = match_regex(dict->regex_root, str);
	if (NULL == regex_name) return false;
	return word_contains(dict, regex_name, ENTITY_MARKER);
}


#if defined HAVE_HUNSPELL || defined HAVE_ASPELL
/**
 * Return true if word is a proper name.
 * XXX This is a cheap hack that works only in English, and is
 * broken for German!  We need to replace this with something
 * language-specific.
 *
 * Basically, if word starts with upper-case latter, we assume
 * its a proper name, and that's that.
 */
static bool is_proper_name(const char * word)
{
	return is_utf8_upper(word);
}
#endif /* defined HAVE_HUNSPELL || defined HAVE_ASPELL */

/**
 * AFDICT_QUOTES defines a string containing anything that can be
 * construed to be a quotation mark. This works, because link-grammar
 * is more or less ignorant of quotes at this time.
 * Return true if the character is a quotation character.
 */
static bool is_quote(Dictionary dict, wchar_t wc)
{
	const wchar_t *quotes =
		(const wchar_t *)AFCLASS(dict->affix_table, AFDICT_QUOTES)->string;

	if (NULL == quotes) return false;
	return (NULL !=  wcschr(quotes, wc));
}

/**
 * AFDICT_BULLETS defines a string containing anything that can be
 * construed to be a bullet.  Return TRUE if the character is a bullet
 * character.
 */
static bool is_bullet(Dictionary dict, wchar_t wc)
{
	const wchar_t *bullets =
		(const wchar_t *)AFCLASS(dict->affix_table, AFDICT_BULLETS)->string;

	if (NULL == bullets) return false;
	return (NULL !=  wcschr(bullets, wc));
}

static bool is_bullet_str(Dictionary dict, const char * str)
{
	mbstate_t mbs;
	wchar_t c;
	int nb;
	memset(&mbs, 0, sizeof(mbs));
	nb = mbrtowc(&c, str, MB_CUR_MAX, &mbs);
	if (0 > nb) return false;
	return is_bullet(dict, c);
}

/**
 * Return TRUE if the character is white-space
 */
static bool is_space(wchar_t wc)
{
	if (iswspace(wc)) return true;

	/* 0xc2 0xa0 is U+00A0, c2 a0, NO-BREAK SPACE */
	/* For some reason, iswspace doesn't get this */
	if (0xa0 == wc) return true;

	/* iswspace seems to use somewhat different rules than what we want,
	 * so over-ride special cases in the U+2000 to U+206F range.
	 * Caution: this potentially screws with arabic, and right-to-left
	 * languages.
	 */
/***  later, not now ..
	if (0x2000 <= wc && wc <= 0x200f) return true;
	if (0x2028 <= wc && wc <= 0x202f) return true;
	if (0x205f <= wc && wc <= 0x206f) return true;
***/

	return false;
}

#if 0
/**
 * Returns true if the word can be interpreted as a number.
 * The ":" is included here so we allow "10:30" to be a number.
 * The "." and "," allow numbers in both US and European notation:
 * e.g. American million: 1,000,000.00  Euro million: 1.000.000,00
 * We also allow U+00A0 "no-break space"
 */
static bool is_number(const char * s)
{
	mbstate_t mbs;
	int nb = 1;
	wchar_t c;
	if (!is_utf8_digit(s)) return false;

	memset(&mbs, 0, sizeof(mbs));
	while ((*s != 0) && (0 < nb))
	{
		nb = mbrtowc(&c, s, MB_CUR_MAX, &mbs);
		if (iswdigit(c)) { s += nb; }

		/* U+00A0 no break space */
		else if (0xa0 == c) { s += nb; }

		else if ((*s == '.') || (*s == ',') || (*s == ':')) { s++; }
		else return false;
	}
	return true;
}
#endif

#if defined HAVE_HUNSPELL || defined HAVE_ASPELL
/**
 * Returns true if the word contains digits.
 */
static bool contains_digits(const char * s)
{
	mbstate_t mbs;
	int nb = 1;
	wchar_t c;

	memset(&mbs, 0, sizeof(mbs));
	while ((*s != 0) && (0 < nb))
	{
		nb = mbrtowc(&c, s, MB_CUR_MAX, &mbs);
		if (nb < 0) return false;
		if (iswdigit(c)) return true;
		s += nb;
	}
	return false;
}
#endif /* defined HAVE_HUNSPELL || defined HAVE_ASPELL */

/**
 * Accumulate different word-stemming possibilities.
 *
 * Add to the sentence prefnum elements from prefix,
 * stemnum elements from stem, and suffnum elements from suffix.
 * Mark the prefixes and suffixes.
 * Balance all alternatives using empty words.
 * Do not add an empty string (as first token)
 * (Can it happen? I couldn't find such a case. [ap])
 *
 * TODO: Support middle morphemes.
 *
 * BALANCING: the parser needs it for now. It is probably better
 * to move it to build_sentence_expressions().
 */
#ifndef USE_ANYSPLIT
static
#endif
void add_alternative(Sentence sent,
				int prefnum, const char **prefix,
				int stemnum, const char **stem,
				int suffnum, const char **suffix)
{
	int t_start = sent->t_start; /* word starting the current token sequence */
	int t_count = sent->t_count; /* number of words in it */
	int len;                     /* word index */
	int ai = 0;                  /* affix index */
	const char **affix;          /* affix list pointer */
	const char **affixlist[] = { prefix, stem, suffix, NULL };
	int numlist[] = { prefnum, stemnum, suffnum };
	enum affixtype { PREFIX, STEM, SUFFIX, END };
	enum affixtype at;
	char infix_mark = INFIX_MARK(sent->dict->affix_table);
	char buff[MAX_WORD+2];

	lgdebug(+3, "Word%s:",
		(prefnum + stemnum + suffnum > 1) ? " split into" : "");

	for (at = PREFIX; at != END; at++)
	{
		int affixnum = numlist[at];
		for (affix = affixlist[at]; affixnum-- > 0; affix++, ai++)
		{
			assert(ai <= t_count, "add_alternative: word index > t_count");
			if (ai == 0 && *affix[0] == '\0')   /* can it happen? */
			{
				lgdebug(+1, "Empty string given - shouldn't happen"
				 " (type %d, argnum %d/%d/%d)\n", at, prefnum, stemnum, suffnum);
				return;
			}

			if (ai == t_count)   /* need to add a word */
			{
				len = t_start + t_count;

				/* allocate new word */
				sent->word = realloc(sent->word, (len+1)*sizeof(Word));
				sent->word[len].x= NULL;
				sent->word[len].d= NULL;
				sent->word[len].alternatives = NULL;
				sent->word[len].unsplit_word = NULL;
				sent->word[len].firstupper = false;

				sent->post_quote = realloc(sent->post_quote, (len+1)*sizeof(bool));
				sent->post_quote[len] = false;

				t_count++;
				if (t_count > 1) /* not first added word */
				{
					/* BALANCING alternative total number at the added new word */
					int i;

					int numalt = altlen(sent->word[t_start].alternatives)-1;
					const char **alt =
					   (const char **)malloc((numalt+1) * sizeof(const char *));
					for (i = 0; i < numalt; i++)
						alt[i] = string_set_add(EMPTY_WORD_MARK, sent->string_set);
					alt[numalt] = NULL;
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
			 * - Mark first word of an alternative sequence if any prefix or stem
			 *   is capitalized.
			 * This is intended to be the same as previously done for en and ru
			 * (prefix is not used).
			 */
			switch (at)
			{
				size_t sz;

				case PREFIX: /* set to word= */
					sz = MIN(strlen(*affix), MAX_WORD);
					strncpy(buff, *affix, sz);
					buff[sz] = infix_mark;
					buff[sz+1] = '\0';
					break;
				case STEM:   /* already word, word.=, word.=x */
					/* Stems are already marked with a stem subscript, if needed.
					 * The possible marks are set in the affix class STEMSUBSCR. */
					sz = MIN(strlen(*affix), MAX_WORD);
					strncpy(buff, *affix, sz);
					buff[sz] = '\0';
					break;
				case SUFFIX: /* set to =word */
					/* If the suffix starts with an apostrophe, don't mark it */
					if ((('\0' != (*affix)[0]) && !is_utf8_alpha(*affix)) ||
					    '\0' == infix_mark || test_enabled("no-suffixes"))
					{
						sz = MIN(strlen(*affix), MAX_WORD);
						strncpy(buff, *affix, sz);
						buff[sz] = '\0';
						break;
					}
					sz = MIN(strlen(*affix) + 1, MAX_WORD);
					buff[0] = infix_mark;
					strncpy(&buff[1], *affix, sz);
					buff[sz] = '\0';
					break;
				case END:
					assert(true, "affixtype END reached");
			}
			/* firstupper is false for suffixes - start with INFIX_MARK */
			if (is_utf8_upper(buff)) sent->word[t_start].firstupper = true;
			lgdebug(3, " %s", ('\0' == buff[0]) ? "[empty_suffix]" : buff);
			altappend(sent, &sent->word[t_start + ai].alternatives, buff);
		}
	}
	lgdebug(3, "\n");

	/* BALANCING: add an empty alternative to the rest of words */
	for (len = t_start + ai; len < t_start + t_count; len++)
	{
		altappend(sent, &sent->word[len].alternatives, EMPTY_WORD_MARK);
	}
	sent->t_count = t_count;
}

#if defined HAVE_HUNSPELL || defined HAVE_ASPELL
/**
 * Return true if an alternative has been issued for the current word.
 * t_count==0 if and only if no alternative has been issued yet.
 */
static bool word_has_alternative(Sentence sent) {
	return (sent->t_count > 0);
}
#endif /* defined HAVE_HUNSPELL || defined HAVE_ASPELL */

/**
 * Terminate issuing alternatives to the current input word.
 * The word argument is the input word.
 */
static bool issue_alternatives(Sentence sent,
		const char *word, bool quote_found)
{
	int t_start = sent->t_start;
	int t_count = sent->t_count;

	if (0 == t_count) return false;

	sent->word[t_start].unsplit_word = string_set_add(word, sent->string_set);
	sent->post_quote[t_start] = quote_found;

	sent->length += t_count;
	sent->t_start = sent->length;
	sent->t_count = 0;

	if (3 < verbosity)
		print_sentence_word_alternatives(sent, true, NULL, NULL);

	return true;
}

/**
 * Make the string 's' be the next word of the sentence.
 * That is, it looks like 's' is a word we can handle, so record it
 * as a bona-fide word in the sentence.  Increment the sentence length
 * when done.
 *
 * Do not issue the empty string (checked in add_alternative()).
 */
static void issue_sentence_word(Sentence sent, const char * s, bool quote_found)
{
	add_alternative(sent, 0,NULL, 1,&s, 0,NULL);
	(void) issue_alternatives(sent, s, quote_found);
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

	So, under this system, the dictionary could have the words "Ill" and
	also the word "Ill."  It could also have the word "i.e.", which could be
	used in a sentence.
*/

#undef		MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

/**
 * Add the given prefix, word and suffix as an alternative.
 * IF STEMSUBSCR is define in the affix file, use its values as possible
 * subscripts for the word.
 *
 */
static bool add_alternative_with_subscr(Sentence sent, const char * prefix,
                                        const char * word, const char * suffix)
{
	Dictionary dict = sent->dict;
	Afdict_class * stemsubscr_list =
	   AFCLASS(dict->affix_table, AFDICT_STEMSUBSCR);
	const char ** stemsubscr = stemsubscr_list->string;
	size_t stemsubscr_count = stemsubscr_list->length;
	bool word_is_in_dict = false;
	char infix_mark = INFIX_MARK(dict->affix_table);

	if (0 == stemsubscr_count)
	{
		add_alternative(sent, (prefix ? 1 : 0),&prefix, 1,&word,
		                      (suffix ? 1 : 0),&suffix);

		/* If this is not a morpheme split (INFIX_MARK==NUL), the word is not
		 * considered to be in the dict. This is important since then it can
		 * match a regex. For example: 1960's, which may get split to: 1960 's */
		if ('\0' != infix_mark) word_is_in_dict = true;
	}
	else
	{
		size_t si;
		size_t wlen = strlen(word);
		size_t slen = 0;
		char *w;

		for (si = 0; si < stemsubscr_count; si++)
		{
			slen = MAX(slen, strlen(stemsubscr[si]));
		}
		w = alloca(wlen + slen + 1);
		strcpy(w, word);

		for (si = 0; si < stemsubscr_count; si++)
		{
			strcpy(&w[wlen], stemsubscr[si]);

			/* We should not match regexes to stems. */
			if (boolean_dictionary_lookup(dict, w))
			{
				add_alternative(sent, (prefix ? 1 : 0),&prefix,
				                1,(const char **)&w, 1,&suffix);
				word_is_in_dict = true;
			}
		}
	}

	return word_is_in_dict;
}

/**
 * Split word into prefix, stem and suffix.
 * In can also split contracted words (like he's).
 *
 * Return true if the word can split.
 * Note: If a word can split it doesn't say it is a real dictionary word,
 * as there can still be no links between some of its parts.
 *
 * The prefix code is only lightly validated by actual use.
 */
static bool suffix_split(Sentence sent, const char *w, const char *wend)
{
	int i, j;
	Afdict_class *prefix_list, *suffix_list;
	int p_strippable, s_strippable;
	const char **prefix, **suffix;
	const char *no_suffix = NULL;
	bool word_can_split = false;
	Dictionary dict = sent->dict;
	char *newword = alloca(wend-w+1);

	/* Set up affix tables. */
	if (NULL == dict->affix_table) return false;
	prefix_list = AFCLASS(dict->affix_table, AFDICT_PRE);
	p_strippable = prefix_list->length;
	prefix = prefix_list->string;
	suffix_list = AFCLASS(dict->affix_table, AFDICT_SUF);
	s_strippable = suffix_list->length;
	suffix = suffix_list->string;

	if (INT_MAX == s_strippable) return false;

	/* Go through once for each suffix; then go through one
	 * final time for the no-suffix case (i.e. to look for
	 * prefixes only, without suffixes). */
	for (i = 0; i <= s_strippable; i++, suffix++)
	{
		bool did_split = false;
		size_t suflen = 0;
		if (i < s_strippable)
		{
			suflen = strlen(*suffix);
			 /* The remaining w is too short for a possible match.
			  * In addition, don't allow empty stems. */
			if ((wend-suflen) < (w+1)) continue;

			/* A lang like Russian allows empty suffixes, which have a real
			 * morphological linkage. In the following check, the empty suffix
			 * always matches. */
			if (0 == strncmp(wend-suflen, *suffix, suflen))
			{
				size_t sz = MIN((wend-w)-suflen, MAX_WORD);
				strncpy(newword, w, sz);
				newword[sz] = '\0';

				/* Check if the remainder is in the dictionary.
				 * In case we handle a contracted word, the first word
				 * may match a regex. Hence find_word_in_dict() is used and
				 * not boolean_dictionary_lookup(). */
				if (find_word_in_dict(dict, newword))
				{
					did_split =
						add_alternative_with_subscr(sent, NULL, newword, *suffix);
					word_can_split |= did_split;
				}
			}
		}
		else
		{
			suflen = 0;
			suffix = &no_suffix;
		}

		/*
		 * Try stripping off prefixes. Avoid double-counting and
		 * other trouble by doing this only if we split off a suffix,
		 * or if there is no suffix.
		 */
		if (did_split || 0==suflen)
		{
			for (j = 0; j < p_strippable; j++)
			{
				size_t prelen = strlen(prefix[j]);
				/* The remaining w is too short for a possible match. */
				if ((wend-w) - suflen < prelen) continue;
				if (strncmp(w, prefix[j], prelen) == 0)
				{
					size_t sz = MIN((wend-w) - suflen - prelen, MAX_WORD);

					strncpy(newword, w+prelen, sz);
					newword[sz] = '\0';
					/* ??? Do we need a regex match? */
					if (boolean_dictionary_lookup(dict, newword))
					{
						word_can_split |=
					 	add_alternative_with_subscr(sent, prefix[j], newword, *suffix);
					}
				}
			}
		}
	}

	return word_can_split;
}

#define HEB_PRENUM_MAX 5   /* no more than 5 prefix "subwords" */
#define HEB_MPREFIX_MAX 16 /* >mp_strippable (13) */
#define HEB_UTF8_BYTES 2   /* Hebrew UTF8 characters are always 2-byte */
#define HEB_CHAREQ(s, c) (strncmp(s, c, HEB_UTF8_BYTES) == 0)
/**
 * Handle "formative letters"  ב, ה, ו, כ, ל, מ, ש.
 * Split word into multiple prefix "subwords" (1-3 characters each)
 * and an unprefixed word (which must be in the dictionary or be null)
 * in all possible ways (even when the prefix combination is not valid,
 * the LG rules will resolve that).
 * If the whole word (i.e. including the prefixes) is in the dictionary,
 * the word will be added in separate_word().
 * Add all the alternatives.
 * The assumptions used prevent a large number of false splits.
 * They may be relaxed later.
 * 
 * XXX Because the grammatical rules of which prefixes are valid for the
 * remaining word are not checked, non-existing words may get split. In such a
 * case there is no opportunity for a regex or spell check of this unknown word.
 * FIXME Before issuing an alternative, validate that the combination is
 * supported by the dict.
 *
 * Note: This function currently does more than absolutely needed for LG,
 * in order to simplify the initial Hebrew dictionary.
 * It may be latter replaced by a simpler version.
 *
 * These algorithm is most probably very Hebrew-specific.
 * These assumptions are used:
 * - the prefix consists of subwords
 * - longer subwords have priority over shorter ones
 * - subwords in a prefix are unique ('ככ' is considered here as one "subword")
 * - input words with length <= 2 don't have a prefix
 * - each character uses 2 bytes
 * - the input word contains only Hebrew characters
 * - the letter "ו" (vav) can only be the first prefix subword
 * - if the last prefix subword is not "ו" and the word (length>2) starts
 *   with 2 "ו", the actual word to be looked up starts with one "ו"
 *   (see also TBD there)
 * - a prefix can be stand-alone (an input word that consists of prefixes)
 *
 * To implement this function in a way which is appropriate for more languages,
 * Hunspell-like definitions (but more general) are needed.
 */
static bool mprefix_split(Sentence sent, const char *word)
{
	int i;
	Afdict_class *mprefix_list;
	int mp_strippable;
	const char **mprefix;
	const char *newword;
	const char *w;
	int sz = 0;
	bool word_is_in_dict;
	int split_prefix_i = 0;      /* split prefix index */
	const char *split_prefix[HEB_PRENUM_MAX]; /* the whole prefix */
	bool *pseen;                 /* prefix "subword" seen (not allowed again) */
	Dictionary dict = sent->dict;
	int wordlen;
	int wlen;
	int plen = 0;

	/* set up affix table  */
	if (NULL == dict->affix_table) return false;
	mprefix_list = AFCLASS(dict->affix_table, AFDICT_MPRE);
	mp_strippable = mprefix_list->length;
	if (0 == mp_strippable) return false;
	/* The mprefix list is revered-sorted according to prefix length.
	 * The code here depends on that. */
	mprefix = mprefix_list->string;

	pseen = alloca(mp_strippable * sizeof(*pseen));
	/* Assuming zeroed-out bytes are interpreted as false. */
	memset(pseen, 0, mp_strippable * sizeof(*pseen));

	word_is_in_dict = false;
	w = word;
	wordlen = strlen(word);  /* guaranteed < MAX_WORD by separate_word() */
	do
	{
		for (i=0; i<mp_strippable; i++)
		{
			/* subwords in a prefix are unique */
			if (pseen[i])
				continue;

			/* the letter "ו" can only be the first prefix subword */
			if ((split_prefix_i > 0) &&
			 HEB_CHAREQ(mprefix[i], "ו") && (HEB_CHAREQ(w, "ו")))
			{
				continue;
			}

			plen = strlen(mprefix[i]);
			wlen = strlen(w);
			sz = wlen - plen;
			if (strncmp(w, mprefix[i], plen) == 0)
			{
				newword = w + plen;
				/* check for non-vav before vav */
				if (!HEB_CHAREQ(mprefix[i], "ו") && (HEB_CHAREQ(newword, "ו")))
				{
					/* non-vav before a single-vav - not in a prefix */
					if (!HEB_CHAREQ(newword+HEB_UTF8_BYTES, "ו"))
						break;

					/* non-vav before 2-vav */
					if (newword[HEB_UTF8_BYTES+1])
						newword += HEB_UTF8_BYTES; /* strip one 'ו' */
					/* TBD: check word also without stripping. */
				}
				pseen[i] = true;
				split_prefix[split_prefix_i++] = mprefix[i];
				if (0 == sz) /* empty word */
				{
					word_is_in_dict = true;
					/* add the prefix alone */
					lgdebug(+3, "Whole-word prefix: %s\n", word);
					add_alternative(sent, split_prefix_i,split_prefix, 0,NULL, 0,NULL);
					/* if the prefix is a valid word,
					 * it has been added in separate_word() as a word */
					break;
				}
				if (find_word_in_dict(dict, newword))
				{
					word_is_in_dict = true;
					lgdebug(+3, "Splitting off a prefix: %.*s-%s\n",
					        wordlen-sz, word, newword);
					add_alternative(sent, split_prefix_i,split_prefix, 1,&newword, 0,NULL);
				}
				w = newword;
				break;
			}
		}
	/* "wlen + sz < wordlen" is true if a vav has been stripped */
	} while ((sz > 0) && (i < mp_strippable) && (newword != w + plen) &&
	 (split_prefix_i < HEB_PRENUM_MAX));

	return word_is_in_dict;
}

/* Return true if the word might be capitalized by convention:
 * -- if its the first word of a sentence
 * -- if its the first word following a colon, a period, or any bullet
 *    (For example:  VII. Ancient Rome)
 * -- if its the first word of a quote (ignored for an incomplete sentence)
 *
 * XXX FIXME: These rules are rather English-centric.  Someone should
 * do something about this someday.
 */
static bool is_capitalizable(Sentence sent, size_t curr_word)
{
	size_t first_word; /* the index of the first word after the wall */
	Dictionary dict = sent->dict;

	if (dict->left_wall_defined) {
		first_word = 1;
	} else {
		first_word = 0;
	}

	/* Words at the start of sentences are capitalizable */
	if (curr_word == first_word) return true;

	/* Words following colons are capitalizable */
	if (curr_word > 0)
	{
		if (strcmp(":", sent->word[curr_word-1].alternatives[0]) == 0 ||
		    strcmp(".", sent->word[curr_word-1].alternatives[0]) == 0 )
			return true;
		if (is_bullet_str(dict, sent->word[curr_word-1].alternatives[0]))
			return true;
	}

	/* First word after a quote mark can be capitalized */
	if ((curr_word < sent->length) && sent->post_quote[curr_word])
		return true;

	return false;
}

#if defined HAVE_HUNSPELL || defined HAVE_ASPELL
/* ??? Is it useful to have a limit to the number of guesses? Will it be
 * better to change !spell to be an integer which will be this limit? */
#define MAX_NUM_SPELL_GUESSES 60
/**
 * Try to spell guess an unknown word, and issue the results as alternatives.
 * There are two kind of guesses:
 * - Separating run-on words into an exact combination of words, usually 2.
 * - Find similar words.
 *
 * Return true if corrections have been issued, else false.
 *
 * Note: spellcheck_suggest(), which is invoked by this function, returns
 * guesses for words containing numbers (including words consisting of digits
 * only). Hence this function should not be called for such words.
 */
static bool guess_misspelled_word(Sentence sent, const char * word,
                                  bool quote_found, Parse_Options opts)
{
	Dictionary dict = sent->dict;
	//int runon_word_corrections = 0;
	int num_guesses = 0;
	int j, n;
	char *sp = NULL;
	const char *wp;
	char **alternates = NULL;

	/* If the spell-checker knows about this word, and we don't ...
	 * Dang. We should fix it someday. Accept it as such. */
	if (spellcheck_test(dict->spell_checker, word)) return false;

	/* Else, ask the spell-checker for alternate spellings
	 * and see if these are in the dict. */
	n = spellcheck_suggest(dict->spell_checker, &alternates, word);
	if (3 < verbosity)
	{
		printf("Info: guess_misspelled_word() spellcheck_suggest for %s:%s\n",
		       word, (0 == n) ? " (nothing)" : "");
		for (j=0; j<n; j++) {
			printf("- %s\n", alternates[j]);
		}
	}
	/* Word split for run-on and guessed words.
	 * FIXME: Since we don't have multi-level hierarchical alternatives
	 * (or even 2-level), we can do it only for certain cases.
	 * For a general implementation of word splits, we need to issue
	 * run-on words as separate words, with a 2nd-level alternatives
	 * marks.
	 */
	for (j=0; j<n; j++)
	{
		/* The word might be a run-on of two or more words. */
		sp = strchr(alternates[j], ' ');
		if (sp)
		{
			/* Run-on words */
			/* It may be 2 run-on words or more. Loop over all */
			const char **runon_word = NULL;

			wp = alternates[j];
			do
			{
				*sp = '\0';
				altappend(sent, &runon_word, wp);
				wp = sp+1;
				sp = strchr(wp, ' ');
			} while (sp);
			altappend(sent, &runon_word, wp);
			add_alternative(sent, 0,NULL, altlen(runon_word),runon_word, 0,NULL);
			free(runon_word);

			//runon_word_corrections++;
			num_guesses++;
		}
		else
		{
			/* A spell guess.
			 * ??? Should regex guess included for spell guesses?  But anyway
			 * build_sentence_expressions() cannot handle that for now.
			 */

			if (boolean_dictionary_lookup(sent->dict, alternates[j]))
			{
				char str[MAX_WORD+1];

				/* Append a [~] tag to the word to signify that it's the result of
				 * guessing. This tag will be redone after fetching the word from
				 * the dictionary.
				 * XXX sent.word.alternatives should have been a struct with
				 * a field to mark corrected words. */
				snprintf(str, MAX_WORD, "%.*s[~]", MAX_WORD-3, alternates[j]);
				wp = str;
				add_alternative(sent, 0,NULL, 1,&wp, 0,NULL);
				num_guesses++;
			}
			//else printf("Spell guess '%s' ignored\n", alternates[j]);
		}

		//if (0 < runon_word_corrections) break;
		if (num_guesses > MAX_NUM_SPELL_GUESSES) break;
	}
	if (alternates) spellcheck_free_suggest(alternates, n);

	if (num_guesses > 0)
	{
		/* Issue the alternatives to the original word. */
		issue_alternatives(sent, word, quote_found);
	}
	return (num_guesses > 0);
}
#endif /* HAVE_HUNSPELL */

/**
 * Strip off punctuation, etc. on the left-hand side.
 */
static const char * strip_left(Sentence sent, const char * w,
                               bool quote_found)
{
	Dictionary afdict = sent->dict->affix_table;
	Afdict_class * lpunc_list;
	const char * const * lpunc;
	size_t l_strippable;
	size_t i;

	if (NULL == afdict) return (w);
	lpunc_list = AFCLASS(afdict, AFDICT_LPUNC);
	l_strippable = lpunc_list->length;
	lpunc = lpunc_list->string;

	do
	{
		for (i=0; i<l_strippable; i++)
		{
			size_t sz = strlen(lpunc[i]);

			if (strncmp(w, lpunc[i], sz) == 0)
			{
				lgdebug(2, "w='%s' issue lpunc '%s'\n", w, lpunc[i]);
				issue_sentence_word(sent, lpunc[i], quote_found);
				w += sz;
				break;
			}
		}
	} while (i != l_strippable);

	return (w);
}

/**
 * Split off punctuation and units from the right.
 *
 * Comments from a previous rewrite try of right stripping, not exactly applied
 * to the current method but discuss some relevant problems.
 * ---
 * FIXME: Adapt and move to separate_word().
 * Punctuation and units are removed from the right-hand side of a word,
 * one at a time, until the stem is found in the dictionary; then the
 * stripping stops.  That is, the first dictionary hit wins. This makes
 * sense for punctuation and units, but wouldn't work for morphology
 * in general.
 *
 * The only thing allowed to precede a units suffix is a number. This
 * is so that strings such as "12ft" (twelve feet) are split, but words
 * that accidentally end in "ft" are not split (e.g. "Delft blue")
 * It is enough to ensure that the string starts with a digit.
 *
 * Multiple passes allow for constructions such as 12sq.ft. That is,
 * first, "ft." is stripped, then "sq." is stripped, then "12" is found
 * in the dict.
 *
 * This code is incredibly delicate and fragile. For example, it needs
 * to parse "November 17th, 1771" with the comma stripped, the 17th
 * matched by a regex, but not get a units split for h, i.e. not get
 * the "17t h" split where h==hours-unit.
 *
 * It must also parse "7grams," with the comma split, the s in grams
 * NOT matched by S-WORD regex.
 *
 * Also, 7am must split, even though there's a regex that matches
 * this (the HMS-TIME regex).
 *
 * However, do NOT strip "sin." into "s in." (seconds, inches)
 * or "mold." into "mol d ." (chemistry mol, dioptre)
 * or "call." int "cal l ." (calorie litre)
 * Here, the period means that "sin." is not in the dict.
 *
 * Finally, it must strip the punctuation off of words that are NOT
 * in the dictionary, but will later be split into morphemes, for
 * example, in Lithuanian: "Skaitau knygą."  The period must come off.
 *
 * Basically, don't fuck with this code, unless you run the full
 * set of units sentences in 4.0.fixes.batch.
 *
 * Perhaps someday, improved morphology handling will render this code
 * obsolete. But don't your breath: the interplay between morphology
 * and regex is also quite nasty.
 * ---
 *
 * The classnum parameter is the affix class of strings to strip.
 * The rootdigit parameter adds a check that the root ends with a digit.  It is
 * used when stripping units, so we don't strip units names off potentially real
 * words.
 *
 * w points to the string starting just to the right of any left-stripped
 * characters.
 * n_r_stripped is the index of the r_stripped array, consisting of strings
 * stripped off; r_stripped[0] is the number of the first string stripped off,
 * etc.
 *
 * When it breaks out of this loop, n_r_stripped will be the number of strings
 * stripped off. It is returned through the parameter, after possibly adjusting
 * it so the root will not be null. A pointer to one character after the end of
 * the remaining word is returned through the parameter wend.
 *
 * The function returns true if an affix has been stripped (even if it
 * adjusts n_r_stripped back to 0 if the root was null), else false.
 *
 * p is a mark of the invocation position, for debugging.
 */
extern const char *const afdict_classname[]; /* For debug message only */
static bool strip_right(Sentence sent,
		                  const char *w,
                        const char **wend,
                        char const *r_stripped[],
                        size_t *n_r_stripped,
                        afdict_classnum classnum,
                        bool rootdigit,
							  	int p)
{
	Dictionary dict = sent->dict;
	Dictionary afdict = dict->affix_table;
	const char * temp_wend = *wend;
	char *word = alloca(temp_wend-w+1);
	size_t sz;
	size_t i;
	size_t nrs = 0;
	size_t len = 0;
	bool stripped = false;

	Afdict_class *rword_list;
	size_t rword_num;
	const char * const * rword;

	if (*n_r_stripped >= MAX_STRIP) return false;

	assert(temp_wend>w, "strip_right: unexpected empty word");
	if (NULL == afdict) return false;

	rword_list = AFCLASS(afdict, classnum);
	rword_num = rword_list->length;
	rword = rword_list->string;

	do
	{
		for (i = 0; i < rword_num; i++)
		{
			const char *t = rword[i];

			len = strlen(t);
			/* The remaining w is too short for a possible match */
			if ((temp_wend-w) < (int)len) continue;

			if (strncmp(temp_wend-len, t, len) == 0)
			{
				lgdebug(2, "%d: strip_right(%s): w='%s' rword '%s'\n",
				        p, afdict_classname[classnum], temp_wend-len, t);
				r_stripped[*n_r_stripped+nrs] = t;
				nrs++;
				temp_wend -= len;
				break;
			}
		}
	} while (i < rword_num && temp_wend > w && rootdigit &&
	         (*n_r_stripped+nrs < MAX_STRIP));
	assert(w <= temp_wend, "A word should never start after its end...");

	sz = temp_wend-w;
	strncpy(word, w, sz);
	word[sz] = '\0';

	/* If there is a non-null root, we require that it ends with a number,
	 * to ensure we stripped off all units. This prevents striping
	 * off "h." from "20th.".
	 * FIXME: is_utf8_digit(temp_wend-1) here can only check ASCII digits,
	 * since it is invoked with the last byte... */
	if (rootdigit && (temp_wend > w) && !is_utf8_digit(temp_wend-1))
	{
		lgdebug(2, "%d: strip_right(%s): return FALSE; root='%s' (%c is not a digit)\n",
			 p, afdict_classname[classnum], word, temp_wend[-1]);
		return false;
	}

	stripped = nrs > 0;
	if (temp_wend == w)
	{
		/* Null root - undo the last strip */
		nrs--;
		temp_wend += len;
	}

	lgdebug(2, "%d: strip_right(%s): return %s; n_r_stripped=%d+%d, wend='%s' temp_wend='%s'\n",
p, afdict_classname[classnum],stripped?"TRUE":"FALSE",(int)*n_r_stripped,(int)nrs,*wend,temp_wend);

	*n_r_stripped += nrs;
	*wend = temp_wend;
	return stripped;
}

/**
 * w points to a string, wend points to the char one after the end.  The
 * "word" w contains no blanks.  This function splits up the word if
 * necessary, and calls "issue_sentence_word()" on each of the resulting
 * parts.  The process is described above.  Returns true if OK, false if
 * too many punctuation marks or other separation error.
 *
 * This is used to split Russian words into stem+suffix, issuing a
 * separate "word" for each.  In addition, there are many English
 * constructions that need splitting:
 *
 * 86mm  -> 86 + mm (millimeters, measurement)
 * $10   ->  $ + 10 (dollar sign plus a number)
 * Surprise!  -> surprise + !  (pry the punctuation off the end of the word)
 * you've   -> you + 've  (undo contraction, treat 've as synonym for 'have')
 *
 * XXX This function is being rewritten (work in progress).
 * FIXME: Rearrange comments.
 */
#define SWLEV +3
static void separate_word(Sentence sent, Parse_Options opts,
                         const char *w, const char *wend,
                         bool quote_found)
{
	int i;
	Dictionary dict = sent->dict;
	bool word_is_in_dict = false;
	bool word_can_split = false;
	bool issued = false;
	bool try_strip_left;     /* try to strip punctuation on the left-hand side */
	bool units_alternative = false;           /* a units alternative is needed */
	bool parallel_regex;                  /* regex unknown word that can split */
	bool stripped;
	const char *wp;
	const char *temp_wend;

	size_t n_r_stripped = 0;
	const char *r_stripped[MAX_STRIP];   /* these were stripped from the right */

	/* For units alternative */
	const char *units_wend = NULL;        /* end of string consisting of units */
	size_t units_n_r_stripped = 0;
	bool start_digit = false;
	const char **r_stripped_alt;
	int ntokens;

	size_t sz = wend - w;
	/* Dynamic allocation of working buffers. */
	char *word = alloca(sz+1);                   /* candidate word main buffer */
	char *input_word = alloca(sz+1);  /* input word, possibly after left-strip */
	int downcase_size = sz+MB_LEN_MAX+1; /* pessimistic max. size of dc buffer */
	char *downcase = alloca(downcase_size);               /* downcasing buffer */
	char *str = alloca(downcase_size+sizeof("[!]"));        /* tmp word buffer */
	char *seen_word = alloca(downcase_size);    /* loop-prevention word buffer */

	downcase[0] = '\0';

	strncpy(word, w, sz);
	word[sz] = '\0';

	lgdebug(SWLEV, "Processing input word: '%s'\n", word);

	/* Strip punctuation and units from candidate word, using
	 * a linear splitting algorithm. FIXME: Handle as alternatives. */

	/* Strip off punctuation, etc. on the left-hand side.  But first check
	 * whether it is better not to do so. This happens when:
	 * 1. The word is known.
	 * 2. The word is known after punctuation right-strip.
	 * This solves the case of: By the '50s, he was very prosperous. */
	try_strip_left = true;
	if (boolean_dictionary_lookup(dict, word))
	{
		lgdebug(SWLEV, "w='%s' in dict, try_strip_left=false\n", word);
		try_strip_left = false;
	}
	if (try_strip_left)
	{
		lgdebug(SWLEV, "Testing for RPUNC: ");
		temp_wend = wend;
		stripped = strip_right(sent, w, &temp_wend, r_stripped, &n_r_stripped,
                              AFDICT_RPUNC, /*rootdigit*/false, 1);
		if (stripped)
		{
			sz = temp_wend-w;
			strncpy(str, w, sz);
			str[sz] = '\0';

			if (boolean_dictionary_lookup(dict, str))
			{
				try_strip_left = false;
				lgdebug(SWLEV, "w='%s' in dict, try_strip_left=false\n", str);
			}
			n_r_stripped = 0;
		}
	}
	if (try_strip_left)
		w = strip_left(sent, w, quote_found);

	/* Its possible that the token consisted entirely of
	 * left-punctuation, in which case, it has all been issued.
	 * So -- we're done, return. */
	if (w >= wend) return;

	/* Remember the input word. */
	sz = wend-w;
	strncpy(input_word, w, sz);
	input_word[sz] = '\0';

	/* Strip off punctuation and units, etc. on the right-hand side.  Try rpunc,
	 * then units, then rpunc, then units again, in a loop. We do this to handle
	 * expressions such as 12sqft. or 12lbs. (notice the period at end). That is,
	 * we want to strip off the "lbs." with the dot, first, rather than stripping
	 * the dot as punctuation, and then coming up empty-handed for "sq.ft"
	 * (without the dot) in the dict.  But if we are NOT able to strip off any
	 * units, then we try punctuation, and then units. This allows commas to be
	 * removed (e.g.  7grams,). */

	seen_word[0] = '\0';
	do
	{
		int temp_n_r_stripped;
		/* First, try to strip off a single punctuation, typically a comma or
		 * period, and see if the resulting word is in the dict (but not the
		 * regex). This allows "sin." and "call." to be recognized. If we don't
		 * do this now, then the next stage will split "sin." into
		 * seconds-inches, and "call." into calories-liters. */
		temp_n_r_stripped = n_r_stripped;
		temp_wend = wend;
		stripped = strip_right(sent, w, &wend, r_stripped, &n_r_stripped,
								 AFDICT_RPUNC, /*rootdigit*/false, 2);
		if (stripped)
		{
			/* w points to the remaining word,
			 * "wend" to the end of the word. */
			sz = wend-w;
			strncpy(word, w, sz);
			word[sz] = '\0';

			/* If the resulting word is in the dict, we are done. */
			if (boolean_dictionary_lookup(dict, word)) break;
			/* Undo the check. */
			wend = temp_wend;
			n_r_stripped = temp_n_r_stripped;
		}

		/* Remember the results, for a potential alternative. */
		units_wend = wend;
		units_n_r_stripped = n_r_stripped;

		/* Strip off all units, if possible. It is not likely that we strip here
		 * a string like "in." which is not a unit since we require a
		 * number before it when only a single component is stripped off. */
		temp_wend = wend;
		stripped = strip_right(sent, w, &wend, r_stripped, &n_r_stripped,
									 AFDICT_UNITS, /*rootdigit*/true, 3);
		if (!stripped)
		{
			units_wend = NULL;
			/* Try to strip off punctuation, typically a comma or period. */
			stripped = strip_right(sent, w, &wend, r_stripped, &n_r_stripped,
									 AFDICT_RPUNC, /*rootdigit*/false, 4);
		}

		/* w points to the remaining word,
		 * "wend" to the end of the word. */
		sz = wend-w;
		strncpy(word, w, sz);
		word[sz] = '\0';

		/* Avoid an infinite loop due to a repeating unknown remaining word */
		if (0 == strcmp(word, seen_word)) break;
		strcpy(seen_word, word);

	/* Any remaining dict word stops the right-punctuation stripping. */
	} while (NULL == units_wend && stripped &&
	         !boolean_dictionary_lookup(dict, word));

	/* Check whether the <number><units> "word" is in the dict
	 * (including regex). In such a case we need to generate an alternative. */
	if (units_n_r_stripped && units_wend) /* units found */
	{
		sz = units_wend-w;
		strncpy(str, w, sz);
		str[sz] = '\0';
		units_alternative = find_word_in_dict(dict, str);
	}

	/* Debug code. */
	lgdebug(SWLEV, "After strip_right: n_r_stripped=%zu (", n_r_stripped);
	if (SWLEV <= verbosity)
	{
		for (i = n_r_stripped - 1; i >= 0; i--)
			printf("[%d]='%s'%s", i, r_stripped[i], i>0 ? "," : "");
	}
	lgdebug(0 SWLEV, "), w='%s' wend='%s' word='%s' units_wend='%s' input_word='%s' "
           "units_alternative=%d\n", w, wend, word, units_wend, input_word,
			  units_alternative);

	/* If needed, add an alternative to the result of strip_right().
	 * The following is a must for cases like that:
	 * Input word: "Mr." - in the dict.
	 * The dot is stripped - "Mr" in the dict.
	 * However, the current dict doesn't accept "Mr .".
	 *
	 * For the word like "in." (units with dot) this problem doesn't happen
	 * because the dict accepts "in ." also as unit.
	 *
	 * FIXME: This code is enough for all the relevant sentence examples in
	 * en/4.0.*batch at the time of writing. It is not general at all.
	 */
	if ((n_r_stripped && (find_word_in_dict(dict, input_word))) ||
	    units_alternative)
	{
		/* The input word is in the dict and also got stripped off
		 * something - "word" here is what remained. */
		word_is_in_dict = find_word_in_dict(dict, word);
		lgdebug(SWLEV, "Check stripped word='%s' find_word_in_dict=%d "
		        "is_utf8_digit=%d\n", word, word_is_in_dict, start_digit);
		if (word_is_in_dict || units_alternative)
		{
			/* Both input and stripped words are in the dict, issue the
			 * stripped word + its stripped tokens as an alternative. */
			wp = word;
			r_stripped_alt = NULL;
			ntokens = 1;
			altappend(sent, &r_stripped_alt, wp);
			lgdebug(SWLEV, "Issue stripped word w='%s' (alt I)\n", wp);
			for (i = n_r_stripped - 1; i >= 0; i--)
			{
				lgdebug(SWLEV, "Issue r_stripped w='%s' (alt I)\n", r_stripped[i]);
				altappend(sent, &r_stripped_alt, r_stripped[i]);
				ntokens++;
			}
			add_alternative(sent, 0,NULL, ntokens,r_stripped_alt, 0,NULL);
			free(r_stripped_alt);
		}

		/* Restore the input word, in case it needs a further handling. */
		strcpy(word, input_word);    /* Recover the input word. */
		n_r_stripped = 0;            /* Forget the stripping. */
	}

	/* If n_r_stripped exceed max, the "word" is most likely a long
	 * sequence of periods.  Just accept it as an unknown "word",
	 * and move on.
	 * FIXME: Word separation may still be needed, e.g. for a table of
	 * contents:
	 * 10............................
	 */
	if (n_r_stripped >= MAX_STRIP)
	{
		n_r_stripped = 0;
		word_is_in_dict = true;
		units_alternative = false; /* just in case */
	}

	/* If the Input word contains units and also got stripped off
	 * punctuation, restore it here so it will be used as an alternative
	 * without the punctuation. This happens if it is a part number, like
	 * 1234-567A. */
	if (units_alternative)
	{
		sz = units_wend-w;
		strncpy(word, w, sz);
		word[sz] = '\0';

		wp = word;
		r_stripped_alt = NULL;
		ntokens = 1;
		altappend(sent, &r_stripped_alt, wp);
		lgdebug(SWLEV, "Issue stripped word w='%s' (alt II)\n", wp);
		for (i = units_n_r_stripped - 1; i >= 0; i--)
		{
			lgdebug(SWLEV, "Issue r_stripped w='%s' (alt II)\n", r_stripped[i]);
			altappend(sent, &r_stripped_alt, r_stripped[i]);
			ntokens++;
		}
		add_alternative(sent, 0,NULL, ntokens,r_stripped_alt, 0,NULL);
		free(r_stripped_alt);

		/* FIXME: We suppose here that the input word cannot morpheme-split.
		 * Anyway, the code infrastructure doesn't allow us to continue after
		 * we issue alternatives for right-stripping. */
		issue_alternatives(sent, input_word, quote_found);
		return;
	}

	lgdebug(SWLEV, "Continue with the input word '%s'\n", word);

	/* From this point we need to handle regex matches separately.
	 * Find if the word is a real dict word.
	 * Regex matches will be tried later.
	 *
	 * Note: In any case we need to make here a new lookup, as this may not be
	 * the candidate word from the start of this function, due to possible
	 * punctuation strip. */
	word_is_in_dict = boolean_dictionary_lookup(dict, word);
	lgdebug(SWLEV, "Recheck word='%s' boolean_dictionary_lookup=%d\n",
	        word, word_is_in_dict);

	wp = word;
	if (word_is_in_dict)
	{
		lgdebug(SWLEV, "Adding '%s' as is, before split tries\n", wp);
		add_alternative(sent, 0,NULL, 1,&wp, 0,NULL);
	}

	/* OK, now try to strip affixes. */

	word_can_split = suffix_split(sent, w, wend);
	lgdebug(SWLEV, "Tried to split word='%s', word_can_split=%d\n",
			  word, word_can_split);

	if ((is_capitalizable(sent, sent->length) || quote_found) &&
		 is_utf8_upper(word))
	{
		downcase_utf8_str(downcase, word, downcase_size);
		word_can_split |= suffix_split(sent, downcase, downcase+(wend-w));
		lgdebug(SWLEV, "Tried to split lc='%s', now word_can_split=%d\n",
		        downcase, word_can_split);
	}

	/* FIXME: Unify with suffix_split(). */
	word_can_split |= mprefix_split(sent, word);

#ifdef USE_ANYSPLIT
	word_can_split |= anysplit(sent, word);
#endif

	lgdebug(SWLEV, "After split step, word='%s' now word_can_split=%d\n",
	        word, word_can_split);

	issued = false;

	/* If the word is capitalized, add as alternatives:
	 * - Add it only in case a regex match of it is needed, to prevent adding an
	 *   unknown word. If it can split, it was already added if needed.
	 *   (FIXME: make a better comment.)
	 * - Add its lowercase if it is in the dict.
	 * FIXME: Capitalization handling should be done using the dict.
	 */
	if (is_utf8_upper(word))
	{
		if (!word_can_split && match_regex(sent->dict->regex_root, wp))
		{
			lgdebug(SWLEV, "Adding uc word=%s\n", wp);
			add_alternative(sent, 0,NULL, 1,&wp, 0,NULL);
		}
		if ((is_capitalizable(sent, sent->length) || quote_found))
		{
			downcase_utf8_str(downcase, word, downcase_size);
			if (boolean_dictionary_lookup(dict, downcase))
			{
				wp = downcase;
				lgdebug(SWLEV, "Adding lc=%s, boolean_dictionary_lookup=1, is_capq=1\n", wp);
				add_alternative(sent, 0,NULL, 1,&wp, 0,NULL);

				word_is_in_dict = true;
			}
		}
	}

	parallel_regex = !word_is_in_dict && word_can_split &&
	    test_enabled("parallel-regex");

	word_is_in_dict |= word_can_split;

	/* Handle regex match. This is done for words which are not in the dict
	 * and cannot split,
	 * The "parallel-regex" test tries regex match for words that are not in the
	 * dict but can split. */
	if (!word_is_in_dict || parallel_regex)
	{
		wp = word;
		if (parallel_regex)
		{
			/* XXX We use the downcased version of the word, for not possibly
			 * matching the regexes for capitalized words as first match. */
			if  ('\0' != downcase[0]) wp = downcase;
			lgdebug(SWLEV, "Before match_regex: word=%s to_lc=%s word_is_in_dict=%d\n",
					  word, word[0] != downcase[0] ? downcase : "", word_is_in_dict);

			strcpy(str, wp); /* str is big enough to contain downcase */
			wp = str;
		}

		if (NULL != match_regex(sent->dict->regex_root, wp))
		{
			if (parallel_regex)
			{
				/* Append a [!] tag to the word to signify that this alternative
				 * is only for regex. This tag will be redone after invoking
				 * match_regex() again.
				 * XXX sent.word.alternatives should have been a struct with
				 * a field to mark such regex alternatives. */
				strcat(str, "[!]"); /* str has extra space for that */
			}
			lgdebug(SWLEV, "Adding '%s' as word to regex (match=%s)\n",
			        wp, match_regex(sent->dict->regex_root, wp));
			add_alternative(sent, 0,NULL, 1,&wp, 0,NULL);

		   word_is_in_dict = true; /* make sure we skip spell guess */
		}

	}

#if defined HAVE_HUNSPELL || defined HAVE_ASPELL
	/* If the word is not found in the dict, then it might be
	 * a run-on of two words or a misspelled word. Ask the spell-checker
	 * to split the word, if possible, and/or offer guesses.
	 *
	 * Do all of this only if the word is not a proper name, and if
	 * spell-checking is enabled. A word which contains digits is considered
	 * a proper name (maybe of a part number).
	 *
	 * Also skip the spell checks if we already issued any alternative for the
	 * word, as currently we don't do spell checks in parallel (and the
	 * infrastructure don't allow that anyway).
	 * Usually word_is_in_dict is true in such cases, but in a case of a
	 * contraction split word_is_in_dict is false, for a potential regex match.
	 *
	 * Spell-guessing is disabled if no spell-checker is specified.
	 *
	 * ??? Should we add spell guesses as alternatives in case:
	 * 1. The word if not in the main dict but matches a regex.
	 * 2. The word is a proper name.
	 */
	if (!word_is_in_dict && !contains_digits(word) && !is_proper_name(word) &&
	    !word_has_alternative(sent) &&
	    opts->use_spell_guess && dict->spell_checker)
	{
		issued = guess_misspelled_word(sent, word, quote_found, opts);
		lgdebug(SWLEV, "Spell suggest=%d\n", issued);
	}
#endif /* defined HAVE_HUNSPELL || defined HAVE_ASPELL */

	if (false == issued)
	{
		/* Terminate issuing the alternatives and record the word that caused
		 * their generation. It can be the original input word or the word after
		 * punctuation strip. */
		issued = issue_alternatives(sent, word, quote_found);
	}

	/* If no alternatives, issue the word. */
	if (false == issued)
		issue_sentence_word(sent, word, quote_found);

	for (i = n_r_stripped - 1; i >= 0; i--)
	{
		lgdebug(SWLEV, "Issue r_stripped w='%s'\n", r_stripped[i]);
		issue_sentence_word(sent, r_stripped[i], false);
	}
}

/**
 * The string s has just been read in from standard input.
 * This function breaks it up into words and stores these words in
 * the sent->word[] array.  Returns true if all is well, false otherwise.
 * Quote marks are treated just like blanks.
 */
bool separate_sentence(Sentence sent, Parse_Options opts)
{
	const char * word_end;
	bool quote_found;
	Dictionary dict = sent->dict;
	mbstate_t mbs;
	const char * word_start = sent->orig_sentence;

	sent->length = 0;

	if (dict->left_wall_defined)
		issue_sentence_word(sent, LEFT_WALL_WORD, false);

	/* Reset the multibyte shift state to the initial state */
	memset(&mbs, 0, sizeof(mbs));

	for(;;)
	{
		bool isq;
		wchar_t c;
		int nb = mbrtowc(&c, word_start, MB_CUR_MAX, &mbs);
		quote_found = false;

		if (0 > nb) goto failure;

		/* Skip all whitespace. Also, ignore *all* quotation marks.
		 * XXX This is sort-of a hack, but that is because LG does
		 * not have any intelligent support for quoted character
		 * strings at this time.
		 */
		isq = is_quote (dict, c);
		if (isq) quote_found = true;
		while (is_space(c) || isq)
		{
			word_start += nb;
			nb = mbrtowc(&c, word_start, MB_CUR_MAX, &mbs);
			if (0 == nb) break;
			if (0 > nb) goto failure;
			isq = is_quote (dict, c);
			if (isq) quote_found = true;
		}

		if ('\0' == *word_start) break;

		/* Loop over non-blank characters until word-end is found. */
		word_end = word_start;
		nb = mbrtowc(&c, word_end, MB_CUR_MAX, &mbs);
		if (0 > nb) goto failure;
		while (!is_space(c) && !is_quote(dict, c) && (c != 0) && (0 < nb))
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
		issue_sentence_word(sent, RIGHT_WALL_WORD, false);

	return (sent->length > (unsigned)
	        (!!dict->left_wall_defined + !!dict->right_wall_defined));

failure:
	prt_error("Unable to process UTF8 input string in current locale %s\n",
		nl_langinfo(CODESET));
	return false;
}

/**
 * Replace the word at each X_node by the given word + mark + word_type;
 * Keep the original subscript.
 */
static void mark_replace_x_node_words(Sentence sent,  X_node * head,
                                      char const * word, const char mark,
                                      const char * word_type )
{
	X_node * e;
	size_t mi_len = strlen(word_type) + 4; /* "[]" mark + + NUL */
	size_t max_len = 0;
	char str[MAX_WORD];
	char * buff = str;
	size_t buff_len = sizeof(str);

	if (NULL != word_type) mi_len += strlen(word_type);

	for (e = head; e != NULL; e = e->next)
	{
		const char * sm = strrchr(e->string, SUBSCRIPT_MARK);

		if (NULL == sm) sm = "";
		max_len = MAX(max_len, strlen(sm));
		/* Handle pathological cases of very long words. */
		if (max_len+mi_len > buff_len)
		{
				buff = alloca(mi_len+max_len+MAX_WORD);
				buff_len = mi_len+max_len+MAX_WORD;
		}
		snprintf(buff, buff_len, "%s[%c%s]%s", word, mark, word_type, sm);
		e->string = string_set_add(buff, sent->string_set);
	}
}

/**
 * Build the word expressions, and add a tag to the word to indicate
 * that it was guessed by means of regular-expression matching.
 * Also, add a subscript to the resulting word to indicate the
 * rule origin. Optionally add the word type (regex name) too.
 */
static X_node * build_regex_expressions(Sentence sent, int i,
                const char * word_type, const char * word, Parse_Options opts)
{
	X_node * we;

	we = build_word_expressions(sent->dict, word_type);
	if (!opts->display_morphology) word_type = "";
	mark_replace_x_node_words(sent, we, word, '!', word_type);

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
	X_node * we = build_word_expressions(sent->dict, UNKNOWN_WORD);

	assert(we, "UNKNOWN_WORD must be defined in the dictionary!");
	mark_replace_x_node_words(sent, we, s, '?', "");

	return we;
}

/**
 * Add a mark to base words (before the SUBSCRIPT_MARK, if any).
 * This addition is carried as part of the word string to the sentence
 * parse results.
 */
static void mark_x_node_words(Sentence sent, X_node * head, char const * mark)
{
	X_node * d;
	size_t max_len = 0;
	char * str;
	size_t strsz;

	for (d = head; d != NULL; d = d->next)
		max_len = MAX(max_len, strlen(d->string));

	strsz = max_len + strlen(mark) + 1;
	str = alloca(strsz);   /* 1 for NUL */

	for (d = head; d != NULL; d = d->next)
	{
		char const * sm = strrchr(d->string, SUBSCRIPT_MARK);

		if (NULL == sm) sm = d->string + strlen(d->string);
		snprintf(str, strsz, "%.*s%s%s", (int)(sm-d->string), d->string, mark, sm);
		d->string = string_set_add(str, sent->string_set);
	}
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
 * A special check (for "[!") has been added to identify an alternative to a
 * dictionary word that is to be handled by a regex match.
 *
 * If both w and w' are in the dict, concatenate these disjuncts.
 * Else if just w' is in dict, use disjuncts of w', together with
 * the CAPITALIZED-WORDS rule.
 * Else leave the disjuncts alone.
 */
void build_sentence_expressions(Sentence sent, Parse_Options opts)
{
	size_t i;
	Dictionary dict = sent->dict;

	/* The following loop treats all words the same
	 * (nothing special for 1st word) */
	for (i=0; i<sent->length; i++)
	{
		size_t ialt;
		/* Optimization - load the empty-word disjuncts only once per word */
		bool empty_word_encountered = false;

		for (ialt=0; NULL != sent->word[i].alternatives[ialt]; ialt++)
		{
			const char * s = sent->word[i].alternatives[ialt];
			const char * origword = s;
			X_node * e;
			X_node * we = NULL;
			const char * regex_name;
			const char * spell_mark;
			char word[MAX_WORD+1];

			const char * regex_mark;
			const char * regex_it = s;

			if (test_enabled("avoid-extra-empty-words") &&
			    (0 == strcmp(s, EMPTY_WORD_MARK)))
			{
				if (empty_word_encountered)
				{
					if (test_enabled("avoid-extra-empty-words-print"))
					{
						static int c = 0;
						fprintf(stderr,
						 "Sentence '%s': %d: Word %zu: EMPTY_WORD_MARK skipped\n",
						 sent->orig_sentence, c++, i);
					}
					continue;
				}
				empty_word_encountered = true;
			}

			/* The word can be a spell-suggested one. */
			spell_mark = strstr(s, "[~");
			if (NULL != spell_mark)
			{
				strncpy(word, s, spell_mark-s); /* XXX we know it fits */
				word[spell_mark-s] = '\0';
				origword = word;
			}

			/* For test_enabled("parallel-regex"). To be removed/modified later. */
			regex_mark  = strstr(s, "[!");
			if (NULL != regex_mark)
			{
				strncpy(word, s, regex_mark-s); /* XXX we know it fits */
				word[regex_mark-s] = '\0';
				regex_it = word;
			}

			if ((NULL == regex_mark) &&
			    boolean_dictionary_lookup(sent->dict, origword))
			{
				we = build_word_expressions(sent->dict, origword);
				if (spell_mark)
				{
					mark_x_node_words(sent, we, spell_mark);
				}
			}
			else if ((NULL != (regex_name = match_regex(sent->dict->regex_root, regex_it))) &&
			         boolean_dictionary_lookup(sent->dict, regex_name))
			{
				we = build_regex_expressions(sent, i, regex_name, regex_it, opts);
			}
			else if (dict->unknown_word_defined && dict->use_unknown_word)
			{
				we = handle_unknown_word(sent, i, s);
			}
			else
			{
				/* The reason I can assert this is that the word
				 * should have been looked up already if we get here.
				 */
				assert(false, "I should have found that word.");
			}

			/* Under certain cases--if it's the first word of the sentence,
			 * or if it follows a colon or a quotation mark--a word that's
			 * capitalized has to be looked up as an uncapitalized word
			 * (possibly also as well as a capitalized word).
			 *
			 * XXX For the first-word case, we should be handling capitalization
			 * as an alternative, when doing separate_word(), and not here.
			 * separate_word() should build capitalized and non-capitalized
			 * alternatives.  This is especially true for Russian, where we
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
			if (3 < verbosity)
			{
				X_node *x;
				printf("Tokenize word#=%zu '%s' alt#=%zu '%s' string='%s' ",
				       i, sent->word[i].unsplit_word, ialt, s, we->string);
				x = sent->word[i].x;
				while (x) {
					printf("xstring='%s' expr=", x->string);
					print_expression(x->exp);
					x = x->next;
				}
			}
		}
	}
}


/**
 * This just looks up all the words in the sentence, and builds
 * up an appropriate error message in case some are not there.
 * It has no side effect on the sentence.  Returns true if all
 * went well.
 *
 * This code is called only is the 'unknown-words' flag is set.
 */
bool sentence_in_dictionary(Sentence sent)
{
	bool ok_so_far;
	size_t w;
	const char * s;
	Dictionary dict = sent->dict;
	char temp[1024];

	ok_so_far = true;
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
					ok_so_far = false;
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
