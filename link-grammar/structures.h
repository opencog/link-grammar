/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2013 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _STRUCTURES_H_
#define _STRUCTURES_H_

#include "api-types.h"
#include "api-structures.h"
#include "dict-structures.h"  /* For Exp, Exp_list */
#include "histogram.h"  /* Count_bin */
#include "utilities.h"  /* Needed for inline defn in Windows */


#define NEGATIVECOST -1000000
/* This is a hack that allows one to discard disjuncts containing
   connectors whose cost is greater than given a bound. This number plus
   the cost of any connectors on a disjunct must remain negative, and
   this number multiplied times the number of costly connectors on any
   disjunct must fit into an integer. */

/* Upper bound on the cost of any connector. */
#define MAX_CONNECTOR_COST 1000.0f

/* The following define the names of the special strings in the dictionary. */
#define LEFT_WALL_WORD   ("LEFT-WALL")
#define RIGHT_WALL_WORD  ("RIGHT-WALL")

/* Word subscripts come after the subscript mark (ASCII ETX)
 * In the dictionary, a dot is used; but that dot interferes with dots
 * in the input stream, and so we convert dictionary dots into the
 * subscript mark, which we don't expect to see in user input.
 */
#define SUBSCRIPT_MARK '\3'
#define SUBSCRIPT_DOT '.'
#define EMPTY_WORD_DOT   "EMPTY-WORD.zzz"  /* Has SUBSCRIPT_DOT in it! */
#define EMPTY_WORD_MARK  "EMPTY-WORD\3zzz" /* Has SUBSCRIPT_MARK in it! */
#define EMPTY_WORD_DISPLAY "∅"   /* Empty word representation for debug */

/* Dictionary capitalization handling */
#define CAP1st "1stCAP" /* Next word is capitalized */
#define CAPnon "nonCAP" /* Next word the lc version of a capitalized word */

/* Stems, by definition, end with ".=x" (when x is usually an empty string, i.e.
 * ".="). The STEMSUBSCR definition in the affix file may include endings with
 * other x values, when x serves as a word subscript, e.g. ".=a".  */
#define STEM_MARK '='

/* Suffixes start with it.
 * This is needed to distinguish suffixes that were stripped off from
 * ordinary words that just happen to be the same as the suffix.
 * Kind-of a weird hack, but I'm not sure what else to do...
 * Similarly, prefixes end with it.
 */
#define INFIX_MARK(afdict) \
 ((NULL == afdict) ? '\0' : (AFCLASS(afdict, AFDICT_INFIXMARK)->string[0][0]))

#define UNLIMITED_CONNECTORS_WORD ("UNLIMITED-CONNECTORS")

#define UNKNOWN_WORD "UNKNOWN-WORD"

#define MAX_PATH_NAME 200     /* file names (including paths)
                                 should not be longer than this */

/*      Some size definitions.  Reduce these for small machines */
/* MAX_WORD is large, because Unicode entries can use a lot of space */
#define MAX_WORD 180          /* maximum number of bytes in a word */
#define MAX_LINE 2500         /* maximum number of chars in a sentence */

/* conditional compiling flags */
#define INFIX_NOTATION
    /* If defined, then we're using infix notation for the dictionary */
    /* otherwise we're using prefix notation */

#define UNLIMITED_LEN 255
#define SHORT_LEN 6
#define NO_WORD 255

/* On a 64-bit machine, this struct should be exactly 4*8=32 bytes long.
 * Lets try to keep it that way.
 */
struct Connector_struct
{
	int hash;
	unsigned short word;
	             /* The nearest word to my left (or right) that
	                this could connect to.  Computed by power pruning */
	unsigned char length_limit;
	             /* If this is a length limited connector, this
	                gives the limit of the length of the link
	                that can be used on this connector.  Since
	                this is strictly a function of the connector
	                name, efficiency is the only reason to store
	                this.  If no limit, the value is set to 255. */
	bool multi;  /* TRUE if this is a multi-connector */
	Connector * next;
	const char * string;  /* The connector name, e.g. AB+ */

	/* Hash table next pointer, used only during pruning. */
	Connector * tableNext;
};

static inline void connector_set_string(Connector *c, const char *s)
{
	c->string = s;
	c->hash = -1;
}
static inline const char * connector_get_string(Connector *c)
{
	return c->string;
}

struct Disjunct_struct
{
	Disjunct *next;
	const char * string;       /* subscripted dictionary word */
	Connector *left, *right;
	double cost;
	bool marked;               /* unmarked disjuncts get deleted */
	const Gword **word;        /* NULL terminated list of originating words */
};

typedef struct Match_node_struct Match_node;
struct Match_node_struct
{
	Match_node * next;
	Disjunct * d;
};

typedef struct X_node_struct X_node;
struct X_node_struct
{
	const char * string;       /* the word itself */
	Exp * exp;
	X_node *next;
	const Gword *word;         /* originating Wordgraph word */
};

/**
 * Word, as represented shortly after tokenization, but before parsing.
 *
 * X_node* x:
 *    Contains a pointer to a list of expressions from the dictionary,
 *    Computed by build_sentence_expressions().
 *
 * Disjunct* d:
 *   Contains a pointer to a list of disjuncts for this word.
 *   Computed by: prepare_to_parse(), but modified by pruning and power
 *   pruning.
 */
struct Word_struct
{
	const char *unsplit_word;

	X_node * x;          /* Sentence starts out with these, */
	Disjunct * d;        /* eventually these get generated. */

	const char **alternatives;
};

typedef enum
{
	MT_INVALID,            /* Initial value, to be changed to the correct type */
	MT_WORD,               /* Regular word */
	MT_FEATURE,            /* Pseudo morpheme, currently capitalization marks */
	MT_INFRASTRUCTURE,     /* Start and end Wordgraph pseudo-words */
	MT_WALL,               /* The LEFT-WALL and RIGHT-WALL pseudo-words */
	MT_EMPTY,              /* Empty word */
	MT_UNKNOWN,            /* Unknown word (FIXME? Unused) */
	/* Experimental for Semitic languages (yet unused) */
	MT_TEMPLATE,
	MT_ROOT,
	/* Experimental - for display purposes.
	 * MT_CONTR is now used in the tokenization step, see the comments there. */
	MT_CONTR,              /* Contracted part of a contraction (e.g. y', 's) */
	MT_PUNC,               /* Punctuation (yet unused) */
	/* We are not going to have >63 types up to here. */
	MT_STEM    = 1<<6,     /* Stem */
	MT_PREFIX  = 1<<7,     /* Prefix */
	MT_MIDDLE  = 1<<8,     /* Middle morpheme (yet unused) */
	MT_SUFFIX  = 1<<9      /* Suffix */
} Morpheme_type;
#define IS_REG_MORPHEME (MT_STEM|MT_PREFIX|MT_MIDDLE|MT_SUFFIX)

/* Word status */
/* - Tokenization */
#define WS_UNKNOWN   1<<0 /* Unknown word (FIXME? Unused) */
#define WS_REGEX     1<<1 /* Matches a regex */
#define WS_SPELL     1<<2 /* Result of a spell guess */
#define WS_RUNON     1<<3 /* Separated from words run-on */
#define WS_HASALT    1<<4 /* Has alternatives (one or more)*/
#define WS_UNSPLIT   1<<5 /* It's an alternative to itself as an unsplit word */
#define WS_INDICT    1<<6 /* boolean_dictionary_lookup() is true */
#define WS_FIRSTUPPER 1<<7 /* Subword is the lc version of its unsplit_word.
                              The idea of marking subwords this way, in order to
                              enable restoring their original capitalization,
                              may be wrong in general, since in some languages
                              the process is not always reversible. Instead,
                              the original word may be saved. */
/* - Post linkage stage. XXX Experimental. */
#define WS_PL        1<<14 /* Post-Linkage, not belonging to tokenization */

#define WS_GUESS (WS_SPELL|WS_RUNON|WS_REGEX)

/* XXX Only TS_DONE is now actually used.
 * FIXME: Change TS_DONE to WS_TDONE, or
 * change Tokenizing_step to "bool tokenizing_step". */

typedef enum
{
	TS_INITIAL,
	TS_LR_STRIP,
	TS_AFFIX_SPLIT,
	TS_REGEX,
	TS_RUNON,
	TS_SPELL,
	TS_DONE                  /* Tokenization done */
} Tokenizing_step;

/* For the "guess" field of Gword_struct. */
typedef enum
{
	GM_REGEX = '!',
	GM_SPELL = '~',
	GM_RUNON = '&',
	GM_UNKNOWN = '?'
} Guess_mark;

#define MAX_SPLITS 10   /* See split_counter below */

struct Gword_struct
{
	const char *subword;

	Gword *unsplit_word; /* Upward-going co-tree */
	Gword **next;        /* Right-going tree */
	Gword **prev;        /* Left-going tree */
	Gword *chain_next;   /* Next word in the chain of all words */

	/* For debug and inspiration. */
	const char *label;   /* Debug label - code locations of tokenization */
	size_t node_num;     /* For differentiating words with identical subwords,
	                        and for indicating the order in which word splits
                           have been done. Shown in the Wordgraph display and in
                           debug messages. Not used otherwise. Could have been
                           used for hier_position instead of pointers in order
                           to optimize its generation and comparison. */

	/* Tokenizer state */
	Tokenizing_step tokenizing_step;
	bool issued_unsplit; /* The word has been issued as an alternative to itself.
	                        It will become an actual alternative to itself only
	                        if it's not the sole alternative, in which case it
	                        will be marked with WS_UNSPLIT. */
	size_t split_counter; /* Incremented on splits. A word cannot split more than
	                         MAX_SPLITS times and a warning is issued then. */

	unsigned int status;         /* See WS_* */
	Morpheme_type morpheme_type; /* See MT_* */
	Gword *alternative_id;       /* Alternative start - a unique identifier of
	                                the alternative to which the word belongs. */
	const char *regex_name;      /* Subword matches this regex.
                                   FIXME? Extend for multiple regexes. */

	/* Only used by wordgraph_flatten() */
	const Gword **hier_position; /* Unsplit_word/alternative_id pointer list, up
                                   to the original sentence word. */
	size_t hier_depth;           /* Number of pointer pairs in hier_position */

	/* XXX Experimental. Only used after the linkage (by compute_chosen_words())
	 * for an element in the linkage display wordgraph path that represents
	 * a block of null words that are morphemes of the same word. */
	Gword **null_subwords;       /* Null subwords represented by this word */
};

/* Wordgraph path word-positions,
 * used in wordgraph_flatten() and sane_linkage_morphism().
 * FIXME Separate to two different structures. */
struct Wordgraph_pathpos_s
{
	Gword *word;      /* Position in the Wordgraph */
	/* Only for wordgraph_flatten(). */
	bool same_word;   /* Still the same word - issue an empty word */
	bool next_ok;     /* OK to proceed to the next Wordgraph word */
	bool used;        /* Debug - the word has been issued */
	/* Only for sane_morphism(). */
	const Gword **path; /* Linkage candidate wordgraph path */
};

/* The regexes are stored as a linked list of the following nodes. */
struct Regex_node_s
{
	char *name;      /* The identifying name of the regex */
	char *pattern;   /* The regular expression pattern */
	void *re;        /* The compiled regex. void * to avoid
	                    having re library details invading the
	                    rest of the LG system; regex-morph.c
	                    takes care of all matching.
	                  */
	Regex_node *next;
};

/* The following three structs comprise what is returned by post_process(). */
typedef struct D_type_list_struct D_type_list;
struct D_type_list_struct
{
	D_type_list * next;
	int type;
};

struct PP_node_struct
{
	size_t dtsz;
	D_type_list **d_type_array;
	const char *violation;
};

/* Davy added these */
struct List_o_links_struct
{
	size_t link;     /* the link number */
	size_t word;     /* the word at the other end of this link */
	signed char dir; /* 0: undirected, 1: away from me, -1: toward me */
	List_o_links * next;
};

typedef struct Parse_choice_struct Parse_choice;

struct Parse_choice_struct
{
	Parse_choice * next;
	Parse_set * set[2];
	Link        link[2];   /* the lc fields of these is NULL if there is no link used */
	Disjunct *ld, *md, *rd;  /* the chosen disjuncts for the relevant three words */
};

struct Parse_set_struct
{
	s64 count;    /* the number of ways */
	s64 recount;  /* count, but only up to the cutoff */
	// double cost_cutoff;
	Parse_choice * first;
	Parse_choice * tail;
};

struct X_table_connector_struct
{
	short             lw, rw;
	unsigned short    null_count; /* number of island words */
	Parse_set         set;
	Connector         *le, *re;
	X_table_connector *next;
};

/* from pp_linkset.c */
typedef struct pp_linkset_node_s
{
	const char *str;
	struct pp_linkset_node_s *next;
} pp_linkset_node;

struct pp_linkset_s
{
	unsigned int hash_table_size;
	unsigned int population;
	pp_linkset_node **hash_table;    /* data actually lives here */
};


/* from pp_lexer.c */
#define PP_LEXER_MAX_LABELS 512

typedef struct pp_label_node_s
{
	/* linked list of strings associated with a label in the table */
	const char *str;
	struct pp_label_node_s *next;
} pp_label_node;                 /* next=NULL: end of list */


/* from pp_knowledge.c */
typedef struct StartingLinkAndDomain_s
{
	const char *starting_link;
	int   domain;       /* domain which the link belongs to (-1: terminator)*/
} StartingLinkAndDomain;

typedef struct pp_rule_s
{
	/* Holds a single post-processing rule. Since rules come in many
	   flavors, not all fields of the following are always relevant  */
	const char *selector; /* name of link to which rule applies      */
	pp_linkset *link_set; /* handle to set of links relevant to rule */
	int   link_set_size;  /* size of this set                        */
	int   domain;         /* type of domain to which rule applies    */
	const char  **link_array; /* array holding the spelled-out names */
	const char  *msg;     /* explanation (NULL=end sentinel in array)*/
	int use_count;        /* Number of times rule has been applied   */
} pp_rule;

typedef struct PPLexTable_s PPLexTable;
struct pp_knowledge_s
{
	PPLexTable *lt; /* Internal rep'n of sets of strings from knowledge file */
	const char *path; /* Name of file we loaded from */

	/* handles to sets of links specified in knowledge file. These constitute
	   auxiliary data, necessary to implement the rules, below. See comments
	   in post-process.c for a description of these. */
	pp_linkset *domain_starter_links;
	pp_linkset *urfl_domain_starter_links;
	pp_linkset *urfl_only_domain_starter_links;
	pp_linkset *domain_contains_links;
	pp_linkset *must_form_a_cycle_links;
	pp_linkset *restricted_links;
	pp_linkset *ignore_these_links;
	pp_linkset *left_domain_starter_links;

	/* arrays of rules specified in knowledge file */
	pp_rule *form_a_cycle_rules;
	pp_rule *contains_one_rules;
	pp_rule *contains_none_rules;
	pp_rule *bounded_rules;

	size_t n_form_a_cycle_rules;
	size_t n_contains_one_rules;
	size_t n_contains_none_rules;
	size_t n_bounded_rules;

	size_t nStartingLinks;
	pp_linkset *set_of_links_starting_bounded_domain;
	StartingLinkAndDomain *starting_link_lookup_table;
	String_set *string_set;
};

#endif
