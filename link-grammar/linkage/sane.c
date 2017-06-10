/*************************************************************************/
/* Copyright 2013, 2014 Linas Vepstas                                    */
/* Copyright 2014 Amir Plivatsky                                         */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <stdlib.h>

#include "api-structures.h"  // for Sentence_s
#include "api-types.h"
#include "dict-common/dict-structures.h"
#include "dict-common/regex-morph.h" // for match_regex
#include "disjunct-utils.h"  // for Disjunct_struct
#include "lg_assert.h"
#include "linkage.h"
#include "print/print.h"  // for print_with_subscript_dot
#include "sane.h"
#include "tokenize/tok-structures.h" // Needed for Wordgraph_pathpos_s
#include "tokenize/wordgraph.h"
#include "utilities.h"

/**
 * Construct word paths (one or more) through the Wordgraph.
 *
 * Add 'current_word" to the potential path.
 * Add "p" to the path queue, which defines the start of the next potential
 * paths to be checked.
 *
 * Each path is up to the current word (not including). It doesn't actually
 * construct a full path if there are null words - they break it. The final path
 * is constructed when the Wordgraph termination word is encountered.
 *
 * Note: The final path doesn't match the linkage word indexing if the linkage
 * contains empty words, at least until empty words are eliminated from the
 * linkage (in compute_chosen_words()). Further processing of the path is done
 * there in case morphology splits are to be hidden or there are morphemes with
 * null linkage.
 */
static void wordgraph_path_append(Wordgraph_pathpos **nwp, const Gword **path,
                                  Gword *current_word, /* add to the path */
                                  Gword *p)      /* add to the path queue */
{
	size_t n = wordgraph_pathpos_len(*nwp);

	assert(NULL != p, "Tried to add a NULL word to the word queue");

	/* Check if the path queue already contains the word to be added to it. */
	if (NULL != *nwp)
	{
		const Wordgraph_pathpos *wpt;

		for (wpt = *nwp; NULL != wpt->word; wpt++)
		{
			if (p == wpt->word)
			{
				/* If we are here, there are 2 or more paths leading to this word
				 * (p) that end with the same number of consecutive null words that
				 * consist an entire alternative. These null words represent
				 * different ways to split the subword upward in the hierarchy, but
				 * since they don't have linkage we don't care which of these
				 * paths is used. */
				return; /* The word is already in the queue */
			}
		}
	}

	/* Not already in the path queue - add it. */
	*nwp = wordgraph_pathpos_resize(*nwp, n+1);
	(*nwp)[n].word = p;

	if (MT_INFRASTRUCTURE == p->prev[0]->morpheme_type)
	{
			/* Previous word is the Wordgraph dummy word. Initialize the path. */
			(*nwp)[n].path = NULL;
	}
	else
	{
		/* We branch to another path. Duplicate it from the current path and add
		 * the current word to it. */
		size_t path_arr_size = (gwordlist_len(path)+1)*sizeof(*path);

		(*nwp)[n].path = malloc(path_arr_size);
		memcpy((*nwp)[n].path, path, path_arr_size);
	}
   /* FIXME (cast) but anyway gwordlist_append() doesn't modify Gword. */
	gwordlist_append((Gword ***)&(*nwp)[n].path, current_word);
}

/**
 * Free the Wordgraph paths and the Wordgraph_pathpos array.
 * In case of a match, the final path is still needed so this function is
 * then invoked with free_final_path=false.
 */
static void wordgraph_path_free(Wordgraph_pathpos *wp, bool free_final_path)
{
	Wordgraph_pathpos *twp;

	if (NULL == wp) return;
	for (twp = wp; NULL != twp->word; twp++)
	{
		if (free_final_path || (MT_INFRASTRUCTURE != twp->word->morpheme_type))
			free(twp->path);
	}
	free(wp);
}

/* ============================================================== */
/* A kind of morphism post-processing */

/* These letters create a string that should be matched by a
 * SANEMORPHISM regex, given in the affix file. The empty word
 * doesn't have a letter. E.g. for the Russian dictionary: "w|ts".
 * It is converted here to: "^((w|ts)b)+$".
 * It matches "wbtsbwbtsbwb" but not "wbtsbwsbtsb".
 * FIXME? In this version of the function, 'b' is not yet supported,
 * so "w|ts" is converted to "^(w|ts)+$" for now.
 */
#define AFFIXTYPE_PREFIX   'p'   /* prefix */
#define AFFIXTYPE_STEM     't'   /* stem */
#define AFFIXTYPE_SUFFIX   's'   /* suffix */
#define AFFIXTYPE_MIDDLE   'm'   /* middle morpheme */
#define AFFIXTYPE_WORD     'w'   /* regular word */
#ifdef WORD_BOUNDARIES
#define AFFIXTYPE_END      'b'   /* end of input word */
#endif

/**
 * This routine solves the problem of mis-linked alternatives,
 * i.e a morpheme in one alternative that is linked to a morpheme in
 * another alternative. This can happen due to the way in which word
 * alternatives are implemented.
 *
 * It does so by checking that all the chosen disjuncts in a linkage
 * (including null words) match, in the same order, a path in the
 * Wordgraph.
 *
 * An important side effect of this check is that if the linkage is
 * good, its Wordgraph path is found.
 *
 * Optionally (if SANEMORPHISM regex is defined in the affix file), it
 * also validates that the morpheme-type sequence is permitted for the
 * language. This is a sanity check of the program and the dictionary.
 *
 * Return true if the linkage is good, else return false.
 */
#define D_SLM 7
bool sane_linkage_morphism(Sentence sent, Linkage lkg, Parse_Options opts)
{
	Wordgraph_pathpos *wp_new = NULL;
	Wordgraph_pathpos *wp_old = NULL;
	Wordgraph_pathpos *wpp;
	Gword **next; /* next Wordgraph words of the current word */
	size_t i;

	bool match_found = true; /* if all the words are null - it's still a match */
	Gword **lwg_path;

	Dictionary afdict = sent->dict->affix_table;       /* for SANEMORPHISM */
	char *const affix_types = alloca(sent->length*2 + 1);   /* affix types */
	affix_types[0] = '\0';

	lkg->wg_path = NULL;

	/* Populate the path word queue, initializing the path to NULL. */
	for (next = sent->wordgraph->next; *next; next++)
	{
		wordgraph_path_append(&wp_new, /*path*/NULL, /*add_word*/NULL, *next);
	}
	assert(NULL != wp_new, "Path word queue is empty");

	for (i = 0; i < lkg->num_words; i++)
	{
		Disjunct *cdj;            /* chosen disjunct */

		lgdebug(D_SLM, "lkg=%p Word %zu: ", lkg, i);

		if (NULL == wp_new)
		{
			lgdebug(D_SLM, "- No more words in the wordgraph\n");
			match_found = false;
			break;
		}

		if (wp_old != wp_new)
		{
			wordgraph_path_free(wp_old, true);
			wp_old = wp_new;
		}
		wp_new = NULL;
		//wordgraph_pathpos_print(wp_old);

		cdj = lkg->chosen_disjuncts[i];
		/* Handle null words */
		if (NULL == cdj)
		{
			lgdebug(D_SLM, "- Null word\n");
			/* A null word matches any word in the Wordgraph -
			 * so, unconditionally proceed in all paths in parallel. */
			match_found = false;
			for (wpp = wp_old; NULL != wpp->word; wpp++)
			{
				if (NULL == wpp->word->next)
					continue; /* This path encountered the Wordgraph end */

				/* The null words cannot be marked here because wpp->path consists
				 * of pointers to the Wordgraph words, and these words are common to
				 * all the linkages, with potentially different null words in each
				 * of them. However, the position of the null words can be inferred
				 * from the null words in the word array of the Linkage structure.
				 */
				for (next = wpp->word->next; NULL != *next; next++)
				{
					match_found = true;
					wordgraph_path_append(&wp_new, wpp->path, wpp->word, *next);
				}
			}
			continue;
		}

		if (!match_found)
		{
			const char *e = "Internal error: Too many words in the linkage";
			lgdebug(D_SLM, "- %s\n", e);
			prt_error("Error: %s.\n", e);
			break;
		}

		if (verbosity_level(D_SLM)) prt_error("%s", cdj->string);

		match_found = false;
		/* Proceed in all the paths in which the word is found. */
		for (wpp = wp_old; NULL != wpp->word; wpp++)
		{
			for (gword_set *gl = cdj->originating_gword; NULL != gl; gl =  gl->next)
			{
				if (gl->o_gword == wpp->word)
				{
					match_found = true;
					for (next = wpp->word->next; NULL != *next; next++)
					{
						wordgraph_path_append(&wp_new, wpp->path, wpp->word, *next);
					}
					break;
				}
			}
		}

		if (!match_found)
		{
			/* FIXME? A message can be added here if there are too many words
			 * in the linkage (can happen only if there is an internal error). */
			lgdebug(D_SLM, "- No Wordgraph match\n");
			break;
		}
		lgdebug(D_SLM, "\n");
	}

	if (match_found)
	{
		match_found = false;
		/* Validate that there are no missing words in the linkage.
		 * It is so, if the dummy termination word is found in the
		 * new pathpos queue.
		 */
		if (NULL != wp_new)
		{
			for (wpp = wp_new; NULL != wpp->word; wpp++)
			{
				if (MT_INFRASTRUCTURE == wpp->word->morpheme_type) {
					match_found = true;
					/* Exit the loop with with wpp of the termination word. */
					break;
				}
			}
		}
		if (!match_found)
		    lgdebug(D_SLM, "%p Missing word(s) at the end of the linkage.\n", lkg);
	}

#define DEBUG_morpheme_type 0
	/* Check the morpheme type combination.
	 * If null_count > 0, the morpheme type combination may be invalid
	 * due to null subwords, so skip this check. */
	if (match_found && (0 == sent->null_count) &&
		(NULL != afdict) && (NULL != afdict->regex_root))
	{
		const Gword **w;
		char *affix_types_p = affix_types;

		/* Construct the affix_types string. */
#if DEBUG_morpheme_type
		print_lwg_path(wpp->path);
#endif
		i = 0;
		for (w = wpp->path; *w; w++)
		{
			i++;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
			switch ((*w)->morpheme_type)
			{
#pragma GCC diagnostic pop
				default:
					/* What to do with the rest? */
				case MT_WORD:
					*affix_types_p = AFFIXTYPE_WORD;
					break;
				case MT_PREFIX:
					*affix_types_p = AFFIXTYPE_PREFIX;
					break;
				case MT_STEM:
					*affix_types_p = AFFIXTYPE_STEM;
					break;
				case MT_MIDDLE:
					*affix_types_p = AFFIXTYPE_MIDDLE;
					break;
				case MT_SUFFIX:
					*affix_types_p = AFFIXTYPE_SUFFIX;
					break;
			}

#if DEBUG_morpheme_type
			lgdebug(D_SLM, "Word %zu: %s affixtype=%c\n",
			     i, (*w)->subword,  *affix_types_p);
#endif

			affix_types_p++;
		}
		*affix_types_p = '\0';

#ifdef WORD_BOUNDARIES /* not yet implemented */
		{
			const Gword *uw;

			/* If w is an "end subword", return its unsplit word, else NULL. */
			uw = word_boundary(w); /* word_boundary() unimplemented */

			if (NULL != uw)
			{
				*affix_types_p++ = AFFIXTYPE_END;
				lgdebug(D_SLM, "%p End of Gword %s\n", lkg, uw->subword);
			}
		}
#endif

		/* Check if affix_types is valid according to SANEMORPHISM. */
		if (('\0' != affix_types[0]) &&
		    (NULL == match_regex(afdict->regex_root, affix_types)))
		{
			/* Morpheme type combination is invalid */
			match_found = false;
			/* Notify to stdout, so it will be shown along with the result.
			 * XXX We should have a better way to notify. */
			if (0 < opts->verbosity)
				prt_error("Warning: Invalid morpheme type combination '%s'.\n"
				          "Run with !bad and !verbosity>"STRINGIFY(D_USER_MAX)
				          " to debug\n", affix_types);
		}
	}

	if (match_found) lwg_path = (Gword **)wpp->path; /* OK to modify */
	wordgraph_path_free(wp_old, true);
	wordgraph_path_free(wp_new, !match_found);

	if (match_found)
	{
		if ('\0' != affix_types[0])
		{
			lgdebug(D_SLM, "%p Morpheme type combination '%s'\n", lkg, affix_types);
		}
		lgdebug(+D_SLM, "%p SUCCEEDED\n", lkg);
		lkg->wg_path = lwg_path;
		return true;
	}

	/* Oh no ... invalid morpheme combination! */
	lgdebug(D_SLM, "%p FAILED\n", lkg);
	return false;
}
#undef D_SLM
