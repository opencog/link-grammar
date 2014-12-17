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

#ifdef USE_ANYSPLIT

/**
 * anysplit.c -- code that splits words into random morphemes.
 * This is used for the language-learning/morpheme-learning project.
 * This code is conditionally compiled, because ...
 */

/* General assumptions:
 * - false is binary 0 (for memset())
 * - int is >= 32 bit (for random number)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <externs.h>
#include <time.h>

#include "dict-common.h"
#include "error.h"
#include "regex-morph.h"
#include "structures.h"
#include "tokenize.h"
#include "utilities.h"

#include "anysplit.h"

extern const char * const afdict_classname[];

typedef int p_start;     /* partition start in a word */
typedef p_start *p_list; /* list of partitions in a word */

typedef struct split_cache /* split cached by word length */
{
	size_t nsplits;      /* number of splits */
	p_list sp;           /* list of splits */
	bool *p_tried;       /* list of tried splits */
	bool *p_selected;    /* list of selected splits */
} split_cache;

#ifndef NUMELEMS
#define NUMELEMS(a) (sizeof(a) / sizeof((a)[0]))
#endif
typedef struct anysplit_params
{
	int nparts;                /* maximum number of parts to split to */
	size_t altsmin;            /* minimum number of alternatives to generate */
	size_t altsmax;            /* maximum number of alternatives to generate */
	Regex_node *regpre, *regmid, *regsuf; /* issue matching combinations  */
	split_cache scl[MAX_WORD]; /* split cache according to word length */
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
	memcpy(pl, ps, sizeof(p_start) * p);
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
 * scl: If NULL, return the index of the last split. Else cache the splits into scl.
 */
static int split_and_cache(int word_length, int nparts, split_cache *scl)
{

	int n;
	int maxindex;
	p_list ps = alloca(sizeof(p_start)*nparts); /* partition start */

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

void free_anysplit(Dictionary afdict)
{
	size_t i;
	anysplit_params *as = afdict->anysplit;

	if (NULL == as) return;

	for (i = 0; i < NUMELEMS(as->scl); i++)
	{
		if (NULL == as->scl[i].sp) continue;
		free(as->scl[i].sp);
		free(as->scl[i].p_selected);
		free(as->scl[i].p_tried);
	}
	free_regexs(as->regpre);
	free_regexs(as->regmid);
	free_regexs(as->regsuf);
	free(as);
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
			fprintf(stderr, "Error: nsplits=%zu (word_length=%d, nparts=%d)\n",
				nsplits, word_length, nparts);
			return 0;
		}
		scl->sp = malloc(sizeof(p_start)*nparts * nsplits);
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
 * No need for a good randomness.
 * We suppose int is 32 bit.
 */
static int rng_uniform(unsigned int *seedp, size_t nsplits)
{
	int res;

	res = rand_r(seedp);

	/* I don't mind the slight skew */
	return res % nsplits;

}

static bool morpheme_match(Sentence sent, const char *word, int l, p_list pl)
{
	Dictionary afdict = sent->dict->affix_table;
	anysplit_params *as = afdict->anysplit;
	int pos = 0;
	int p;
	Regex_node *re;
	char *prefix_string = alloca(l+1);

	lgdebug(+2, "word=%s: ", word);
	for (p = 0; p < as->nparts; p++)
	{
		strncpy(prefix_string, &word[pos], pl[p]-pos);
		prefix_string[pl[p]-pos] = '\0';

		/* For flexibility, REGRPE is matched only to the prefix part,
		 * REGMID only to the middle suffixes, and REGSUF only to the suffix part -
		 * which cannot be the prefix. */
		if (0 == p) re = as->regpre;
		else if (pl[p] == l) re = as->regsuf;
		else re = as->regmid;
		lgdebug(2, "re=%s part%d=%s: ", re->name, p, prefix_string);

		/* A NULL regex always matches */
		if ((NULL != re) && (NULL == match_regex(re ,prefix_string)))
		{
			lgdebug(2, "No match\n");
			return false;
		}

		pos = pl[p];
		if (pos == l) break;
	}

	lgdebug(2, "Match\n");
	return true;
}

static Regex_node * regbuild(const char **regstring, int n, int classnum)
{
	Regex_node *regex_root = NULL;
	Regex_node **tail = &regex_root; /* Last Regex_node in list */
	Regex_node *new_re;
	int i;

	char *s;
	char *sm;

	for (i = 0; i < n; i++)
	{
		/* read_entry() (read-dict.c) invokes patch_subscript() also for the affix
		 * file. As a result, if a regex contains a dot it is patched by
		 * SUBSCRIPT_MARK. We undo it here. */
		s = strdup(regstring[i]);
		sm = strrchr(s, SUBSCRIPT_MARK);
		if (sm) *sm = SUBSCRIPT_DOT;

		/* Create a new Regex_node and add to the list. */
		new_re = malloc(sizeof(*new_re));
		new_re->name    = strdup(afdict_classname[classnum]);
		new_re->pattern = s;
		new_re->re      = NULL;
		new_re->next    = NULL;
		*tail = new_re;
		tail	= &new_re->next;
	}
	return regex_root;
}


/* Affix classes:
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
 */
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
		/* FIXME: Early assignment of verbosity by -v=x argument. */
		if (verbosity > 1)
			prt_error("Warning: File %s: Anysplit disabled (%s not defined)",
		             afdict->name, afdict_classname[AFDICT_REGPARTS]);
		return true;
	}
	if (1 != regparts->length)
	{
		prt_error("Error: File %s: Must have %s defined with one value",
		          afdict->name, afdict_classname[AFDICT_REGPARTS]);
		return false;
	}

	as = malloc(sizeof(anysplit_params));
	for (i = 0; i < NUMELEMS(as->scl); i++) as->scl[i].sp = NULL;
	afdict->anysplit = as;

	as->regpre = regbuild(regpre->string, regpre->length, AFDICT_REGPRE);
	as->regmid = regbuild(regmid->string, regmid->length, AFDICT_REGMID);
	as->regsuf = regbuild(regsuf->string, regsuf->length, AFDICT_REGSUF);

	if (compile_regexs(as->regpre, NULL) != 0) return false;
	if (compile_regexs(as->regmid, NULL) != 0) return false;
	if (compile_regexs(as->regsuf, NULL) != 0) return false;

	as->nparts = atoi(regparts->string[0]);
	if (as->nparts < 0)
	{
		prt_error("Error: File %s: Value of %s must be a non-negative number",
		          afdict->name, afdict_classname[AFDICT_REGPARTS]);
		return false;
	}
	if (0 == as->nparts)
	{
		prt_error("Warning: File %s: Anysplit disabled (0: %s)\n",
		          afdict->name, afdict_classname[AFDICT_REGPARTS]);
		return true;
	}

	if (2 != regalts->length)
	{
		prt_error("Error: File %s: Must have %s defined with 2 values",
		          afdict->name, afdict_classname[AFDICT_REGALTS]);
		return false;
	}
	as->altsmin = atoi(regalts->string[0]);
	as->altsmax = atoi(regalts->string[1]);
	if ((atoi(regalts->string[0]) <= 0) || (atoi(regalts->string[1]) <= 0))
	{
		prt_error("Error: File %s: Value of %s must be 2 positive numbers",
		          afdict->name, afdict_classname[AFDICT_REGALTS]);
		return false;
	}

	return true;
}

/**
 * Split randomly.
 * Return true on success.
 * Return false when:
 * - disabled (i.e. when doing regular language processing).
 * - an error occurs (the behavior then is undefined).
 *   Such an error has not been observed yet.
 */
bool anysplit(Sentence sent, const char *word)
{
	Dictionary afdict = sent->dict->affix_table;
	anysplit_params *as;
	Afdict_class * stemsubscr;
	size_t stemsubscr_len;

	size_t l = strlen(word);
	p_list pl;
	size_t pos;
	int p;
	int sample_point;
	size_t nsplits;
	size_t rndtried = 0;
	size_t rndissued = 0;
	size_t i;
	unsigned int seed = 0;
	char *prefix_string = alloca(l+2+1); /* word + ".=" + NUL */
	char *suffix_string = alloca(l+1);   /* word + NUL */
	bool use_sampling = true;
	const char infix_mark = INFIX_MARK(afdict);


	if (NULL == afdict) return false;
	as = afdict->anysplit;

	if ((NULL == as) || (0 == as->nparts)) return false; /* Anysplit disabled */

	if (0 == l)
	{
		prt_error("Warning: anysplit(): word length 0\n");
		return false;
	}

	stemsubscr = AFCLASS(afdict, AFDICT_STEMSUBSCR);
	stemsubscr_len = (NULL == stemsubscr->string[0]) ? 0 :
		strlen(stemsubscr->string[0]);

	/* Don't split morphemes again. If INFIXMARK and/or SUBSCRMARK are
	 * not defined in the affix file, then morphemes may get split again unless
	 * restricted by REGPRE/REGMID/REGSUF. */
	if (word[0] == infix_mark) return true;
	if ((l > stemsubscr_len) &&
	    (0 == strcmp(word+l-stemsubscr_len, stemsubscr->string[0])))
		return true;

	// seed = time(NULL)+(unsigned int)(long)&seed;

#if DEBUG_ANYSPLIT
	gw = word;
#endif

	nsplits = split(l, as->nparts, &as->scl[l]);
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

	lgdebug(+2, "Start%s sampling: word=%s, nsplits=%zu, maxsplits=%d, "
	        "as->altsmin=%zu, as->altsmax=%zu\n", use_sampling ? "" : " no",
	        word, nsplits, as->nparts, as->altsmin, as->altsmax);

	while (rndtried < nsplits && (!use_sampling || (rndissued < as->altsmin)))
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

		lgdebug(2, "Sample: %d ", sample_point);
		if (as->scl[l].p_tried[sample_point])
		{
			lgdebug(4, "(repeated)\n");
			continue;
		}
		lgdebug(4, "(new)");
		rndtried++;
		as->scl[l].p_tried[sample_point] = true;
		if (morpheme_match(sent, word, l, &as->scl[l].sp[sample_point*as->nparts]))
		{
			as->scl[l].p_selected[sample_point] = true;
			rndissued++;
		}
		else
		{
			lgdebug(2, "\n");
		}
	}

	lgdebug(2, "Results: word '%s' (length=%zu): %zu/%zu:\n", word, l, rndissued, nsplits);

	for (i = 0; i < nsplits; i++)
	{
		const char **suffixes = NULL;
		int num_suffixes = 0;

		if (!as->scl[l].p_selected[i]) continue;

		pl = &as->scl[l].sp[i*as->nparts];
		pos = 0;
		for (p = 0; p < as->nparts; p++)
		{
			if (pl[0] == (int)l)  /* This is the whole word */
			{
				strncpy(prefix_string, &word[pos], pl[p]-pos);
				prefix_string[pl[p]-pos] = '\0';
			}
			else
			if (0 == pos)   /* The first but not the only morpheme */
			{
				strncpy(prefix_string, &word[pos], pl[p]-pos);
				prefix_string[pl[p]-pos] = '\0';

				if (0 != stemsubscr->length)
				    strcat(prefix_string, stemsubscr->string[0]);
			}
			else           /* 2nd and on morphemes */
			{
				strncpy(suffix_string, &word[pos], pl[p]-pos);
				suffix_string[pl[p]-pos] = '\0';
				altappend(sent, &suffixes, suffix_string);
				num_suffixes++;
			}

			pos = pl[p];
			if (pos == l) break;
		}

		/* Here a leading INFIX_MARK is added to the suffixes if needed. */
		add_alternative(sent,
		   0,NULL, 1,(const char **)&prefix_string, num_suffixes,suffixes);
		free(suffixes);
	}

	return true;
}

#endif /* USE_ANYSPLIT */
