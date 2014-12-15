%module clinkgrammar
%{

#include <link-grammar/link-includes.h>

%}

typedef struct Dictionary_s * Dictionary;
typedef struct Parse_Options_s * Parse_Options;
typedef struct Sentence_s * Sentence;
typedef struct Linkage_s * Linkage;
typedef struct Postprocessor_s PostProcessor;

typedef enum
{
   NO_DISPLAY = 0,        /** Display is disabled */
   MULTILINE = 1,         /** multi-line, indented display */
   BRACKET_TREE = 2,      /** single-line, bracketed tree */
   SINGLE_LINE = 3,       /** single line, round parenthesis */
   MAX_STYLES = 3         /* this must always be last, largest */
} ConstituentDisplayStyle;

typedef enum
{
   VDAL=1, /* Sort by Violations, Disjunct cost, Link cost */
   CORPUS, /* Sort by Corpus cost */
} Cost_Model_type;


const char * linkgrammar_get_version(void);
const char * linkgrammar_get_dict_version(Dictionary dict);

/**********************************************************************
*
* Functions to manipulate Dictionaries
*
***********************************************************************/

Dictionary dictionary_create_lang(const char * lang);
Dictionary dictionary_create_default_lang(void);
const char * dictionary_get_lang(Dictionary dict);

void dictionary_delete(Dictionary dict);
void dictionary_set_data_dir(const char * path);
char * dictionary_get_data_dir(void);

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
void parse_options_set_disjunct_cost(Parse_Options opts, double disjunct_cost);
double parse_options_get_disjunct_cost(Parse_Options opts);
void parse_options_set_min_null_count(Parse_Options opts, int null_count);
int  parse_options_get_min_null_count(Parse_Options opts);
void parse_options_set_max_null_count(Parse_Options opts, int null_count);
int  parse_options_get_max_null_count(Parse_Options opts);
void parse_options_set_islands_ok(Parse_Options opts, int islands_ok);
int  parse_options_get_islands_ok(Parse_Options opts);
void parse_options_set_short_length(Parse_Options opts, int short_length);
int  parse_options_get_short_length(Parse_Options opts);
void parse_options_set_max_memory(Parse_Options  opts, int mem);
int  parse_options_get_max_memory(Parse_Options opts);
void parse_options_set_max_parse_time(Parse_Options  opts, int secs);
int  parse_options_get_max_parse_time(Parse_Options opts);
void parse_options_set_cost_model_type(Parse_Options opts, Cost_Model_type cm);
Cost_Model_type  parse_options_get_cost_model_type(Parse_Options opts);
int  parse_options_timer_expired(Parse_Options opts);
int  parse_options_memory_exhausted(Parse_Options opts);
int  parse_options_resources_exhausted(Parse_Options opts);
void parse_options_set_display_morphology(Parse_Options opts, int val);
int  parse_options_get_display_morphology(Parse_Options opts);
void parse_options_set_spell_guess(Parse_Options opts, int val);
int  parse_options_get_spell_guess(Parse_Options opts);
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
int  sentence_split(Sentence sent, Parse_Options opts);
int  sentence_parse(Sentence sent, Parse_Options opts);
int  sentence_length(Sentence sent);
int  sentence_null_count(Sentence sent);
int  sentence_num_linkages_found(Sentence sent);
int  sentence_num_valid_linkages(Sentence sent);
int  sentence_num_linkages_post_processed(Sentence sent);
int  sentence_num_violations(Sentence sent, int i);
double sentence_disjunct_cost(Sentence sent, int i);
int  sentence_link_cost(Sentence sent, int i);

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
* These functions allocate strings to be returned, so need to be
* newobject'd to avoid memory leaks
*
***********************************************************************/
%newobject linkage_print_senses;
%newobject linkage_print_links_and_domains;
%newobject linkage_print_diagram;
%newobject linkage_print_postscript;

Linkage linkage_create(int index, Sentence sent, Parse_Options opts);
void linkage_delete(Linkage linkage);
char * linkage_print_diagram(Linkage linkage, bool display_walls, size_t screen_width);
char * linkage_print_postscript(Linkage linkage, bool display_walls, bool print_ps_header);

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
char * linkage_print_constituent_tree(Linkage linkage, ConstituentDisplayStyle mode);
int linkage_unused_word_cost(Linkage linkage);
double linkage_disjunct_cost(Linkage linkage);
int linkage_link_cost(Linkage linkage);
double linkage_corpus_cost(Linkage linkage);
const char * linkage_get_violation_name(Linkage linkage);

