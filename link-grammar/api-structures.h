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

/*****************************************************************************
*
* NOTE: There are five basic "types" in the link parser API.  These are:
*
*       Dictionary, Parse_Options, Sentence, Linkage, PostProcessor
*
* To make the use of the API simpler, each of these is typedef'ed as a pointer
* to a data structure.  As a result, some of the code may look a little funny,
* since it uses pointers in a way that is syntactically inconsistent.  After
* working a bit with these basic types enough, this should not be confusing.
*
******************************************************************************/

#ifndef _API_STRUCTURESH_
#define _API_STRUCTURESH_

typedef enum {VDAL, /* sort by Violations, Disjunct cost, And cost, Link cost */
}   Cost_Model_type;

typedef struct Cost_Model_s Cost_Model;
struct Cost_Model_s {
    Cost_Model_type type;
    int (*compare_fn)(Linkage_info *, Linkage_info *);  
};

typedef struct Resources_s * Resources;
struct Resources_s {
    int    max_parse_time;  /* was double before --DS */
    int    max_memory;
    double time_when_parse_started;
    int    space_when_parse_started;
    double when_created;
    double when_last_called;
    double cumulative_time;
    int    memory_exhausted;
    int    timer_expired;
};

typedef struct Parse_Options_s * Parse_Options;
struct Parse_Options_s {
  int verbosity;         /* Level of detail to give about the computation 0 */
  int linkage_limit;     /* The maximum number of linkages processed 10000 */
  int disjunct_cost;     /* Max disjunct cost to allow */
  int min_null_count;    /* The minimum number of null links to allow */
  int max_null_count;    /* The maximum number of null links to allow */
  int null_block;        /* consecutive blocks of this many words are
			    considered as one null link  (default=1) */
  int islands_ok;        /* If TRUE, then linkages with islands
			    (separate component of the link graph)
			    will be generated (default=FALSE) */
  int twopass_length;    /* min length for two-pass post processing */
  int max_sentence_length;
  int short_length;      /* Links that are limited in length can be
			    no longer than this.  Default = 6 */
  int all_short;         /* If true, there can be no connectors that are exempt */
  Cost_Model cost_model; /* For sorting linkages in post_processing */
  Resources resources;   /* For deciding when to "abort" the parsing */
  int display_short;
  int display_word_subscripts;  /* as in "dog.n" as opposed to "dog" */
  int display_link_subscripts;  /* as in "Ss" as opposed to "S" */
  int display_walls;           
  int display_union;            /* print squashed version of linkage with conjunction? */
  int allow_null;               /* true if we allow null links in parsing */
  int echo_on;                  /* true if we should echo the input sentence */
  int batch_mode;               /* if true, process sentences non-interactively */
  int panic_mode;               /* if true, parse in "panic mode" after all else fails */
  int screen_width;             /* width of screen for displaying linkages */
  int display_on;               /* if true, output graphical linkage diagram */
  int display_postscript;       /* if true, output postscript linkage */
  int display_constituents;     /* if true, output treebank-style constituent structure */
  int display_bad;              /* if true, bad linkages are displayed */
  int display_links;            /* if true, a list o' links is printed out */
};

typedef struct {
    Connector ** hash_table;
    int          table_size;
    int          is_defined;  /* if 0 then there is no such set */
} Connector_set;

typedef struct Postprocessor_s Postprocessor;
typedef struct Postprocessor_s * PostProcessor;

typedef struct Dictionary_s * Dictionary;
struct Dictionary_s {
    Dict_node *     root;
    char *          name;
    /* char *          post_process_filename; */  /* was not being used *DS* */
    int             use_unknown_word;
    int             unknown_word_defined;
    int             capitalized_word_defined;
    int             pl_capitalized_word_defined;
    int             hyphenated_word_defined;
    int             number_word_defined;
    int             ing_word_defined;
    int             s_word_defined;
    int             ed_word_defined;
    int             ly_word_defined;
    int             left_wall_defined;
    int             right_wall_defined;
    Postprocessor * postprocessor;
    Postprocessor * constituent_pp;
    Dictionary      affix_table;
    int             andable_defined;
    Connector_set * andable_connector_set;  /* NULL=everything is andable */
    Connector_set * unlimited_connector_set; /* NULL=everthing is unlimited */
    int             max_cost;  
    String_set *    string_set;  /* Set of link names constructed during parsing */
    int             num_entries;
    /*   Dict_node *     dict_root; OBSOLETE, replaced by "root" above? */  
    Word_file *     word_file_header;
    Exp *           exp_list;   /* We link together all the Exp structs that are
				   allocated in reading this dictionary.  Used for
				   freeing the dictionary */
    FILE *          fp;
    char            token[MAX_TOKEN_LENGTH];
    int             is_special;
    int             already_got_it;
    int             line_number;
};

typedef struct Label_node_s Label_node;
struct Label_node_s {
    int          label;
    Label_node * next;
};

#define HT_SIZE (1<<10)

typedef struct And_data_s And_data;
struct And_data_s {
    int          LT_bound;
    int          LT_size;
    Disjunct **  label_table;
    Label_node * hash_table[HT_SIZE];
};


typedef struct Parse_info_struct Parse_info;
struct Parse_info_struct {
    int            x_table_size;
    X_table_connector ** x_table;
    Parse_set *    parse_set;
    int            N_words;
    Disjunct *     chosen_disjuncts[MAX_SENTENCE];
    int            N_links;
    struct Link_s  link_array[MAX_LINKS];
};    

typedef struct Sentence_s * Sentence;
struct Sentence_s {
    Dictionary  dict;           /* words are defined from this dictionary */
    int    length;              /* number of words */
    Word   word[MAX_SENTENCE];  /* array of words after tokenization */
    char * is_conjunction;      /* TRUE if conjunction, as defined by dictionary */
    char** deletable;           /* deletable regions in a sentence with conjunction */
    char** effective_dist;     
    int    num_linkages_found;  /* total number before postprocessing.  This
				   is returned by the count() function */
    int    num_linkages_alloced;/* total number of linkages allocated.
				   the number post-processed might be fewer
				   because some are non-canonical */
    int    num_linkages_post_processed;
                                /* The number of linkages that are actually
				   put into the array that was alloced.
				   this is not the same as num alloced
				   because some may be non-canonical. */
    int    num_valid_linkages;  /* number with no pp violations */
    int    null_count;          /* number of null links in linkages */
    Parse_info *   parse_info;  /* set of parses for the sentence */
    Linkage_info * link_info;   /* array of valid and invalid linkages (sorted) */
    String_set *   string_set;  /* used for word names, not connectors */
    And_data       and_data;    /* used to keep track of fat disjuncts */ 
    char  q_pruned_rules;       /* don't prune rules more than once in p.p. */
};

/*********************************************************
*
* Post processing
*
**********************************************************/

typedef struct DTreeLeaf_s DTreeLeaf;
typedef struct Domain_s Domain;
struct Domain_s {
  char *         string;
  int            size;
  List_o_links * lol;
  int            start_link;  /* the link that started this domain */
  int            type;        /* one letter name */
  DTreeLeaf *    child; 
  Domain *       parent;
};


struct DTreeLeaf_s {
  Domain *    parent;
  int         link;
  DTreeLeaf * next;
};

typedef struct PP_data_s PP_data;
struct PP_data_s {
  int N_domains;
  List_o_links * word_links[MAX_SENTENCE];
  List_o_links * links_to_ignore;
  Domain domain_array[MAX_LINKS]; /* the domains, sorted by size */
  int length;                     /* length of current sentence */
};

struct PP_info_s {
    int       num_domains;
    char **   domain_name;
};

struct Postprocessor_s
{
  pp_knowledge *knowledge;             /* internal rep'n of the actual rules */
  int n_global_rules_firing;           /* this & the next are diagnostic     */
  int n_local_rules_firing;      
  pp_linkset *set_of_links_of_sentence;     /* seen in *any* linkage of sent */
  pp_linkset *set_of_links_in_an_active_rule;/*used in *some* linkage of sent*/
  int *relevant_contains_one_rules;        /* -1-terminated list of indices  */
  int *relevant_contains_none_rules;
  /* the following maintain state during a call to post_process() */
  String_set *sentence_link_name_set;        /* link names seen for sentence */
  int visited[MAX_SENTENCE];                   /* for the depth-first search */
  PP_node *pp_node;                 
  PP_data pp_data;
};


/*********************************************************
*
* Linkages 
*
**********************************************************/

typedef struct PP_info_s PP_info;

typedef struct Sublinkage_s Sublinkage;
struct Sublinkage_s {
    int       num_links;          /* Number of links in array */
    Link *    link;               /* Array of links */
    PP_info * pp_info;            /* PP info for each link */
    char *    violation;          /* Name of violation, if any */
    PP_data   pp_data;
};


typedef struct Linkage_s * Linkage;
struct Linkage_s {
    int             num_words;  /* number of (tokenized) words */
    char *        * word;       /* array of word spellings */
    Linkage_info    info;       /* index and cost information */
    int             num_sublinkages; /* One for thin linkages, bigger for fat */
    int             current;    /* Allows user to select particular sublinkage */
    Sublinkage *    sublinkage; /* A parse with conjunctions will have several */
    int             unionized;  /* if TRUE, union of links has been computed */
    Sentence        sent;
    Parse_Options   opts;
};



#endif

