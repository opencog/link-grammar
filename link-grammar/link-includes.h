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
#ifndef _LINKINCLUDESH_
#define _LINKINCLUDESH_

#include <link-grammar/link-features.h>

/*****************************************************************************
*
* Functions to manipulate Dictionaries
*
*****************************************************************************/

LINK_BEGIN_DECLS

typedef struct Dictionary_s * Dictionary;

link_public_api(Dictionary) 
     dictionary_create(char * dict_name, char * pp_name, char * cons_name, char * affix_name);
link_public_api(Dictionary)
     dictionary_create_lang(char * lang);
link_public_api(Dictionary)
     dictionary_create_default_lang(void);

link_public_api(int)
     dictionary_delete(Dictionary dict);
link_public_api(int)
     dictionary_get_max_cost(Dictionary dict);

/*****************************************************************************
*
* Functions to manipulate Parse Options 
*
*****************************************************************************/

typedef struct Parse_Options_s * Parse_Options;

link_public_api(Parse_Options)
     parse_options_create();
link_public_api(int)
     parse_options_delete(Parse_Options opts);
link_public_api(void)
     parse_options_set_verbosity(Parse_Options opts, int verbosity);
link_public_api(int)
     parse_options_get_verbosity(Parse_Options opts);
link_public_api(void)
     parse_options_set_linkage_limit(Parse_Options opts, int linkage_limit);
link_public_api(int)
     parse_options_get_linkage_limit(Parse_Options opts);
link_public_api(void)
     parse_options_set_disjunct_cost(Parse_Options opts, int disjunct_cost);
link_public_api(int)
     parse_options_get_disjunct_cost(Parse_Options opts);
link_public_api(void)
     parse_options_set_min_null_count(Parse_Options opts, int null_count);
link_public_api(int)
     parse_options_get_min_null_count(Parse_Options opts);
link_public_api(void)
     parse_options_set_max_null_count(Parse_Options opts, int null_count);
link_public_api(int)
     parse_options_get_max_null_count(Parse_Options opts);
link_public_api(void)
     parse_options_set_null_block(Parse_Options opts, int null_block);
link_public_api(int)
     parse_options_get_null_block(Parse_Options opts);
link_public_api(void)
     parse_options_set_islands_ok(Parse_Options opts, int islands_ok);
link_public_api(int)
     parse_options_get_islands_ok(Parse_Options opts);
link_public_api(void)
     parse_options_set_short_length(Parse_Options opts, int short_length);
link_public_api(int)
     parse_options_get_short_length(Parse_Options opts);
link_public_api(void)
     parse_options_set_max_memory(Parse_Options  opts, int mem);
link_public_api(int)
     parse_options_get_max_memory(Parse_Options opts);
link_public_api(void)
     parse_options_set_max_sentence_length(Parse_Options  opts, int len);
link_public_api(int)
     parse_options_get_max_sentence_length(Parse_Options opts);
link_public_api(void)
     parse_options_set_max_parse_time(Parse_Options  opts, int secs);
link_public_api(int)
     parse_options_get_max_parse_time(Parse_Options opts);
link_public_api(void)
     parse_options_set_cost_model_type(Parse_Options opts, int cm);
link_public_api(int)
     parse_options_timer_expired(Parse_Options opts);
link_public_api(int)
     parse_options_memory_exhausted(Parse_Options opts);
link_public_api(int)
     parse_options_resources_exhausted(Parse_Options opts);
link_public_api(void)
     parse_options_set_screen_width(Parse_Options opts, int val);
link_public_api(int)
     parse_options_get_screen_width(Parse_Options opts);
link_public_api(void)
     parse_options_set_allow_null(Parse_Options opts, int val);
link_public_api(int)
     parse_options_get_allow_null(Parse_Options opts);
link_public_api(void)
     parse_options_set_display_walls(Parse_Options opts, int val);
link_public_api(int)
     parse_options_get_display_walls(Parse_Options opts);
link_public_api(void)
     parse_options_set_all_short_connectors(Parse_Options opts, int val);
link_public_api(int)
     parse_options_get_all_short_connectors(Parse_Options opts);
link_public_api(void)
     parse_options_reset_resources(Parse_Options opts);


/*****************************************************************************
*
* The following Parse_Options functions do not directly affect the
* operation of the parser, but they can be useful for organizing the
* search, or displaying the results.  They were included as switches for
* convenience in implementing the "standard" version of the link parser
* using the API.
*
*****************************************************************************/

link_public_api(void)
     parse_options_set_batch_mode(Parse_Options opts, int val);
link_public_api(int)
     parse_options_get_batch_mode(Parse_Options opts);
link_public_api(void)
     parse_options_set_panic_mode(Parse_Options opts, int val);
link_public_api(int)
     parse_options_get_panic_mode(Parse_Options opts);
link_public_api(void)
     parse_options_set_display_on(Parse_Options opts, int val);
link_public_api(int)
     parse_options_get_display_on(Parse_Options opts);
link_public_api(void)
     parse_options_set_display_postscript(Parse_Options opts, int val);
link_public_api(int)
     parse_options_get_display_postscript(Parse_Options opts);
link_public_api(void)
     parse_options_set_display_constituents(Parse_Options opts, int val);
link_public_api(int)
     parse_options_get_display_constituents(Parse_Options opts);
link_public_api(void)
     parse_options_set_display_bad(Parse_Options opts, int val);
link_public_api(int)
     parse_options_get_display_bad(Parse_Options opts);
link_public_api(void)
     parse_options_set_display_links(Parse_Options opts, int val);
link_public_api(int)
     parse_options_get_display_links(Parse_Options opts);
link_public_api(void)
     parse_options_set_display_union(Parse_Options opts, int val);
link_public_api(int)
     parse_options_get_display_union(Parse_Options opts);
link_public_api(void)
     parse_options_set_echo_on(Parse_Options opts, int val);
link_public_api(int)
     parse_options_get_echo_on(Parse_Options opts);

/*****************************************************************************
*
* Functions to manipulate Sentences
*
*****************************************************************************/

typedef struct Sentence_s * Sentence;

link_public_api(Sentence)
     sentence_create(char *input_string, Dictionary dict);
link_public_api(void)
     sentence_delete(Sentence sent);
link_public_api(int)
     sentence_parse(Sentence sent, Parse_Options opts);
link_public_api(int)
     sentence_length(Sentence sent);
link_public_api(char *)
     sentence_get_word(Sentence sent, int wordnum);
link_public_api(int)
     sentence_null_count(Sentence sent);
link_public_api(int)
     sentence_num_linkages_found(Sentence sent);
link_public_api(int)
     sentence_num_valid_linkages(Sentence sent);
link_public_api(int)
     sentence_num_linkages_post_processed(Sentence sent);
link_public_api(int)
     sentence_num_violations(Sentence sent, int i);
link_public_api(int)
     sentence_disjunct_cost(Sentence sent, int i);

link_public_api(char *)
     sentence_get_nth_word(Sentence sent, int i);
link_public_api(int)
     sentence_nth_word_has_disjunction(Sentence sent, int i);

/*****************************************************************************
*
* Functions that create and manipulate Linkages.
* When a Linkage is requested, the user is given a
* copy of all of the necessary information, and is responsible
* for freeing up the storage when he/she is finished, using
* the routines provided below.
*
*****************************************************************************/

typedef struct Linkage_s * Linkage;

link_public_api(Linkage)
     linkage_create(int index, Sentence sent, Parse_Options opts);
link_public_api(int)
     linkage_set_current_sublinkage(Linkage linkage, int index);
link_public_api(void)
     linkage_delete(Linkage linkage);
link_public_api(Sentence)
     linkage_get_sentence(Linkage linkage);
link_public_api(int)
     linkage_get_num_sublinkages(Linkage linkage);
link_public_api(int)
     linkage_get_num_words(Linkage linkage);
link_public_api(int)
     linkage_get_num_links(Linkage linkage);
link_public_api(int)
     linkage_get_link_lword(Linkage linkage, int index);
link_public_api(int)
     linkage_get_link_rword(Linkage linkage, int index);
link_public_api(int)
     linkage_get_link_length(Linkage linkage, int index);
link_public_api(char *)
     linkage_get_link_label(Linkage linkage, int index);
link_public_api(char *)
     linkage_get_link_llabel(Linkage linkage, int index);
link_public_api(char *)
     linkage_get_link_rlabel(Linkage linkage, int index);
link_public_api(int)
     linkage_get_link_num_domains(Linkage linkage, int index);
link_public_api(char **)
     linkage_get_link_domain_names(Linkage linkage, int index);
link_public_api(char **)
     linkage_get_words(Linkage linkage);
link_public_api(char *)
     linkage_get_word(Linkage linkage, int w);
link_public_api(char *)
     linkage_print_links_and_domains(Linkage linkage);
link_public_api(char *)
     linkage_print_constituent_tree(Linkage linkage, int mode);
link_public_api(char *)
     linkage_print_postscript(Linkage linkage, int mode);
link_public_api(char *)
     linkage_print_diagram(Linkage linkage);
link_public_api(int)
     linkage_compute_union(Linkage linkage);
link_public_api(int)
     linkage_unused_word_cost(Linkage linkage);
link_public_api(int)
     linkage_disjunct_cost(Linkage linkage);
link_public_api(int)
     linkage_and_cost(Linkage linkage);
link_public_api(int)
     linkage_link_cost(Linkage linkage);
link_public_api(int)
     linkage_is_canonical(Linkage linkage);
link_public_api(int)
     linkage_is_improper(Linkage linkage);
link_public_api(int)
     linkage_has_inconsistent_domains(Linkage linkage);
link_public_api(char *)
     linkage_get_violation_name(Linkage linkage);


/*****************************************************************************
* 
* Functions that allow special-purpose post-processing of linkages 
*
*****************************************************************************/

typedef struct Postprocessor_s * PostProcessor;

link_public_api(PostProcessor)
     post_process_open(char *dictname, char *path);
link_public_api(void)
     post_process_close(PostProcessor postprocessor);
link_public_api(void)
     linkage_post_process(Linkage linkage, PostProcessor postprocessor);

link_public_api(void)
     issue_special_command(char * line, Parse_Options opts, Dictionary dict);

/* from error.c */
extern link_public_api(int) 
     lperrno;
extern link_public_api(char) 
     lperrmsg[];

LINK_END_DECLS

#endif
