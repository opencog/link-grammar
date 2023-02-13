/*
 * local-as.h
 *
 * Local copy of AtomSpace
 *
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 */

#include <mutex>
#include <opencog/atomspace/AtomSpace.h>
#include <opencog/persist/api/StorageNode.h>

using namespace opencog;

class Local
{
public:
	std::mutex dict_mutex; // Avoid corruption of Dictionary

	// General housekeeping
	bool using_external_as;
	AtomSpacePtr asp;
	StorageNodePtr stnp;   // Might be nullptr
	Handle linkp;          // (Predicate "*-LG link string-*")
	Handle prk;            // (Predicate "*-fetched-pair-*")

	// Section config
	bool enable_sections;  // Enable use of sections
	int extra_pairs;       // Supplement sections with pairs
	bool extra_any;        // Supplement sections with ANY

	// Sections
	Handle miks;           // (Predicate "*-Mutual Info Key cover-section")
	int cost_index;        // Offset into the FloatValue
	double cost_cutoff;    // MI below this is rejected.
	double cost_default;   // This MI is used if key is missing.
	double cost_scale;
	double cost_offset;

	// Word-pair config -- Disjuncts made from pairs
	int pair_disjuncts;    // Max number of pair connectors in a disjunct.
	bool pair_with_any;    // Add ANY pairs to pair disjuncts.

	// Word-pairs
	Dictionary pair_dict;  // Cache of pre-computed word-pair exprs.
	Handle prp;            // (Predicate "*-word pair-*")
	Handle mikey;          // (Predicate "*-Mutual Info Key-*")
	Handle miformula;      // (DefinedProcedure "*-dynamic MI ANY")
	int pair_index;        // Offset into the FloatValue
	double pair_cutoff;    // MI below this is rejected.
	double pair_default;   // This MI is used if key is missing.
	double pair_scale;
	double pair_offset;

	// Any link type
	bool any_disjuncts;   // Create disjuncts created entirely from ANY
	double any_default;   // Default cost (post-scaling)

	bool enable_unknown_word;
};

Dictionary create_pair_cache_dict(Dictionary);

bool section_boolean_lookup(Dictionary dict, const char *s);
bool pair_boolean_lookup(Dictionary dict, const char *s);

Exp* make_sect_exprs(Dictionary dict, const Handle& germ);
Exp* make_cart_pairs(Dictionary dict, const Handle& germ,
                     const std::vector<std::string>&,
                     int arity, bool any);
Exp* make_any_exprs(Dictionary dict);

void make_dn(Dictionary, Exp*, const char*);

void or_enchain(Dictionary, Exp* &orhead, Exp*);
void and_enchain_left(Dictionary, Exp* &orhead, Exp* &ortail, Exp*);
void and_enchain_right(Dictionary, Exp* &orhead, Exp* &ortail, Exp*);

std::string cached_linkname(Local*, const Handle& pair);

//----
