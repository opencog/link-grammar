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
#include "and.h"
#include "api-structures.h"
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

#define SUFFIX_WORD ("=")      /* suffixes start with this */
#define SUFFIX_WORD_L 1        /* length of above */

#define SUFFIX_SUPPRESS ("LL") /* suffix links start with this */
#define SUFFIX_SUPPRESS_L 2    /* length of above */

#define STEM_MARK "="        /* stems end with this. */

#define HIDE_MORPHO   (!display_morphology)

/* FIXME for is_*:
 * - Use INFIX_MARK and STEMSUBSCR, to match the corresponding affix classes.
 * - There are versions of these functions in api.c - unify them.
 */

/**
 * Return TRUE if the word is a suffix.
 *
 * Suffixes have the form =asdf.asdf or possibly just =asdf without
 * the dot (subscript mark). The "null" suffixes have the form
 * =.asdf (always with the ubscript mark, as there are several).
 * Ordinary equals signs appearing in regular text are either = or =[!].
 */
static bool is_suffix(const char* w)
{
	if (0 != strncmp(SUFFIX_WORD, w, SUFFIX_WORD_L)) return false;
	if (1 == strlen(w)) return false;
	if (0 == strcmp("=[!]", w)) return false;
#if SUBSCRIPT_MARK == '.'
	/* Hmmm ... equals signs look like suffixes, but they are not ... */
	if (0 == strcmp("=.v", w)) return false;
	if (0 == strcmp("=.eq", w)) return false;
#endif
	return true;
}

/* Return TRUE if the word seems to be in stem form.
 * Stems have the distinctive 'shape', that the end with the = sign
 * and are preceded by the subscript mark.
 * Examples (. represented the subscript mark): word.= word.=[!]
 */
static bool is_stem(const char* w)
{
	size_t l = strlen(w);
	const char *subscrmark;

	if (l < 3) return false;

	subscrmark = strchr(w, SUBSCRIPT_MARK);
	if (NULL == subscrmark) return false;
	if (0 != strncmp(subscrmark, STEM_MARK, sizeof(STEM_MARK)-1)) return false;

	return true;
}

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
	bool display_word_subscripts = true;

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
		else if (display_word_subscripts) 
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
					if (is_suffix(t) && pi->chosen_disjuncts[i-1] &&
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
							strcat(join, t + SUFFIX_WORD_L);
							t = string_set_add(join, sent->string_set);
						}
						free(join);
					}

					/* Suppress printing of the stem, if the next word is the suffix */
					if (is_stem(t) && (i+1 < sent->length) &&
					    pi->chosen_disjuncts[i+1])
					{
						const char * next = pi->chosen_disjuncts[i+1]->string;
						if (is_suffix(next) && 0 != strcmp(next, EMPTY_WORD_MARK))
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
		else
		{
			/* XXX This is wrong, since it fails to indicate what
			 * was actually used for the parse, which might not actually
			 * be alternative 0.  We should do like the above, and then
			 * manually strip the subscript.
			 * Except that this code is never ever reached, because
			 * display_word_subscripts is always true...
			 */
			t = sent->word[i].alternatives[0];
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
#ifdef USE_FAT_LINKAGES
	for (i=0, j=0; i<linkage->sublinkage[linkage->current].num_links; i++)
#else
	for (i=0, j=0; i<linkage->sublinkage.num_links; i++)
#endif
	{
#ifdef USE_FAT_LINKAGES
		Link * lnk = linkage->sublinkage[linkage->current].link[i];
#else
		Link * lnk = linkage->sublinkage.link[i];
#endif
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
#ifdef USE_FAT_LINKAGES
			linkage->sublinkage[linkage->current].link[j] = lnk;
#else
			linkage->sublinkage.link[j] = lnk;
#endif
			j++;
		}
	}
#ifdef USE_FAT_LINKAGES
	linkage->sublinkage[linkage->current].num_links = j;
#else
	linkage->sublinkage.num_links = j;
#endif
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
#ifdef USE_FAT_LINKAGES
	linkage->sublinkage = NULL;
	linkage->unionized = false;
	linkage->current = 0;
	linkage->num_sublinkages = 0;
	linkage->dis_con_tree = NULL;

	if (set_has_fat_down(sent))
	{
		extract_fat_linkage(sent, opts, linkage);
	}
	else
#endif /* USE_FAT_LINKAGES */
	{
		extract_thin_linkage(sent, opts, linkage);
	}

	compute_chosen_words(sent, linkage);

	if (sent->dict->postprocessor != NULL)
	{
		linkage_post_process(linkage, sent->dict->postprocessor);
	}

	return linkage;
}

int linkage_get_current_sublinkage(const Linkage linkage)
{
#ifdef USE_FAT_LINKAGES
	return linkage->current;
#else
	return 0;
#endif /* USE_FAT_LINKAGES */
}

bool linkage_set_current_sublinkage(Linkage linkage, LinkageIdx index)
{
#ifdef USE_FAT_LINKAGES
	if (index >= linkage->num_sublinkages)
	{
		return false;
	}
	linkage->current = index;
#endif /* USE_FAT_LINKAGES */
	return true;
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
#ifdef USE_FAT_LINKAGES
	size_t i;
#endif /* USE_FAT_LINKAGES */
	size_t j;

	/* Can happen on panic timeout or user error */
	if (NULL == linkage) return;

	exfree((void *) linkage->word, sizeof(const char *) * linkage->num_words);

#ifdef USE_FAT_LINKAGES
	for (i = 0; i < linkage->num_sublinkages; ++i)
#endif /* USE_FAT_LINKAGES */
	{
#ifdef USE_FAT_LINKAGES
		Sublinkage *s = &(linkage->sublinkage[i]);
#else
		Sublinkage *s = &(linkage->sublinkage);
#endif /* USE_FAT_LINKAGES */
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
	}
#ifdef USE_FAT_LINKAGES
	exfree(linkage->sublinkage, sizeof(Sublinkage)*linkage->num_sublinkages);
#endif /* USE_FAT_LINKAGES */

#ifdef USE_FAT_LINKAGES
	if (linkage->dis_con_tree)
		free_DIS_tree(linkage->dis_con_tree);
#endif /* USE_FAT_LINKAGES */
	exfree(linkage, sizeof(struct Linkage_s));
}

#ifdef USE_FAT_LINKAGES
static bool links_are_equal(Link *l, Link *m)
{
	return ((l->lw == m->lw) && (l->rw == m->rw) && (strcmp(l->link_name, m->link_name)==0));
}

static int link_already_appears(Linkage linkage, Link *link, int a)
{
	int i, j;

	for (i=0; i<a; ++i) {
		for (j=0; j<linkage->sublinkage[i].num_links; ++j) {
			if (links_are_equal(linkage->sublinkage[i].link[j], link)) return true;
		}
	}
	return false;
}

static PP_info excopy_pp_info(PP_info ppi)
{
	PP_info newppi;
	int i;

	newppi.num_domains = ppi.num_domains;
	newppi.domain_name = (const char **) exalloc(sizeof(const char *)*ppi.num_domains);
	for (i=0; i<newppi.num_domains; ++i)
	{
		newppi.domain_name[i] = ppi.domain_name[i];
	}
	return newppi;
}

static Sublinkage unionize_linkage(Linkage linkage)
{
	int i, j, num_in_union=0;
	Sublinkage u;
	Link *link;
	const char *p;

	for (i=0; i<linkage->num_sublinkages; ++i) {
		for (j=0; j<linkage->sublinkage[i].num_links; ++j) {
			link = linkage->sublinkage[i].link[j];
			if (!link_already_appears(linkage, link, i)) num_in_union++;
		}
	}

	u.link = (Link **) exalloc(sizeof(Link *)*num_in_union);
	u.num_links = num_in_union;
	zero_sublinkage(&u);

	u.pp_info = (PP_info *) exalloc(sizeof(PP_info)*num_in_union);
	u.violation = NULL;
	u.num_links = num_in_union;

	num_in_union = 0;

	for (i=0; i<linkage->num_sublinkages; ++i) {
		for (j=0; j<linkage->sublinkage[i].num_links; ++j) {
			link = linkage->sublinkage[i].link[j];
			if (!link_already_appears(linkage, link, i)) {
				u.link[num_in_union] = excopy_link(link);
				u.pp_info[num_in_union] = excopy_pp_info(linkage->sublinkage[i].pp_info[j]);
				if (((p=linkage->sublinkage[i].violation) != NULL) &&
					(u.violation == NULL)) {
					char *s = (char *) exalloc((strlen(p)+1)*sizeof(char));
					strcpy(s, p);
					u.violation = s;
				}
				num_in_union++;
			}
		}
	}

	return u;
}
#endif /* USE_FAT_LINKAGES */

int linkage_compute_union(Linkage linkage)
{
#ifdef USE_FAT_LINKAGES
	int i, num_subs=linkage->num_sublinkages;
	Sublinkage * new_sublinkage, *s;

	if (linkage->unionized) {
		linkage->current = linkage->num_sublinkages-1;
		return 0;
	}
	if (num_subs == 1) {
		linkage->unionized = true;
		return 1;
	}

	new_sublinkage =
		(Sublinkage *) exalloc(sizeof(Sublinkage)*(num_subs+1));

	for (i=0; i<num_subs; ++i) {
		new_sublinkage[i] = linkage->sublinkage[i];
	}
	exfree(linkage->sublinkage, sizeof(Sublinkage)*num_subs);
	linkage->sublinkage = new_sublinkage;

	/* Zero out the new sublinkage, then unionize it. */
	s = &new_sublinkage[num_subs];
	s->link = NULL;
	s->num_links = 0;
	zero_sublinkage(s);
	linkage->sublinkage[num_subs] = unionize_linkage(linkage);

	linkage->num_sublinkages++;

	linkage->unionized = true;
	linkage->current = linkage->num_sublinkages-1;
	return 1;
#else
	return 0;
#endif /* USE_FAT_LINKAGES */
}

int linkage_get_num_sublinkages(const Linkage linkage)
{
	if (!linkage) return 0;
#ifdef USE_FAT_LINKAGES
	return linkage->num_sublinkages;
#else
	return 1;
#endif /* USE_FAT_LINKAGES */
}

size_t linkage_get_num_words(const Linkage linkage)
{
	if (!linkage) return 0;
	return linkage->num_words;
}

size_t linkage_get_num_links(const Linkage linkage)
{
#ifdef USE_FAT_LINKAGES
	int current;
	if (!linkage) return 0;
	current = linkage->current;
	return linkage->sublinkage[current].num_links;
#else
	if (!linkage) return 0;
	return linkage->sublinkage.num_links;
#endif /* USE_FAT_LINKAGES */
}

static inline bool verify_link_index(const Linkage linkage, LinkIdx index)
{
	if (!linkage) return false;
#ifdef USE_FAT_LINKAGES
	if	(index >= linkage->sublinkage[linkage->current].num_links)
#else
	if	(index >= linkage->sublinkage.num_links)
#endif /* USE_FAT_LINKAGES */
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
#ifdef USE_FAT_LINKAGES
	int current = linkage->current;
#endif /* USE_FAT_LINKAGES */

	if (!verify_link_index(linkage, index)) return -1;

	for (i=0; i<linkage->num_words+1; ++i) {
		word_has_link[i] = false;
	}

#ifdef USE_FAT_LINKAGES
	for (i=0; i<linkage->sublinkage[current].num_links; ++i)
	{
		link = linkage->sublinkage[current].link[i];
		word_has_link[link->lw] = true;
		word_has_link[link->rw] = true;
	}
	link = linkage->sublinkage[current].link[index];
#else
	for (i=0; i<linkage->sublinkage.num_links; ++i)
	{
		link = linkage->sublinkage.link[i];
		word_has_link[link->lw] = true;
		word_has_link[link->rw] = true;
	}
	link = linkage->sublinkage.link[index];
#endif /* USE_FAT_LINKAGES */

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
#ifdef USE_FAT_LINKAGES
	link = linkage->sublinkage[linkage->current].link[index];
#else
	link = linkage->sublinkage.link[index];
#endif /* USE_FAT_LINKAGES */
	return link->lw;
}

WordIdx linkage_get_link_rword(const Linkage linkage, LinkIdx index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return SIZE_MAX;
#ifdef USE_FAT_LINKAGES
	link = linkage->sublinkage[linkage->current].link[index];
#else
	link = linkage->sublinkage.link[index];
#endif /* USE_FAT_LINKAGES */
	return link->rw;
}

const char * linkage_get_link_label(const Linkage linkage, LinkIdx index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return NULL;
#ifdef USE_FAT_LINKAGES
	link = linkage->sublinkage[linkage->current].link[index];
#else
	link = linkage->sublinkage.link[index];
#endif /* USE_FAT_LINKAGES */
	return link->link_name;
}

const char * linkage_get_link_llabel(const Linkage linkage, LinkIdx index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return NULL;
#ifdef USE_FAT_LINKAGES
	link = linkage->sublinkage[linkage->current].link[index];
#else
	link = linkage->sublinkage.link[index];
#endif /* USE_FAT_LINKAGES */
	return link->lc->string;
}

const char * linkage_get_link_rlabel(const Linkage linkage, LinkIdx index)
{
	Link *link;
	if (!verify_link_index(linkage, index)) return NULL;
#ifdef USE_FAT_LINKAGES
	link = linkage->sublinkage[linkage->current].link[index];
#else
	link = linkage->sublinkage.link[index];
#endif /* USE_FAT_LINKAGES */
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

int linkage_is_fat(const Linkage linkage)
{
#ifdef USE_FAT_LINKAGES
	/* The sat solver (currently) fails to fill in info */
	if (!linkage->info) return 0;
	return linkage->info->fat;
#else
	return false;
#endif /* USE_FAT_LINKAGES */
}

int linkage_and_cost(const Linkage linkage)
{
#ifdef USE_FAT_LINKAGES
	/* The sat solver (currently) fails to fill in info */
	if (!linkage->info) return 0;
	return linkage->info->and_cost;
#else
	return 0;
#endif /* USE_FAT_LINKAGES */
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
#ifdef USE_FAT_LINKAGES
	pp_info = &linkage->sublinkage[linkage->current].pp_info[index];
#else
	pp_info = &linkage->sublinkage.pp_info[index];
#endif /* USE_FAT_LINKAGES */
	return pp_info->num_domains;
}

const char ** linkage_get_link_domain_names(const Linkage linkage, LinkIdx index)
{
	PP_info *pp_info;
	if (!verify_link_index(linkage, index)) return NULL;
#ifdef USE_FAT_LINKAGES
	pp_info = &linkage->sublinkage[linkage->current].pp_info[index];
#else
	pp_info = &linkage->sublinkage.pp_info[index];
#endif /* USE_FAT_LINKAGES */
	return pp_info->domain_name;
}

const char * linkage_get_violation_name(const Linkage linkage)
{
#ifdef USE_FAT_LINKAGES
	return linkage->sublinkage[linkage->current].violation;
#else
	return linkage->sublinkage.violation;
#endif /* USE_FAT_LINKAGES */
}

int linkage_is_canonical(const Linkage linkage)
{
#ifdef USE_FAT_LINKAGES
	/* The sat solver (currently) fails to fill in info */
	if (!linkage->info) return true;
	return linkage->info->canonical;
#else
	return true;
#endif /* USE_FAT_LINKAGES */
}

int linkage_is_improper(const Linkage linkage)
{
#ifdef USE_FAT_LINKAGES
	/* The sat solver (currently) fails to fill in info */
	if (!linkage->info) return false;
	return linkage->info->improper_fat_linkage;
#else
	return false;
#endif /* USE_FAT_LINKAGES */
}

int linkage_has_inconsistent_domains(const Linkage linkage)
{
#ifdef USE_FAT_LINKAGES
	/* The sat solver (currently) fails to fill in info */
	if (!linkage->info) return false;
	return linkage->info->inconsistent_domains;
#else
	return false;
#endif /* USE_FAT_LINKAGES */
}

void linkage_post_process(Linkage linkage, Postprocessor * postprocessor)
{
#ifdef USE_FAT_LINKAGES
	int N_sublinkages = linkage_get_num_sublinkages(linkage);
#endif /* USE_FAT_LINKAGES */
	Parse_Options opts = linkage->opts;
	Sentence sent = linkage->sent;
	Sublinkage * subl;
	PP_node * pp;
	size_t j, k;
	D_type_list * d;

#ifdef USE_FAT_LINKAGES
	int i;
	for (i = 0; i < N_sublinkages; ++i)
#endif /* USE_FAT_LINKAGES */
	{
#ifdef USE_FAT_LINKAGES
		subl = &linkage->sublinkage[i];
#else
		subl = &linkage->sublinkage;
#endif /* USE_FAT_LINKAGES */
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

#ifdef USE_FAT_LINKAGES
		if (linkage->info->improper_fat_linkage)
		{
			pp = NULL;
		}
		else
#endif /* USE_FAT_LINKAGES */
		{
			pp = do_post_process(postprocessor, opts, sent, subl, false);
			/* This can return NULL, for example if there is no
			   post-processor */
		}

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
	}
	post_process_close_sentence(postprocessor);
}

