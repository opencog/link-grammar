/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2013, 2014 Linas Vepstas                                */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _LG_DICT_COMMON_H_
#define  _LG_DICT_COMMON_H_

#include "api-types.h"                  // pp_knowledge
#include "connectors.h"                 // ConTable
#include "dict-defines.h"
#include "dict-structures.h"
#include "dict-ram/dict-ram.h"
#include "memory-pool.h"                // Pool_desc
#include "utilities.h"                  // locale_t


// Dict may have `#define empty-connector ZZZ` in it.
#define EMPTY_CONNECTOR "empty-connector"
#define UNLIMITED_CONNECTORS_WORD ("UNLIMITED-CONNECTORS")
#define LIMITED_CONNECTORS_WORD ("LENGTH-LIMIT-")
#define IS_GENERATION(dict) (dict->category != NULL)

/* If the maximum disjunct cost is not initialized, the value defined
 * in the dictionary is used. If not defined, then DEFAULT_MAX_DISJUNCT_COST
 * is used. */
static const float UNINITIALIZED_MAX_DISJUNCT_COST = -10000.0f;
static const float DEFAULT_MAX_DISJUNCT_COST = 2.7f;
static const float UNINITIALIZED_MAX_DISJUNCTS = -1;

/* We need some of these as literal strings. */
#define LG_DICTIONARY_VERSION_NUMBER            "dictionary-version-number"
#define LG_DICTIONARY_LOCALE                    "dictionary-locale"
#define LG_DISABLE_DOWNCASING                   "disable-downcasing"
#define LG_DISJUNCT_COST                        "max-disjunct-cost"
#define LG_MAX_DISJUNCTS                        "max-disjuncts"

/* Forward decls */
typedef struct Afdict_class_struct Afdict_class;
typedef struct Regex_node_s Regex_node;

/* The regexes are stored as a linked list of the following nodes. */
struct Regex_node_s
{
	const char *name;/* The identifying name of the regex */
	char *pattern;   /* The regular expression pattern */
	void *re;        /* The compiled regex. void * to avoid
	                    having re library details invading the
	                    rest of the LG system; regex-morph.c
	                    takes care of all matching.
	                  */
	Regex_node *next;
	bool neg;        /* Negate the match */
	int capture_group;  /* Capture group number (-1 if none) for ovector */
};

static inline Regex_node *regex_new(const char *name, const char *pattern)
{
	Regex_node *rn = (Regex_node *)malloc(sizeof(Regex_node));

	rn->name = name;
	rn->pattern = strdup(pattern);
	rn->re = NULL;
	rn->neg = false;
	rn->capture_group = -1;
	rn->next = NULL;

	return rn;
}

struct Afdict_class_struct
{
	uint16_t mem_elems;     /* number of memory elements allocated */
	uint16_t length;        /* number of elements */
	uint16_t Nregexes;      /* number of regexes */
	const char ** string;   /* indexed by [0..length) */
	Regex_node ** regex;    /* indexed by [0..Nregexes) */
};

#define MAX_TOKEN_LENGTH 250     /* Maximum number of chars in a token */
#define IDIOM_LINK_SZ 16

#if defined HAVE_SQLITE3 || defined HAVE_ATOMESE
#define IS_DYNAMIC_DICT(dict) dict->dynamic_lookup
#else
#define IS_DYNAMIC_DICT(dict) false
#endif /* HAVE_SQLITE3 or HAVE_ATOMESE */

#ifdef HAVE_SQLITE3
#define IS_SQL_DICT(dict) (NULL != dict->db_handle)
#else
#define IS_SQL_DICT(dict) false
#endif /* HAVE_SQLITE3 */

/* "#define name value" */
typedef struct
{
	String_id *set;                    /* "define" name set */
	const char **name;
	const char **value;
	unsigned int size;                 /* Allocated value array size */
} dfine_s;

typedef struct
{
	String_id *set;                    /* Expression tag names */
	const char **name;                 /* Tag name (indexed by tag id) */
	unsigned int num;                  /* Number of tags */
	unsigned int size;                 /* Allocated tag array size */
} expression_tag;

struct Dictionary_s
{
	Dict_node *  root;
	Regex_node * regex_root;
	const char * name;
	const char * lang;
	const char * version;
	const char * locale;    /* Locale name */
	locale_t     lctype;    /* Locale argument for the *_l() functions */

	int          num_entries;
	dfine_s      dfine;    /* Name-value definitions */

	/* Parse options for which defaults are provided by the dictionary. */
	float        default_max_disjunct_cost; /* Dictionary-specific scale. */
	int          default_max_disjuncts;     /* Max number of disjuncts. */

	const char * zzz_connector;

	/* Dictionary-defined parameters. Control behavior of how
	 * words are looked up in the dictionary. */
	bool         use_unknown_word;
	bool         unknown_word_defined;
	bool         left_wall_defined;
	bool         right_wall_defined;
	bool         shuffle_linkages;
	bool         dynamic_lookup;
	bool         disable_downcasing;

	/* Duplicate words are disallowed in 4.0.dict unless
	 * allow_duplicate_words is defined to "true".
	 * Duplicate idioms are allowed, unless the "test" parse option
	 * is set to "disalow-dup-idioms" (listing them for debug).
	 * If these variables are 0, they get their allow/disallow values
	 * when the first duplicate word/idiom is encountered.
	 * 0: not set; 1: allow; -1: disallow */
	int8_t       allow_duplicate_words;
	int8_t       allow_duplicate_idioms;

	Dialect *dialect;                  /* "4.0.dialect" info */
	expression_tag dialect_tag;        /* Expression dialect tag info */
	expression_tag *macro_tag;         /* Macro tags for expression debug */
	void *cached_dialect;              /* Only for dialect cache validation */

	/* Affixes are used during the tokenization stage. */
	Dictionary      affix_table;
	Afdict_class *  afdict_class;
	bool pre_suf_class_exists;         /* True iff PRE or SUF exists */

	/* Random morphology generator */
	struct anysplit_params * anysplit;

	/* If not null, then use spelling guesser for unknown words */
	void *          spell_checker;     /* spell checker handle */
#ifdef HAVE_SQLITE3
	void *          db_handle;         /* database handle */
#endif
#ifdef HAVE_ATOMESE
	void *          as_server;         /* cogserver connection */
#endif

	void (*insert_entry)(Dictionary, Dict_node *, int);

	void (*start_lookup)(Dictionary, Sentence);
	void (*end_lookup)(Dictionary, Sentence);
	Dict_node* (*lookup_list)(Dictionary, const char*);
	Dict_node* (*lookup_wild)(Dictionary, const char*);
	void (*free_lookup)(Dictionary, Dict_node*);
	bool (*exists_lookup)(Dictionary, const char*);

	void (*clear_cache)(Dictionary);
	void (*close)(Dictionary);

	String_set *    string_set;        /* Set of link names in the dictionary */
	Word_file *     word_file_header;
	ConTable        contable;
	Pool_desc *     Exp_pool;

	/* Post-processing */
	pp_knowledge  * base_knowledge;    /* Core post-processing rules */
	pp_knowledge  * hpsg_knowledge;    /* Head-Phrase Structure rules */

	/* Sentence generation */
	unsigned int num_categories;
	unsigned int num_categories_alloced;
	Category * category;      /* Word lists - indexed by category number */
	bool generate_walls;      /* Generate walls too for wildcard words */

	/* File I/O cruft */
	int             line_number;
	char            current_idiom[IDIOM_LINK_SZ];
};

bool is_stem(const char *);
bool is_wall(const char *);
bool is_macro(const char *);

bool dictionary_generation_request(const Dictionary);

/* The functions here are intended for use by the tokenizer, only,
 * and pretty much no one else. If you are not the tokenizer, you
 * probably don't need these. */
bool dict_has_word(const Dictionary dict, const char *);

static inline const char *subscript_mark_str(void)
{
	static const char sm[] = { SUBSCRIPT_MARK, '\0' };
	return sm;
}

/**
 * Get the subscript of the given word.
 * In case of more than one subscript, return the last subscript.
 * The returned value constness is the same as the argument.
 */
static inline char *get_word_subscript(const char *word)
{
	return (char *)strrchr(word, SUBSCRIPT_MARK); /* (char *) for MSVC */
}
#define get_word_subscript(word) _Generic((word), \
	const char * : (const char *)(get_word_subscript)((word)), \
	char *       :               (get_word_subscript)((word)))
#endif /* _LG_DICT_COMMON_H_ */
