/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2009 Linas Vepstas                                      */
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

#include <link-grammar/api.h>
#include "error.h"
#include "regex-morph.h"
#include "spellcheck-hun.h"
#include "utilities.h"

#define MAX_STRIP 10

/* These are no longer in use, but are read from the 4.0.affix file */
/* I've left these here, as an axample of what to expect. */
/*static char * strip_left[] = {"(", "$", "``", NULL}; */
/*static char * strip_right[] = {")", "%", ",", ".", ":", ";", "?", "!", "''", "'", "'s", NULL};*/

#define COMMON_ENTITY_MARKER   "<marker-common-entity>"

/**
 * is_common_entity - Return true if word is a common noun or adjective
 * Common nouns and adjectives are typically used in corporate entity
 * names -- e.g. "Sun State Bank" -- "sun", "state" and "bank" are all 
 * common nouns.
 */
static int is_common_entity(Dictionary dict, const char * str)
{
	if (word_contains(dict, str, COMMON_ENTITY_MARKER) == 1)
		return 1;
	return 0;
}

/**
 * Is the word entirely composed of single-letter abreviations
 * (followed by a period)?
 * It returns TRUE if the word matches the pattern /[A-Z]\.]+/
 */
static int is_initials_word(const char * word)
{
	int i=0;
	while (word[i])
	{
		int nb = is_utf8_upper(&word[i]);
		if (!nb) return FALSE;
		i += nb;
		if (word[i] != '.') return FALSE;
		i++;
	}
	return TRUE;
}

/**
 * Return TRUE if word is a proper name.
 * XXX This is a cheap hack that works only in English, and is 
 * broken for German!  We need to replace this with something
 * language-specific. 
 *
 * Basically, if word starts with upper-case latter, we assume
 * its a proper name, and that's that.
 */
static int is_proper_name(const char * word)
{
	return is_utf8_upper(word);
}

/* Create a string containing anything that can be construed to
 * be a quotation mark. This works, because link-grammar is more
 * or less ignorant of quotes at this time.
 */
static const wchar_t *list_of_quotes(void) 
{
#define QUSZ 50
	static wchar_t wqs[QUSZ];
	mbstate_t mbs;
	/* Single-quotes are used for abbreviations, don't mess with them */
	/* const char * qs = "\"\'«»《》【】『』‘’`„“"; */
	const char * qs = "\"«»《》【】『』`„“";

	const char *pqs = qs;

	memset(&mbs, 0, sizeof(mbs));

	mbsrtowcs(wqs, &pqs, QUSZ, &mbs);

	return wqs;
}

/**
 * Return TRUE if the character is a quotation character.
 */
static int is_quote(wchar_t wc)
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
static int is_number(const char * s)
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
static int contains_digits(const char * s)
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

#if DONT_USE_REGEX_GUESSING
/**
 * Returns TRUE iff it's an appropriately formed hyphenated word.
 * This means all letters, numbers, or hyphens, not beginning and
 * ending with a hyphen.
 */
static int ishyphenated(const char * s)
{
	mbstate_t mbs;
	wchar_t c;
	int hyp, nonalpha;
	hyp = nonalpha = 0;

	if (*s == '-') return FALSE;

	memset(&mbs, 0, sizeof(mbs));
	while (*s != '\0')
	{
		int nb = mbrtowc(&c, s, MB_CUR_MAX, &mbs);
		if (0 > nb) break;

		if (!iswalpha(c) && !iswdigit(c) && (*s!='.') && (*s!=',')
			&& (*s!='-')) return FALSE;
		if (*s == '-') hyp++;
		s += nb;
	}
	return ((*(s-1)!='-') && (hyp>0));
}

/** 
 * The following four routines implement a cheap-hack morphology 
 * guessing of unkown words for English.
 * XXX Now obsolete, replaced by regex-based guessing.
 * Remove dead code at liesure.
 */
static int is_ing_word(const char * s)
{
	int i=0;
	for (; *s != '\0'; s++) i++;
	if (i<5) return FALSE;
	if (strncmp("ing", s-3, 3)==0) return TRUE;
	return FALSE;
}

static int is_s_word(const char * s)
{
	for (; *s != '\0'; s++);
	s--;
	if(*s != 's') return FALSE;
	s--;
	if(*s == 'i' || *s == 'u' || *s == 'o' || *s == 'y' || *s == 's') return FALSE;
	return TRUE;
}

static int is_ed_word(const char * s)
{
	int i=0;
	for (; *s != '\0'; s++) i++;
	if (i<4) return FALSE;
	if (strncmp("ed", s-2, 2)==0) return TRUE;
	return FALSE;
}

static int is_ly_word(const char * s)
{
	int i=0;
	for (; *s != '\0'; s++) i++;
	if (i<4) return FALSE;
	if (strncmp("ly", s-2, 2)==0) return TRUE;
	return FALSE;
}
#endif /* DONT_USE_REGEX_GUESSING */

/** 
 * The string s is the next word of the sentence. 
 * Do not issue the empty string.  
 * Return false if too many words or the word is too long. 
 */
static int issue_sentence_word(Sentence sent, const char * s)
{
	if (*s == '\0') return TRUE;
	if (strlen(s) > MAX_WORD)
	{
		err_ctxt ec;
		ec.sent = sent;
		err_msg(&ec, Error, 
		   "Error separating sentence. The word \"%s\" is too long.\n"
		   "A word can have a maximum of %d characters.\n", s, MAX_WORD);
		return FALSE;
	}

	if (sent->length == MAX_SENTENCE)
	{
		err_ctxt ec;
		ec.sent = sent;
		err_msg(&ec, Error, 
		   "Error separating sentence. The sentence has too many words.\n");
		return FALSE;
	}

	strcpy(sent->word[sent->length].string, s);

	/* Now we record whether the first character of the word is upper-case.
	   (The first character may be made lower-case
	   later, but we may want to get at the original version) */
	if (is_utf8_upper(s)) sent->word[sent->length].firstupper=1;
	else sent->word[sent->length].firstupper = 0;
	sent->length++;
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

static int downcase_is_in_dict(Dictionary dict, char * word)
{
	int i, rc;
	char low[MB_LEN_MAX];
	char save[MB_LEN_MAX];
	wchar_t c;
	int nbl, nbh;
	mbstate_t mbs, mbss;

	if (!is_utf8_upper(word)) return FALSE;

	memset(&mbs, 0, sizeof(mbs));
	memset(&mbss, 0, sizeof(mbss));

	nbh = mbrtowc (&c, word, MB_CUR_MAX, &mbs);
	c = towlower(c);
	nbl = wctomb_check(low, c, &mbss);
	if (nbh != nbl)
	{
		prt_error("Warning: can't downcase multi-byte string: %s\n", word);
		return FALSE;
	}

	/* Downcase */
	for (i=0; i<nbl; i++) { save[i] = word[i]; word[i] = low[i]; }

	/* Look it up, then restore old value */
	rc = boolean_dictionary_lookup(dict, word);
	for (i=0; i<nbh; i++) { word[i] = save[i]; }

	return rc; 
}

/**
 * w points to a string, wend points to the char one after the end.  The
 * "word" w contains no blanks.  This function splits up the word if
 * necessary, and calls "issue_sentence_word()" on each of the resulting
 * parts.  The process is described above.  Returns TRUE if OK, FALSE if
 * too many punctuation marks 
 */
static int separate_word(Sentence sent, Parse_Options opts,
                         const char *w, const char *wend,
                         int is_first_word, int quote_found)
{
	int i, j, len;
	int r_strippable=0, l_strippable=0, u_strippable=0;
	int s_strippable=0, p_strippable=0;
	int  n_r_stripped, s_stripped;
	int word_is_in_dict, s_ok;
	int issued = FALSE;

	int found_number = 0;
	int n_r_stripped_save;
	const char * wend_save;

	const char ** strip_left = NULL;
	const char ** strip_right = NULL;
	const char ** strip_units = NULL;
	const char ** prefix = NULL;
	const char ** suffix = NULL;
	char word[MAX_WORD+1];
	char newword[MAX_WORD+1];

	const char *r_stripped[MAX_STRIP];  /* these were stripped from the right */

	/* First, see if we can already recognize the word as-is. If
	 * so, then we are done. Else we'll try stripping prefixes, suffixes.
	 */
	strncpy(word, w, MIN(wend-w, MAX_WORD));
	word[MIN(wend-w, MAX_WORD)] = '\0';
	word_is_in_dict = FALSE;

	if (boolean_dictionary_lookup(sent->dict, word))
		word_is_in_dict = TRUE;
	else if (is_initials_word(word))
		word_is_in_dict = TRUE;
	else if (is_first_word && downcase_is_in_dict (sent->dict,word))
		word_is_in_dict = TRUE;

	if (word_is_in_dict)
	{
		issue_sentence_word(sent, word);
		return TRUE;
	}

	/* Set up affix tables.  */
	if (sent->dict->affix_table != NULL)
	{
		Dictionary dict = sent->dict->affix_table;
		r_strippable = dict->r_strippable;
		l_strippable = dict->l_strippable;
		u_strippable = dict->u_strippable;
		p_strippable = dict->p_strippable;
		s_strippable = dict->s_strippable;

		strip_left = dict->strip_left;
		strip_right = dict->strip_right;
		strip_units = dict->strip_units;
		prefix = dict->prefix;
		suffix = dict->suffix;
	}

	/* Strip off punctuation, etc. on the left-hand side. */
	for (;;)
	{
		for (i=0; i<l_strippable; i++)
		{
			/* This is UTF8-safe, I beleive ... */
			if (strncmp(w, strip_left[i], strlen(strip_left[i])) == 0)
			{
				if (!issue_sentence_word(sent, strip_left[i])) return FALSE;
				w += strlen(strip_left[i]);
				break;
			}
		}
		if (i == l_strippable) break;
	}

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
		size_t sz = MIN(wend-w, MAX_WORD);
		strncpy(word, w, sz);
		word[sz] = '\0';
		if (wend == w) break;  /* it will work without this */

		if (boolean_dictionary_lookup(sent->dict, word) ||
		    is_initials_word(word)) break;

		/* This could happen if it's a word after a colon, also! */
		if (is_first_word && downcase_is_in_dict (sent->dict, word)) break;

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
	if (contains_digits(word))
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

	/* Now we strip off suffixes...w points to the remaining word, 
	 * "wend" to the end of the word. */

	s_stripped = -1;
	strncpy(word, w, MIN(wend-w, MAX_WORD));
	word[MIN(wend-w, MAX_WORD)] = '\0';
	word_is_in_dict = FALSE;

	if (boolean_dictionary_lookup(sent->dict, word))
		word_is_in_dict = TRUE;
	else if (is_initials_word(word))
		word_is_in_dict = TRUE;
	else if (is_first_word && downcase_is_in_dict (sent->dict,word))
		word_is_in_dict = TRUE;

	if (FALSE == word_is_in_dict)
	{
		j=0;
		for (i=0; i <= s_strippable; i++)
		{
			s_ok = 0;
			/* Go through once for each suffix; then go through one 
			 * final time for the no-suffix case */
			if (i < s_strippable)
			{
				len = strlen(suffix[i]);

				/* The remaining w is too short for a possible match */
				if ((wend-w) < len) continue;
				if (strncmp(wend-len, suffix[i], len) == 0) s_ok=1;
			}
			else
				len = 0;

			if (s_ok || i == s_strippable)
			{
				strncpy(newword, w, MIN((wend-len)-w, MAX_WORD));
				newword[MIN((wend-len)-w, MAX_WORD)] = '\0';

				/* Check if the remainder is in the dictionary;
				 * for the no-suffix case, it won't be */
				if (boolean_dictionary_lookup(sent->dict, newword))
				{
					if ((verbosity>1) && (i < s_strippable))
						printf("Splitting word into two: %s-%s\n", newword, suffix[i]);
					s_stripped = i;
					wend -= len;
					strncpy(word, w, MIN(wend-w, MAX_WORD));
					word[MIN(wend-w, MAX_WORD)] = '\0';
					word_is_in_dict = TRUE;
					break;
				}

				/* If the remainder isn't in the dictionary, 
				 * try stripping off prefixes */
				else
				{
					for (j=0; j<p_strippable; j++)
					{
						if (strncmp(w, prefix[j], strlen(prefix[j])) == 0)
						{
							int sz = MIN((wend-len)-(w+strlen(prefix[j])), MAX_WORD);
							strncpy(newword, w+strlen(prefix[j]), sz);
							newword[sz] = '\0';
							if (boolean_dictionary_lookup(sent->dict, newword))
							{
								if ((verbosity>1) && (i < s_strippable))
									printf("Splitting word into three: %s-%s-%s\n",
										prefix[j], newword, suffix[i]);
								if (!issue_sentence_word(sent, prefix[j])) return FALSE;
								if (i < s_strippable) s_stripped = i;
								wend -= len;
								w += strlen(prefix[j]);
								sz = MIN(wend-w, MAX_WORD);
								strncpy(word, w, sz);
								word[sz] = '\0';
								word_is_in_dict = TRUE;
								break;
							}
						}
					}
				}
				if (j != p_strippable) break;
			}
		}
	}

	/* word is now what remains after all the stripping has been done */

	if (n_r_stripped >= MAX_STRIP)
	{
		prt_error("Error separating sentence.\n"
		          "\"%s\" is followed by too many punctuation marks.\n", word);
		return FALSE;
	}

	if (quote_found == TRUE) sent->post_quote[sent->length] = 1;

	/* If the word is still not being found, then it might be 
	 * a run-on of two words. Ask the spell-checker to split
	 * the word in two, if possible. Do this only if the word
	 * is not a proper name, and if spell-checking is enabled.
	 */
	issued = FALSE;
	if ((FALSE == word_is_in_dict) && 
	    TRUE == opts->use_spell_guess &&
	    sent->dict->spell_checker &&
	    (FALSE == is_proper_name(word)))
	{
		char **alternates = NULL;
		char *sp = NULL;
		char *wp;
		int j, n;
		n = spellcheck_suggest(sent->dict->spell_checker, &alternates, word);
		for (j=0; j<n; j++)
		{
			/* Uhh, XXX this is not utf8 safe! */
			sp = strchr(alternates[j], ' ');
			if (sp) break;
		}

		if (sp) issued = TRUE;

		wp = alternates[j];
		while (sp)
		{
			*sp = 0x0;
			issue_sentence_word(sent, wp);
			wp = sp+1;
			sp = strchr(wp, ' ');
			if (NULL == sp)
			{
				issue_sentence_word(sent, wp);
			}
		}
		if (alternates) free(alternates);
	}

	if (FALSE == issued)
	{
		if (!issue_sentence_word(sent, word)) return FALSE;
	}

	if (s_stripped != -1)
	{
	  if (!issue_sentence_word(sent, suffix[s_stripped])) return FALSE;
	}

	for (i = n_r_stripped-1; i>=0; i--)
	{
		if (!issue_sentence_word(sent, r_stripped[i])) return FALSE;
	}

	return TRUE;
}

/**
 * The string s has just been read in from standard input.
 * This function breaks it up into words and stores these words in
 * the sent->word[] array.  Returns TRUE if all is well, FALSE otherwise.
 * Quote marks are treated just like blanks.
 */
int separate_sentence(Sentence sent, Parse_Options opts)
{
	const char *t;
	int i, is_first, quote_found;
	Dictionary dict = sent->dict;
	mbstate_t mbs;
	const char * s = sent->orig_sentence;

	for(i=0; i<MAX_SENTENCE; i++) sent->post_quote[i] = 0;
	sent->length = 0;

	if (dict->left_wall_defined)
		if (!issue_sentence_word(sent, LEFT_WALL_WORD)) return FALSE;

	/* Reset the multibyte shift state to the initial state */
	memset(&mbs, 0, sizeof(mbs));

	is_first = TRUE;
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

		if (!separate_word(sent, opts, s, t, is_first, quote_found)) return FALSE;
		is_first = FALSE;
		s = t;
		if (*s == '\0') break;
	}

	if (dict->right_wall_defined)
		if (!issue_sentence_word(sent, RIGHT_WALL_WORD)) return FALSE;

	return (sent->length > dict->left_wall_defined + dict->right_wall_defined);

failure:
	prt_error("Unable to process UTF8 input string in current locale %s\n",
		nl_langinfo(CODESET));
	return FALSE;
}

#if DONT_USE_REGEX_GUESSING
static int special_string(Sentence sent, int i, const char * s)
{
	X_node * e;
	if (boolean_dictionary_lookup(sent->dict, s))
	{
		sent->word[i].x = build_word_expressions(sent, s);
		for (e = sent->word[i].x; e != NULL; e = e->next)
		{
			e->string = sent->word[i].string;
		}
		return TRUE;
	}
	else
	{
		err_ctxt ec;
		ec.sent = sent;
		err_msg(&ec, Error, "Error: Could not build sentence expressions.\n"
		          "To process this sentence your dictionary "
 		          "needs the word \"%s\".\n", s);
		return FALSE;
	}
}
#endif /* DONT_USE_REGEX_GUESSING */

/**
 * Build the word expressions, and add a tag to the word to indicate
 * that it was guess by means of regular-expression matching.
 * Also, add a subscript to the resulting word to indicate the
 * rule origin.
 */
static void tag_regex_string(Sentence sent, int i, const char * type)
{
	char str[MAX_WORD+1];
	char * t;
	X_node * e;
	sent->word[i].x = build_word_expressions(sent, type);
	for (e = sent->word[i].x; e != NULL; e = e->next)
	{
		t = strchr(e->string, '.');
		e->string = sent->word[i].string;
		if (NULL != t)
		{
			snprintf(str, MAX_WORD, "%.50s[!].%.5s", e->string, t+1);
		}
		else
		{
			snprintf(str, MAX_WORD, "%.50s", e->string);
		}
		e->string = string_set_add(str, sent->string_set);
	}
}

#if DONT_USE_REGEX_GUESSING
/**
 * Performes morphology-based word guessing.
 * Obsolete with the new regex infratructure ... remove this code at
 * liesure.
 */
static int guessed_string(Sentence sent, int i, const char * s, const char * type)
{
	X_node * e;
	char *t;
	char str[MAX_WORD+1];
	if (boolean_dictionary_lookup(sent->dict, type))
	{
		sent->word[i].x = build_word_expressions(sent, type);
		e = sent->word[i].x;
		if(is_s_word(s))
		{
			for (; e != NULL; e = e->next)
			{
				t = strchr(e->string, '.');
				if (t != NULL)
				{
					sprintf(str, "%.50s[!].%.5s", s, t+1);
			  	}
				else
				{
					sprintf(str, "%.50s[!]", s);
				}
				e->string = string_set_add(str, sent->string_set);
			}
		}
		else
		{
			if(is_ed_word(s))
			{
				sprintf(str, "%.50s[!].v", s);
			}
			else if(is_ing_word(s))
			{
				sprintf(str, "%.50s[!].g", s);
			}
			else if(is_ly_word(s))
			{
				sprintf(str, "%.50s[!].e", s);
			}
			else

			sprintf(str, "%.50s[!]", s);
			e->string = string_set_add(str, sent->string_set);
		}
		return TRUE;
	}
	else
	{
		err_ctxt ec;
		ec.sent = sent;
		err_msg(&ec, Error, 
		        "Error: Could not build sentence expressions.\n"
		        "To process this sentence your dictionary "
		        "needs the word \"%s\".\n", type);
		return FALSE;
	}
}
#endif /* DONT_USE_REGEX_GUESSING */

/**
 * Puts into word[i].x the expression for the unknown word 
 * the parameter s is the word that was not in the dictionary 
 * it massages the names to have the corresponding subscripts 
 * to those of the unknown words
 * so "grok" becomes "grok[?].v" 
 */
static void handle_unknown_word(Sentence sent, int i, char * s)
{
	char *t;
	X_node *d;
	char str[MAX_WORD+1];

	sent->word[i].x = build_word_expressions(sent, UNKNOWN_WORD);
	if (sent->word[i].x == NULL)
		assert(FALSE, "UNKNOWN_WORD should have been there");

	for (d = sent->word[i].x; d != NULL; d = d->next)
	{
		t = strchr(d->string, '.');
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
}

/**
 * If a word appears to be mis-spelled, then add alternate
 * spellings. Maybe one of those will do ... 
 */
static void guess_misspelled_word(Sentence sent, int i, char * s)
{
	int spelling_ok;
	char str[MAX_WORD+1];
	Dictionary dict = sent->dict;
	X_node *d, *head = NULL;
	int j, n;
	char **alternates = NULL;

	/* Spell-guessing is disabled if no spell-checker is speficified */
	if (NULL == dict->spell_checker)
	{
		handle_unknown_word(sent, i, s);
		return;
	}

	/* If the spell-checker knows about this word, and we don't ... 
	 * Dang. We should fix it  someday. Accept it as such. */
	spelling_ok = spellcheck_test(dict->spell_checker, s);
	if (spelling_ok)
	{
		handle_unknown_word(sent, i, s);
		return;
	}

	/* Else, ask the spell-checker for alternate spellings
	 * and see if these are in the dict. */
	n = spellcheck_suggest(dict->spell_checker, &alternates, s);
	for (j=0; j<n; j++)
	{
		if (boolean_dictionary_lookup(sent->dict, alternates[j]))
		{
			X_node *x = build_word_expressions(sent, alternates[j]);
			head = catenate_X_nodes(x, head);
		}
	}
	sent->word[i].x = head;
	if (alternates) free(alternates);

	/* Add a [~] to the output to signify that its the result of
	 * guessing. */
	for (d = sent->word[i].x; d != NULL; d = d->next)
	{
		const char * t = strchr(d->string, '.');
		if (t != NULL)
		{
			size_t off = t - d->string;
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
		handle_unknown_word(sent, i, s);
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
 * Now, we correct the first word, w.
 * If w is upper case, let w' be the lower case version of w.
 * If both w and w' are in the dict, concatenate these disjncts.
 * Else if just w' is in dict, use disjuncts of w', together with
 * the CAPITALIZED-WORDS rule.
 * Else leave the disjuncts alone.
 */
int build_sentence_expressions(Sentence sent, Parse_Options opts)
{
	int i, first_word;  /* the index of the first word after the wall */
	char *s, temp_word[MAX_WORD+1];
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
		s = sent->word[i].string;
		if (boolean_dictionary_lookup(sent->dict, s))
		{
			sent->word[i].x = build_word_expressions(sent, s);
		}
#if DONT_USE_REGEX_GUESSING
		else if (is_utf8_upper(s) && is_s_word(s) && dict->pl_capitalized_word_defined) 
		{
			if (!special_string(sent, i, PL_PROPER_WORD)) return FALSE;
		}
		else if (is_utf8_upper(s) && dict->capitalized_word_defined)
		{
			if (!special_string(sent, i, PROPER_WORD)) return FALSE;
		}
#endif /* DONT_USE_REGEX_GUESSING */
		else if ((NULL != (regex_name = match_regex(sent->dict, s))) &&
		         boolean_dictionary_lookup(sent->dict, regex_name))
		{
			tag_regex_string(sent, i, regex_name);
		}
#if DONT_USE_REGEX_GUESSING
		else if (is_number(s) && dict->number_word_defined)
		{
			/* we know it's a plural number, or 1 */
			/* if the string is 1, we'll only be here if 1's not in the dictionary */
			if (!special_string(sent, i, NUMBER_WORD)) return FALSE;
		}
		else if (ishyphenated(s) && dict->hyphenated_word_defined)
		{
			/* singular hyphenated */
			if (!special_string(sent, i, HYPHENATED_WORD)) return FALSE;
		} 
		/* XXX
		 * The following does some morphology-guessing for words that
		 * that are not in the dictionary. This has been replaced by a
		 * regex-based morphlogy guesser. Remove this obsolete code at
		 * liesure.
		 */
		else if (is_ing_word(s) && dict->ing_word_defined) 
		{
			if (!guessed_string(sent, i, s, ING_WORD)) return FALSE;
		}
		else if (is_s_word(s) && dict->s_word_defined)
		{
			if (!guessed_string(sent, i, s, S_WORD)) return FALSE;
		}
		else if (is_ed_word(s) && dict->ed_word_defined)
		{
			if (!guessed_string(sent, i, s, ED_WORD)) return FALSE;
		}
		else if (is_ly_word(s) && dict->ly_word_defined)
		{
			if (!guessed_string(sent, i, s, LY_WORD)) return FALSE;
		}
#endif /* DONT_USE_REGEX_GUESSING */

		else if (dict->unknown_word_defined && dict->use_unknown_word)
		{
			if (opts->use_spell_guess)
			{
				guess_misspelled_word(sent, i, s);
			}
			else
			{
				handle_unknown_word(sent, i, s);
			}
		}
		else 
		{
			/* The reason I can assert this is that the word
			 * should have been looked up already if we get here.
			 */
			assert(FALSE, "I should have found that word.");
		}
	}

	/* Under certain cases--if it's the first word of the sentence,
	 * or if it follows a colon or a quotation mark--a word that's 
	 * capitalized has to be looked up as an uncapitalized word
	 * (as well as a capitalized word).
	 * XXX This rule is English-language-oriented, and should be
	 * abstracted.
	 */
	for (i=0; i<sent->length; i++)
	{
		if (! (i == first_word ||
		      (i > 0 && strcmp(":", sent->word[i-1].string)==0) || 
		       sent->post_quote[i] == 1)) continue;
		s = sent->word[i].string;

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
		 * This is actually a great example of a combo of an algorithm
		 * together with a list of words used to determine grammatical
		 * function.
		 */
		if (is_utf8_upper(s))
		{
			const char * lc;
			downcase_utf8_str(temp_word, s, MAX_WORD);
			lc = string_set_add(temp_word, sent->string_set);

			if (boolean_dictionary_lookup(sent->dict, lc))
			{
				if (is_common_entity(sent->dict,lc))
				{
					e = build_word_expressions(sent, lc);
					sent->word[i].x =
						catenate_X_nodes(sent->word[i].x, e);
				}
				else
				{
					safe_strcpy(s, lc, MAX_WORD);
					e = build_word_expressions(sent, s);
					free_X_nodes(sent->word[i].x);
					sent->word[i].x = e;
				}
			}
		}
	}

	return TRUE;
}


/**
 * This just looks up all the words in the sentence, and builds
 * up an appropriate error message in case some are not there.
 * It has no side effect on the sentence.  Returns TRUE if all
 * went well.
 */
int sentence_in_dictionary(Sentence sent)
{
	int w, ok_so_far;
	char * s;
	Dictionary dict = sent->dict;
	char temp[1024];

	ok_so_far = TRUE;
	for (w=0; w<sent->length; w++)
	{
		s = sent->word[w].string;
		/* xx we should also check if the regex rules know this word */
		/* However, this code is only used if the 'unknown word' is 
		 * turned off ... we punt for now, since its always on. */
		if (!boolean_dictionary_lookup(dict, s))
#if DONT_USE_REGEX_GUESSING
		    !(is_utf8_upper(s)   && dict->capitalized_word_defined) &&
		    !(is_utf8_upper(s) && is_s_word(s) && dict->pl_capitalized_word_defined) &&
		    !(ishyphenated(s) && dict->hyphenated_word_defined)  &&
		    !(is_number(s)	&& dict->number_word_defined) &&
		    !(is_ing_word(s)  && dict->ing_word_defined)  &&
		    !(is_s_word(s)	&& dict->s_word_defined)  &&
		    !(is_ed_word(s)   && dict->ed_word_defined)  &&
		    !(is_ly_word(s)   && dict->ly_word_defined))
#endif /* DONT_USE_REGEX_GUESSING */
		{
			if (ok_so_far)
			{
				safe_strcpy(temp, "The following words are not in the dictionary:", sizeof(temp));
				ok_so_far = FALSE;
			}
			safe_strcat(temp, " \"", sizeof(temp));
			safe_strcat(temp, sent->word[w].string, sizeof(temp));
			safe_strcat(temp, "\"", sizeof(temp));
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
