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
	bool using_external_as;
	AtomSpacePtr asp;
	StorageNodePtr stnp;   // Might be nullptr
	Handle linkp;          // (Predicate "*-LG link string-*")
	Handle prk;            // (Predicate "*-fetched-pair-*")

	// Sections
	Handle miks;           // (Predicate "*-Mutual Info Key cover-section")
	int cost_index;        // Offset into the FloatValue
	double cost_cutoff;    // MI below this is rejected
	double cost_scale;
	double cost_offset;
	double cost_default;

	// Word-pairs
	Handle prp;            // (Predicate "*-word pair-*")
	Handle mikey;          // (Predicate "*-Mutual Info Key-*")
	Handle miformula;      // (DefinedProcedure "*-dynamic MI ANY")
	int pair_index;        // Offset into the FloatValue
	double pair_cutoff;    // MI below this is rejected
	double pair_scale;
	double pair_offset;
	double pair_default;

	// Any link type
	double any_default;

	// Basic Sections
	bool enable_sections;
	int extra_pairs;
	bool extra_any;

	// Disjuncts made from pairs
	int pair_disjuncts;
	bool pair_with_any;
	bool any_disjuncts;

	bool enable_unknown_word;
};

bool section_boolean_lookup(Dictionary dict, const char *s);
bool pair_boolean_lookup(Dictionary dict, const char *s);

Exp* make_exprs(Dictionary dict, const Handle& germ);
Exp* make_sect_exprs(Dictionary dict, const Handle& germ);
Exp* make_pair_exprs(Dictionary dict, const Handle& germ);
Exp* make_cart_pairs(Dictionary dict, const Handle& germ, int arity, bool any);
Exp* make_any_exprs(Dictionary dict);

void or_enchain(Dictionary, Exp* &orhead, Exp*);
void and_enchain_left(Dictionary, Exp* &orhead, Exp* &ortail, Exp*);
void and_enchain_right(Dictionary, Exp* &orhead, Exp* &ortail, Exp*);

std::string cached_linkname(Local*, const Handle& pair);

//----
