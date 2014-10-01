/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

/* This file is somewhat misnamed, as everything here defines the
 * link-private, internal-use-only "api", which is subject to change
 * from revision to revision. No external code should link to this
 * stuff.
 */

#ifndef _API_TYPES_H_
#define _API_TYPES_H_

#define MAX_TOKEN_LENGTH 250     /* Maximum number of chars in a token */

/* MAX_SENTENCE cannot be more than 254, because word MAX_SENTENCE+1 is 
 * used to indicate that nothing can connect to this connector, and this 
 * should fit in one byte (if the word field of a connector is an 
 * unsigned char).
 */
#define MAX_SENTENCE 250       /* Maximum number of words in a sentence */
#define MAX_LINKS (2*MAX_SENTENCE-3) /* maximum number of links allowed */

/* Widely used private typedefs */
#ifdef USE_FAT_LINKAGES
typedef struct And_data_s And_data;
typedef struct DIS_node_struct DIS_node;
#endif /* USE_FAT_LINKAGES */
typedef struct Connector_struct Connector;
typedef struct Cost_Model_s Cost_Model;
typedef struct Domain_s Domain;
typedef struct DTreeLeaf_s DTreeLeaf;
typedef struct Image_node_struct Image_node;
typedef struct Label_node_s Label_node;
typedef struct Linkage_info_struct Linkage_info;
typedef struct Parse_info_struct *Parse_info;
typedef struct Postprocessor_s Postprocessor;
typedef struct PP_data_s PP_data;
typedef struct PP_info_s PP_info;
typedef struct Regex_node_s Regex_node;
typedef struct Resources_s * Resources;
typedef struct Sublinkage_s Sublinkage;

/* Some of the more obscure typedefs */
typedef struct count_context_s count_context_t;
typedef struct fast_matcher_s fast_matcher_t;
#ifdef USE_FAT_LINKAGES
typedef struct analyze_context_s analyze_context_t;
#endif /* USE_FAT_LINKAGES */

typedef struct Connector_set_s Connector_set;
typedef struct Disjunct_struct Disjunct;
typedef struct Link_s Link;
typedef struct List_o_links_struct List_o_links;
typedef struct Parse_set_struct Parse_set;
typedef struct String_set_s String_set;
typedef struct Afdict_class_struct Afdict_class;
typedef struct Word_struct Word;
typedef struct X_table_connector_struct X_table_connector;


/* Post-processing structures */
typedef struct pp_knowledge_s pp_knowledge;
typedef struct pp_linkset_s pp_linkset;
typedef struct PP_node_struct PP_node;

typedef struct corpus_s Corpus;
typedef struct sense_s Sense;
typedef struct cluster_s Cluster;

#endif

