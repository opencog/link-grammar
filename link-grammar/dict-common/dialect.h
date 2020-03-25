/*************************************************************************/
/* Copyright (C) 2019-2020 Amir Plivatsky                                     */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _DIALECT_H_
#define _DIALECT_H_

#include <ctype.h>
#include <stdint.h>

#include "api-types.h"
#include "dict-structures.h"            // cost_eq
#include "string-id.h"

/* dialect_tag costs with a special meaning. See also link-includes.h. */
#define DIALECT_COST_MAX         9999.0    /* Less than that is a real cost */
#define DIALECT_COST_DISABLE    10000.0
#define DIALECT_SUB             10001.0    /* Sub-dialect (a vector name) */
#define DIALECT_SECTION         10002.0    /* Section header (a vector name) */


/* Used for dialect table entries and Dialect_Option component cost array. */
typedef struct
{
	const char *name;
	float cost;        /* Component cost, DIALECT_SUB or DIALECT_SECTION */
} dialect_tag;

/* An array of dialect table section names (which are dialect vector
 * names), indexed by their ordinal number (starting with 1) in the
 * "4.0.dialect" file. The 0'th element indicates the index of the default
 * section in the dialect table (NO_INDEX if no default entry). The rest
 * of the elements (which also include the default section if exists)
 * contain the index of their section in the dialect table.) */
typedef struct
{
	const char *name;
	unsigned int index;            /* Dialect table index */
} dialect_section_tag;

#define NO_INDEX ((unsigned int)-1)

/* This struct is kept in the Dictionary struct. */
struct Dialect_s
{
	dialect_tag *table;            /* A representation of "4.0.dialect." */
	String_id *section_set;        /* Dialect table section names. */
	dialect_section_tag *section;  /* Section tags (indexed by section_set id) */
	char *kept_input;              /* For dialect table string memory XXX */
	unsigned int num_table_tags;   /* Number of entries in dialect_tag table */
	unsigned int num_sections;     /* Dialect_section_tag number of entries */
};

/* Dialect object for parse_options_*_dialect(). */
struct dialect_option_s
{
	Dictionary dict;
	char *conf;                    /* User dialect setup */
	float *cost_table;             /* Indexed by Exptag index field */
};

typedef struct dialect_option_s dialect_info;

Dialect *dialect_alloc(void);
void free_dialect(Dialect *);
unsigned int exptag_dialect_add(Dictionary, const char *);
bool setup_dialect(Dictionary, Parse_Options);
void free_cost_table(Parse_Options opts);
bool apply_dialect(Dictionary, Dialect *, unsigned int, Dialect *, dialect_info *);

/**
 * Validate that \p name is a valid dialect tag name.
 * @name Dialect tag name
 * @return NULL if valid, else the first offending character.
 */
static inline const char *valid_dialect_name(const char *name)
{
	if (!isalpha(name[0])) return name;
	while (*++name != '\0')
	{
		if (!isalnum(name[0]) && name[0] != '_' && name[0] != '-')
			return name;
	}
	return NULL;
}
#endif /* _DIALECT_H_ */
