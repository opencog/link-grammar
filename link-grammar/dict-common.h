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

#include "api-types.h"
#include "dict-structures.h"

/* The functions here are for link-grammar internal use only.
 * They are not part of the public API. */

bool find_word_in_dict(Dictionary dict, const char *);
Afdict_class * afdict_find(Dictionary, const char *, bool);

Exp * Exp_create(Exp_list *);
Exp * add_empty_word(Dictionary const, Exp_list *, Dict_node *);
void free_Exp_list(Exp_list *);

void patch_subscript(char *);

bool is_suffix(const char, const char *);
bool is_stem(const char *);

/* Connector names for the affix class lists in the affix file */

typedef enum {
	AFDICT_RPUNC=1,
	AFDICT_LPUNC,
	AFDICT_UNITS,
	AFDICT_SUF,
	AFDICT_PRE,
	AFDICT_MPRE,
	AFDICT_QUOTES,
	AFDICT_BULLETS,
	AFDICT_INFIXMARK,
	AFDICT_STEMSUBSCR,
	AFDICT_SANEMORPHISM,
#ifdef USE_ANYSPLIT
	AFDICT_REGPRE,
	AFDICT_REGMID,
	AFDICT_REGSUF,
	AFDICT_REGALTS,
	AFDICT_REGPARTS,
#endif
	AFDICT_NUM_ENTRIES
} afdict_classnum;

#define AFDICT_CLASSNAMES1 \
	"invalid classname", \
	"RPUNC", \
	"LPUNC", \
	"UNITS", \
	"SUF",         /* SUF is used in the Russian dict */ \
	"PRE",         /* PRE is not used anywhere, yet... */ \
	"MPRE",        /* Multi-prefix, currently for Hebrew */ \
	"QUOTES", \
	"BULLETS", \
	"INFIXMARK",   /* Prepended to suffixes, appended to pefixes */ \
	"STEMSUBSCR",  /* Subscripts for stems */ \
	"SANEMORPHISM", /* Regex for sane_morphism() */

#ifdef USE_ANYSPLIT
#define AFDICT_CLASSNAMES2 \
	"REGPRE",      /* Regex for prefix */ \
	"REGMID",      /* Regex for middle parts */ \
	"REGSUF",      /* Regex for suffix */ \
	"REGALTS",     /* Min&max number of alternatives to issue for a word */\
	"REGPARTS",    /* Max number of word partitions */
#else
#define AFDICT_CLASSNAMES2
#endif

#define AFDICT_CLASSNAMES AFDICT_CLASSNAMES1 AFDICT_CLASSNAMES2 "last classname"
#define AFCLASS(afdict, class) (&afdict->afdict_class[class])

#endif /* _LG_DICT_COMMON_H_ */
