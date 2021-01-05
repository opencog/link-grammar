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

#include <stdlib.h>

#include "api-structures.h"
#include "error.h"
#include "string-set.h"
#include "tok-structures.h"
#include "tokenize.h"
#include "wordgraph.h"

/* === Gword utilities === */
/* Many more Gword utilities, that are used only in particular files,
 * are defined in these files statically. */

Gword *gword_new(Sentence sent, const char *s)
{
	Gword *gword = malloc(sizeof(*gword));

	memset(gword, 0, sizeof(*gword));
	assert(NULL != s, "Null-string subword");
	gword->subword = string_set_add(s, sent->string_set);

	if (NULL != sent->last_word) sent->last_word->chain_next = gword;
	sent->last_word = gword;
	gword->node_num = sent->gword_node_num++;

	gword->gword_set_head = (gword_set){0};
	gword->gword_set_head.o_gword = gword;

	return gword;
}

static Gword **gwordlist_resize(Gword **arr, size_t len)
{
	arr = realloc(arr, (len+2) * sizeof(Gword *));
	arr[len+1] = NULL;
	return arr;
}

size_t gwordlist_len(const Gword **arr)
{
	size_t len = 0;
	if (arr)
		while (arr[len] != NULL) len++;
	return len;
}

void gwordlist_append(Gword ***arrp, Gword *p)
{
	size_t n = gwordlist_len((const Gword **)*arrp);

	*arrp = gwordlist_resize(*arrp, n);
	(*arrp)[n] = p;
}

#if 0
/**
 * Append a Gword list to a given Gword list (w/o duplicates).
 */
void gwordlist_append_list(const Gword ***to_word, const Gword **from_word)
{
	size_t to_word_arr_len = gwordlist_len(*to_word);

	for (const Gword **f = from_word; NULL != *f; f++)
	{
		size_t l;

		/* Note: Must use indexing because to_word may get realloc'ed. */
		for (l = 0; l < to_word_arr_len; l++)
			if (*f == (*to_word)[l]) break; /* Filter duplicates. */

		if (l == to_word_arr_len)
			gwordlist_append((Gword ***)to_word, (Gword *)*f);
	}
}

/**
 * Replace "count" words from the position "start" by word "wnew".
 */
static void wordlist_replace(Gword ***arrp, size_t start, size_t count,
                             const Gword *wnew)
{
	size_t n = gwordlist_len((const Gword **)(*arrp+start+count));

	memmove(*arrp+start+1, *arrp+start+count, (n+1) * sizeof(Gword *));
	(*arrp)[start] = (Gword *)wnew;
}
#endif

size_t wordgraph_pathpos_len(Wordgraph_pathpos *wp)
{
	size_t len = 0;
	if (wp)
		while (wp[len].word != NULL) len++;
	return len;
}

/**
 * `len` is the new length, not counting the terminating null entry.
 */
/* FIXME (efficiency): Initially allocate more than 2 elements */
Wordgraph_pathpos *wordgraph_pathpos_resize(Wordgraph_pathpos *wp,
                                            size_t len)
{
	wp = realloc(wp, (len+1) * sizeof(*wp));
	wp[len].word = NULL;
	return wp;
}

/**
 * Insert the gword into the path queue in reverse order of its hier_depth.
 *
 * The deepest wordgraph alternatives must be scanned first.
 * Otherwise, this sentence causes a flattening mess:
 * "T" this is a flattening test
 * (The mess depends on both "T" and "T matching EMOTICON, and any
 * 5 words after "T".)
 *
 * Parameters:
 *   same_word: mark that the same word is queued again.
 * For validation code only (until the wordgraph version is mature):
 *   used: mark that the word has already been issued into the 2D-array.
 *   diff_alternative: validate we don't queue words from the same alternative.
 */
bool wordgraph_pathpos_add(Wordgraph_pathpos **wp, Gword *p, bool used,
                              bool same_word, bool diff_alternative)
{
	size_t n = wordgraph_pathpos_len(*wp);
	Wordgraph_pathpos *wpt;
	size_t insert_here = n;

	assert(NULL != p, "No Gword to insert");

#ifdef DEBUG
	if (verbosity_level(+9)) print_hier_position(p);
#endif

	if (NULL != *wp)
	{
		for (wpt = *wp; NULL != wpt->word; wpt++)
		{
			if (p == wpt->word)
				return false; /* already in the pathpos queue - nothing to do */

			/* Insert in reverse order of hier_depth. */
			if ((n == insert_here) && (p->hier_depth >= wpt->word->hier_depth))
				insert_here = wpt - *wp;

			/* Validate that there are no words in the pathpos queue from the same
			 * alternative. This can be commented out when the wordgraph code is
			 * mature. FIXME */
			if (diff_alternative)
			{
				assert(same_word||wpt->same_word||!in_same_alternative(p,wpt->word),
				       "wordgraph_pathpos_add(): "
				       "Word%zu '%s' is from same alternative of word%zu '%s'",
				       p->node_num, p->subword,
				       wpt->word->node_num, wpt->word->subword);
			}
		}
	}

	*wp = wordgraph_pathpos_resize(*wp, n+1);

	if (insert_here < n)
	{
		/* n+1 because n is the length of the array, not including the
		 * terminating null entry. We need to protect the terminating null.
		 */
		memmove(&(*wp)[insert_here+1], &(*wp)[insert_here],
		        (n+1 - insert_here) * sizeof (*wpt));
	}

	(*wp)[insert_here].word = p;
	(*wp)[insert_here].same_word = same_word;
	(*wp)[insert_here].used = used;
	(*wp)[insert_here].next_ok = false;

	return true;
}

/**
 *  Print linkage wordgraph path.
 */
void print_lwg_path(Gword **w, const char *title)
{
	lgdebug(+0, "%s: ", title);
	for (; *w; w++) lgdebug(0, "%s ", (*w)->subword);
	lgdebug(0, "\n");
}

#ifdef DEBUG
GNUC_UNUSED static const char *debug_show_subword(const Gword *w)
{
	return w->unsplit_word ? w->subword : "S";
}

GNUC_UNUSED void print_hier_position(const Gword *word)
{
	const Gword **p;

	err_msg(lg_Debug, "[Word %zu:%s hier_position(hier_depth=%zu): ",
	        word->node_num, word->subword, word->hier_depth);
	assert(2*word->hier_depth==gwordlist_len(word->hier_position), "word '%s'",
	       word->subword);

	for (p = word->hier_position; NULL != *p; p += 2)
	{
		err_msg(lg_Debug, "(%zu:%s/%zu:%s)",
		        p[0]->node_num, debug_show_subword(p[0]),
		        p[1]->node_num, debug_show_subword(p[1]));
	}
	err_msg(lg_Debug, "]\n");
}

/* Debug printout of a wordgraph Gword list. */
GNUC_UNUSED void gword_set_print(const gword_set *gs)
{
	printf("Gword list: ");

	if (NULL == gs)
	{
		printf("(null)\n");
		return;
	}

	for (; NULL != gs; gs = gs->next)
	{
		printf("word %p '%s' unsplit '%s'%s", gs->o_gword, (gs->o_gword)->subword,
		       (gs->o_gword)->unsplit_word->subword, NULL==gs->next ? "" : ", ");
	}
	printf("\n");

}
#endif

/**
 * Given a word, find its alternative ID.
 * An alternative is identified by a pointer to its first word, which is
 * getting set at the time the alternative is created at
 * issue_word_alternative(). (It could be any unique identifier - for coding
 * convenience it is a pointer.)
 *
 * Return the alternative_id of this alternative.
 */
static Gword *find_alternative(Gword *word)
{
	assert(NULL != word, "find_alternative(NULL)");
	assert(NULL != word->alternative_id, "find_alternative(%s): NULL id",
	       word->subword);

#if 0
	lgdebug(+0, "find_alternative(%s): '%s'\n",
	        word->subword, debug_show_subword(word->alternative_id));
#endif

	return word->alternative_id;
}

/**
 * Generate an hierarchy-position vector for the given word.
 * It consists of list of (unsplit_word, alternative_id) pairs, leading
 * to the word, starting from a sentence word. It is NULL terminated.
 * Original sentence words don't have any such pair.
 */
const Gword **wordgraph_hier_position(Gword *word)
{
	const Gword **hier_position; /* NULL terminated */
	size_t i = 0;
	Gword *w;
	bool is_leaf = true; /* the word is in the bottom of the hierarchy */

	if (NULL != word->hier_position) return word->hier_position; /* from cache */

	/*
	 * Compute the length of the hier_position vector.
	 */
	for (w = find_real_unsplit_word(word, true); NULL != w; w = w->unsplit_word)
		i++;
	if (0 == i) i = 1; /* Handle the dummy start/end words, just in case. */
	/* Original sentence words (i==1) have zero (i-1) elements. Each deeper
	 * unsplit word has an additional element. Each element takes 2 word pointers
	 * (first one the unsplit word, second one indicating the alternative in
	 * which it is found). The last +1 is for a terminating NULL. */
	word->hier_depth = i - 1;
	i = (2 * word->hier_depth)+1;
	hier_position = malloc(i * sizeof(*hier_position));

	/* Stuff the hierarchical position in a reverse order. */
	hier_position[--i] = NULL;
	w = word;
	while (0 != i)
	{
		hier_position[--i] = find_alternative(w);
		w = find_real_unsplit_word(w, is_leaf);
		hier_position[--i] = w;
		is_leaf = false;
	}

	word->hier_position = hier_position; /* cache it */
	return hier_position;
}

/**
 * Find if 2 words are in the same alternative of their common ancestor
 * unsplit_word.
 * "Same alternative" means at the direct alternative or any level below it.
 * A
 * |
 * +-B C D
 * |
 * +-E F
 *     |
 *     +-G H
 *     |
 *     +-I J
 * J and E (but not J and B) are in the same alternative of their common
 * ancestor unsplit_word A.
 * J and G are not in the same alternative (common ancestor unsplit_word F).
 *
 * Return true if they are, false otherwise.
 */
bool in_same_alternative(Gword *w1, Gword *w2)
{
	const Gword **hp1 = wordgraph_hier_position(w1);
	const Gword **hp2 = wordgraph_hier_position(w2);
	size_t i;

#if 0 /* DEBUG */
	print_hier_position(w1); print_hier_position(w2);
#endif

#if 0 /* BUG */
	/* The following is wrong!  Comparison to the hier_position of the
	 * termination word is actually needed when there are alternatives of
	 * different lengths at the end of a sentence.  This check then prevents
	 * the generation of empty words on the shorter alternative. */
	if ((NULL == w1->next) || (NULL == w2->next)) return false;/* termination */
#endif

	for (i = 0; (NULL != hp1[i]) && (NULL != hp2[i]); i++)
	{
		if (hp1[i] != hp2[i]) break;
	}

	/* In the even positions we have an unsplit_word.
	 * In the odd positions we have an alternative_id.
	 *
	 * If we are here when i is even, it means the preceding alternative_id was
	 * the same in the two words - so they belong to the same alternative.  If
	 * i is 0, it means these are sentence words, and sentence words are all in
	 * the same alternative (including the dummy termination word).
	 * If the hierarchy-position vectors are equal, i is also even, and words
	 * with equal hierarchy-position vectors are in the same alternative.
	 *
	 * If we are here when i is odd, it means the alternative_id at i is not
	 * the same in the given words, but their preceding unsplit_words are the
	 * same - so they clearly not in the same alternative.
	 */
	if (0 == i%2) return true;

	return false;
}

/**
 * Get the real unsplit word of the given word.
 * While the Wordgraph is getting constructed, when a subword has itself as one
 * of its own alternatives, it appears in the wordgraph only once, still
 * pointing to its original unsplit_word. It appears once in order not to
 * complicate the graph, and the unsplit_word is not changed in order not loss
 * information (all of these are implementation decisions). However, for the
 * hierarchy position of the word (when it is a word to be issued, i.e. a leaf
 * node) the real unsplit word is needed, which is the word itself. It is fine
 * since such a word cannot get split further.
 */
Gword *find_real_unsplit_word(Gword *word, bool is_leaf)
{
	/* For the terminating word, return something unique. */
	if (NULL == word->unsplit_word)
		return word;

	if (is_leaf && (word->status & WS_UNSPLIT))
		return word;

	return word->unsplit_word;
}

/**
 * Find the sentence word of which the given word has part.
 * This is done by going upward in the wordgraph along the unsplit_word
 * path until finding a word that is an original sentence word.
 */
Gword *wg_get_sentence_word(const Sentence sent, Gword *word)
{
		if (MT_INFRASTRUCTURE == word->morpheme_type) return NULL;

		while (!IS_SENTENCE_WORD(sent, word))
		{
			word = word->unsplit_word;
			assert(NULL != word, "wg_get_sentence_word(): NULL unsplit word");
		}

		assert(NULL != word->subword, "wg_get_sentence_word(): NULL subword");
		return word;
}

/* FIXME The following debug functions can be generated by a script running
 * from a Makefile and taking the values from structures.h, instead of hard
 * coding the strings as done here.  */

/**
 * Create a short form of flags summary for displaying in a word node.
 */
const char *gword_status(Sentence sent, const Gword *w)
{
	dyn_str *s = dyn_str_new();
	const char *r;
	size_t len;

	if (w->status & WS_UNKNOWN)
		dyn_strcat(s, "UNK|");
	if (w->status & WS_INDICT)
		dyn_strcat(s, "IN|");
	if (w->status & WS_REGEX)
		dyn_strcat(s, "RE|");
	if (w->status & WS_SPELL)
		dyn_strcat(s, "SP|");
	if (w->status & WS_RUNON)
		dyn_strcat(s, "RU|");
	if (w->status & WS_HASALT)
		dyn_strcat(s, "HA|");
	if (w->status & WS_UNSPLIT)
		dyn_strcat(s, "UNS|");
	if (w->status & WS_PL)
		dyn_strcat(s, "PL|");


	char *status_str = dyn_str_take(s);
	len = strlen(status_str);
	if (len > 0) status_str[len-1] = '\0'; /* ditch the last '|' */
	r = string_set_add(status_str, sent->string_set);
	free(status_str);
	return r;
}

static void word_queue_delete(Sentence sent)
{
	word_queue_t *wq = sent->word_queue;
	while (NULL != wq)
	{
		word_queue_t *wq_tofree = wq;
		wq = wq->next;
		free(wq_tofree);
	}
	sent->word_queue = NULL;
	sent->word_queue_last = NULL;
}

/**
 * Delete the gword_set associated with the Wordgraph.
 * @param w First Wordgraph word.
 */
static void gword_set_delete(Gword *w)
{
	if (NULL == w) return;
	for (w = w->chain_next; NULL != w; w = w->chain_next)
	{
		gword_set *n;
		for (gword_set *f = w->gword_set_head.chain_next; NULL != f; f = n)
		{
			n = f->chain_next;
			free(f);
		}
	}
}

void wordgraph_delete(Sentence sent)
{
	word_queue_delete(sent);

	Gword *w = sent->wordgraph;
	gword_set_delete(w);

	while (NULL != w)
	{
		Gword *w_tofree = w;

		free(w->prev);
		free(w->next);
		free(w->hier_position);
		free(w->null_subwords);
		w = w->chain_next;
		free(w_tofree);
	}
	sent->last_word = NULL;
	sent->wordgraph = NULL;
}
