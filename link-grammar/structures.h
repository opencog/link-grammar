/********************************************************************************/
/* Copyright (c) 2004                                                           */
/* Daniel Sleator, David Temperley, and John Lafferty                           */
/* All rights reserved                                                          */
/*                                                                              */
/* Use of the link grammar parsing system is subject to the terms of the        */
/* license set forth in the LICENSE file included with this software,           */ 
/* and also available at http://www.link.cs.cmu.edu/link/license.html           */
/* This license allows free redistribution and use in source and binary         */
/* forms, with or without modification, subject to certain conditions.          */
/*                                                                              */
/********************************************************************************/

#ifndef _STRUCTURESH_
#define _STRUCTURESH_

#if defined(WIN32)
#define strncasecmp(a,b,s) strnicmp((a),(b),(s))
#endif

#if defined(__sun__)
int strncasecmp(const char *s1, const char *s2, size_t n);
/* This does not appear to be in string.h header file in sunos
   (Or in linux when I compile with -ansi) */
#endif

/*
 Global variable descriptions
  -- Most of these global variables have been eliminated.
     I've lefe this comment here for historical purposes --DS 4/98

 N_words:
    The number of words in the current sentence.  Computed by
    separate_sentence().

 N_links:
    The number of links in the current linkage.  Computed by
    extract_linkage().

 sentence[].string:
    Contains a slightly modified form of the words typed by the user.
    Computed by separate_sentence().

 sentence[].x:
    Contains, for each word, a pointer to a list of expressions from the
    dictionary that match the word in sentence[].string.
    Computed by build_sentence_expressions().

 sentence[].d
    Contains for each word, a pointer to a list of disjuncts for this word.
    Computed by: parepare_to_parse(), but modified by pruning and power
    pruning.
 
 link_array[]
    This is an array of links.  These links define the current linkage.
    It is computed by extract_links().  It is used by analyze_linkage() to
    compute pp_linkage[].  It may contain fat links.

 pp_link_array[]   ** eliminated (ALB) 
    Another array of links.  Here all fat links have been expunged.
    It is computed by analyze_linkage(), and used by post_process() and by
    print_links();   

 chosen_disjuncts[]
    This is an array pointers to disjuncts, one for each word, that is
    computed by extract_links().  It represents the chosen disjuncts for the
    current linkage.  It is used to compute the cost of the linkage, and
    also by compute_chosen_words() to compute the chosen_words[].

 chosen_words[]
    An array of pointers to strings.  These are the words to be displayed
    when printing the solution, the links, etc.  Computed as a function of
    chosen_disjuncts[] by compute_chosen_words().  This differs from
    sentence[].string because it contains the suffixes.  It differs from
    chosen_disjunct[].string in that the idiom symbols have been removed.

 has_fat_down[]
    An array of chars, one for each word.  TRUE if there is a fat link
    down from this word, FALSE otherwise.  (Only set if there is at least
    one fat link.)  Set by set_has_fat_down_array() and used by
    analyze_linkage() and is_canonical().

 is_conjunction[]
    An array of chars, one for each word.  TRUE if the word is a conjunction
    ("and", "or", "nor", or "but" at the moment).  False otherwise. */                                                         
#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#define OBS

#define assert(ex,string) {                                               \
    if (!(ex)) {							  \
	printf("Assertion failed: %s\n", string); 		          \
	exit(1);                            				  \
    }									  \
}

#define NEGATIVECOST -1000000
/* This is a hack that allows one to discard disjuncts containing
   connectors whose cost is greater than given a bound. This number plus
   the cost of any connectors on a disjunct must remain negative, and
   this number multiplied times the number of costly connectors on any
   disjunct must fit into an integer. */

#define NOCUTOFF 1000
/* no connector will have cost this high */

#if !defined(MIN)
#define MIN(X,Y)  ( ((X) < (Y)) ? (X) : (Y))
#endif
#if !defined(MAX)
#define MAX(X,Y)  ( ((X) > (Y)) ? (X) : (Y))
#endif

#define LEFT_WALL_DISPLAY  ("LEFT-WALL")   /* the string to use to show the wall */
#define LEFT_WALL_SUPPRESS ("Wd") /* If this connector is used on the wall, */
				  /* then suppress the display of the wall  */
                                  /* bogus name to prevent ever suppressing */
#define RIGHT_WALL_DISPLAY  ("RIGHT-WALL")   /* the string to use to show the wall */
#define RIGHT_WALL_SUPPRESS ("RW") /* If this connector is used on the wall, */

/* The following define the names of the special strings in the dictionary. */
#define LEFT_WALL_WORD   ("LEFT-WALL")
#define RIGHT_WALL_WORD  ("RIGHT-WALL")
#define POSTPROCESS_WORD ("POSTPROCESS")
#define ANDABLE_CONNECTORS_WORD ("ANDABLE-CONNECTORS")
#define UNLIMITED_CONNECTORS_WORD ("UNLIMITED-CONNECTORS")

/*
OBS #define COMMA_WORD       ("COMMA")
OBS #define COLON_WORD       ("COLON")
OBS #define SEMICOLON_WORD   ("SEMI-COLON")
OBS #define PERCENT_WORD     ("PERCENT-SIGN")
OBS #define DOLLAR_WORD      ("DOLLAR-SIGN")
OBS #define LP_WORD          ("LEFT-PAREN")
OBS #define RP_WORD          ("RIGHT-PAREN")
OBS #define AMPERSAND_WORD   ("AMPERSAND")
*/

#define PROPER_WORD      ("CAPITALIZED-WORDS")
#define PL_PROPER_WORD      ("PL-CAPITALIZED-WORDS")
#define HYPHENATED_WORD  ("HYPHENATED-WORDS")
#define NUMBER_WORD      ("NUMBERS")
#define ING_WORD         ("ING-WORDS")
#define S_WORD           ("S-WORDS")
#define ED_WORD          ("ED-WORDS")
#define LY_WORD          ("LY-WORDS")
#define UNKNOWN_WORD     ("UNKNOWN-WORD")

#define MAX_PATH_NAME 200     /* file names (including paths)
				 should not be longer than this */

/*      Some size definitions.  Reduce these for small machines */
#define MAX_WORD 60           /* maximum number of chars in a word */
#define MAX_LINE 1500         /* maximum number of chars in a sentence */
#define MAX_SENTENCE 250      /* maximum number of words in a sentence */
   /* This cannot be more than 254, because I use word MAX_SENTENCE+1 to
      indicate that nothing can connect to this connector, and this
      should fit in one byte (if the word field of a connector is an
      unsigned char) */
#define MAX_LINKS (2*MAX_SENTENCE-3) /* maximum number of links allowed */
#define MAX_TOKEN_LENGTH 50          /* maximum number of chars in a token */
#define MAX_DISJUNCT_COST 10000

/* conditional compiling flags */
#define PLURALIZATION
      /* If defined, Turns on the pluralization operation in        */
      /* "and", "or" and "nor" */
#define INFIX_NOTATION
      /* If defined, then we're using infix notation for the dictionary */
      /* otherwise we're using prefix notation */

#define DOWN_priority 2
#define UP_priority   1
#define THIN_priority 0

#define NORMAL_LABEL  (-1) /* used for normal connectors            */
                           /* the labels >= 0 are used by fat links */

#define UNLIMITED_LEN 255
#define SHORT_LEN 6
#define NO_WORD 255

typedef struct Connector_struct Connector;
struct Connector_struct{
    short label;
    unsigned char word;
                   /* The nearest word to my left (or right) that
                      this could connect to.  Computed by power pruning */
    unsigned char length_limit; 
                  /* If this is a length limited connector, this
		     gives the limit of the length of the link
		     that can be used on this connector.  Since
		     this is strictly a funcion of the connector
		     name, efficiency is the only reason to store
		     this.  If no limit, the value is set to 255. */
  /*    unsigned char my_word; */  /* not used now */
                  /* The word that this connector arises from */
    char priority;/* one of the three priorities above */
    char multi;   /* TRUE if this is a multi-connector */
    Connector * next;
    char * string;
};

typedef struct Disjunct_struct Disjunct;
struct Disjunct_struct {
    Disjunct *next;
    short cost;
    char marked;
    char * string;
    Connector *left, *right;
};

typedef struct Link_s * Link;
struct Link_s {
    int l, r;
    Connector * lc, * rc;
    char * name;              /* spelling of full link name */
};

typedef struct Match_node_struct Match_node;
struct Match_node_struct {
    Match_node * next;
    Disjunct * d;
};

typedef struct Exp_struct Exp;
typedef struct X_node_struct X_node;
struct X_node_struct {
    char * string;  /* the word itself */
    Exp * exp;
    X_node *next;
};

typedef struct Word_struct Word;
struct Word_struct {
    char string[MAX_WORD+1];
    X_node * x;      /* sentence starts out with these */
    Disjunct * d;    /* eventually these get generated */
    int firstupper;
};

/* The E_list an Exp structures defined below comprise the expression      */
/* trees that are stored in the dictionary.  The expression has a type     */
/* (AND, OR or TERMINAL).  If it is not a terminal it has a list           */
/* (an E_list) of children.                                                */

typedef struct E_list_struct E_list;

struct Exp_struct {
    char type;            /* one of three types, see below */
    unsigned char cost;   /* The cost of using this expression.
			     Only used for non-terminals */
    char dir;   /* '-' means to the left, '+' means to right (for connector) */
    char multi; /* TRUE if a multi-connector (for connector)  */
    union {
	E_list * l;       /* only needed for non-terminals */
	char * string;    /* only needed if it's a connector */
    }u;
    Exp * next;
};

struct E_list_struct {
    E_list * next;
    Exp * e;
};

/* Here are the types */
#define OR_type 0
#define AND_type 1
#define CONNECTOR_type 2

/* The structure below stores a list of dictionary word files. */

typedef struct Word_file_struct Word_file;
struct Word_file_struct {
    char file[MAX_PATH_NAME+1];   /* the file name */
    int changed;             /* TRUE if this file has been changed */
    Word_file * next;
};

/* The dictionary is stored as a binary tree comprised of the following   */
/* nodes.  A list of these (via right pointers) is used to return         */
/* the result of a dictionary lookup.                                     */

typedef struct Dict_node_struct Dict_node;
struct Dict_node_struct {
    char      * string;  /* the word itself */
    Word_file * file;    /* the file the word came from (NULL if dict file) */
    Exp       * exp;     
    Dict_node *left, *right;
};

/* The following three structs comprise what is returned by post_process(). */
typedef struct D_type_list_struct D_type_list;
struct D_type_list_struct {
    D_type_list * next;
    int type;
};
 
typedef struct PP_node_struct PP_node;
struct PP_node_struct {
    D_type_list *d_type_array[MAX_LINKS];
    char *violation;
};

/* Davy added these */

typedef struct Andlist_struct Andlist;
struct Andlist_struct {
    Andlist * next;
    int conjunction;
    int num_elements;
    int element[MAX_SENTENCE];
    int num_outside_words;
    int outside_word[MAX_SENTENCE];
    int cost;
};

/* This is for building the graphs of links in post-processing and          */
/* fat link extraction.                                                     */

typedef struct Linkage_info_struct Linkage_info;
struct Linkage_info_struct {
    int index;
    char fat;
    char canonical;
    char improper_fat_linkage;
    char inconsistent_domains;
    short N_violations, null_cost, unused_word_cost, disjunct_cost, and_cost, link_cost;
    Andlist * andlist;
    int island[MAX_SENTENCE];
};

typedef struct List_o_links_struct List_o_links;
struct List_o_links_struct{
    int link;       /* the link number */
    int word;       /* the word at the other end of this link */
    int dir;        /* 0: undirected, 1: away from me, -1: toward me */
    List_o_links * next;
};

#define GENTLE 1
#define RUTHLESS 0
/* These parameters tell power_pruning, to tell whether this is before or after
   generating and disjuncts.  GENTLE is before RUTHLESS is after. */

/* int srandom(int);
int random(void); */


typedef struct string_node_struct String_node;
struct string_node_struct {
    char * string;
    int size;
    String_node * next;
};

typedef struct Parse_choice_struct Parse_choice;
typedef struct Parse_set_struct Parse_set;

struct Parse_choice_struct {
    Parse_choice * next;
    Parse_set * set[2];
    struct Link_s link[2];   /* the lc fields of these is NULL if there is no link used */
    Disjunct *ld, *md, *rd;  /* the chosen disjuncts for the relevant three words */
};
struct Parse_set_struct {
    int count;  /* the number of ways */
    Parse_choice * first;
    Parse_choice * current;  /* used to enumerate linkages */
};

typedef struct X_table_connector_struct X_table_connector;
struct X_table_connector_struct {
    short             lw, rw;
    short             cost;
    Parse_set         *set;
    Connector         *le, *re;
    X_table_connector *next;
};

/* from string-set.c */
typedef struct {
    int size;       /* the current size of the table */
    int count;      /* number of things currently in the table */
    char ** table;  /* the table itself */
} String_set;


/* from pp_linkset.c */
typedef struct pp_linkset_node_s
{   
  char *str;
  struct pp_linkset_node_s *next;
} pp_linkset_node;

typedef struct pp_linkset_s
{
  int hash_table_size;
  int population;
  pp_linkset_node **hash_table;    /* data actually lives here */
} pp_linkset;


/* from pp_lexer.c */
#define PP_LEXER_MAX_LABELS 512

typedef struct pp_label_node_s 
{
  /* linked list of strings associated with a label in the table */
  char *str;                     
  struct pp_label_node_s *next;
} pp_label_node;                 /* next=NULL: end of list */


typedef struct PPLexTable_s
{
  String_set *string_set;
  char *labels[PP_LEXER_MAX_LABELS];    /* array of labels (NULL-terminated) */
  pp_label_node *nodes_of_label[PP_LEXER_MAX_LABELS]; /* str. for each label */
  pp_label_node *last_node_of_label[PP_LEXER_MAX_LABELS];      /* efficiency */
  pp_label_node *current_node_of_active_label;/*state: current node of label */
  int idx_of_active_label;                      /* read state: current label */
} PPLexTable;

/* from pp_knowledge.c */
typedef struct StartingLinkAndDomain_s 
{
  char* starting_link;              
  int   domain;       /* domain which the link belongs to (-1: terminator)*/
} StartingLinkAndDomain;

typedef struct pp_rule_s
{
  /* Holds a single post-processing rule. Since rules come in many 
     flavors, not all fields of the following are always relevant  */
  char *selector;       /* name of link to which rule applies      */
  int   domain;         /* type of domain to which rule applies    */
  pp_linkset *link_set; /* handle to set of links relevant to rule */
  int   link_set_size;  /* size of this set                        */
  char  **link_array;   /* array containing the spelled-out names  */
  char  *msg;           /* explanation (NULL=end sentinel in array)*/
} pp_rule;

typedef struct pp_knowledge_s
{
  PPLexTable *lt; /* Internal rep'n of sets of strings from knowledge file */
  char *path;     /* Name of file we loaded from */
  
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
  pp_rule *connected_rules, *form_a_cycle_rules;
  pp_rule *contains_one_rules, *contains_none_rules;
  pp_rule *bounded_rules;

  int n_connected_rules, n_form_a_cycle_rules;
  int n_contains_one_rules, n_contains_none_rules;
  int n_bounded_rules;

  pp_linkset *set_of_links_starting_bounded_domain;
  StartingLinkAndDomain *starting_link_lookup_table; 
  int nStartingLinks;
  String_set *string_set;              
} pp_knowledge;


#endif

