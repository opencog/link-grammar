/*************************************************************************/
/* Copyright (c) 2014 Amir Plivatsky                                     */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#define D_ANYS 5               /* Debug level for this file (6 for more) */

/**
 * anysplit.c -- code that splits words into random morphemes.
 * This is used for the language-learning/morpheme-learning project.
 */

/* General assumptions:
 * - false is binary 0 (for memset())
 * - int is >= 32 bit (for random number)
 */

#include "utilities.h" /* included first, for MSVC rand_s() */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#if HAVE_PCRE2_H // use it to split in grapheme boundaries
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#endif // HAVE_PCRE2_H

#include "api-structures.h"
#include "dict-common/dict-affix.h"
#include "dict-common/dict-common.h"
#include "print/print-util.h" // For patch_subscript_mark()
#include "dict-common/regex-morph.h"
#include "error.h"
#include "externs.h"
#include "tokenize.h"
#include "tok-structures.h"

#include "anysplit.h"


#define MAX_WORD_TO_SPLIT 63 /* in codepoints (or graphemes if HAVE_PCRE2_H) */

extern const char * const afdict_classname[];

typedef int p_end;     /* partition end in a word (end char position + 1) */
typedef p_end *p_list; /* list of partitions in a word */

typedef struct split_cache /* split cached by word length */
{
	size_t nsplits;      /* number of splits */
	p_list sp;           /* list of splits */
	bool *p_tried;       /* list of tried splits */
	bool *p_selected;    /* list of selected splits */
} split_cache;

#if HAVE_PCRE2_H
typedef struct {
	char *pattern;
	pcre2_code *code;
	pcre2_match_data* match_data;
} grapheme_regex;
#endif

typedef struct anysplit_params
{
	int nparts;                /* maximum number of suffixes to split to */
	size_t altsmin;            /* minimum number of alternatives to generate */
	size_t altsmax;            /* maximum number of alternatives to generate */
	Regex_node *regpre, *regmid, *regsuf; /* issue matching combinations  */
#if HAVE_PCRE2_H
	grapheme_regex gr;
#endif // HAVE_PCRE2_H
	split_cache scl[MAX_WORD_TO_SPLIT+1]; /* split cache according to word length */
} anysplit_params;

#define DEBUG_ANYSPLIT 0


#if DEBUG_ANYSPLIT
static const char *gw;
/* print the current partitions */
static void printsplit(int *ps, int n)
{
	static int sn = 0; /* split number */
	int pos = 0;
	int p;
	int l = strlen(gw);

	printf("split %d: ", sn++);
	for (pos = 0, p = 0; pos < l && p <= n; pos++)
	{
		if (pos == ps[p])
		{
			p++;
			putchar(' ');
		}
		putchar(gw[pos]);
	}
	putchar('\n');
}
static void printps(int *ps, int n)
{
	int i;

	printf("printps:");
	for (i = 0; i<=n; i++) printf(" ps[%d]=%d", i, ps[i]);
	printf("\n");
}
#endif

static void cache_partitions(p_list pl, int *ps, int p)
{
	memcpy(pl, ps, sizeof(p_end) * p);
}

	/* p = 5      */
	/*   0  1 2 3 */
	/*   |  | | | */
	/* 123456789  */
	/* l = 9      */
	/*            */
	/* n = 4      */
	/* ps[0] = 2  */
	/* ps[1] = 5  */
	/* ps[2] = 7  */
	/* ps[3] = 9  */

/**
 * `scl`: If NULL, return the index of the last split, else cache the
 * splits into scl.
 */
static int split_and_cache(int word_length, int nparts, split_cache *scl)
{

	int n;
	int maxindex;
	p_list ps = alloca(sizeof(p_end)*nparts);

	if (0 == word_length) return 0;

	/* The first partitioning is the whole word.
	 * (Using a first dummy partition would make this code unneeded.)
	 * But in any case the whole word here is unneeded, and I'm
	 * too lazy to change that.
	 */
	ps[0] = word_length;
	maxindex = 0;
	if (scl) cache_partitions(&scl->sp[0], ps, nparts);

	/* Generate all possible partitions up to nparts partitions */
	for (n = 1; n < nparts; n++)
	{
		/* increase the number of partitions */
		int m = 0;
		int t;

		ps[0] = 1;
		ps[n] = word_length; /* set last partition end (dummy partition start) */

		//printf("New number of partitions: n=%d\n", n);
		do
		{
			/* set next initial partitions lengths to 1 */
			//printf("Initialize: m=%d\n", m);
			for (t = m; t < n; t++)
			{
				ps[t] = ps[m] + (t-m);
				//printf("ps[%d]=%d ", t, ps[t]);
			}
			//printf("\n");

			/* move last partition */
			//printf("Moving m=%d ps[m]=%d ps[m+1]=%d\n", n-1, ps[n-1], ps[n]);
			for (m = n-1; ps[m] < ps[m+1]; ps[m]++)
			{
				maxindex++;
				if (scl) cache_partitions(&scl->sp[maxindex*nparts], ps, nparts);

#if DEBUG_ANYSPLIT
				printsplit(ps, n);
				printps(ps, n);
#endif
			}

			/* last partition got to size 1, backtrack */
			do
			{
				//printf("Backtrack m %d->%d\n", m, m-1);
				m--;
				/* continue as long as there is a place to move for partition m */
			} while (m >= 0 && ps[m] + 1 == ps[m+1]);
			if (m >= 0) ps[m]++;
		} while (m >= 0); /* we have still positions to move */
		//printf("End (n=%d)\n", n);
	}

	return maxindex+1;
}

/*
 * Returns: Number of splits.
 */
static int split(int word_length, int nparts, split_cache *scl)
{
	size_t nsplits;

	if (NULL == scl->sp)
	{
		nsplits = split_and_cache(word_length, nparts, NULL);
		//printf("nsplits %zu\n", nsplits);
		if (0 == nsplits)
		{
			prt_error("Error: nsplits=0 (word_length=%d, nparts=%d)\n",
				word_length, nparts);
			return 0;
		}
		scl->sp = malloc(sizeof(p_end)*nparts * nsplits);
		scl->p_selected = malloc(sizeof(*(scl->p_selected)) * nsplits);
		scl->p_tried = malloc(sizeof(*(scl->p_tried)) * nsplits);
		split_and_cache(word_length, nparts, scl);
		scl->nsplits = nsplits;
	}

	memset(scl->p_selected, false, sizeof(*(scl->p_selected)) * scl->nsplits);
	memset(scl->p_tried, false, sizeof(*(scl->p_tried)) * scl->nsplits);
	return scl->nsplits;
}

/**
 * Return a number between 0 and nsplits-1, including.
 * No need for a good randomness; mediocre randomness is enough.
 * We suppose int is 32 bit.
 */
static int rng_uniform(unsigned int *seedp, size_t nsplits)
{
	int res;

	res = rand_r(seedp);

	/* I don't mind the slight skew */
	return res % nsplits;

}

/**
 * Match the \p word parts \p pl to REGPRE, REGMID, and REGSUF.
 * All the parts must match.
 */
#define D_MM 7
static bool morpheme_match(Sentence sent, const char *word, unsigned int nunits,
                           unsigned int *word_upos, p_list pl)
{
	Dictionary afdict = sent->dict->affix_table;
	anysplit_params *as = afdict->anysplit;
	char *word_part = alloca(strlen(word) + 1);

	lgdebug(+D_MM, "word=%s: ", word);
	for (int p = 0; p < as->nparts; p++)
	{
		size_t bos = 0, upos = 0; /* word offset, unit offset (both in bytes) */
		size_t b = word_upos[pl[p] - 1] - upos;
		Regex_node *re;

		memcpy(word_part, &word[bos], b);
		word_part[b] = '\0';
		bos += b;

		/* For flexibility, REGRPE is matched only to the prefix part,
		 * REGMID only to the middle suffixes, and REGSUF only to the
		 * suffix part - which cannot be the prefix. */
		if (0 == p) re = as->regpre;
		else if (pl[p] == (p_end)nunits) re = as->regsuf;
		else re = as->regmid;
		lgdebug(D_MM, "re=%s part%d=%s: ", re?re->name:"(nil)", p, word_part);

		/* A NULL regex always matches */
		if ((NULL != re) && (NULL == match_regex(re, word_part)))
		{
			lgdebug(D_MM, "No match\n");
			return false;
		}

		if (pl[p] == (int)nunits) break;
		upos = word_upos[pl[p] - 1];
	}

	lgdebug(D_MM, "Match\n");
	return true;
}
#undef D_MM

static Regex_node * regbuild(const char **regstring, int n, int classnum)
{
	Regex_node *regex_root = NULL;
	Regex_node **tail = &regex_root; /* Last Regex_node in list */
	int i;

	for (i = 0; i < n; i++)
	{
		const char *r = regstring[i];

		bool neg = ('!' == r[0]);
		if (neg || (0 == strncmp(r, "\\!", 2))) r++;

		/* Create a new Regex_node and add to the list. */
		Regex_node *new_re = regex_new(afdict_classname[classnum], r);
		new_re->neg = neg;

		/* read_entry() (read-dict.c) invokes patch_subscript() also for the affix
		 * file. As a result, if a regex contains a dot it is patched by
		 * SUBSCRIPT_MARK. We undo it here. */
		patch_subscript_mark(new_re->pattern);

		*tail = new_re;
		tail = &new_re->next;
	}
	return regex_root;
}

#if HAVE_PCRE2_H
static bool gr_reg_comp(grapheme_regex *re)
{
	PCRE2_SIZE erroffset;
	int rc;

	re->code = pcre2_compile((PCRE2_SPTR)re->pattern, PCRE2_ZERO_TERMINATED,
	                         PCRE2_UTF, &rc, &erroffset, NULL);
	if (re->code != NULL)
	{
		re->match_data = pcre2_match_data_create_from_pattern(re->code, NULL);
		if (re->match_data == NULL)
		{
			prt_error("Error: pcre2_match_data_create_from_pattern() failed\n");
			pcre2_code_free(re->code);
			return false;
		}
		return true;
	}

	/* We have an error. */
#define ERRBUFFLEN 120
	PCRE2_UCHAR errbuf[ERRBUFFLEN];
	pcre2_get_error_message(rc, errbuf, ERRBUFFLEN);
	prt_error("Error: Failed to compile grapheme regex \"%s\": %s (code %d) at %d\n",
	          re->pattern, errbuf, rc, (int)erroffset);
	return false;
}

static int gr_reg_match(const char *word, grapheme_regex *re)
{
	int rc = pcre2_match(re->code, (PCRE2_SPTR)word,
	                     PCRE2_ZERO_TERMINATED, /*startoffset*/0,
	                     PCRE2_NO_UTF_CHECK, re->match_data, NULL);
	if (rc == PCRE2_ERROR_NOMATCH) return rc;
	if (rc > 0) return rc;
	if (rc == 0)
	{
		prt_error("Error: pcre2_match(): ovector: Internal error\"\n");
		return rc;
	}

	/* We have an error. */
	PCRE2_UCHAR errbuf[ERRBUFFLEN];
	pcre2_get_error_message(rc, errbuf, ERRBUFFLEN);
	prt_error("Error: pcre2_match(): \"%s\": %s (code %d)\n",
	          re->pattern, errbuf, rc);
	return rc;
}

static void gr_pcre2_free(grapheme_regex *re)
{
	free(re->pattern);
	pcre2_match_data_free(re->match_data);
	pcre2_code_free(re->code);
}
#endif // HAVE_PCRE2_H

void free_anysplit(Dictionary afdict)
{
	size_t i;
	anysplit_params *as = afdict->anysplit;

	if (NULL == as) return;

	for (i = 0; i < ARRAY_SIZE(as->scl); i++)
	{
		if (NULL == as->scl[i].sp) continue;
		free(as->scl[i].sp);
		free(as->scl[i].p_selected);
		free(as->scl[i].p_tried);
	}
	free_regexs(as->regpre);
	free_regexs(as->regmid);
	free_regexs(as->regsuf);

#if HAVE_PCRE2_H
	gr_pcre2_free(&as->gr);
#endif // HAVE_PCRE2_H

	free(as);
	afdict->anysplit = NULL;
}

/**
 * Affix classes:
 * REGPARTS  Max number of word partitions. Value 0 disables anysplit.
 * REGPRE    Regex for prefix
 * REGMID    Regex for middle suffixes
 * REGSUF    Regex for suffix
 * REGALTS   Number of alternatives to issue for a word.
 *           Two values: minimum and maximum.
 *           If the word has more possibilities to split than the minimum,
 *           but less then the maximum, then issue them unconditionally.
 */

/**
 * Initialize the anysplit parameter and cache structure.
 * Return true if initialization succeeded, or if dictionary does not use
 * anysplit (its not an error to not use anysplit!).  Return false if
 * init failed.
 */
#define D_AI (D_DICT+0)
bool anysplit_init(Dictionary afdict)
{
	anysplit_params *as;
	size_t i;

	Afdict_class *regpre = AFCLASS(afdict, AFDICT_REGPRE);
	Afdict_class *regmid = AFCLASS(afdict, AFDICT_REGMID);
	Afdict_class *regsuf = AFCLASS(afdict, AFDICT_REGSUF);

	Afdict_class *regalts = AFCLASS(afdict, AFDICT_REGALTS);
	Afdict_class *regparts = AFCLASS(afdict, AFDICT_REGPARTS);

	if (0 == regparts->length)
	{
		if (verbosity_level(+D_AI))
			prt_error("Warning: File %s: Anysplit disabled (%s not defined)\n",
		             afdict->name, afdict_classname[AFDICT_REGPARTS]);
		return true;
	}
	if (1 != regparts->length)
	{
		prt_error("Error: File %s: Must have %s defined with one value\n",
		          afdict->name, afdict_classname[AFDICT_REGPARTS]);
		return false;
	}

	as = malloc(sizeof(anysplit_params));
	for (i = 0; i < ARRAY_SIZE(as->scl); i++) as->scl[i].sp = NULL;
	afdict->anysplit = as;

	as->regpre = regbuild(regpre->string, regpre->length, AFDICT_REGPRE);
	as->regmid = regbuild(regmid->string, regmid->length, AFDICT_REGMID);
	as->regsuf = regbuild(regsuf->string, regsuf->length, AFDICT_REGSUF);

	if (!compile_regexs(as->regpre, NULL) != 0) return false;
	if (!compile_regexs(as->regmid, NULL) != 0) return false;
	if (!compile_regexs(as->regsuf, NULL) != 0) return false;

	as->nparts = atoi(regparts->string[0]);
	if (as->nparts < 0)
	{
		free_anysplit(afdict);
		prt_error("Error: File %s: Value of %s must be a non-negative number\n",
		          afdict->name, afdict_classname[AFDICT_REGPARTS]);
		return false;
	}
	if (0 == as->nparts)
	{
		free_anysplit(afdict);
		prt_error("Warning: File %s: Anysplit disabled (0: %s)\n",
		          afdict->name, afdict_classname[AFDICT_REGPARTS]);
		return true;
	}

	if (2 != regalts->length)
	{
		free_anysplit(afdict);
		prt_error("Error: File %s: Must have %s defined with 2 values\n",
		          afdict->name, afdict_classname[AFDICT_REGALTS]);
		return false;
	}
	as->altsmin = atoi(regalts->string[0]);
	as->altsmax = atoi(regalts->string[1]);
	if ((atoi(regalts->string[0]) <= 0) || (atoi(regalts->string[1]) <= 0))
	{
		free_anysplit(afdict);
		prt_error("Error: File %s: Value of %s must be 2 positive numbers\n",
		          afdict->name, afdict_classname[AFDICT_REGALTS]);
		return false;
	}

#if HAVE_PCRE2_H
	const char upat[] = "\\X";
	const char bpat[] = "^(?>";
	const char epat[] = "(.+)?)$";

	// Build an optional match fore a single grapheme.
	const unsigned int ubuf_strlen = strlen(upat) + /*()?*/3;
	char *ubuf = alloca(ubuf_strlen + 1);
	snprintf(ubuf, ubuf_strlen + 1, "(%s)?", upat);

	// Build a pattern to match all the graphemes in a word: "^(>(\\X)?...)$"
	as->gr.pattern =
		malloc(sizeof(bpat)-1 + ubuf_strlen * MAX_WORD_TO_SPLIT + sizeof(epat));
	strcpy(as->gr.pattern, bpat);
	unsigned int n = strlen(as->gr.pattern);
	for (i = 0; i < MAX_WORD_TO_SPLIT; i++, n+= ubuf_strlen)
		strcpy(&as->gr.pattern[n], ubuf);
	strcpy(&as->gr.pattern[n], epat);

	if (!gr_reg_comp(&as->gr)) return false;
#endif // HAVE_PCRE2_H

	return true;
}
#undef D_AI

/*
 * Return the number of units (codepoints or graphemes) in \p word.
 * On error, return 0;
 */
static unsigned int strlen_units(anysplit_params *as, const char *word)
{
#if !HAVE_PCRE2_H
	// Number of codepoints.
	return (unsigned int)utf8_strlen(word);
#else // HAVE_PCRE2_H
	// Number of graphemes.
	int rc = gr_reg_match(word, &as->gr);
	if (rc <= 1) return 0;
	return (unsigned int)(rc - 1);
#endif // !HAVE_PCRE2_H
}

/**
 * Set the elements of \p word_pos (containing \p nunits elements) to the
 * end positions (last char position + 1) of the atomic units in \p word.
 */
static void	build_unit_positions(anysplit_params *as, const char *word,
											unsigned int nunits, unsigned int *word_pos)
{
	 dassert(nunits != 0, "At least one atomic unit is expected");
	 const unsigned int *word_pos_base = word_pos;

#if HAVE_PCRE2_H
	PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(as->gr.match_data);

	/* The first [start,end) is of the whole match (the word in this case). */
	for (unsigned int i = 1; i < nunits + 1; i++)
		*word_pos++ = (unsigned int)ovector[2*i + 1];

#else
	unsigned int pos = 0;

	for (unsigned int i = 0; word[i] != '\0'; i = pos)
	{
		pos += utf8_charlen(&word[i]);
		*word_pos++ = pos;
	}

#endif // HAVE_PCRE2_H

	if (verbosity_level(D_ANYS+1))
	{
		unsigned int bos = 0;

		prt_error("Debug: %u atomic units:\n\\", nunits);
		for (unsigned int i = 0; i < nunits; i ++)
		{
			prt_error("%u) %.*s\n\\", i+1,(int)(word_pos_base[i]-bos), &word[bos]);
			bos = word_pos_base[i];
		}
		prt_error("\n");

	}
	dassert(word_pos[-1] == strlen(word), "Inconsistent word end");
}

/**
 * Split randomly.
 * Return true on success.
 * Return false when:
 * - disabled (i.e. when doing regular language processing).
 * - an error occurs (the behavior then is undefined).
 *   Such an error has not been observed yet.
 */
bool anysplit(Sentence sent, Gword *unsplit_word)
{
	Dictionary afdict = sent->dict->affix_table;
	if (NULL == afdict) return false;
	anysplit_params * as = afdict->anysplit;
	if ((NULL == as) || (0 == as->nparts)) return false; /* Anysplit disabled */

	const char * word = unsplit_word->subword;
	Afdict_class * stemsubscr;

	int sample_point;
	size_t nsplits;
	size_t rndtried = 0;
	size_t rndissued = 0;
	size_t i;
	unsigned int seed = sent->rand_state;
	bool use_sampling = true;
	unsigned int nunits = strlen_units(as, word);

	size_t l = strlen(word);
	char *affix = alloca(l+2+1); /* word + ".=" + NUL: Max. affix length */

	if (0 == l)
	{
		prt_error("Warning: anysplit(): word length 0\n");
		return false;
	}

	if ((nunits > MAX_WORD_TO_SPLIT) || (nunits == 0))
	{
		Gword *alt = issue_word_alternative(sent, unsplit_word, "AS>",
		                       0,NULL, 1,&word, 0,NULL);
		tokenization_done(sent, alt);
		return true;
	}

	unsigned int *word_upos = alloca(sizeof(int) * nunits);
	build_unit_positions(as, word, nunits, word_upos);

	stemsubscr = AFCLASS(afdict, AFDICT_STEMSUBSCR);

	// seed = time(NULL)+(unsigned int)(long)&seed;

#if DEBUG_ANYSPLIT
	gw = word;
#endif

	nsplits = split(nunits, as->nparts, &as->scl[nunits]);
	if (0 == nsplits)
	{
		prt_error("Warning: anysplit(): split() failed (shouldn't happen)\n");
		return false;
	}

	if (as->altsmax >= nsplits)
	{
		/* Issue everything */
		sample_point = -1;
		use_sampling = false;
	}

	lgdebug(+D_ANYS, "Start%s sampling: word=%s, nsplits=%zu, maxsplits=%d, "
	        "as->altsmin=%zu, as->altsmax=%zu\n", use_sampling ? "" : " no",
	        word, nsplits, as->nparts, as->altsmin, as->altsmax);

	while (rndtried < nsplits && (!use_sampling || (rndissued < as->altsmax)))
	{
		if (use_sampling)
		{
			sample_point = rng_uniform(&seed, nsplits);

			if (sample_point < 0) /* Cannot happen with rand_r() */
			{
				prt_error("Error: rng: %s\n", strerror(errno));
				return false;
			}
		}
		else
		{
			sample_point++;
		}

		lgdebug(D_ANYS, "Sample: %d ", sample_point);
		if (as->scl[nunits].p_tried[sample_point])
		{
			lgdebug(D_ANYS+1, "(repeated)\n");
			continue;
		}
		lgdebug(D_ANYS+1, "(new)");
		rndtried++;
		as->scl[nunits].p_tried[sample_point] = true;
		/* The regexes in the affix file can be used to reject partitioning
		 * that break graphemes. */
		if (morpheme_match(sent, word, nunits, word_upos, &as->scl[nunits].sp[sample_point*as->nparts]))
		{
			as->scl[nunits].p_selected[sample_point] = true;
			rndissued++;
		}
		else
		{
			lgdebug(D_ANYS, "\n");
		}
	}

	lgdebug(D_ANYS, "Results: word '%s' (units=%u byte-length=%zu): %zu/%zu:\n",
	        word, nunits, l, rndissued, nsplits);

	for (i = 0; i < nsplits; i++)
	{
		size_t bos = 0, upos = 0; /* byte offset, codepoint offset */
		const char **affixes = NULL;
		int num_sufixes;
		int num_affixes = 0;

		if (!as->scl[nunits].p_selected[i]) continue;

		p_list pl = &as->scl[nunits].sp[i*as->nparts];
		for (int p = 0; p < as->nparts; p++)
		{
			size_t b = word_upos[pl[p] - 1] - upos;

			memcpy(affix, &word[bos], b);
			affix[b] = '\0';
			bos += b;
			altappend(sent, &affixes, affix);

			if (bos == l) break;
			upos = word_upos[pl[p] - 1];
			num_affixes++;
		}

		const char **prefix_position, **stem_position , **suffix_position;
		switch (num_affixes)
		{
			case 0:
				prefix_position = NULL;
				stem_position = &affixes[0]; /* May be just a word here */
				suffix_position = NULL;
				num_sufixes = 0;
				break;
			case 1:
				prefix_position = NULL;
				stem_position = &affixes[0];
				suffix_position = &affixes[1];
				num_sufixes = 1;
				break;
			default:
				prefix_position =&affixes[0];
				stem_position = &affixes[1];
				suffix_position = &affixes[2];
				num_sufixes = num_affixes - 1;
				break;
		}
		if (num_affixes > 0)
		{
			if (0 != stemsubscr->length) {
				strcpy(affix, stem_position[0]);
				strcat(affix, stemsubscr->string[0]);
				stem_position[0] = affix;
			}
		}

		// XXX FIXME -- this is wrong - it assumes a
		// variable number of suffixes.
		/* Here a leading INFIX_MARK is added to the suffixes if needed. */
		Gword *alt = issue_word_alternative(sent, unsplit_word, "AS",
		        (NULL == prefix_position) ? 0 : 1, prefix_position,
		        1, stem_position,
		        num_sufixes, suffix_position);
		tokenization_done(sent, alt);
		free(affixes);
	}

	/* 0 == sent->rand_state denotes "repeatable rand". */
	if (0 != sent->rand_state) sent->rand_state = seed;
	return true;
}
