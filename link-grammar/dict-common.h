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

Exp * Exp_create(Dictionary);
void add_empty_word(Dictionary, Dict_node *);

void patch_subscript(char *);

/* Connector names for the affix class lists in the affix file */

enum {
	AFDICT_RPUNC=0,
	AFDICT_LPUNC,
	AFDICT_QUOTES,
	AFDICT_UNITS,
	AFDICT_SUF,
	AFDICT_PRE,
	AFDICT_MPRE,
	AFDICT_SANEMORPHISM,
	AFDICT_END /* must be last */
} afdict_classnum;

#define AFDICT_CLASSNAMES \
	"RPUNC", \
	"LPUNC", \
	"QUOTES", \
	"UNITS", \
	"SUF",          /* SUF is used in the Russian dict */ \
	"PRE",          /* PRE is not used anywhere, yet... */ \
	"MPRE",         /* Multi-prefix, currently for Hebrew */ \
	"SANEMORPHISM"  /* Regexp for sane_morphism() */

#define AFCLASS(afdict, class) (&afdict->afdict_class[class])

#define AFFIX_COUNT_MEM_INCREMENT 64

#endif /* _LG_DICT_COMMON_H_ */
