/*************************************************************************/
/* Copyright (c) 2026 Amir Plivatsky                                     */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _EXTRACT_LINKS_INTERNAL_H
#define _EXTRACT_LINKS_INTERNAL_H

#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "connectors.h"
#include "count.h"
#include "disjunct-utils.h"
#include "extract-links.h"
#include "fast-match.h"
#include "linkage/analyze-linkage.h"
#include "linkage/linkage.h"
#include "memory-pool.h"
#include "post-process/post-process.h"
#include "resources.h"
#include "tokenize/word-structures.h"
#include "utilities.h"

#define D_EXTRACT 5 /* General debug level for extraction internals. */

typedef struct Parse_choice_struct Parse_choice;
typedef struct Metric_heap_struct Metric_heap;
typedef struct Metric_state_stream_struct Metric_state_stream;
typedef struct Metric_set_cache_struct Metric_set_cache;
typedef struct Metric_ranker_struct Metric_ranker;
typedef struct Metric_bounded_domain_feedback_struct Metric_bounded_domain_feedback;

typedef uint32_t Metric_link_id;
typedef uint64_t Metric_state;

typedef struct
{
	float disjunct_cost;
	int link_cost;
} Parse_metric;

#define METRIC_BOUNDED_DOMAIN_MAX_MARKS 8
#define METRIC_GLOBAL_CONTAINS_ONE_MAX_MARKS (8 * sizeof(uint64_t))
#define METRIC_GLOBAL_CONTAINS_ONE_EXACT_MAX_MARKS 2
#define METRIC_LINK_ID_NONE ((Metric_link_id)0)

#define METRIC_LINK_IGNORED ((uint8_t)1)
#define METRIC_LINK_RESTRICTED ((uint8_t)2)
#define METRIC_LINK_MUST_FORM_CYCLE ((uint8_t)4)
#define METRIC_LINK_DOMAIN_CONTAINS_START ((uint8_t)8)

typedef struct
{
	connector_uc_hash_t uc_num;
	lc_enc_t lc_label;
} Metric_link_key;

typedef struct
{
	Metric_link_key key;
	Metric_link_id id;
} Metric_link_id_entry;

typedef struct
{
	Metric_link_key key;
	const char *name;
	Metric_state global_contains_one_state; /* Exact root-state bits. */
	uint64_t global_contains_one_selector;  /* Sentence-wide selectors. */
	uint64_t global_contains_one_criterion; /* Sentence-wide criteria. */
	uint64_t parse_contains_one_selector;   /* PARSE_* selectors. */
	uint64_t parse_contains_one_criterion;  /* PARSE_* criteria. */
	uint64_t parse_contains_none_selector;  /* PARSE_* none selectors. */
	uint64_t parse_contains_none_forbidden; /* PARSE_* forbidden links. */
	size_t epoch;                          /* Cached classification era. */
	PP_domain_kind domain_kind;             /* Domain opened by this link. */
	uint8_t flags;                         /* Restricted/link-kind bits. */
} Metric_link_class;

/* Parse_choice records a parse of the word range set[0]->lw to
 * set[1]->rw, when the middle disjunct md is of word set[0]->rw
 * (which is always equal to set[1]->lw).
 * See make_choice() below.
 * The number of linkages in this parse is the product of the
 * counts of the two Parse_set elements. */
typedef struct Parse_set_struct Parse_set;
struct Parse_choice_struct
{
	Parse_choice * next;
	Parse_set * set[2];
	Disjunct    *md;           /* the chosen disjunct for the middle word */
	int32_t     l_id, r_id;    /* the tracon IDs used in this disjunct */
	Metric_link_id metric_link_id[2];
	uint8_t     metric_link_id_done;
#ifdef PC_DISPLAY
	bool done;
	bool dolr;
#endif
};

/* Parse_set serves as a header of Parse_choice chained elements, that
 * describe the possible parses with the specified null_count, using
 * tracons l_id and r_id on words lw and rw, correspondingly. */
struct Parse_set_struct
{
	Connector      *le, *re;
	Parse_choice   *first;
	unsigned int   num_pc;     /* number of Parse_choice elements */
	uint8_t        lw, rw;     /* left and right word index */
	uint8_t        null_count; /* number of island words */

	count_t count;             /* The number of ways to parse. */
#ifdef RECOUNT
	count_t recount;  /* Exactly the same as above, but counted at a later stage. */
	count_t cut_count;  /* Count only low-cost parses, i.e. below the cost cutoff */
	//float cost_cutoff;
#undef RECOUNT
#define RECOUNT(X) X
#else
#define RECOUNT(X)  /* Make it disappear... */
#endif
};

typedef struct Pset_bucket_struct Pset_bucket;
struct Pset_bucket_struct
{
	Parse_set set;
	Pset_bucket *next;
};


/* Metric extraction is optional sentence-local state.  It is grouped so the
 * ordinary extractor fields stay readable and metric PP handling does not
 * spread a flat namespace across extractor_s. */
typedef struct
{
	Metric_link_class *classes;       /* Interned link-name classes. */
	size_t num_classes;
	size_t classes_size;
	Metric_link_id_entry *id_table;   /* Open-addressed class lookup. */
	size_t id_table_size;
	size_t id_table_used;
	size_t class_epoch;               /* Invalidates cached class flags. */
} Metric_link_store;

typedef struct
{
	uint64_t *hash;                  /* Candidate-tree signatures. */
	size_t size;
	size_t count;
	size_t duplicate_skipped;
} Metric_seen_set;

typedef struct
{
	size_t next_work;                 /* Next progress-report threshold. */
	size_t extract_calls;
	size_t streams_started;
	size_t choices_scanned;
	size_t stream_candidates_emitted;
	size_t root_candidates_emitted;
	size_t state_assignments_considered;
	size_t state_assignments_pushed;
	bool enabled;
	bool started;
} Metric_trace_state;

typedef struct
{
	Metric_bounded_domain_feedback *feedbacks;
	size_t num_feedbacks;
	size_t feedbacks_size;
	size_t duplicates;
	size_t ignored;
	size_t dirty;
	size_t rejected;
} Metric_bounded_domain_state;

typedef struct
{
	size_t rejected;
	size_t num_exact;
	size_t exact_rule_idx[METRIC_GLOBAL_CONTAINS_ONE_EXACT_MAX_MARKS];
	uint64_t exact_rule_mask;
	uint64_t active_selector_mask;
	size_t fallback_rejects[METRIC_GLOBAL_CONTAINS_ONE_MAX_MARKS];
	size_t promotions;
	bool exact_ready;
} Metric_global_contains_one_state;

typedef struct
{
	Metric_bounded_domain_state bounded;
	Metric_global_contains_one_state global;
	size_t parse_constraint_rejected;
	size_t parse_constraint_reject_limit;
	PP_failure prediction;           /* Would-reject validation result. */
	bool mfc_enabled;
	bool bounded_enabled;
	bool global_enabled;
	bool constraints_enabled;
	bool validate_enabled;
	bool feedback_trace_enabled;
	bool trace_all_links_enabled;
	bool parse_constraint_reject_limit_hit;
} Metric_pp_state;

typedef struct
{
	Pool_desc *ranker_pool;
	Metric_ranker *rankers;
	Metric_ranker *ranker;
	Metric_candidate *trace_root;
	Metric_link_store links;
	Metric_seen_set seen;
	Metric_trace_state trace;
	Metric_pp_state pp;
	Resources resources;
	size_t parse_choice_pool_size;
	size_t next_rank;
	uint64_t serial;
	unsigned int resource_check_count;
	bool enabled;
} Metric_extraction_state;

struct extractor_s
{
	/* Parse-set ownership and hash lookup state used by both indexed
	 * extraction and metric extraction. */
	unsigned int   x_table_size;
	unsigned int   log2_x_table_size; /* Not used */
	Pset_bucket ** x_table;           /* Hash table */
	Parse_set *    parse_set;
	Word           *words;
	Pool_desc *    Pset_bucket_pool;
	Pool_desc *    Parse_choice_pool;
	String_set    *string_set;
	Postprocessor *postprocessor;
	Metric_extraction_state metric;
	bool           islands_ok;

	/* thread-safe random number state */
	unsigned int rand_state;
};

struct Metric_bounded_domain_feedback_struct
{
	Parse_set *set;
	int32_t l_id;
	int32_t r_id;
	int side;
	int domain;
	WordIdx root;
	WordIdx seed;
	char *link_name;
};

struct Metric_candidate_struct
{
	Metric_ranker *ranker;
	Parse_choice *choice;
	/* child_state/rank identify the concrete left and right child
	 * candidates.  A candidate is therefore a lazy recipe for one
	 * linkage, not a copied linkage or copied parse tree. */
	size_t rank[2];
	Metric_state state;
	Metric_state child_state[2];
	Parse_metric metric;
	uint8_t bounded_domain_state[METRIC_BOUNDED_DOMAIN_MAX_MARKS];
	uint64_t serial;
	bool parse_constraint_relevant;
	/* Bounded feedback skips this candidate only after the stream has
	 * advanced its successor frontier, so later valid rank combinations are
	 * not hidden behind a PP-rejected parent candidate. */
	bool bounded_domain_rejected;
};

struct Metric_heap_struct
{
	Metric_candidate **array;
	size_t size;
	size_t capacity;
};

struct Metric_state_stream_struct
{
	Metric_state state;
	/* One stream enumerates the K-best candidates for one Parse_set and
	 * one requested state.  The heap starts with the best child ranks for
	 * every compatible Parse_choice; after a candidate is emitted, its
	 * neighboring child ranks are pushed. */
	Metric_heap *heap;
	Metric_candidate **ranked;
	size_t num_ranked;
	size_t ranked_size;
	size_t next_emit_rank;
	bool started;
	bool done;
};

struct Metric_set_cache_struct
{
	Parse_set *set;
	Metric_state_stream *streams;
	size_t num_streams;
	size_t streams_size;
	Metric_candidate **ranked;
	size_t num_ranked;
	size_t ranked_size;
	Metric_state possible_state;
	bool possible_state_done;
	bool done;
	Metric_set_cache *next;
};

struct Metric_ranker_struct
{
	Pool_desc *candidate_pool;
	Pool_desc *set_cache_pool;
	/* Metric ranking is memoized per Parse_set.  Learned feedback changes
	 * the meaning and count of states, so rankers are discarded when new
	 * summary marks are learned. */
	size_t state_count;
	Metric_state *root_states;
	size_t num_root_states;
	Metric_set_cache **cache_table;
	size_t cache_table_size;
	size_t cache_table_limit;
	size_t num_set_caches;
	Metric_ranker *next;
};


static inline bool is_zero_tracon(const Connector *c)
{
	return (c == NULL) || (c->tracon_id < NULL_TRACON_BLOCK);
}

static inline Connector *get_tracon_by_id(const Disjunct *d,
                                          int32_t tracon_id, int dir)
{
	if (tracon_id < 0) return NULL; /* See make_choice(). */
	for (Connector *c = dir ? d->right : d->left; c != NULL; c = c->next)
		if (tracon_id == c->tracon_id) return c;

	assert(0, "tracon_id %d not found on disjunct %p in direction %d\n",
	       tracon_id, (void *)d, dir);
}

static inline void issue_link(Linkage lkg, int lr, Parse_choice *pc,
                              const Parse_set *set)
{
	Connector *lc = lr ? get_tracon_by_id(pc->md, pc->r_id, 1) : set->le;
	if (is_zero_tracon(lc)) return; /* No choice to record. */

	lkg->chosen_disjuncts[lr ? pc->set[1]->lw : pc->set[0]->rw] = pc->md;

	Connector *rc = lr ? set->re : get_tracon_by_id(pc->md, pc->l_id, 0);
	if (is_zero_tracon(rc)) return; /* No choice to record. */

	assert(lkg->num_links < lkg->lasz, "Linkage array too small!");
	Link *link = &lkg->link_array[lkg->num_links];
	link->lw = pc->set[lr]->lw;
	link->rw = pc->set[lr]->rw;
	link->lc = lc;
	link->rc = rc;
	lkg->num_links++;
}

static inline void issue_links_for_choice(Linkage lkg, Parse_choice *pc,
                                          const Parse_set *set)
{
	issue_link(lkg, /*lr*/0, pc, set);
	issue_link(lkg, /*lr*/1, pc, set);
}

void metric_extractor_free(extractor_t *pex);

#endif /* _EXTRACT_LINKS_INTERNAL_H */
