/*************************************************************************/
/* Copyright 2013, 2014 Linas Vepstas                                    */
/* Copyright 2014, 2015 Amir Plivatsky                                   */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <string.h>

#include "dict-affix.h"
#include "dict-affix-impl.h"
#include "dict-api.h"
#include "dict-common.h"
#include "print/print-util.h"           // patch_subscript_mark
#include "regex-morph.h"
#include "string-set.h"

/* ======================================================================= */

/* The affix dictionary is represented as a dynamically allocated array with
 * an element for each class (connector type) in the affix file. Each element
 * has a pointer to an array of strings which are the punctuation/affix
 * names. */

/** initialize the affix class table */
void afclass_init(Dictionary dict)
{
	const size_t sz = sizeof(*dict->afdict_class) * AFDICT_NUM_ENTRIES;

	dict->afdict_class = malloc(sz);
	memset(dict->afdict_class, 0, sz);
}

/**
 * Find the affix table entry for given connector name.
 * If the connector name is not in the table, return NULL.
 */
Afdict_class * afdict_find(Dictionary afdict, const char * con, bool notify_err)
{
	const char * const * ac;

	for (ac = afdict_classname;
	     ac < &afdict_classname[ARRAY_SIZE(afdict_classname)]; ac++)
	{
		if (0 == strcmp(*ac, con))
			return &afdict->afdict_class[ac - afdict_classname];
	}
	if (notify_err) {
		prt_error("Warning: Unknown class name %s found near line %d of %s.\n"
		          "\tThis class name will be ignored.\n",
		          con, afdict->line_number, afdict->name);
	}
	return NULL;
}

#define AFFIX_COUNT_MEM_INCREMENT 64

void affix_list_add(Dictionary afdict, Afdict_class * ac,
		const char * affix)
{
	if (NULL == ac)  return; /* ignore unknown class name */
	if (ac->mem_elems <= ac->length)
	{
		size_t new_sz;
		ac->mem_elems += AFFIX_COUNT_MEM_INCREMENT;
		new_sz = ac->mem_elems * sizeof(const char *);
		ac->string = realloc(ac->string, new_sz);
	}
	ac->string[ac->length] = string_set_add(affix, afdict->string_set);
	ac->length++;
}

#ifdef AFDICT_ORDER_NOT_PRESERVED
static int revcmplen(const void *a, const void *b)
{
	return strlen(*(char * const *)b) - strlen(*(char * const *)a);
}
#endif /* AFDICT_ORDER_NOT_PRESERVED */

/**
 * Traverse the main dict in dictionary order, and extract all the suffixes
 * and prefixes - every time we see a new suffix/prefix (the previous one is
 * remembered by w_last), we save it in the corresponding affix-class list.
 * The saved affixes don't include the infix mark.
 */
static void get_dict_affixes(Dictionary dict, Dict_node * dn,
                             char infix_mark,
                             const char** plast, size_t *lastlen)
{
	const char *w;         /* current dict word */
	const char *w_sm;      /* SUBSCRIPT_MARK position in the dict word */
	size_t w_len;          /* length of the dict word */
	Dictionary afdict = dict->affix_table;

	if (dn == NULL) return;
	get_dict_affixes(dict, dn->right, infix_mark, plast, lastlen);

	w = dn->string;
	w_sm = get_word_subscript(w);
	w_len = (NULL == w_sm) ? strlen(w) : (size_t)(w_sm - w);

	if (*lastlen != w_len || 0 != strncmp(*plast, w, w_len))
	{
		char* wtrunc = strdupa(w);
		wtrunc[w_len] = '\0';

		if (infix_mark == w[0])
		{
			affix_list_add(afdict, &afdict->afdict_class[AFDICT_SUF], wtrunc+1);
		}
		else if (infix_mark == w[w_len-1])
		{
			wtrunc[w_len-1] = '\0';
			affix_list_add(afdict, &afdict->afdict_class[AFDICT_PRE], wtrunc);
		}

		*plast = w;
		*lastlen = w_len;
	}

	get_dict_affixes(dict, dn->left, infix_mark, plast, lastlen);
}

/**
 * Concatenate the definitions for the given affix class.
 * This allows specifying the characters in different definitions
 * instead in a one long string, e.g. instead of:
 * ""«»《》【】『』`„": QUOTES+;
 * One can specify (note the added spaces):
 * """  «»  《》 【】 『』  ` „: QUOTES+;
 * Or even:
 * """: QUOTES+;
 * «» : QUOTES+;
 * etc.
 * Note that if there are no definitions or only one definition, there is
 * nothing to do.
 * The result is written to the first entry.
 * @param classno The given affix class.
 */
static void concat_class(Dictionary afdict, int classno)
{
	Afdict_class * ac;
	size_t i;
	dyn_str * qs;

	ac = AFCLASS(afdict, classno);
	if (1 >= ac->length) return;

	qs = dyn_str_new();
	for (i = 0; i < ac->length; i++)
		dyn_strcat(qs, ac->string[i]);

	ac->string[0] = string_set_add(qs->str, afdict->string_set);
	dyn_str_delete(qs);
}

/**
 * Compare lengths of strings, for affix class qsort.
 * Sort order:
 * 1. Longest base words first.
 * 2. Equal base words one after the other.
 * 3. Regexes last, in file order.
 */
static int split_order(const void *a, const void *b)
{
	const char * const *sa = a;
	const char * const *sb = b;

	size_t len_a = strcspn(*sa, subscript_mark_str());
	size_t len_b = strcspn(*sb, subscript_mark_str());
	bool is_regex_a = (get_affix_regex_cg(*sa) >= 0);
	bool is_regex_b = (get_affix_regex_cg(*sb) >= 0);

	if (is_regex_a && is_regex_b) return 0;
	if (is_regex_a) return 1;
	if (is_regex_b) return -1;

	int len_order = (int)(len_b - len_a);
	if (0 == len_order) return strncmp(*sa, *sb, len_a);

	return len_order;
}

/**
 * Initialize several classes.
 * In case of a future dynamic change of the affix table, this function needs to
 * be invoked again after the affix table is re-constructed (changes may be
 * needed - especially to first free memory and initialize the affix dict
 * structure.).
 */
#define D_AI (D_DICT+1)
bool afdict_init(Dictionary dict)
{
	Afdict_class * ac;
	Dictionary afdict = dict->affix_table;
	bool error_found = false;

	/* FIXME: read_entry() builds word lists in reverse order (can we
	 * just create the list top-down without breaking anything?). Unless
	 * it is fixed to preserve the order, reverse here the word list for
	 * each affix class. */
	for (ac = afdict->afdict_class;
		  ac < &afdict->afdict_class[ARRAY_SIZE(afdict_classname)]; ac++)
	{
		int i;
		int l = ac->length - 1;
		const char * t;

		for (i = 0;  i < l; i++, l--)
		{
			t = ac->string[i];
			ac->string[i] = ac->string[l];
			ac->string[l] = t;
		}
	}

	/* Create the affix lists */
	ac = AFCLASS(afdict, AFDICT_INFIXMARK);
	if ((1 < ac->length) || ((1 == ac->length) && (1 != strlen(ac->string[0]))))
	{
		prt_error("Error: afdict_init: Invalid value for class %s in file %s"
		          " (should have been one ASCII punctuation - ignored)\n",
		          afdict_classname[AFDICT_INFIXMARK], afdict->name);
		free(ac->string);
		ac->length = 0;
		ac->mem_elems = 0;
		ac->string = NULL;
	}
	/* XXX For now there is a possibility to use predefined SUF and PRE lists.
	 * So if SUF or PRE are defined, don't extract any of them from the dict. */
	if (1 == ac->length)
	{
		if ((0 == AFCLASS(afdict, AFDICT_PRE)->length) &&
		    (0 == AFCLASS(afdict, AFDICT_SUF)->length))
		{
			const char *last = 0x0;
			size_t len = 0;
			get_dict_affixes(dict, dict->root, ac->string[0][0], &last, &len);
		}
		else
		{
			afdict->pre_suf_class_exists = true;
		}
	}
	else
	{
		/* No INFIX_MARK - create a dummy one that always mismatches */
		affix_list_add(afdict, &afdict->afdict_class[AFDICT_INFIXMARK], "");
	}

	/* Store the SANEMORPHISM regex in the unused (up to now)
	 * regex_root element of the affix dictionary, and precompile it */
	assert(NULL == afdict->regex_root, "SM regex is already assigned");
	ac = AFCLASS(afdict, AFDICT_SANEMORPHISM);
	if (0 != ac->length)
	{
		dyn_str *rebuf = dyn_str_new();

		/* The regex used to be converted to: ^((original-regex)b)+$
		 * In the initial wordgraph version word boundaries are not supported,
		 * so instead it is converted to: ^(original-regex)+$ */
#ifdef WORD_BOUNDARIES
		dyn_strcat(rebuf, "^((");
#else
		dyn_strcat(rebuf, "^(");
#endif
		dyn_strcat(rebuf, ac->string[0]);
#ifdef WORD_BOUNDARIES
		dyn_strcat(rebuf, ")b)+$");
#else
		dyn_strcat(rebuf, ")+$");
#endif

		afdict->regex_root = regex_new(afdict_classname[AFDICT_SANEMORPHISM],
		                               rebuf->str);
		dyn_str_delete(rebuf);

		if (!compile_regexs(afdict->regex_root, afdict))
		{
			prt_error("Error: afdict_init: Failed to compile "
			          "regex /%s/ in file \"%s\"\n",
			          afdict_classname[AFDICT_SANEMORPHISM], afdict->name);
			error_found = true;
		}
		else
		{
			lgdebug(+D_AI, "%s regex %s\n", afdict_classname[AFDICT_SANEMORPHISM],
			        afdict->regex_root->pattern);
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(affix_strippable); i++)
	{
		ac = AFCLASS(afdict, affix_strippable[i]);
		int *capture_group = alloca(ac->length * sizeof(*capture_group));
		ac->Nregexes = 0;

		if (AFDICT_UNITS != affix_strippable[i])
		{
			/* Locate the affix regexes, find their capture
			 * groups and count them for memory allocation. */
			for (size_t n = 0;  n < ac->length; n++)
			{
				capture_group[n] = get_affix_regex_cg(ac->string[n]);
				if (capture_group[n] < 0) continue;

				ac->Nregexes++;
			}

			/* Allocate the regexes at the end of the existing string pointer
			 * memory block, and compile them. */
			if (ac->Nregexes > 0)
			{
				ac->regex = malloc(ac->Nregexes * sizeof(Regex_node *));
				/* Zero-out to prevent uninitialized access on error. */
				memset(ac->regex, 0, ac->Nregexes * sizeof(Regex_node *));

				size_t re_index = 0;
				for (size_t n = 0;  n < ac->length; n++)
				{
					if (capture_group[n] < 0) continue;

					/* Extract the regex pattern. */
					const size_t regex_len =
						strlen(ac->string[n]) - (sizeof("//.\\N") - 1/* \0 */);
					const char *pattern = strndupa(ac->string[n] + 1, regex_len);

					ac->regex[re_index] = regex_new(ac->string[n], pattern);
					ac->regex[re_index]->capture_group = capture_group[n];
					if (!compile_regexs(ac->regex[re_index], afdict))
					{
						prt_error("Error: afdict_init: Class %s in file %s: "
						          "regex \"%s\" Failed to compile\n",
						          afdict_classname[affix_strippable[i]],
						          afdict->name, ac->regex[re_index]->name);
						return false;
					}
					re_index++;
				}
			}
		}

		/* Sort the affix-classes of tokens to be stripped. */
		/* Longer unit names must get split off before shorter ones.
		 * This prevents single-letter splits from screwing things
		 * up. e.g. split 7gram before 7am before 7m.
		 * Another example: The ellipsis "..." must appear before the dot ".".
		 * Regexes are just being moved to the end, at the same order.
		 */
		for (size_t s = 0; s < ARRAY_SIZE(affix_strippable); s++)
		{
			ac = AFCLASS(afdict, affix_strippable[i]);
			if (0 < ac->length)
			{
				qsort(ac->string, ac->length, sizeof(char *), split_order);
			}
		}
	}

	if (!IS_DYNAMIC_DICT(dict))
	{
		/* Validate that the strippable tokens are in the dict. UNITS are
		 * assumed to be from the dict only. */
		for (size_t i = 0; i < ARRAY_SIZE(affix_strippable); i++)
		{
			if (AFDICT_UNITS != affix_strippable[i])
			{
				ac = AFCLASS(afdict, affix_strippable[i]);
				bool not_in_dict = false;

				for (int n = 0;  n < ac->length - ac->Nregexes; n++)
				{
					if (!dictionary_word_is_known(dict, ac->string[n]))
					{
						if (!not_in_dict)
						{
							not_in_dict = true;
							prt_error("Warning: afdict_init: Class %s in file %s: "
							          "Token(s) not in the dictionary:",
							          afdict_classname[affix_strippable[i]],
							          afdict->name);
						}

						char *s = strdupa(ac->string[n]);
						patch_subscript_mark(s);
						prt_error(" \"%s\"", s);
					}
				}
				if (not_in_dict) prt_error("\n");
			}
		}
	}

#ifdef AFDICT_ORDER_NOT_PRESERVED
	/* pre-sort the MPRE list */
	ac = AFCLASS(afdict, AFDICT_MPRE);
	if (0 < ac->length)
	{
		/* Longer subwords have priority over shorter ones,
		 * reverse-sort by length.
		 * XXX mprefix_split() for Hebrew depends on that. */
		qsort(ac->string, ac->length, sizeof(char *), revcmplen);
	}
#endif /* AFDICT_ORDER_NOT_PRESERVED */

	concat_class(afdict, AFDICT_QUOTES);
	concat_class(afdict, AFDICT_BULLETS);

	if (verbosity_level(D_AI))
	{
		size_t l;

		for (ac = afdict->afdict_class;
		     ac < &afdict->afdict_class[ARRAY_SIZE(afdict_classname)]; ac++)
		{
			if (0 == ac->length) continue;
			lgdebug(+0, "Class %s, %d items:",
			        afdict_classname[ac-afdict->afdict_class], ac->length);
			for (l = 0; l < ac->length; l++)
				lgdebug(0, " '%s'", ac->string[l]);
			lgdebug(0, "\n\\");
		}
		lg_error_flush();
	}

	return !error_found;
}
#undef D_AI
