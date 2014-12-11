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
void compute_chosen_words(Sentence sent, Linkage linkage)
{
	size_t i, j, l;
	char * s, *u;
	Parse_info pi = sent->parse_info;
	const char * chosen_words[sent->length];
	size_t remap[sent->length];
	Parse_Options opts = linkage->opts;
	bool display_morphology = opts->display_morphology;
	const Dictionary afdict = sent->dict->affix_table; /* for INFIX_MARK only */
	const char infix_mark = INFIX_MARK;

	for (i=0; i<sent->length; i++)
	{
		const char *t;
		/* If chosen_disjuncts is NULL, then this is an 'island' word
		 * that has not been linked to. */
		if (pi->chosen_disjuncts[i] == NULL)
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
				sprintf(s, "[%s]", t);
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
			t = pi->chosen_disjuncts[i]->string;
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
					if (is_suffix(infix_mark, t) && pi->chosen_disjuncts[i-1] &&
					    is_stem(pi->chosen_disjuncts[i-1]->string))
					{
						const char * stem = pi->chosen_disjuncts[i-1]->string;
						size_t len = strlen(stem) + strlen(t);
						char * join = (char *)malloc(len+1);
						strcpy(join, stem);
						u = strrchr(join, SUBSCRIPT_MARK);

						/* u can be null, if the the sentence happens to have
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
					    pi->chosen_disjuncts[i+1])
					{
						const char * next = pi->chosen_disjuncts[i+1]->string;
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

	/* At this time, we expect both the sent length and the linkage
	 * length to be identical.  In the future, this may change ...
	 */
	assert (sent->length == linkage->num_words, "Unexpected linkage length");

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
	for (i=0, j=0; i<linkage->subly.num_links; i++)
	{
		Link * lnk = linkage->subly.link[i];
		if (NULL == chosen_words[lnk->lw] ||
		    NULL == chosen_words[lnk->rw])
		{
			bool sane = (0 == strcmp(lnk->link_name, EMPTY_WORD_SUPPRESS));
			sane = sane || (HIDE_MORPHO &&
			     0 == strncmp(lnk->link_name, SUFFIX_SUPPRESS, SUFFIX_SUPPRESS_L));
			assert(sane, "Something wrong with linkage processing");
			exfree_link(lnk);
		}
		else
		{
			lnk->lw = remap[lnk->lw];
			lnk->rw = remap[lnk->rw];
			linkage->subly.link[j] = lnk;
			j++;
		}
	}
	linkage->subly.num_links = j;
}


Linkage linkage_create(LinkageIdx k, Sentence sent, Parse_Options opts)
{
	Linkage linkage;

	if (opts->use_sat_solver)
	{
		return sat_create_linkage(k, sent, opts);
	}

	if (k >= sent->num_linkages_post_processed) return NULL;

	/* bounds check: link_info array is this size */
	if (sent->num_linkages_alloced <= k) return NULL;
	extract_links(sent->link_info[k].index, sent->parse_info);

	/* Using exalloc since this is external to the parser itself. */
	linkage = (Linkage) exalloc(sizeof(struct Linkage_s));

	linkage->num_words = sent->length;
	linkage->word = (const char **) exalloc(linkage->num_words*sizeof(char *));
	linkage->sent = sent;
	linkage->opts = opts;
	linkage->info = &sent->link_info[k];
	extract_thin_linkage(sent, opts, linkage);

	compute_chosen_words(sent, linkage);

	if (sent->dict->postprocessor != NULL)
	{
		linkage_post_process(linkage, sent->dict->postprocessor);
	}

	return linkage;
}

static void exfree_pp_info(PP_info *ppi)
{
	if (ppi->num_domains > 0)
		exfree((void *) ppi->domain_name, sizeof(const char *) * ppi->num_domains);
	ppi->domain_name = NULL;
	ppi->num_domains = 0;
}

void linkage_delete(Linkage linkage)
{
	size_t j;
	Sublinkage *s;

	/* Can happen on panic timeout or user error */
	if (NULL == linkage) return;

	exfree((void *) linkage->word, sizeof(const char *) * linkage->num_words);

	s = &(linkage->subly);
	for (j = 0; j < s->num_links; ++j) {
		exfree_link(s->link[j]);
	}
	exfree(s->link, sizeof(Link*) * s->num_links);
	if (s->pp_info != NULL) {
		for (j = 0; j < s->num_links; ++j) {
			exfree_pp_info(&s->pp_info[j]);
		}
		exfree(s->pp_info, sizeof(PP_info) * s->num_links);
		s->pp_info = NULL;
		post_process_free_data(&s->pp_data);
	}
	if (s->violation != NULL) {
		exfree((void *) s->violation, sizeof(char) * (strlen(s->violation)+1));
	}
	exfree(linkage, sizeof(struct Linkage_s));
}

size_t linkage_get_num_words(const Linkage linkage)
{
	if (!linkage) return 0;
	return linkage->num_words;
}

size_t linkage_get_num_links(const Linkage linkage)
{
	if (!linkage) return 0;
	return linkage->subly.num_links;
}

static inline bool verify_link_index(const Linkage linkage, LinkIdx index)
{
	if (!linkage) return false;
	if	(index >= linkage->subly.num_links)
	{
		return false;
	}
	return true;
}

int linkage_get_link_length(const Linkage linkage, LinkIdx index)
{
	Link *link;
	bool word_has_link[linkage->num_words+1];
	size_t i, length;

	if (!verify_link_index(linkage, index)) return -1;

	for (i=0; i<linkage->num_words+1; ++i) {
		word_has_link[i] = false;
	}

	for (i=0; i<linkage->subly.num_links; ++i)
	{
		link = linkage->subly.link[i];
		word_has_link[link->lw] = true;
		word_has_link[link->rw] = true;
	}
	link = linkage->subly.link[index];

	length = link->rw - link->lw;
	for (i= link->lw+1; i < link->rw; ++i) {
		if (!word_has_link[i]) length--;
	}
	return length;
}

WordIdx linkage_get_link_lword(const Linkage linkage, LinkIdx index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return SIZE_MAX;
	link = linkage->subly.link[index];
	return link->lw;
}

WordIdx linkage_get_link_rword(const Linkage linkage, LinkIdx index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return SIZE_MAX;
	link = linkage->subly.link[index];
	return link->rw;
}

const char * linkage_get_link_label(const Linkage linkage, LinkIdx index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return NULL;
	link = linkage->subly.link[index];
	return link->link_name;
}

const char * linkage_get_link_llabel(const Linkage linkage, LinkIdx index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return NULL;
	link = linkage->subly.link[index];
	return link->lc->string;
}

const char * linkage_get_link_rlabel(const Linkage linkage, LinkIdx index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return NULL;
	link = linkage->subly.link[index];
	return link->rc->string;
}

const char ** linkage_get_words(const Linkage linkage)
{
	return linkage->word;
}

Sentence linkage_get_sentence(const Linkage linkage)
{
	return linkage->sent;
}

const char * linkage_get_disjunct_str(const Linkage linkage, WordIdx w)
{
	Disjunct *dj;

	if (NULL == linkage) return "";
	if (NULL == linkage->info->disjunct_list_str)
	{
		lg_compute_disjunct_strings(linkage->sent, linkage->info);
	}

	/* XXX FIXME in the future, linkage->num_words might not match sent->length */
	if (linkage->num_words <= w) return NULL; /* bounds-check */

	/* dj will be null if the word wasn't used in the parse. */
	dj = linkage->sent->parse_info->chosen_disjuncts[w];
	if (NULL == dj) return "";

	return linkage->info->disjunct_list_str[w];
}

double linkage_get_disjunct_cost(const Linkage linkage, WordIdx w)
{
	Disjunct *dj;
	/* XXX FIXME in the future, linkage->num_words might not match sent->length */
	if (linkage->num_words <= w) return 0.0; /* bounds-check */

	dj = linkage->sent->parse_info->chosen_disjuncts[w];

	/* dj may be null, if the word didn't participate in the parse. */
	if (dj) return dj->cost;
	return 0.0;
}

double linkage_get_disjunct_corpus_score(const Linkage linkage, WordIdx w)
{
	Disjunct *dj;

	/* XXX FIXME in the future, linkage->num_words might not match sent->length */
	if (linkage->num_words <= w) return 99.999; /* bounds-check */
	dj = linkage->sent->parse_info->chosen_disjuncts[w];

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
	if (!linkage->info) return 0;
	return linkage->info->unused_word_cost;
}

double linkage_disjunct_cost(const Linkage linkage)
{
	/* The sat solver (currently) fails to fill in info */
	if (!linkage->info) return 0.0;
	return linkage->info->disjunct_cost;
}

int linkage_link_cost(const Linkage linkage)
{
	/* The sat solver (currently) fails to fill in info */
	if (!linkage->info) return 0;
	return linkage->info->link_cost;
}

double linkage_corpus_cost(const Linkage linkage)
{
	/* The sat solver (currently) fails to fill in info */
	if (!linkage->info) return 0.0;
	return linkage->info->corpus_cost;
}

int linkage_get_link_num_domains(const Linkage linkage, LinkIdx index)
{
	PP_info *pp_info;
	if (!verify_link_index(linkage, index)) return -1;
	pp_info = &linkage->subly.pp_info[index];
	return pp_info->num_domains;
}

const char ** linkage_get_link_domain_names(const Linkage linkage, LinkIdx index)
{
	PP_info *pp_info;
	if (!verify_link_index(linkage, index)) return NULL;
	pp_info = &linkage->subly.pp_info[index];
	return pp_info->domain_name;
}

const char * linkage_get_violation_name(const Linkage linkage)
{
	return linkage->subly.violation;
}

void linkage_post_process(Linkage linkage, Postprocessor * postprocessor)
{
	Parse_Options opts = linkage->opts;
	Sentence sent = linkage->sent;
	Sublinkage * subl;
	PP_node * pp;
	size_t j, k;
	D_type_list * d;

	subl = &linkage->subly;
	if (subl->pp_info != NULL)
	{
		for (j = 0; j < subl->num_links; ++j)
		{
			exfree_pp_info(&subl->pp_info[j]);
		}
		post_process_free_data(&subl->pp_data);
		exfree(subl->pp_info, sizeof(PP_info) * subl->num_links);
	}
	subl->pp_info = (PP_info *) exalloc(sizeof(PP_info) * subl->num_links);
	for (j = 0; j < subl->num_links; ++j)
	{
		subl->pp_info[j].num_domains = 0;
		subl->pp_info[j].domain_name = NULL;
	}
	if (subl->violation != NULL)
	{
		exfree((void *)subl->violation, sizeof(char) * (strlen(subl->violation)+1));
		subl->violation = NULL;
	}

	/* This can return NULL, for example if there is no
	   post-processor */
	pp = do_post_process(postprocessor, opts, sent, subl, false);
	if (pp == NULL)
	{
		for (j = 0; j < subl->num_links; ++j)
		{
			subl->pp_info[j].num_domains = 0;
			subl->pp_info[j].domain_name = NULL;
		}
	}
	else
	{
		for (j = 0; j < subl->num_links; ++j)
		{
			k = 0;
			for (d = pp->d_type_array[j]; d != NULL; d = d->next) k++;
			subl->pp_info[j].num_domains = k;
			if (k > 0)
			{
				subl->pp_info[j].domain_name = (const char **) exalloc(sizeof(const char *)*k);
			}
			k = 0;
			for (d = pp->d_type_array[j]; d != NULL; d = d->next)
			{
				char buff[5];
				sprintf(buff, "%c", d->type);
				subl->pp_info[j].domain_name[k] = string_set_add (buff, sent->string_set);

				k++;
			}
		}
		subl->pp_data = postprocessor->pp_data;
		if (pp->violation != NULL)
		{
			char * s = (char *) exalloc(sizeof(char)*(strlen(pp->violation)+1));
			strcpy(s, pp->violation);
			subl->violation = s;
		}
	}
	post_process_close_sentence(postprocessor);
}

