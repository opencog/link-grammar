/*
 * local-as.h
 *
 * Local copy of AtomSpace
 *
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 */

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/persist/api/StorageNode.h>

using namespace opencog;

class Local
{
public:
	bool using_external_as;
	const char* node_str; // (StorageNode \"foo://bar/baz\")
	AtomSpacePtr asp;
	StorageNodePtr stnp;
	Handle linkp;     // (Predicate "*-LG connector string-*")
	// Handle djp;       // (Predicate "*-LG disjunct string-*")
	Handle prk;       // (Predicate "*-fetched-pair-*")

	// Sections
	Handle miks;      // (Predicate "*-Mutual Info Key cover-section")
	int cost_index;   // Offset into the FloatValue
	double cost_scale;
	double cost_offset;
	double cost_cutoff;
	double cost_default;

	// Word-pairs
	Handle prp;       // (Predicate "*-word pair-*")
	Handle mikp;      // (Predicate "*-Mutual Info Key-*")
	int pair_index;   // Offset into the FloatValue
	double pair_scale;
	double pair_offset;
	double pair_cutoff;
	double pair_default;

	// Any link type
	double any_default;

	// Basic Sections
	bool enable_sections;
	int extra_pairs;
	int extra_any;

	// Disjuncts made from pairs
	int pair_disjuncts;
	int pair_with_any;
	int any_disjuncts;
};

bool section_boolean_lookup(Dictionary dict, const char *s);
bool pair_boolean_lookup(Dictionary dict, const char *s);

Exp* make_exprs(Dictionary dict, const Handle& germ);
Exp* make_sect_exprs(Dictionary dict, const Handle& germ);
Exp* make_pair_exprs(Dictionary dict, const Handle& germ);
Exp* make_cart_pairs(Dictionary dict, const Handle& germ, int arity);
Exp* make_any_exprs(Dictionary dict, int arity);

void or_enchain(Dictionary, Exp* &orhead, Exp*);
void and_enchain_left(Dictionary, Exp* &orhead, Exp* &ortail, Exp*);
void and_enchain_right(Dictionary, Exp* &orhead, Exp* &ortail, Exp*);

std::string cached_linkname(Local*, const Handle& pair);
