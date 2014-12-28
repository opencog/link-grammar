/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright 2008, 2009, 2013, 2014 Linas Vepstas                        */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <stdint.h>
#include <stdlib.h>

#include "analyze-linkage.h"
#include "api-structures.h"
#include "dict-common.h"
#include "disjuncts.h"
#include "extract-links.h"
#include "idiom.h"
#include "link-includes.h"
#include "linkage.h"
#include "post-process.h"
#include "print.h"
#include "print-util.h"
#include "sat-solver/sat-encoder.h"
#include "string-set.h"
#include "structures.h"
#include "word-utils.h"

/**
 * The functions defined in this file are primarily a part of the user API
 * for working with linkages.
 */

/*
 * The "empty word" is a concept used in order to make the current parser able
 * to parse "alternatives" within a sentence. The "empty word" can link to any
 * word, hence it is issued when a word is optional.  Currently, for coding
 * convenience, the "empty * word" is also automatically inserted into a
 * sentence, so as to balance the total count of words in alternatives.
 *
 * However, the empty words are not needed after the linkage step, and so,
 * below, we remove them from the linkage, as well as the links that
 * connect to them.
 *
 * The empty word device is employed by every non-English dictionary. (viz.
 * Russian, German, Lithuanian, ...). Recently it has been added to the English
 * dictionary, and is issued in cases of contraction splitting, spell guesses,
 * and unit-strip alternatives. For more information, see EMPTY_WORD.zzz in the
 * dict file.
 */
#define EMPTY_WORD_SUPPRESS ("ZZZ") /* link to pure whitespace */

#define INFIX_MARK_L 1         /* INFIX_MARK is 1 character */

#define SUFFIX_SUPPRESS ("LL") /* suffix links start with this */
#define SUFFIX_SUPPRESS_L 2    /* length of above */

#define HIDE_MORPHO   (!display_morphology)

/**
 * This takes the current chosen_disjuncts array and uses it to
 * compute the chosen_words array.  "I.xx" suffixes are eliminated.
 *
 * chosen_words[]
 *    An array of pointers to strings.  These are the words to be displayed
 *    when printing the solution, the links, etc.  Computed as a function of
 *    chosen_disjuncts[] by compute_chosen_words().  This differs from
 *    sentence[].string because it contains the suffixes.  It differs from
 *    chosen_disjunct[].string in that the idiom symbols have been removed.
 *
 */
void compute_chosen_words(Sentence sent, Linkage linkage, Parse_Options opts)
{
	size_t i, j, l;
	char * s, *u;
	const char ** chosen_words = alloca(sent->length*sizeof(char*));
	size_t *remap = alloca(sent->length*sizeof(size_t));
	bool display_morphology = opts->display_morphology;
	const char infix_mark = INFIX_MARK(sent->dict->affix_table);

	/* XXX TODO -- this should be not sent->length but
	 * linkage->num_words ...and likewise, we should not be looking at
	 * sent->word[i].unsplit_word but the correct path-word for the linkage */
	for (i=0; i<sent->length; i++)
	{
		const char *t;
		/* If chosen_disjuncts is NULL, then this is an 'island' word
		 * that has not been linked to. */
		if (linkage->chosen_disjuncts[i] == NULL)
		{
			/* The unsplit_word is the original word; if its been split
			 * into stem+suffix, and either one hasn't been chosen, then
			 * neither should be printed.  Do, however, put brackets around
			 * the original word, and print that.
			 */
			t = sent->word[i].unsplit_word;
			if (t)
			{
				l = strlen(t) + 2;
				s = (char *) xalloc(l+1);
				snprintf(s, l+1, "[%s]", t);
				t = string_set_add(s, sent->string_set);
				xfree(s, l+1);
			}
			else
			{
				/* Alternative token island.
				 * Show the internal word number and its list of alternatives. */
				String * s = string_new();
				const char ** a;
				char * a_list;

				append_string(s, "[%zu", i);
				for (a = sent->word[i].alternatives; *a; a++) {
					/* Don't show an empty word - it is not an island. */
					if (0 == strcmp(*a, EMPTY_WORD_MARK)) continue;
					append_string(s, " %s", *a);
				}
				append_string(s, "]");
				a_list = string_copy(s);
				t = string_set_add(a_list, sent->string_set);
				string_delete(s);
				exfree(a_list, strlen(a_list)+1);
			}
		}
		else
		{
			/* print the subscript, as in "dog.n" as opposed to "dog" */
			t = linkage->chosen_disjuncts[i]->string;
			/* get rid of those ugly ".Ixx" */
			if (is_idiom_word(t)) {
				s = strdup(t);
				u = strrchr(s, SUBSCRIPT_MARK);
				*u = '\0';
				t = string_set_add(s, sent->string_set);
				free(s);
			} else {

				/* Suppress the empty word. */
				if (0 == strcmp(t, EMPTY_WORD_MARK))
				{
					t = NULL;
				}

				/* Concatenate the stem and the suffix together into one word */
				if (t && HIDE_MORPHO)
				{
					if (is_suffix(infix_mark, t) && linkage->chosen_disjuncts[i-1] &&
					    is_stem(linkage->chosen_disjuncts[i-1]->string))
					{
						const char * stem = linkage->chosen_disjuncts[i-1]->string;
						size_t len = strlen(stem) + strlen(t);
						char * join = (char *)malloc(len+1);
						strcpy(join, stem);
						u = strrchr(join, SUBSCRIPT_MARK);

						/* u can be null, if the sentence happens to have
						 * an equals sign in it, for other reasons. */
						if (u)
						{
							*u = '\0';
							strcat(join, t + INFIX_MARK_L);
							t = string_set_add(join, sent->string_set);
						}
						free(join);
					}

					/* Suppress printing of the stem, if the next word is the suffix */
					if (is_stem(t) && (i+1 < sent->length) &&
					    linkage->chosen_disjuncts[i+1])
					{
						const char * next = linkage->chosen_disjuncts[i+1]->string;
						if (is_suffix(infix_mark, next))
						{
							t = NULL;
						}
					}
				}

				/* Convert the badly-printing ^C into a period */
				if (t)
				{
					s = strdup(t);
					u = strrchr(s, SUBSCRIPT_MARK);
					if (u) *u = SUBSCRIPT_DOT;
					t = string_set_add(s, sent->string_set);
					free(s);
				}
			}
		}
		chosen_words[i] = t;
	}

	if (sent->dict->left_wall_defined)
	{
		chosen_words[0] = LEFT_WALL_DISPLAY;
	}
	if (sent->dict->right_wall_defined)
	{
		chosen_words[sent->length-1] = RIGHT_WALL_DISPLAY;
	}

	/* We alloc a little more than needed, but so what... */
	linkage->word = (const char **) exalloc(sent->length*sizeof(char *));

	/* Copy over the chosen words, dropping the empty words. */
	for (i=0, j=0; i<sent->length; ++i)
	{
		if (chosen_words[i])
		{
			linkage->word[j] = chosen_words[i];
			remap[i] = j;
			j++;
		}
	}
	linkage->num_words = j;

	/* Now, discard links to the empty word.  If morphology printing
	 * is being suppressed, then all links connecting morphemes will
	 * be discarded as well.
	 */
	for (i=0, j=0; i<linkage->num_links; i++)
	{
		Link * lnk = &(linkage->link_array[i]);
		if (NULL == chosen_words[lnk->lw] ||
		    NULL == chosen_words[lnk->rw])
		{
			bool sane = (0 == strcmp(lnk->link_name, EMPTY_WORD_SUPPRESS));
			sane = sane || (HIDE_MORPHO &&
			     0 == strncmp(lnk->link_name, SUFFIX_SUPPRESS, SUFFIX_SUPPRESS_L));
			assert(sane, "Something wrong with linkage processing");
		}
		else
		{
			/* Copy the entire link contents, thunking the word numbers.
			 * Note that j is always <= i so this is always safe. */
			linkage->link_array[j].lw = remap[lnk->lw];
			linkage->link_array[j].rw = remap[lnk->rw];
			linkage->link_array[j].lc = linkage->link_array[i].lc;
			linkage->link_array[j].rc = linkage->link_array[i].rc;
			linkage->link_array[j].link_name = linkage->link_array[i].link_name;
			j++;
		}
	}
	linkage->num_links = j;
}

Linkage linkage_create(LinkageIdx k, Sentence sent, Parse_Options opts)
{
	Linkage linkage;

	if (opts->use_sat_solver)
	{
		linkage = sat_create_linkage(k, sent, opts);
		if (!linkage) return NULL;
	}
	else
	{
		/* Cannot create a Linkage for a discarded linkage. */
		if (sent->num_linkages_post_processed <= k) return NULL;
		linkage = &sent->lnkages[k];
	}

	/* Perform remaining initialization we haven't done yet...*/
	compute_chosen_words(sent, linkage, opts);

	/* We've already done core post-processing earlier.
	 * Run the post-processing needed for constituent (hpsg)
	 * printing. */
	linkage_post_process(linkage, sent->constituent_pp, opts);
	linkage->hpsg_pp_data = sent->constituent_pp->pp_data;

	return linkage;
}

void linkage_delete(Linkage linkage)
{
	/* Currently a no-op */
}

size_t linkage_get_num_words(const Linkage linkage)
{
	if (!linkage) return 0;
	return linkage->num_words;
}

size_t linkage_get_num_links(const Linkage linkage)
{
	if (!linkage) return 0;
	return linkage->num_links;
}

static inline bool verify_link_index(const Linkage linkage, LinkIdx index)
{
	if (!linkage) return false;
	if	(index >= linkage->num_links) return false;
	return true;
}

int linkage_get_link_length(const Linkage linkage, LinkIdx index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return -1;
	link = &(linkage->link_array[index]);
	return link->rw - link->lw;
}

WordIdx linkage_get_link_lword(const Linkage linkage, LinkIdx index)
{
	if (!verify_link_index(linkage, index)) return SIZE_MAX;
	return linkage->link_array[index].lw;
}

WordIdx linkage_get_link_rword(const Linkage linkage, LinkIdx index)
{
	if (!verify_link_index(linkage, index)) return SIZE_MAX;
	return linkage->link_array[index].rw;
}

const char * linkage_get_link_label(const Linkage linkage, LinkIdx index)
{
	if (!verify_link_index(linkage, index)) return NULL;
	return linkage->link_array[index].link_name;
}

const char * linkage_get_link_llabel(const Linkage linkage, LinkIdx index)
{
	if (!verify_link_index(linkage, index)) return NULL;
	return linkage->link_array[index].lc->string;
}

const char * linkage_get_link_rlabel(const Linkage linkage, LinkIdx index)
{
	if (!verify_link_index(linkage, index)) return NULL;
	return linkage->link_array[index].rc->string;
}

const char ** linkage_get_words(const Linkage linkage)
{
	return linkage->word;
}

const char * linkage_get_disjunct_str(const Linkage linkage, WordIdx w)
{
	Disjunct *dj;

	if (NULL == linkage) return "";
	if (NULL == linkage->disjunct_list_str)
	{
		lg_compute_disjunct_strings(linkage);
	}

	if (linkage->num_words <= w) return NULL; /* bounds-check */

	/* dj will be null if the word wasn't used in the parse. */
	dj = linkage->chosen_disjuncts[w];
	if (NULL == dj) return "";

	return linkage->disjunct_list_str[w];
}

double linkage_get_disjunct_cost(const Linkage linkage, WordIdx w)
{
	Disjunct *dj;

	if (linkage->num_words <= w) return 0.0; /* bounds-check */

	dj = linkage->chosen_disjuncts[w];

	/* dj may be null, if the word didn't participate in the parse. */
	if (dj) return dj->cost;
	return 0.0;
}

double linkage_get_disjunct_corpus_score(const Linkage linkage, WordIdx w)
{
	Disjunct *dj;

	if (linkage->num_words <= w) return 99.999; /* bounds-check */
	dj = linkage->chosen_disjuncts[w];

	/* dj may be null, if the word didn't participate in the parse. */
	if (NULL == dj) return 99.999;

	return lg_corpus_disjunct_score(linkage, w);
}

const char * linkage_get_word(const Linkage linkage, WordIdx w)
{
	if (!linkage) return NULL;
	if (linkage->num_words <= w) return NULL; /* bounds-check */
	return linkage->word[w];
}

int linkage_unused_word_cost(const Linkage linkage)
{
	/* The sat solver (currently) fails to fill in info */
	if (!linkage) return 0;
	return linkage->lifo.unused_word_cost;
}

double linkage_disjunct_cost(const Linkage linkage)
{
	/* The sat solver (currently) fails to fill in info */
	if (!linkage) return 0.0;
	return linkage->lifo.disjunct_cost;
}

int linkage_link_cost(const Linkage linkage)
{
	/* The sat solver (currently) fails to fill in info */
	if (!linkage) return 0;
	return linkage->lifo.link_cost;
}

double linkage_corpus_cost(const Linkage linkage)
{
	/* The sat solver (currently) fails to fill in info */
	if (!linkage) return 0.0;
	return linkage->lifo.corpus_cost;
}

int linkage_get_link_num_domains(const Linkage linkage, LinkIdx index)
{
	if (!verify_link_index(linkage, index)) return -1;
	return linkage->pp_info[index].num_domains;
}

const char ** linkage_get_link_domain_names(const Linkage linkage, LinkIdx index)
{
	if (!verify_link_index(linkage, index)) return NULL;
	return linkage->pp_info[index].domain_name;
}

const char * linkage_get_violation_name(const Linkage linkage)
{
	return linkage->lifo.pp_violation_msg;
}

static void exfree_domain_names(PP_info *ppi)
{
	if (ppi->num_domains > 0)
		exfree((void *) ppi->domain_name, sizeof(const char *) * ppi->num_domains);
	ppi->domain_name = NULL;
	ppi->num_domains = 0;
}

void linkage_free_pp_info(Linkage lkg)
{
	size_t j;
	if (!lkg || !lkg->pp_info) return;

	for (j = 0; j < lkg->num_links; ++j)
		exfree_domain_names(&lkg->pp_info[j]);
	exfree(lkg->pp_info, sizeof(PP_info) * lkg->num_links);
	lkg->pp_info = NULL;
}

/**
 * Part of the API. Although maybe it shouldn't be.
 * This is just a wrapper around do_post_process, and all that it does
 * is to set up some domain data in the linkage->pp_info pointer.
 * There do not seem to be any users for this data !?  Should this
 * be removed?
 *
 * The domain data is printed, using linkage_get_link_domain_names(),
 * etc. but these are for the hpsg knowledge, and not for the base
 * knowledge, mostly because in linkage_create, this function is called,
 * and the first thing it does is to clobber the base pp_info.  So WTF;
 * I just don't see a lot of utlity or value-add here ... this could be
 * reduced to a plain do_post_process
 */
void linkage_post_process(Linkage linkage, Postprocessor * postprocessor, Parse_Options opts)
{
	PP_node * pp;
	size_t j, k;
	D_type_list * d;
	bool is_long;

	if (NULL == linkage) return;
	if (NULL == postprocessor) return;

	/* Step one: because we might be called multiple times, first we clean
	 * up and free the domain name data left over from a previous call. */
	if (linkage->pp_info != NULL)
	{
		for (j = 0; j < linkage->num_links; ++j)
			exfree_domain_names(&linkage->pp_info[j]);
	}
	else
	{
		linkage->pp_info = (PP_info *) exalloc(sizeof(PP_info) * linkage->num_links);
	}

	post_process_free_data(&postprocessor->pp_data);

	for (j = 0; j < linkage->num_links; ++j)
	{
		linkage->pp_info[j].num_domains = 0;
		linkage->pp_info[j].domain_name = NULL;
	}

	/* Step two: actually do the post-processing */
	is_long = (linkage->num_words >= opts->twopass_length);
	pp = do_post_process(postprocessor, linkage, is_long);

	/* Step three: copy the post-processing results over into the linkage */
	if (pp->violation != NULL)
	{
		linkage->lifo.N_violations ++;
		/* Keep only the earliest violation message */
		if (NULL == linkage->lifo.pp_violation_msg)
			linkage->lifo.pp_violation_msg = pp->violation;
	}
	else
	{
		for (j = 0; j < linkage->num_links; ++j)
		{
			k = 0;
			for (d = pp->d_type_array[j]; d != NULL; d = d->next) k++;
			linkage->pp_info[j].num_domains = k;
			if (k > 0)
			{
				linkage->pp_info[j].domain_name = (const char **) exalloc(sizeof(const char *)*k);
			}
			k = 0;
			for (d = pp->d_type_array[j]; d != NULL; d = d->next)
			{
				char buff[5];
				snprintf(buff, 5, "%c", d->type);
				linkage->pp_info[j].domain_name[k] =
				      string_set_add (buff, postprocessor->string_set);

				k++;
			}
		}
	}
}

