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
#include "externs.h"
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
#include "wordgraph.h"
#include "word-utils.h"

#define INFIX_MARK_L 1 /* INFIX_MARK is 1 character */
#define STEM_MARK_L  1 /* stem mark is 1 character */

/**
 * Append an unmarked (i.e. without INFIXMARK) morpheme to join_buff.
 * join_buff is a zeroed-out buffer which has enough room for morpheme to be
 * added + terminating NUL.
 * In case INFIXMARK is not defined in the affix file, it is '\0'. However,
 * in that case are no MT_PREFIX, MT_SUFFIX and MT_MIDDLE morpheme types.
 *
 * FIXME Combining contracted words is not handled yet, because combining
 * morphemes which have non-LL links to other words is not yet implemented.
 */
static void add_morpheme_unmarked(char *join_buff, const char *wm,
                                  Morpheme_type mt)
{
	const char *sm =  strrchr(wm, SUBSCRIPT_MARK);

	if (NULL == sm) sm = (char *)wm + strlen(wm);

	if (MT_PREFIX == mt)
		strncat(join_buff, wm, sm-wm-INFIX_MARK_L);
	else if (MT_SUFFIX == mt)
		strncat(join_buff, INFIX_MARK_L+wm, sm-wm-INFIX_MARK_L);
	else if (MT_MIDDLE == mt)
		strncat(join_buff, INFIX_MARK_L+wm, sm-wm-2*INFIX_MARK_L);
	else
		strncat(join_buff, wm, sm-wm);
}

static const char *join_null_word(Sentence sent, Gword **wgp, size_t count)
{
	size_t i;
	char *join_buff;
	const char *s;
	size_t join_len = 0;

	for (i = 0; i < count; i++)
		join_len += strlen(wgp[i]->subword);

	join_buff = alloca(join_len+1);
	memset(join_buff, '\0', join_len+1);

	for (i = 0; i < count; i++)
		add_morpheme_unmarked(join_buff, wgp[i]->subword, wgp[i]->morpheme_type);

	s = string_set_add(join_buff, sent->string_set);

	return s;
}

/**
 * Add a null word node that represents two or more null morphemes.
 * Used for "unifying" null morphemes that are part of a single subword,
 * when only some of its morphemes (2 or more) don't have a linkage.
 * The words "start" to "end" (including) are unified by the new node.
 * XXX Experimental.
 */
static Gword *wordgraph_null_join(Sentence sent, Gword **start, Gword **end)
{
	Gword *new_word;
	Gword **w;
	char *usubword;
	size_t join_len = 0;

	for (w = start; w <= end; w++) join_len += strlen((*w)->subword);
	usubword = calloc(join_len+1, 1); /* zeroed out */

	for (w = start; w <= end; w++)
		add_morpheme_unmarked(usubword, (*w)->subword, (*w)->morpheme_type);

	new_word = gword_new(sent, usubword);
	free(usubword);
	new_word->status |= WS_PL;
	new_word->label = "NJ";
	new_word->null_subwords = NULL;

	/* Link the null_subwords links of the added unifying node to the null
	 * subwords it unified. */
	for (w = start; w <= end; w++)
		gwordlist_append(&new_word->null_subwords, (Gword *)(*w));
	/* Removing const qualifier, but gwordlist_append doesn't change w->... .  */

	return new_word;
}

/**
 * The functions defined in this file are primarily a part of the user API
 * for working with linkages.
 */

/*
 * The "empty word" is a concept used in order to make the current parser able
 * to parse "alternatives" within a sentence. The "empty word" can link to any
 * word, hence it is issued when a word is optional, a thing that happens when
 * there are alternatives with different number of tokens in each of them.
 *
 * However, the empty words are not needed after the linkage step, and so,
 * below, we remove them from the linkage, as well as the links that
 * connect to them.
 *
 * The empty word device must be defined in the dictionary of every language for
 * which alternatives can be generated. This may happen in case of morpheme and
 * contraction splitting, spell ->regex_names, and unit-strip.
 * For more information, see EMPTY_WORD.zzz in the dict file.
 */
#define EMPTY_WORD_SUPPRESS ("ZZZ") /* link to pure whitespace */

#define SUBSCRIPT_SEP SUBSCRIPT_DOT /* multiple-subscript separator */

#define SUFFIX_SUPPRESS ("LL") /* suffix links start with this */
#define SUFFIX_SUPPRESS_L 2    /* length of above */

#define HIDE_MORPHO   (!display_morphology)
/* TODO? !display_guess_marks is not implemented. */
#define DISPLAY_GUESS_MARKS true // (opts->display_guess_marks)

/**
 * This takes the Wordgraph path array and uses it to
 * compute the chosen_words array.  "I.xx" suffixes are eliminated.
 *
 * chosen_words
 *    A pointer to an array of pointers to strings.  These are the words to be
 *    displayed when printing the solution, the links, etc.  Computed as a
 *    function of chosen_disjuncts[] by compute_chosen_words().  This differs
 *    from sentence.word[].alternatives because it contains the subscripts.  It
 *    differs from chosen_disjunct[].string in that the idiom symbols have been
 *    removed.  Furthermore, several chosen_disjuncts[].string elements may be
 *    combined into one chosen_words[] element if opts->display_morphology==0 or
 *    that they where linkage null-words that are morphemes of the same original
 *    word (i.e. subwords of an unsplit_word which are marked as morphemes).
 *
 * wg_path
 *    A pointer to a NULL-terminated array of pointers to Wordgraph words.
 *    It corresponds 1-1 to the chosen_disjuncts array in Linkage structure.
 *    A new one is constructed below to correspond 1-1 to chosen_words.
 *
 *    FIXME Sometimes the word strings are taken from chosen_disjuncts,
 *    and sometimes from wordgraph subwords.
 */
#define D_CCW 5
void compute_chosen_words(Sentence sent, Linkage linkage, Parse_Options opts)
{
	WordIdx i;   /* index of chosen_words */
	WordIdx j;
	Disjunct **cdjp = linkage->chosen_disjuncts;
	const char **chosen_words = alloca(linkage->num_words * sizeof(*chosen_words));
	size_t *remap = alloca(linkage->num_words * sizeof(*remap));
	bool display_morphology = opts->display_morphology;

	Gword **lwg_path = linkage->wg_path;
	Gword **n_lwg_path = NULL; /* new Wordgraph path, to match chosen_words */

#if 0 /* FIXME? Not implemented. */
	size_t len_n_lwg_path = 0;
	/* For opts->display_morphology==0: Mapping of the chosen_words indexing to
	 * original parse indexing. */
	size_t *wg_path_display = alloca(linkage->num_words * sizeof(*lwg_path_display));
#endif

	Gword **nullblock_start = NULL; /* start of a null block, to be put in [] */
	size_t nbsize = 0;              /* number of word in a null block */
	Gword *unsplit_word = NULL;

	if (D_CCW <= opts->verbosity)
		print_lwg_path(lwg_path);

	for (i = 0; i < linkage->num_words; i++)
	{
		Disjunct *cdj = cdjp[i];
		Gword *w;           /* current word */
		const Gword *nw;    /* next word (NULL if none) */
		Gword **wgp;        /* wordgraph_path traversing pointer */

		const char *t;      /* current word string */
		bool nb_end;        /* current word is at end of a nullblock */
		bool join_alt = false; /* morpheme-join this alternative */
		char *s;
		size_t l;
		size_t m;

		lgdebug(D_CCW, "Loop start, word%zu: cdj %s, path %s\n",
				  i, cdj ? cdj->string : "NULL",
				  lwg_path[i] ? lwg_path[i]->subword : "NULL");

		w = lwg_path[i];
		nw = lwg_path[i+1];
		wgp = &lwg_path[i];
		unsplit_word = w->unsplit_word;

		/* FIXME If the original word was capitalized in a capitalizable
		 * position, the displayed null word may be its downcase version. */

		if (NULL == cdj) /* a null word (the chosen disjunct was NULL) */
		{
			nbsize++;
			if (NULL == nullblock_start) /* it starts a new null block */
				nullblock_start = wgp;

			nb_end = (NULL == nw) || (nw->unsplit_word != unsplit_word) ||
				(MT_INFRASTRUCTURE == w->unsplit_word->morpheme_type);

			/* Accumulate null words in this alternative */
			if (!nb_end && (NULL == cdjp[i+1]))
			{
				lgdebug(D_CCW, "Skipping word%zu cdjp=NULL#%zu, path %s\n",
				         i, nbsize, lwg_path[i]->subword);
				chosen_words[i] = NULL;
				continue;
			}

			if (NULL != nullblock_start)
			{
				/* If we are here, this null word is an end of a null block */
				lgdebug(+D_CCW, "Handling %zu null words at %zu: ", nbsize, i);

				if (1 == nbsize)
				{
					/* Case 1: A single null subword. */
					lgdebug(D_CCW, "A single null subword.\n");
					t = join_null_word(sent, wgp, nbsize);

					gwordlist_append(&n_lwg_path, w);
#if 0 /* Not implemented */
					lwg_path_display[len_n_lwg_path++] = i;
#endif
				}
				else
				{
					lgdebug(D_CCW, "Combining null subwords");
					/* Use alternative_id to check for start of alternative. */
					if (((*nullblock_start)->alternative_id == *nullblock_start)
								&& nb_end)
					{
						/* Case 2: A null unsplit_word (all-nulls alternative).*/
						lgdebug(D_CCW, " (null alternative)\n");
						t = unsplit_word->subword;

						gwordlist_append(&n_lwg_path, unsplit_word);
#if 0 /* Not implemented */
						for (j = 0; j < nbsize; j++)
							lwg_path_display[len_n_lwg_path++] = i-j+1;
#endif
					}
					else
					{
						/* Case 3: Join together >=2 null morphemes. */
						Gword *wgnull;

						lgdebug(D_CCW, " (null partial word)\n");
						wgnull = wordgraph_null_join(sent, wgp-nbsize+1, wgp);
						gwordlist_append(&n_lwg_path, wgnull);
						t = wgnull->subword;
					}
				}

				nullblock_start = NULL;
				nbsize = 0;

				/* Put brackets around the null word. */
				l = strlen(t) + 2;
				s = (char *) alloca(l+1);
				s[0] = '[';
				strcpy(&s[1], t);
				s[l-1] = ']';
				s[l] = '\0';
				t = string_set_add(s, sent->string_set);
				lgdebug(D_CCW, " %s\n", t);
			}
		}
		else
		{
			/* This word has a linkage. */

			/* FIXME If the original word was capitalized in a capitalizable
			 * position, the displayed word may be its downcase version. */

			/* TODO: Suppress "virtual-morphemes", currently the dictcap ones. */
			char *sm;

			/* XXX FIXME: the assert below sometimes crashes, because
			 * I guess cdj->word[0] is a null pointer, or something like
			 * that.  Not sure, its hard to reproduce.  */
			/* assert(MT_EMPTY != cdj->word[0]->morpheme_type); already discarded */

			t = cdj->string;
			/* Print the subscript, as in "dog.n" as opposed to "dog". */

			if (0)
			{
				/* TODO */
			}
			else
			{
				/* Get rid of those ugly ".Ixx" */
				if (is_idiom_word(t))
				{
					s = strdup(t);
					sm = strrchr(s, SUBSCRIPT_MARK);
					*sm = '\0';
					t = string_set_add(s, sent->string_set);
					free(s);
				}
				else if (HIDE_MORPHO)
				{
					/* Concatenate the word morphemes together into one word.
					 * Concatenate their subscripts into one subscript.
					 * Use subscript separator SUBSCRIPT_SEP.
					 * XXX Check whether we can encounter an idiom word here.
					 * FIXME Combining contracted words is not handled yet, because
					 * combining morphemes which have non-LL links to other words is
					 * not yet implemented.
					 * FIXME Move to a separate function. */
					Gword **wgaltp;
					size_t join_len = 0;
					size_t mcnt = 0;

					/* If the alternative contains morpheme subwords, mark it
					 * for joining... */
					for (wgaltp = wgp, j = i; NULL != *wgaltp; wgaltp++, j++)
					{

						if ((*wgaltp)->unsplit_word != unsplit_word) break;
						if (MT_INFRASTRUCTURE ==
						    (*wgaltp)->unsplit_word->morpheme_type) break;

						mcnt++;

						if (NULL == cdjp[j])
						{
							/* ... but not if it contains a null word */
							join_alt = false;
							break;
						}
						join_len += strlen(cdjp[j]->string);
						if ((*wgaltp)->morpheme_type & IS_REG_MORPHEME)
							join_alt = true;
					}

					if (join_alt)
					{
						/* Join it in two steps: 1. Base words. 2. Subscripts.
						 * FIXME? Can be done in one step (more efficient but maybe
						 * less clear).
						 * Put SUBSCRIPT_SEP between the subscripts.
						 * XXX No 1-1 correspondence between the hidden base words
						 * and the subscripts after the join, in case there are base
						 * words with and without subscripts. */

						const char subscript_sep_str[] = { SUBSCRIPT_SEP, '\0'};
						const char subscript_mark_str[] = { SUBSCRIPT_MARK, '\0'};
						char *join = calloc(join_len + 1, 1); /* zeroed out */

						join[0] = '\0';

						/* 1. Join base words. (Could just use the unsplit_word.) */
						for (wgaltp = wgp, m = 0; m < mcnt; wgaltp++, m++)
						{
							add_morpheme_unmarked(join, cdjp[i+m]->string,
							                      (*wgaltp)->morpheme_type);
						}

						strcat(join, subscript_mark_str); /* tentative */

						/* 2. Join subscripts. */
						for (wgaltp = wgp, m = 0; m < mcnt; wgaltp++, m++)
						{
							/* Cannot NULLify the word - we may have links to it. */
							if (m != mcnt-1) chosen_words[i+m] = "";

							sm =  strrchr(cdjp[i+m]->string, SUBSCRIPT_MARK);

							if (NULL != sm)
							{
								/* Supposing stem subscript is .=x (x optional) */
								if (MT_STEM == (*wgaltp)->morpheme_type)
								{
									sm += 1 + STEM_MARK_L; /* sm+strlen(".=") */
									if ('\0' == *sm) sm = NULL;
									/* FIXME: By NULLifying the stem subword here, we
									 * suppose thats a stem has only LL-type link, which
									 * will get removed here later. In case it has non-LL
									 * links, then drawing the links diagram would
									 * segfault. We could use " " but it draws an
									 * extra blank in the diagram. */
									chosen_words[i+m] = NULL;
#if 0
									if ((cnt-1) == m)
									{
										/* Support a prefix-stem combination. In that case
										 * we have just nullified the combined word, so we
										 * need to move it to the position of the prefix.
										 * FIXME: May still not be good enough. */
										move_combined_word = i+m-1;

										/* And the later chosen_word assignment should be:
										 * chosen_words[-1 == move_combined_word ?
										 *    move_combined_word : i] = t;
										 */
									}
									else
									{
										move_combined_word = -1;
									}
#endif
								}
							}
							if (NULL != sm)
							{
								strcat(join, sm+1);
								strcat(join, subscript_sep_str);
							}
						}

						/* Remove an extra mark, if any */
						join_len = strlen(join);
						if ((SUBSCRIPT_SEP == join[join_len-1]) ||
							 (SUBSCRIPT_MARK == join[join_len-1]))
							join[join_len-1] = '\0';

						gwordlist_append(&n_lwg_path, unsplit_word);
						t = string_set_add(join, sent->string_set);
						free(join);

						i += mcnt-1;
					}
				}
			}

			if (!join_alt) gwordlist_append(&n_lwg_path, *wgp);

			/*
			 * Add ->regex_name marks in [] if needed, at the end of the base word.
			 * Convert the badly-printing ^C into a period.
			 */
			if (t)
			{
				const char *sm = strrchr(t, SUBSCRIPT_MARK);

				if ((!(w->status & WS_GUESS) && (w->status & WS_INDICT))
				    || !DISPLAY_GUESS_MARKS)
				{
					s = alloca(strlen(t)+1);
					strcpy(s, t);
					if (sm) s[sm-t] = SUBSCRIPT_DOT;
					t = string_set_add(s, sent->string_set);
				}
				else
				{
					size_t baselen;
					const char *regex_name;
					char guess_mark;

					switch (w->status & WS_GUESS)
					{
						case WS_SPELL:
							guess_mark = GM_SPELL;
							break;
						case WS_RUNON:
							guess_mark = GM_RUNON;
							break;
						case WS_REGEX:
							guess_mark = GM_REGEX;
							break;
						case 0:
							guess_mark = GM_UNKNOWN;
							break;
						default:
							assert(0, "Missing 'case: %2x'", w->status & WS_GUESS);
					}

					/* In the case of display_morphology==0, the guess indication of
					 * the last subword is used as the guess indication of the whole
					 * word.
					 * FIXME? The guess indications of other subwords are ignored in
					 * this mode. This implies that if a first or middle subword has
					 * a guess indication but the last subword doesn't have, no guess
					 * indication would be shown at all. */

					regex_name = w->regex_name;
					if ((NULL == regex_name) || HIDE_MORPHO) regex_name = "";
					/* 4 = 1(null) + 1(guess_mark) + 2 (sizeof "[]") */
					baselen = NULL == sm ? strlen(t) : (size_t)(sm-t);
					s = alloca(strlen(t) + strlen(regex_name) + 4);
					strncpy(s, t, baselen);
					s[baselen] = '[';
					s[baselen + 1] = guess_mark;
					strcpy(s + baselen + 2, regex_name);
					strcat(s, "]");
					if (NULL != sm) strcat(s, sm);
					t = s;
					sm = strrchr(t, SUBSCRIPT_MARK);
					if (sm) s[sm-t] = SUBSCRIPT_DOT;
					t = string_set_add(s, sent->string_set);
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
		chosen_words[linkage->num_words-1] = RIGHT_WALL_DISPLAY;
	}

	/* Conditional test removal of quotation marks and the "capdict" tokens,
	 * to facilitate using diff on sentence batch runs. */
	if (test_enabled("removeZZZ"))
	{
		for (i=0, j=0; i<linkage->num_links; i++)
		{
			Link *lnk = &(linkage->link_array[i]);

			if (0 == strcmp("ZZZ", lnk->link_name))
				chosen_words[lnk->rw] = NULL;
		}
	}

	/* We alloc a little more than needed, but so what... */
	linkage->word = (const char **) exalloc(linkage->num_words*sizeof(char *));

	/* Copy over the chosen words, dropping the discarded words. */
	for (i=0, j=0; i<linkage->num_words; ++i)
	{
		if (chosen_words[i])
		{
			linkage->word[j] = chosen_words[i];
			remap[i] = j;
			j++;
		}
	}
	linkage->num_words = j;

#if 0
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
#else
	/* A try to be more general (FIXME: It is not enough).
	 * Discard the LL links unconditionally.
	 * The NULL||'\0'||' ' is to allow easy feature experiments.
	 * FIXME: Define an affix class MORPHOLOGY_LINKS. */
	for (i=0, j=0; i<linkage->num_links; i++)
	{
		Link *old_lnk = &(linkage->link_array[i]);
		const char *lcw = chosen_words[old_lnk->lw];
		const char *rcw = chosen_words[old_lnk->rw];

		if (((NULL == lcw) || ('\0' == *lcw) || (' ' == *lcw) ||
			 (NULL == rcw) || ('\0' == *rcw) || (' ' == *rcw)) &&
			 ((0 == strcmp(old_lnk->link_name, EMPTY_WORD_SUPPRESS)) ||
			  (HIDE_MORPHO &&
			   (0 == strncmp(old_lnk->link_name, SUFFIX_SUPPRESS, SUFFIX_SUPPRESS_L)))))
		{
			;
		}
		else
		{
			Link * new_lnk = &(linkage->link_array[j]);

			/* Copy the entire link contents, thunking the word numbers.
			 * Note that j is always <= i so this is always safe. */
			new_lnk->lw = remap[old_lnk->lw];
			new_lnk->rw = remap[old_lnk->rw];
			new_lnk->lc = old_lnk->lc;
			new_lnk->rc = old_lnk->rc;
			new_lnk->link_name = old_lnk->link_name;
			j++;
		}
	}
#endif

	linkage->num_links = j;
	linkage->wg_path_display = n_lwg_path;

	if (D_CCW <= opts->verbosity)
	{
		print_lwg_path(lwg_path);
		print_lwg_path(n_lwg_path);
	}
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
	 * printing.
	 *
	 * FIXME: For efficiency (in case the user doesn't need to print the
	 * constituents) linkage_post_process() can be moved to
	 * linkage_print_constituent_tree() and
	 * linkage_post_process()/do_post_process() need to be changed
	 * accordingly. In any case, for additional efficiency, the linkage
	 * struct can be changed to include only the domain_array stuff, but
	 * then we need in it also a pointer to the Sentence struct (or
	 * to constituent_pp in it). [ap] */
	linkage_post_process(linkage, sent->constituent_pp, opts);
	linkage->hpsg_pp_data = sent->constituent_pp->pp_data;
	post_process_new_domain_array(sent->constituent_pp);

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
 * I just don't see a lot of utility or value-add here ... this could be
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
