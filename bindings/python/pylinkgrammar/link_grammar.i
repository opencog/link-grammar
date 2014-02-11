%module clinkgrammar
%{

#include <link-grammar/link-includes.h>

%}

typedef struct Dictionary_s * Dictionary;
typedef struct Parse_Options_s * Parse_Options;
typedef struct Sentence_s * Sentence;
typedef struct Linkage_s * Linkage;
typedef struct Postprocessor_s PostProcessor;
typedef struct CNode_s CNode;


const char * linkgrammar_get_version(void);

/**********************************************************************
*
* Functions to manipulate Dictionaries
*
***********************************************************************/

Dictionary dictionary_create(char * dict_name, char * pp_name, char * cons_name, char * affix_name);
Dictionary dictionary_create_lang(const char * lang);
Dictionary dictionary_create_default_lang(void);
int dictionary_delete(Dictionary dict);
int dictionary_get_max_cost(Dictionary dict);
void dictionary_set_data_dir(const char * path);
char * dictionary_get_data_dir(void);
int dictionary_is_past_tense_form(Dictionary dict, const char * str);
int dictionary_is_entity(Dictionary dict, const char * str);

/**********************************************************************
*
* Functions to manipulate Parse Options
*
***********************************************************************/

Parse_Options parse_options_create(void);
int parse_options_delete(Parse_Options opts);
void parse_options_set_verbosity(Parse_Options opts, int verbosity);
int  parse_options_get_verbosity(Parse_Options opts);
void parse_options_set_linkage_limit(Parse_Options opts, int linkage_limit);
int  parse_options_get_linkage_limit(Parse_Options opts);
void parse_options_set_disjunct_cost(Parse_Options opts, int disjunct_cost);
int  parse_options_get_disjunct_cost(Parse_Options opts);
void parse_options_set_min_null_count(Parse_Options opts, int null_count);
int  parse_options_get_min_null_count(Parse_Options opts);
void parse_options_set_max_null_count(Parse_Options opts, int null_count);
int  parse_options_get_max_null_count(Parse_Options opts);
void parse_options_set_null_block(Parse_Options opts, int null_block);
int  parse_options_get_null_block(Parse_Options opts);
void parse_options_set_islands_ok(Parse_Options opts, int islands_ok);
int  parse_options_get_islands_ok(Parse_Options opts);
void parse_options_set_short_length(Parse_Options opts, int short_length);
int  parse_options_get_short_length(Parse_Options opts);
void parse_options_set_max_memory(Parse_Options  opts, int mem);
int  parse_options_get_max_memory(Parse_Options opts);
void parse_options_set_max_sentence_length(Parse_Options  opts, int len);
int  parse_options_get_max_sentence_length(Parse_Options opts);
void parse_options_set_max_parse_time(Parse_Options  opts, int secs);
int  parse_options_get_max_parse_time(Parse_Options opts);
void parse_options_set_cost_model_type(Parse_Options opts, int cm);
int  parse_options_get_cost_model_type(Parse_Options opts);
int  parse_options_timer_expired(Parse_Options opts);
int  parse_options_memory_exhausted(Parse_Options opts);
int  parse_options_resources_exhausted(Parse_Options opts);
void parse_options_set_screen_width(Parse_Options opts, int val);
int  parse_options_get_screen_width(Parse_Options opts);
void parse_options_set_allow_null(Parse_Options opts, int val);
int  parse_options_get_allow_null(Parse_Options opts);
void parse_options_set_display_walls(Parse_Options opts, int val);
int  parse_options_get_display_walls(Parse_Options opts);
void parse_options_set_all_short_connectors(Parse_Options opts, int val);
int  parse_options_get_all_short_connectors(Parse_Options opts);
void parse_options_reset_resources(Parse_Options opts);

/**********************************************************************
*
* Functions to manipulate Sentences
*
***********************************************************************/

Sentence sentence_create(char *input_string, Dictionary dict);
void sentence_delete(Sentence sent);
int  sentence_parse(Sentence sent, Parse_Options opts);
int  sentence_length(Sentence sent);
char * sentence_get_word(Sentence sent, int wordnum);
int  sentence_null_count(Sentence sent);
int  sentence_num_linkages_found(Sentence sent);
int  sentence_num_valid_linkages(Sentence sent);
int  sentence_num_linkages_post_processed(Sentence sent);
int  sentence_num_violations(Sentence sent, int i);
int  sentence_and_cost(Sentence sent, int i);
int  sentence_disjunct_cost(Sentence sent, int i);
int  sentence_link_cost(Sentence sent, int i);
char * sentence_get_nth_word(Sentence sent, int i);
int sentence_nth_word_has_disjunction(Sentence sent, int i);

/**********************************************************************
*
* Functions that create and manipulate Linkages.
* When a Linkage is requested, the user is given a
* copy of all of the necessary information, and is responsible
* for freeing up the storage when he/she is finished, using
* the routines provided below.
*
***********************************************************************/

/**********************************************************************
*
*These functions allocate strings to be returned, so need to be
*newobject'd to avoid memory leaks
*
***********************************************************************/
%newobject linkage_print_senses;
%newobject linkage_print_links_and_domains;
%newobject linkage_print_postscript;
%newobject linkage_print_postscript;
%newobject linkage_print_diagram;

Linkage linkage_create(int index, Sentence sent, Parse_Options opts);
void linkage_delete(Linkage linkage);
char * linkage_print_diagram(Linkage linkage);

int linkage_get_current_sublinkage(Linkage linkage); 
int linkage_set_current_sublinkage(Linkage linkage, int index);

Sentence linkage_get_sentence(Linkage linkage);
int linkage_get_num_sublinkages(Linkage linkage);
int linkage_get_num_words(Linkage linkage);
int linkage_get_num_links(Linkage linkage);
int linkage_get_link_lword(Linkage linkage, int index);
int linkage_get_link_rword(Linkage linkage, int index);
int linkage_get_link_length(Linkage linkage, int index);
const char * linkage_get_link_label(Linkage linkage, int index);
const char * linkage_get_link_llabel(Linkage linkage, int index);
const char * linkage_get_link_rlabel(Linkage linkage, int index);
int linkage_get_link_num_domains(Linkage linkage, int index);
const char ** linkage_get_link_domain_names(Linkage linkage, int index);
const char ** linkage_get_words(Linkage linkage);
//const char *  linkage_get_disjunct(Linkage linkage, int w);
const char *  linkage_get_word(Linkage linkage, int w);
char * linkage_print_links_and_domains(Linkage linkage);
char * linkage_print_senses(Linkage linkage);
char * linkage_print_constituent_tree(Linkage linkage, int mode);
char * linkage_print_postscript(Linkage linkage, int mode);
int linkage_compute_union(Linkage linkage);
int linkage_unused_word_cost(Linkage linkage);
int linkage_disjunct_cost(Linkage linkage);
int linkage_and_cost(Linkage linkage);
int linkage_link_cost(Linkage linkage);
double linkage_corpus_cost(Linkage linkage);
int linkage_is_canonical(Linkage linkage);
int linkage_is_improper(Linkage linkage);
int linkage_has_inconsistent_domains(Linkage linkage);
const char * linkage_get_violation_name(Linkage linkage);


/**********************************************************************
*
* Functions that allow special-purpose post-processing of linkages
*
***********************************************************************/
PostProcessor * post_process_open(char *path);
void post_process_close(PostProcessor *);
void linkage_post_process(Linkage, PostProcessor *);

/**********************************************************************
*
* Constituent node
*
***********************************************************************/

CNode * linkage_constituent_tree(Linkage linkage);
void linkage_free_constituent_tree(CNode * n);
char * linkage_constituent_node_get_label(const CNode *n);
CNode * linkage_constituent_node_get_child(const CNode *n);
CNode * linkage_constituent_node_get_next(const CNode *n);
int linkage_constituent_node_get_start(const CNode *n);
int linkage_constituent_node_get_end(const CNode *n);
