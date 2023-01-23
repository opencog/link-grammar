/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright 2013, 2014 Linas Vepstas                                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <string.h>

#include "dict-common/dict-common.h"
#include "dict-common/dict-utils.h"     // patch_subscript
#include "dict-common/idiom.h"
#include "dict-file/read-dict.h"        // dict_error2
#include "string-id.h"
#include "string-set.h"

#include "dict-ram.h"

/* ======================================================================== */

static void free_dict_node_recursive(Dict_node * dn)
{
	if (dn == NULL) return;
	free_dict_node_recursive(dn->left);
	free_dict_node_recursive(dn->right);
	free(dn);
}

void free_dictionary_root(Dictionary dict)
{
	free_dict_node_recursive(dict->root);
	pool_delete(dict->Exp_pool);
	dict->root = NULL;
	dict->Exp_pool = NULL;
}

void dict_node_noop(Dictionary dict)
{
}

/* ======================================================================== */

Dict_node * dict_node_new(void)
{
	return (Dict_node*) malloc(sizeof(Dict_node));
}

void dict_node_free_list(Dict_node *llist)
{
	Dict_node * n;
	while (llist != NULL)
	{
		n = llist->right;
		free(llist);
		llist = n;
	}
}

/* ======================================================================== */
/**
 * Dictionary entry comparison and ordering functions.
 *
 * The data structure storing the dictionary is simply a binary tree.
 * The entries in the binary tree are sorted by alphabetical order.
 * There is one catch, however: words may have suffixes (a dot, followed
 * by the suffix), and these suffixes are to be handled appropriately
 * during sorting and comparison.
 *
 * The use of suffixes means that the ordering of the words is not
 * exactly the order given by strcmp.  The order must be such that, for
 * example, "make" < "make.n" < "make-up" -- suffixed words come after
 * the bare words, but before any other other words with non-alphabetic
 * characters (such as the hyphen in "make-up", or possibly UTF8
 * characters). Thus, plain "strcmp" can't be used to determine
 * dictionary order.
 *
 * Thus, a set of specialized string comparison and ordering functions
 * are provided. These "do the right thing" when matching string with
 * and without suffixes.
 */
/**
 * dict_order_strict - order two dictionary words in proper sort order.
 * Return zero if the strings match, else return in a unique order.
 * The order is NOT (locale-dependent) UTF8 sort order; its ordered
 * based on numeric values of single bytes.  This will uniquely order
 * UTF8 strings, just not in a LANG-dependent (locale-dependent) order.
 * But we don't need/want locale-dependent ordering!
 */
/* verbose version, for demonstration only */
/*
int dict_order_strict(char *s, char *t)
{
	int ss, tt;
	while (*s != '\0' && *s == *t) {
		s++;
		t++;
	}
	if (*s == SUBSCRIPT_MARK) {
		ss = 1;
	} else {
		ss = (*s)<<1;
	}
	if (*t == SUBSCRIPT_MARK) {
		tt = 1;
	} else {
		tt = (*t)<<1;
	}
	return (ss - tt);
}
*/

/* terse version */
/* If one word contains a dot, the other one must also! */
NO_SAN_DICT
static inline int dict_order_strict(const char *s, const Dict_node * dn)
{
	const char * t = dn->string;
	while ((*s == *t) && (*s != '\0')) { s++; t++; }
	return (*s - *t);
}

/**
 * dict_order_bare() -- order user vs. dictionary string.
 *
 * Similar to above, except that a "bare" search string will match
 * a dictionary entry with a dot.
 *
 * Assuming that s is a pointer to the search string, and that t is
 * a pointer to a dictionary string, this returns 0 if they match,
 * returns >0 if s>t, and <0 if s<t.
 *
 * The matching is done as follows.  Walk down the strings until you
 * come to the end of one of them, or until you find unequal characters.
 * If the dictionary string contains a SUBSCRIPT_MARK, then replace the
 * mark by "\0", and take the difference.
 */
NO_SAN_DICT
static inline int dict_order_bare(const char *s, const Dict_node * dn)
{
	const char * t = dn->string;
	while ((*s == *t) && (*s != '\0')) { s++; t++; }
	return (*s)  -  ((*t == SUBSCRIPT_MARK)?(0):(*t));
}

/**
 * dict_order_wild() -- order dictionary strings, with wildcard.
 *
 * This routine is used to support command-line parser users who
 * want to search for all dictionary entries of some given word or
 * partial word, containing a wild-card. This is done by using the
 * !!blah* command at the command-line.  Users need this function to
 * debug the dictionary.  This is the ONLY place in the link-parser
 * where wild-card search is needed; ordinary parsing does not use it.
 *
 * !!blah*.sub is also supported.
 *
 * Assuming that s is a pointer to a search string, and that
 * t is a pointer to a dictionary string, this returns 0 if they
 * match, >0 if s>t, and <0 if s<t.
 *
 * The matching is done as follows.  Walk down the strings until
 * you come to the end of one of them, or until you find unequal
 * characters.  A "*" matches anything before the subscript mark.
 * Otherwise, replace SUBSCRIPT_MARK by "\0", and take the difference.
 * This behavior matches that of the function dict_order_bare().
 */
#define WILD_TYPE '*'
#define D_DOW 6
static inline int dict_order_wild(const char * s, const Dict_node * dn)
{
	const char * t = dn->string;

	lgdebug(+D_DOW, "search-word='%s' dict-word='%s'\n", s, t);
	while((*s == *t) && (*s != SUBSCRIPT_MARK) && (*s != '\0')) { s++; t++; }

	if (*s == WILD_TYPE) return 0;

	lgdebug(D_DOW, "Result: '%s'-'%s'=%d\n",
	 s, t, ((*s == SUBSCRIPT_MARK)?(0):(*s)) - ((*t == SUBSCRIPT_MARK)?(0):(*t)));
	return ((*s == SUBSCRIPT_MARK)?(0):(*s)) - ((*t == SUBSCRIPT_MARK)?(0):(*t));
}
#undef D_DOW

/* ======================================================================== */

static bool subscr_match(const char *s, const Dict_node * dn)
{
	const char * s_sub = get_word_subscript(s);
	const char * t_sub = get_word_subscript(dn->string);

	if (NULL == s_sub)
	{
		if (NULL == t_sub) return true;
		return !is_idiom_word(t_sub);
	}
	if (NULL == t_sub) return false;
	if (0 == strcmp(s_sub, t_sub)) return true;

	return false;
}

/**
 * rdictionary_lookup() -- recursive dictionary lookup
 * Walk binary tree, given by 'dn', looking for the string 's'.
 * For every node in the tree where 's' matches,
 * make a copy of that node, and append it to llist.
 */
static Dict_node *
rdictionary_lookup(Dict_node * restrict llist,
                   Dict_node * restrict dn,
                   const char * restrict s,
                   bool boolean_lookup,
                   int (*dict_order)(const char *, const Dict_node *))
{
	if (dn == NULL) return llist;

	int m = dict_order(s, dn);

	// The tree is in alphabetical order. Move down either the
	// left or right branch, until a string match is found.
	while (m != 0)
	{
		if (m > 0)
			dn = dn->right;
		else if (m < 0)
			dn = dn->left;

		if (dn == NULL) return llist;

		m = dict_order(s, dn);
	}

	// There might be more than one perfect match. Recurse to get it.
	if (dn->right)
		llist = rdictionary_lookup(llist, dn->right, s, boolean_lookup, dict_order);

	if (dict_order != dict_order_wild || subscr_match(s, dn))
	{
		if (boolean_lookup) return dn;
		Dict_node * dn_new = dict_node_new();
		*dn_new = *dn;
		dn_new->right = llist;
		dn_new->left = dn; /* Currently only used for inserting idioms */
		llist = dn_new;
	}

	// There might be more than one perfect match. Recurse to get it.
	if (dn->left)
		llist = rdictionary_lookup(llist, dn->left, s, boolean_lookup, dict_order);

	return llist;
}

/**
 * dict_node_lookup() - return list of words in the RAM-cached dictionary.
 *
 * Returns a pointer to a lookup list of the words in the dictionary.
 *
 * This list is made up of Dict_nodes, linked by their right pointers.
 * The node, file and string fields are copied from the dictionary.
 *
 * The returned list must be freed with dict_node_free_lookup().
 */
Dict_node * dict_node_lookup(const Dictionary dict, const char *s)
{
	return rdictionary_lookup(NULL, dict->root, s, false, dict_order_bare);
}

bool dict_node_exists_lookup(Dictionary dict, const char *s)
{
	return !!rdictionary_lookup(NULL, dict->root, s, true, dict_order_bare);
}

/**
 * strict_lookup_list() - return exact match in the dictionary
 *
 * Returns a pointer to a lookup list of the words in the dictionary.
 *
 * This list is made up of Dict_nodes, linked by their right pointers.
 * The node, file and string fields are copied from the dictionary.
 *
 * The list normally has 0 or 1 elements, unless the given word
 * appears more than once in the dictionary.
 *
 * The returned list must be freed with dict_node_free_lookup().
 */
Dict_node * strict_lookup_list(const Dictionary dict, const char *s)
{
	return rdictionary_lookup(NULL, dict->root, s, false, dict_order_strict);
}

/**
 * dict_node_wild_lookup -- allows for wildcard searches (globs)
 * Used to support the !! command in the parser command-line tool.
 */
Dict_node * dict_node_wild_lookup(Dictionary dict, const char *s)
{
	char * ds = strrchr(s, SUBSCRIPT_DOT); /* Only the rightmost dot is a
	                                          candidate for SUBSCRIPT_DOT */
	char * ws = strrchr(s, WILD_TYPE);     /* A SUBSCRIPT_DOT can only appear
                                             after a wild-card */
	Dict_node * result;
	char * stmp = strdupa(s);

	/* It is not a SUBSCRIPT_DOT if it is at the end or before the wild-card.
	 * E.g: "Dr.", "i.*", "." */
	if ((NULL != ds) && ('\0' != ds[1]) && ((NULL == ws) || (ds > ws)))
		stmp[ds-s] = SUBSCRIPT_MARK;

	result = rdictionary_lookup(NULL, dict->root, stmp, false, dict_order_wild);
	return result;
}

/* ======================================================================== */
/*
 * "Expressions" -- these encode the binary-tree structure of the
 * hand-written dictionaries. These are binary trees, with leaves of
 * connectors joined up by the binary operators `&`, `or`.
 *
 * The in-RAM representation is NOT a binary tree; instead it is a tree
 * of lists. An Exp node contains a tag: `AND_type`, `OR_type`, etc.
 * The `operand_next` field is used to hold a linked list which is
 * joined up with the given Exp type. This is more efficent to traverse,
 * and also saves space, as compared to an ordinary binary tree.
 *
 * Example:  (A or B or C or D) becomes
 *
 *   or
 *    \
 *     A -> B -> C -> D
 *
 * The slash \ is the `operand_first` field. The horizontal arrows are
 * the `operand_next` field. Here's a more complicated example:
 *
 *    (A or (E & F) or C or D)
 *
 * becomes
 *
 *   or
 *    \
 *     A -> and -> C -> D
 *            \
 *             E -> F
 *
 * This structure has performance implications: the `build_disjuncts()`
 * code will traverse the `operand_next` fields in a loop, but will make
 * a recursive call for each `operand_first` (thus buying a stack frame.)
 * The original example (A or B or C or D) could have been written as
 * (A or (B or (C or D))) and so
 *
 *   or
 *    \
 *     A -> or
 *           \
 *            B -> or
 *                  \
 *                   C -> D
 *
 * But this would be a bad idea, since (1) more Exp nodes are required,
 * and (2) the recursive calls in `build_disjuncts()` really do add up
 * for large expressions.
 */

/**
 * Allocate a new Exp node.
 */
Exp *Exp_create(Pool_desc *mp)
{
	Exp *e = pool_alloc(mp);
	e->tag_type = Exptag_none;
	e->operand_next = NULL;
	e->cost = 0.0;
	return e;
}

/**
 * Duplicate the given Exp node.
 * This is needed in case it participates more than once in a
 * single expression.
 */
Exp *Exp_create_dup(Pool_desc *mp, Exp *old_e)
{
	Exp *new_e = pool_alloc(mp);

	*new_e = *old_e;

	return new_e;
}

/**
 * This creates a node with zero children.  Initializes
 * the cost to zero.
 */
Exp * make_zeroary_node(Pool_desc *mp)
{
	Exp * n = Exp_create(mp);
	n->type = AND_type;  /* these must be AND types */
	n->operand_first = NULL;
	return n;
}

/**
 * This creates a node with one child (namely e).  Initializes
 * the cost to zero.
 */
Exp *make_unary_node(Pool_desc *mp, Exp * e)
{
	Exp * n;
	n = Exp_create(mp);
	n->type = AND_type;  /* these must be AND types */
	n->operand_first = e;
	return n;
}

/**
 * Create an expression that joins together `nl` and `nr`.
 * The join type can be either `AND_type` or `OR_type`.
 */
Exp * make_join_node(Pool_desc *mp, Exp* nl, Exp* nr, Exp_type t)
{
	Exp* n;

	n = Exp_create(mp);
	n->type = t;

	n->operand_first = nl;
	nl->operand_next = nr;
	// nr->operand_next = NULL;

	return n;
}

/**
 * Create an AND_type expression. The expressions nl, nr will be
 * AND-ed together.
 */
Exp * make_and_node(Pool_desc *mp, Exp* nl, Exp* nr)
{
	return make_join_node(mp, nl, nr, AND_type);
}

/**
 * Create an OR_type expression. The expressions nl, nr will be
 * OR-ed together.
 */
Exp * make_or_node(Pool_desc *mp, Exp* nl, Exp* nr)
{
	return make_join_node(mp, nl, nr, OR_type);
}

Exp * make_connector_node(Dictionary dict, Pool_desc *mp,
                          const char* linktype, char dir, bool multi)
{
	Exp* n = Exp_create(mp);
	n->type = CONNECTOR_type;

	n->condesc = condesc_add(&dict->contable,
		string_set_add(linktype, dict->string_set));
	n->dir = dir;
	n->multi = multi;

	return n;
}

/**
 * This creates an OR node with two children, one the given node,
 * and the other as zeroary node.  This has the effect of creating
 * what used to be called an optional node.
 */
Exp *make_optional_node(Pool_desc *mp, Exp *e)
{
	return make_or_node(mp, make_zeroary_node(mp), e);
}

/* ======================================================================== */
/* Implementation of the DSW algo for rebalancing a binary tree.
 * The point is -- after building the dictionary tree, we rebalance it
 * once at the end. This is a **LOT LOT** quicker than maintaining an
 * AVL tree along the way (less than quarter-of-a-second vs. about
 * a minute or more!) FWIW, the DSW tree is even more balanced than
 * the AVL tree is (it's less deep, more full).
 *
 * The DSW algo, with C++ code, is described in
 *
 * Timothy J. Rolfe, "One-Time Binary Search Tree Balancing:
 * The Day/Stout/Warren (DSW) Algorithm", inroads, Vol. 34, No. 4
 * (December 2002), pp. 85-88
 * http://penguin.ewu.edu/~trolfe/DSWpaper/
 */

static Dict_node *rotate_right(Dict_node *root)
{
	Dict_node *pivot = root->left;
	root->left = pivot->right;
	pivot->right = root;
	return pivot;
}

Dict_node * dsw_tree_to_vine (Dict_node *root)
{
	Dict_node *vine_tail, *vine_head, *rest;
	Dict_node vh;

	vine_head = &vh;
	vine_head->left = NULL;
	vine_head->right = root;
	vine_tail = vine_head;
	rest = root;

	while (NULL != rest)
	{
		/* If no left, we are done, do the right */
		if (NULL == rest->left)
		{
			vine_tail = rest;
			rest = rest->right;
		}
		/* eliminate the left subtree */
		else
		{
			rest = rotate_right(rest);
			vine_tail->right = rest;
		}
	}

	return vh.right;
}

NO_SAN_DICT
static void dsw_compression (Dict_node *root, unsigned int count)
{
	unsigned int j;
	for (j = 0; j < count; j++)
	{
		/* Compound left rotation */
		Dict_node * pivot = root->right;
		root->right = pivot->right;
		root = pivot->right;
		pivot->right = root->left;
		root->left = pivot;
	}
}

/* Return size of the full portion of the tree
 * Gets the next pow(2,k)-1
 */
static inline unsigned int full_tree_size (unsigned int size)
{
	unsigned int pk = 1;
	while (pk < size) pk = 2*pk + 1;
	return pk/2;
}

Dict_node * dsw_vine_to_tree (Dict_node *root, int size)
{
	Dict_node vine_head;
	unsigned int full_count = full_tree_size(size +1);

	vine_head.left = NULL;
	vine_head.right = root;

	dsw_compression(&vine_head, size - full_count);
	for (size = full_count; size > 1; size /= 2)
	{
		dsw_compression(&vine_head, size / 2);
	}
	return vine_head.right;
}

/* ======================================================================== */
/**
 * Notify about a duplicate word, unless allowed or it is an idiom definition.
 * Idioms are exempt because historically they couldn't be
 * differentiated using a subscript if duplicate definitions were
 * convenience (and also they were not inserted into the dictionary so
 * their duplicate check got neglected).
 *
 * The following dictionary definition allows duplicate words:
 * #define allow-duplicate-words true
 * An idiom duplicate check can be requested using the test:
 * "disallow_dup-idioms"
 */

/**
 * Return the relevant status of allow_duplicate_words/idioms.
 */
static int dup_word_status(Dictionary dict, const Dict_node *newnode)
{
	if (dict->allow_duplicate_words == dict->allow_duplicate_idioms)
		return dict->allow_duplicate_words;

	if (contains_underbar(newnode->string))
	{
		return dict->allow_duplicate_idioms;
	}
	else
	{
		return dict->allow_duplicate_words;
	}
}

static bool dup_word_error(Dictionary dict, Dict_node *newnode)
{

	if (dup_word_status(dict, newnode) == 1) return false;

	if (dict->allow_duplicate_words == 0)
	{
		const char *s = linkgrammar_get_dict_define(dict, "allow-duplicate-words");
		dict->allow_duplicate_words =
			((s != NULL) && (0 == strcasecmp(s, "true"))) ? 1 : -1;

		bool disallow_dup_idioms = !!test_enabled("disallow-dup-idioms");
		dict->allow_duplicate_idioms = disallow_dup_idioms ? -1 : 1;

		if (dup_word_status(dict, newnode) == 1) return false;
	}

	dict_error2(dict, "Ignoring word which has been multiply defined:",
	            newnode->string);

	/* Too late to skip insertion - insert it with a null expression. */
	newnode->exp = make_zeroary_node(dict->Exp_pool);

	return true;
}

/**
 * Insert the new node into the dictionary below node n.
 * "newnode" left and right fields are NULL, and its string is already
 * there.  If the string is already found in the dictionary, give an error
 * message and effectively ignore it.
 *
 * The resulting tree is highly unbalanced. It needs to be rebalanced
 * before being used.  The DSW algo below is ideal for that.
 */
NO_SAN_DICT
Dict_node *dict_node_insert(Dictionary dict, Dict_node *n, Dict_node *newnode)
{
	if (NULL == n) return newnode;

	int comp = dict_order_strict(newnode->string, n);

	if ((0 == comp) && dup_word_error(dict, newnode))
	    comp = -1;

	if (comp < 0)
	{
		if (NULL == n->left)
		{
			n->left = newnode;
			return n;
		}
		n->left = dict_node_insert(dict, n->left, newnode);
	}
	else
	{
		if (NULL == n->right)
		{
			n->right = newnode;
			return n;
		}
		n->right = dict_node_insert(dict, n->right, newnode);
	}

	return n;
	/* return rebalance(n); Uncomment to get an AVL tree */
}

void add_define(Dictionary dict, const char *name, const char *value)
{
	int id = string_id_add(name, dict->dfine.set);

	if (dict->dfine.size >= (unsigned int)id)
	{
		prt_error("Warning: Redefinition of \"%s\", "
		          "found near line %d of \"%s\"\n",
		          name, dict->line_number, dict->name);
	}
	else
	{
		dict->dfine.size++;
		dict->dfine.value =
			realloc(dict->dfine.value, dict->dfine.size * sizeof(char *));
		dict->dfine.name =
			realloc(dict->dfine.name, dict->dfine.size * sizeof(char *));
		assert(dict->dfine.size == (unsigned int)id,
		       "\"dfine\" array size inconsistency");
		dict->dfine.name[id - 1] = string_set_add(name, dict->string_set);
	}
	dict->dfine.value[id - 1] = string_set_add(value, dict->string_set);
}

static bool is_directive(const char *s)
{
	return
		(strcmp(s, UNLIMITED_CONNECTORS_WORD) == 0) ||
		(strncmp(s, LIMITED_CONNECTORS_WORD, sizeof(LIMITED_CONNECTORS_WORD)-1) == 0);
}

static bool is_correction(const char *s)
{
	static const char correction_mark[] = { SUBSCRIPT_MARK, '#' , '\0'};
	return strstr(s, correction_mark) != 0;
}

void add_category(Dictionary dict, Exp *e, Dict_node *dn, int n)
{
	if (n == 1)
	{
		if (is_macro(dn->string)) return;
		if (!dict->generate_walls && is_wall(dn->string)) return;
		if (is_correction(dn->string)) return;
		if (is_directive(dn->string)) return;
	}

	/* Add a category with a place for n words. */
	dict->num_categories++;
	if (dict->num_categories >= dict->num_categories_alloced)
	{
		dict->num_categories_alloced *= 2;
		dict->category =
			realloc(dict->category,
			        sizeof(*dict->category) * dict->num_categories_alloced);
	}
	dict->category[dict->num_categories].word =
		malloc(sizeof(*dict->category[0].word) * n);

	n = 0;
	for (Dict_node *dnx = dn; dnx != NULL; dnx = dnx->left)
	{
		if (is_macro(dnx->string)) continue;
		if (!dict->generate_walls && is_wall(dnx->string)) continue;
		if (is_correction(dnx->string)) continue;
		if (is_directive(dnx->string)) return;
		dict->category[dict->num_categories].word[n] = dnx->string;
		n++;
	}

	if (n == 0)
	{
		free(dict->category[dict->num_categories].word);
		--dict->num_categories;
	}
	else
	{
		assert(dict->num_categories < 1024 * 1024, "Insane number of categories");
		char category_string[16]; /* For the tokenizer - not used here */
		snprintf(category_string, sizeof(category_string), " %x",
		         dict->num_categories);
		string_set_add(category_string, dict->string_set);
		dict->category[dict->num_categories].exp = e;
		dict->category[dict->num_categories].num_words = n;
		dict->category[dict->num_categories].name = "";
	}
}

void print_dictionary_defines(Dictionary dict)
{
	#define SPECIAL "(){};[]&^|:"
	for (size_t i = 0; i < dict->dfine.size; i++)
	{
		const char *value = dict->dfine.value[i];
		int q = (int)(strcspn(value, SPECIAL) == strlen(value));
		printf("#define %s %s%s%s\n",
		       dict->dfine.name[i], &"\""[q], value, &"\""[q]);
	}
}

static void rprint_dictionary_data(Dict_node * n)
{
	if (n == NULL) return;
	rprint_dictionary_data(n->left);
	printf("%s: %s\n", n->string, exp_stringify(n->exp));
	rprint_dictionary_data(n->right);
}

/**
 * Dump the entire contents of the dictionary
 * XXX This is not currently called by anything, but is a "good thing
 * to keep around".
 */
void print_dictionary_data(Dictionary dict)
{
	rprint_dictionary_data(dict->root);
}

/* ======================================================================= */
