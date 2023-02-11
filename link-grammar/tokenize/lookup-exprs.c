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

#include "api-structures.h"
#include "dict-common/dict-api.h"
#include "dict-common/dict-common.h"
#include "dict-common/dict-defines.h" // for RIGHT_WALL_WORD
#include "dict-common/dict-utils.h"
#include "error.h"
#include "lookup-exprs.h"
#include "print/print.h"
#include "tokenize.h"
#include "tok-structures.h"
#include "wordgraph.h"
#include "word-structures.h"

static Dict_node *dictionary_all_categories(Dictionary dict)
{
	Dict_node * dn = malloc(sizeof(*dn) * dict->num_categories);

	for (size_t i = 0; i < dict->num_categories; i++)
	{
		dn[i].exp = dict->category[i + 1].exp;
		char category_string[16];
		snprintf(category_string, sizeof(category_string), " %x",
		         (unsigned int)i + 1);
		dn[i].string = string_set_add(category_string, dict->string_set);
		dn[i].right = &dn[i + 1];
	}
	dn[dict->num_categories-1].right = NULL;

	return dn;
}

/**
 * build_word_expressions() -- build list of expressions for a word.
 *
 * Looks up a word in the dictionary, fetching from it matching words and their
 * expressions.  Returns NULL if it's not there.  If there, it builds the list
 * of expressions for the word, and returns a pointer to it.
 * The subword of Gword w is used for this lookup, unless the subword is
 * explicitly given as parameter s. The subword of Gword w is always used as
 * the base word for each expression, and its subscript is the one from the
 * dictionary word of the expression.
 */
static X_node * build_word_expressions(Sentence sent, const Gword *w,
                                       const char *s, Parse_Options opts)
{
	const Dictionary dict = sent->dict;

	Dict_node * dn_head = NULL;
	if (IS_GENERATION(dict) && (NULL != strstr(w->subword, WILDCARD_WORD)))
	{
		if (0 == strcmp(w->subword, WILDCARD_WORD))
		{
			dn_head = dictionary_all_categories(dict);
		}
		else
		{
			char *t = alloca(strlen(w->subword) + 1);
			const char *backslash = strchr(w->subword, '\\');

			strcpy(t, w->subword);
			strcpy(t+(backslash - w->subword), backslash+1);
			dn_head = dictionary_lookup_wild(dict, t);
		}
	}
	else
	{
		dn_head = dictionary_lookup_list(dict, NULL == s ? w->subword : s);
	}

	X_node * x = NULL;
	Dict_node * dn = dn_head;
	while (dn != NULL)
	{
		X_node * y = (X_node *) pool_alloc(sent->X_node_pool);
		y->next = x;
		x = y;
		x->exp = copy_Exp(dn->exp, sent->Exp_pool, opts);
		if (NULL == s)
		{
			x->string = dn->string;
		}
		else
		{
			dyn_str *xs = dyn_str_new();
			const char *sm = get_word_subscript(dn->string);

			dyn_strcat(xs, w->subword);
			if (NULL != sm) dyn_strcat(xs, sm);
			x->string = string_set_add(xs->str, sent->string_set);
			dyn_str_delete(xs);
		}
		x->word = w;
		dn = dn->right;
	}
	if (!IS_GENERATION(dict) || (0 != strcmp(w->subword, WILDCARD_WORD)))
		free_lookup_list (dict, dn_head);
	else
		free(dn_head);

	if (IS_GENERATION(dict) && (NULL == dn_head) &&
	    (NULL != strstr(w->subword, WILDCARD_WORD)))
	{
		/* In case of a wild-card word ("\*" or "word\*"), a no-expression
		 * result is valid (in case of "\*" it means an empty dict or a dict
		 * only with walls that are not used).  Use a null-expression
		 * instead, to prevent an error at the caller. */
		X_node * y = pool_alloc(sent->X_node_pool);
		y->next = NULL;
		y->exp = make_zeroary_node(sent->Exp_pool);
	}

	assert(NULL != x, "Word '%s': NULL X-node", w->subword);
	return x;
}

/**
 * Destructively catenates the two disjunct lists d1 followed by d2.
 * Doesn't change the contents of the disjuncts.
 * Traverses the first list, but not the second.
 */
static X_node * catenate_X_nodes(X_node *d1, X_node *d2)
{
	X_node * dis = d1;

	if (d1 == NULL) return d2;
	if (d2 == NULL) return d1;
	while (dis->next != NULL) dis = dis->next;
	dis->next = d2;
	return d1;
}

#ifdef DEBUG
GNUC_UNUSED static void print_x_node(X_node *x)
{
	if (x == NULL) printf("NULL X_node\n");
	for (; x != NULL; x = x->next)
	{
		printf("%p: exp=%p next=%p\n", x, x->exp, x->next);
	}
}
#endif

/* ======================================================================== */
/* Empty-word handling. */

/** Insert ZZZ+ connectors.
 *  This function was mainly used to support using empty-words, a concept
 *  that has been eliminated. However, it is still used to support linking of
 *  quotes that don't get the QUc/QUd links.
 */
static void add_empty_word(Sentence sent, X_node *x)
{
	Exp *zn, *an;
	const char *ZZZ = string_set_lookup(EMPTY_CONNECTOR, sent->dict->string_set);
	/* This function is called only if ZZZ is in the dictionary. */

	/* The left-wall already has ZZZ-. The right-wall will not arrive here. */
	if (MT_WALL == x->word->morpheme_type) return;

	/* Replace plain-word-exp by {ZZZ+} & (plain-word-exp) in each X_node.  */
	for(; NULL != x; x = x->next)
	{
		/* Ignore stems for now, decreases a little the overhead for
		 * stem-suffix languages. */
		if (is_stem(x->string)) continue; /* Avoid an unneeded overhead. */
		//lgdebug(+0, "Processing '%s'\n", x->string);

		/* zn points at {ZZZ+} */
		zn = make_connector_node(sent->dict, sent->Exp_pool, ZZZ, '+', false);
		zn = make_optional_node(sent->Exp_pool, zn);

		/* an will be {ZZZ+} & (plain-word-exp) */
		an = make_and_node(sent->Exp_pool, zn, x->exp);

		x->exp = an;
	}
}

/* ======================================================================== */

/**
 * Build the expression lists for a given word at the current word-array word.
 *
 * The resulted word-array is later used as an input to the parser.
 *
 * Algorithm:
 * Apply the following step to all words w:
 *   - If w is in the dictionary, use it.
 *   - Else if w is identified by regex matching, use the appropriately
 *     matched disjunct collection.
 *   - Otherwise w is unknown - use the disjunct collection of UNKNOWN_WORD.
 *
 * FIXME For now, also add an element to the alternatives array, so the rest of
 * program will work fine (print_sentence_word_alternatives(),
 * sentence_in_dictionary(), verr_msg()).
 */
#define D_X_NODE 9
#define D_DWE 8
static bool determine_word_expressions(Sentence sent, Gword *w,
                                       unsigned int *ZZZ_added,
                                       Parse_Options opts)
{
	Dictionary dict = sent->dict;
	const size_t wordpos = w->sent_wordidx;

	const char *s = w->subword;
	X_node * we = NULL;

	lgdebug(+D_DWE, "Word %zu subword %zu:'%s' status %s",
	        wordpos, w->node_num, s, gword_status(sent, w));
	if (NULL != sent->word[wordpos].unsplit_word)
		lgdebug(D_DWE, " (unsplit '%s')", sent->word[wordpos].unsplit_word);

	/* Generate an "alternatives" component. */
	altappend(sent, &sent->word[wordpos].alternatives, s);

	if (w->status & WS_INDICT)
	{
		we = build_word_expressions(sent, w, NULL, opts);
	}
	else if (w->status & WS_REGEX)
	{
		we = build_word_expressions(sent, w, w->regex_name, opts);
	}
	else if (IS_GENERATION(dict) && (NULL != strstr(s, WILDCARD_WORD)))
	{
		lgdebug(+D_DWE, "Wildcard word %s\n", s);
		we = build_word_expressions(sent, w, NULL, opts);
		w->status = WS_INDICT; /* Prevent marking as "unknown word". */
	}
	else if (dict->unknown_word_defined && dict->use_unknown_word)
	{
		we = build_word_expressions(sent, w, UNKNOWN_WORD, opts);
		assert(we, UNKNOWN_WORD " supposed to be defined in the dictionary!");
		w->status |= WS_UNKNOWN;
	}
	else
	{
		prt_error("Error: Word '%s': word is unknown\n", w->subword);
		return false;
	}

	/* If the current word is an empty-word (or like it), add a
	 * connector for an empty-word (EMPTY_CONNECTOR - ZZZ+) to the
	 * previous word. See the comments at add_empty_word().
	 * As a shortcut, only the first x-node is checked here for ZZZ-,
	 * supposing that the word has it in all of its dict entries
	 * (in any case, currently there is only 1 entry for each such word).
	 * Note that ZZZ_added starts by 0 and so also wordpos, and that the
	 * first sentence word (usually LEFT-WALL) doesn't need a check. */
	if ((wordpos != *ZZZ_added) && is_exp_like_empty_word(dict, we->exp))
	{
		lgdebug(D_DWE, " (has ZZZ-)");
		add_empty_word(sent, sent->word[wordpos-1].x);
		*ZZZ_added = wordpos; /* Remember it for not doing it again */
	}
	lgdebug(D_DWE, "\n");

	/* At last .. concatenate the word expressions we build for
	 * this alternative. */
	sent->word[wordpos].x = catenate_X_nodes(sent->word[wordpos].x, we);
	if (verbosity_level(D_X_NODE))
	{
		/* Print the X_node details for the word. */
		prt_error("Debug: Tokenize word/alt=%zu/%zu '%s' re=%s\n\\",
				 wordpos, altlen(sent->word[wordpos].alternatives), s,
				 w->regex_name ? w->regex_name : "");
		while (we)
		{
			prt_error("Debug:  string='%s' status=%s expr=%s\n",
			          we->string, gword_status(sent, w), exp_stringify(we->exp));
			we = we->next;
		}
	}

	return true;
}
#undef D_DWE

/**
 * Loop over all words in the sentence, and all of the split
 * alternatives, and look up the expressions for those words
 * in the dictionary.
 */
#define D_BSE 8
bool build_sentence_expressions(Sentence sent, Parse_Options opts)
{
	Dictionary dict = sent->dict;
	bool error_encountered = false;
	unsigned int ZZZ_added = 0;   /* ZZZ+ has been added to previous word */

	dict->start_lookup(dict, sent);
	for (size_t i=0; i<sent->length; i++)
	{
		Gword *gw = sent->word[i].gwords[0];
		int igw = 0;
		while (gw)
		{
			error_encountered |=
				!determine_word_expressions(sent, gw, &ZZZ_added, opts);
			igw ++;
			gw = sent->word[i].gwords[igw];
		}
	}
	dict->end_lookup(dict, sent);

	lgdebug(+D_BSE, "sent->length %zu\n", sent->length);
	if (verbosity_level(D_BSE))
	{
		dyn_str *s = dyn_str_new();
		print_sentence_word_alternatives(s, sent, true, NULL, NULL, NULL);
		char *out = dyn_str_take(s);
		prt_error("Debug: Sentence words and alternatives:\n%s", out);
		free(out);
	}

	return !error_encountered;
}
#undef D_BSE

/**
 * This just looks up all the words in the sentence, and builds
 * up an appropriate error message in case some are not there.
 * It has no side effect on the sentence.  Returns true if all
 * went well.
 *
 * This code is called only if the 'unknown-words' flag is set.
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
			if (!dictionary_word_is_known(dict, s) &&
			    !(IS_GENERATION(dict) && (NULL != strstr(s, WILDCARD_WORD))))
			{
				if (ok_so_far)
				{
					lg_strlcpy(temp, "The following words are not in the dictionary:", sizeof(temp));
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
		err_ctxt ec = { sent };
		err_msgc(&ec, lg_Error, "Sentence not in dictionary\n%s\n", temp);
	}
	return ok_so_far;
}
