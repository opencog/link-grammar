/*
 * lookup-atomese.cc
 *
 * Implement the word-lookup callbacks
 *
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 */
#ifdef HAVE_ATOMESE

#include <cstdlib>
#include <opencog/atomspace/AtomSpace.h>
#include <opencog/persist/api/StorageNode.h>
#include <opencog/persist/cog-simple/CogSimpleStorage.h>
#include <opencog/persist/cog-storage/CogStorage.h>
#include <opencog/persist/file/FileStorage.h>
#include <opencog/persist/monospace/MonoStorage.h>
#include <opencog/persist/rocks/RocksStorage.h>
#include <opencog/persist/sexpr/Sexpr.h>
#include <opencog/nlp/types/atom_types.h>
#include <opencog/util/Logger.h>

#undef STRINGIFY

extern "C" {
#include "../link-includes.h"              // For Dictionary
#include "../dict-common/dict-common.h"    // for Dictionary_s
#include "../dict-common/dict-internals.h" // for dict_node_new()
#include "../dict-common/dict-utils.h"     // for size_of_expression()
#include "../dict-ram/dict-ram.h"
#include "lookup-atomese.h"
};

#include "dict-atomese.h"
#include "local-as.h"

using namespace opencog;

// Strings we expect to find in the dictionary.
#define STORAGE_NODE_STRING "storage-node"
#define COST_KEY_STRING "cost-key"
#define COST_INDEX_STRING "cost-index"
#define COST_CUTOFF_STRING "cost-cutoff"
#define COST_DEFAULT_STRING "cost-default"
#define COST_SCALE_STRING "cost-scale"
#define COST_OFFSET_STRING "cost-offset"

#define PAIR_PREDICATE_STRING "pair-predicate"
#define PAIR_KEY_STRING "pair-key"
#define PAIR_INDEX_STRING "pair-index"
#define PAIR_FORMULA_STRING "pair-formula"
#define PAIR_CUTOFF_STRING "pair-cutoff"
#define PAIR_DEFAULT_STRING "pair-default"
#define PAIR_SCALE_STRING "pair-scale"
#define PAIR_OFFSET_STRING "pair-offset"

#define ANY_DEFAULT_STRING "any-default"
#define ENABLE_SECTIONS_STRING "enable-sections"
#define EXTRA_PAIRS_STRING "extra-pairs"
#define EXTRA_ANY_STRING "extra-any"

#define PAIR_DISJUNCTS_STRING "pair-disjuncts"
#define PAIR_WITH_ANY_STRING "pair-with-any"

#define ANY_DISJUNCTS_STRING "any-disjuncts"

#define ENABLE_UNKNOWN_WORD_STRING "enable-unknown-word"

/// Shared global
static AtomSpacePtr external_atomspace;
static StorageNodePtr external_storage;

/// Specify an AtomSpace, and optionally, a StorageNode that will be
/// used to fetch dictionary contents. This should be done before
/// opening the dictionary; the values passed here are grabbed by the
/// dictionary when it is opened.
void lg_config_atomspace(AtomSpacePtr asp, StorageNodePtr sto)
{
	external_atomspace = asp;
	external_storage = sto;
}

static const char* get_dict_define(Dictionary dict, const char* namestr)
{
	const char* val_str =
		linkgrammar_get_dict_define(dict, namestr);
	if (nullptr == val_str) return nullptr;

	// Brute-force unescape quotes. Simple, dumb.
	char* unescaped = (char*) alloca(strlen(val_str)+1);
	const char* p = val_str;
	char* q = unescaped;
	while (*p) { if ('\\' != *p) { *q = *p; q++; } p++; }
	*q = 0x0;

	return string_set_add(unescaped, dict->string_set);
}

static const char* ldef(Dictionary dict, const char* name, const char* def)
{
	const char* str = linkgrammar_get_dict_define(dict, name);
	if (str) return str;
	prt_error("Warning: missing `%s` in config; default to %s\n",
		name, def);
	return def;
}

/// Open a connection to a StorageNode.
bool as_open(Dictionary dict)
{
	Local* local = new Local;
	local->stnp = nullptr;

	// If an external atomspace is specified, then use that.
	if (external_atomspace)
	{
		local->using_external_as = true;
		local->asp = external_atomspace;
		if (external_storage)
		{
			Handle hsn = local->asp->add_atom(external_storage);
			local->stnp = StorageNodeCast(hsn);
			if (nullptr == local->stnp)
			{
				prt_error("Error: Not a StorageNode! %s\n", hsn->to_string().c_str());
				return false;
			}
		}
	}
	else
	{
		local->using_external_as = false;
		local->asp = createAtomSpace();

		const char * stns = get_dict_define(dict, STORAGE_NODE_STRING);

		// A local AtomSpace means that there *must* be local storage,
		// too, else there is no where to get dict data from. The grand
		// exception would be "any" parsing, with random parse trees,
		// which doesn't require a dictionary. So issue a strong warning,
		// but not an error.
		if (stns)
		{
			Handle hsn = Sexpr::decode_atom(stns);
			hsn = local->asp->add_atom(hsn);
			local->stnp = StorageNodeCast(hsn);
			if (nullptr == local->stnp)
			{
				prt_error("Error: Not a StorageNode! %s\n", stns);
				return false;
			}
		}
		else
			prt_error("Warning: No StorageNode was specified! Are you sure?\n");
	}

	// Create the connector predicate.
	// This will be used to cache LG connector strings.
	local->linkp = local->asp->add_node(PREDICATE_NODE, "*-LG link string-*");

	// Internal-use only. Do we have this pair yet?
	local->prk = local->asp->add_node(PREDICATE_NODE, "*-fetched-pair-*");

	// -----------------------------------------------------
	// Rationale for default values for costs:
	//
	// Word-pair MI's only "make sense" when they are 4.0 or greater.
	// The highest-possible MI depends on the dataset, but is roughly
	// equal to log_2 of the number of word-pairs in the dataset. The
	// MI for most "good" word-pairs will be in the 4 to 20 range.
	//
	// Multiplying by -0.25 gives them a cost of -1.0 or more negative.
	// Setting the offset to 3.0 gives them a cost of 2.0 or less.
	// This maps the MI interval [4,12] to [2,0] with MI's larger than
	// 12 being mapped to negative costs (thus encouraging cycles).
	//
	// Setting the cutoff to 2.0 causes MI of less than 4 to be dropped.
	// Setting the default assigns a default cost to pairs without MI.

#define LDEF(NAME,DEF) ldef(dict, NAME, DEF)

	local->enable_sections = atoi(LDEF(ENABLE_SECTIONS_STRING, "1"));
	if (local->enable_sections)
	{
		const char* miks = ldef(dict, COST_KEY_STRING,
			"(Predicate \"*-Mutual Info Key cover-section\")");
		Handle mikh = Sexpr::decode_atom(miks);
		local->miks = local->asp->add_atom(mikh);

		local->cost_index = atoi(LDEF(COST_INDEX_STRING, "1"));
		local->cost_cutoff = atof(LDEF(COST_CUTOFF_STRING, "4.0"));
		local->cost_default = atof(LDEF(COST_DEFAULT_STRING, "4.01"));
		local->cost_scale = atof(LDEF(COST_SCALE_STRING, "-0.25"));
		local->cost_offset = atof(LDEF(COST_OFFSET_STRING, "3.0"));
		local->extra_pairs = atoi(LDEF(EXTRA_PAIRS_STRING, "1"));
		local->extra_any = atoi(LDEF(EXTRA_ANY_STRING, "1"));
	}

	// -----------------------------------------------------
	// Pair stuff.

	// Costs might be static, precomputed, located at a given key.
	// Or they may be dynamic, coming from a formula.
	const char* mikp = get_dict_define(dict, PAIR_KEY_STRING);
	const char* mifm = get_dict_define(dict, PAIR_FORMULA_STRING);
	if (mikp and mifm)
		prt_error("Error: Only one of `pair-key` or `pair-formula` allowed!\n");
	else if (nullptr == mikp and nullptr == mifm)
		prt_error("Error: One of `pair-key` or `pair-formula` must be given!\n");

	if (mikp)
	{
		Handle miki = Sexpr::decode_atom(mikp);
		local->mikey = local->asp->add_atom(miki);
	}
	if (mifm)
	{
		Handle mifl = Sexpr::decode_atom(mifm);
		local->miformula = local->asp->add_atom(mifl);
	}

	const char* prps = LDEF(PAIR_PREDICATE_STRING, "(BondNode \"ANY\")");
	Handle prph = Sexpr::decode_atom(prps);
	local->prp = local->asp->add_atom(prph);

	local->pair_index = atoi(LDEF(PAIR_INDEX_STRING, "1"));
	local->pair_cutoff = atof(LDEF(PAIR_CUTOFF_STRING, "4.0"));
	local->pair_default = atof(LDEF(PAIR_DEFAULT_STRING, "4.01"));
	local->pair_scale = atof(LDEF(PAIR_SCALE_STRING, "-0.25"));
	local->pair_offset = atof(LDEF(PAIR_OFFSET_STRING, "3.0"));

	local->any_default = atof(LDEF(ANY_DEFAULT_STRING, "2.6"));

	local->pair_disjuncts = atoi(LDEF(PAIR_DISJUNCTS_STRING, "4"));
	local->pair_with_any = atoi(LDEF(PAIR_WITH_ANY_STRING, "1"));
	local->any_disjuncts = atoi(LDEF(ANY_DISJUNCTS_STRING, "0"));

	local->enable_unknown_word = atoi(LDEF(ENABLE_UNKNOWN_WORD_STRING, "1"));

	dict->as_server = (void*) local;

	if (local->using_external_as) return true;
	if (nullptr == local->stnp) return true;

	// --------------------
	// If we are here, then we manage our own private connection
	// to storage. We will be managing opening and closing of it,
	// and the fetching of data from it.

	std::string stone = local->stnp->to_short_string();
	const char * stoname = stone.c_str();

#define SHLIB_CTOR_HACK 1
#ifdef SHLIB_CTOR_HACK
	/* The cast below forces the shared lib constructor to run. */
	/* That's needed to force the factory to get installed. */
	/* We need a more elegant solution to this. */
	Type snt = local->stnp->get_type();
	if (COG_SIMPLE_STORAGE_NODE == snt)
		local->stnp = CogSimpleStorageNodeCast(local->stnp);
	else if (COG_STORAGE_NODE == snt)
		local->stnp = CogStorageNodeCast(local->stnp);
	else if (FILE_STORAGE_NODE == snt)
		local->stnp = FileStorageNodeCast(local->stnp);
	else if (MONO_STORAGE_NODE == snt)
		local->stnp = MonoStorageNodeCast(local->stnp);
	else if (ROCKS_STORAGE_NODE == snt)
		local->stnp = RocksStorageNodeCast(local->stnp);
	else
	{
		prt_error("Error: Unknown storage %s\n", stoname);
		return false;
	}
#endif

	local->stnp->open();

	if (local->stnp->connected())
		prt_error("Info: Connected to %s\n", stoname);
	else
	{
		prt_error("Error: Failed to connect to %s\n", stoname);
		return false;
	}

	// If a formula is in use, then we need to fetch the
	// formula definition (the miformula is just it's name;
	// its a DefinedProcedureNode)
	if (local->miformula)
	{
		local->stnp->fetch_incoming_by_type(local->miformula, DEFINE_LINK);
		local->stnp->barrier();
	}

	return true;
}

/// Close the connection to the StorageNode (e.g. cogserver.)
/// To be used only if the everything has been fetched, and the
/// dict is now in local RAM. The dict remains usable, after
/// closing the connection. Only local StorageNodes are closed.
/// External storage nodes will remain open, but will no longer
/// be used.
void as_storage_close(Dictionary dict)
{
	if (nullptr == dict->as_server) return;
	Local* local = (Local*) (dict->as_server);

	if (not local->using_external_as and local->stnp)
		local->stnp->close();

	local->stnp = nullptr;
}

/// Close the connection to the AtomSpace. This will also empty out
/// the local dictionary, and so the dictionary will no longer be
/// usable after a close.
void as_close(Dictionary dict)
{
	if (nullptr == dict->as_server) return;
	Local* local = (Local*) (dict->as_server);
	if (not local->using_external_as and local->stnp)
		local->stnp->close();

	delete local;
	dict->as_server = nullptr;

	// Clear the cache as well
	free_dictionary_root(dict);
	dict->num_entries = 0;
}

// ===============================================================

void as_start_lookup(Dictionary dict, Sentence sent)
{
}

void as_end_lookup(Dictionary dict, Sentence sent)
{
	Local* local = (Local*) (dict->as_server);
	std::lock_guard<std::mutex> guard(local->dict_mutex);

	// Deal with any new connector descriptors that have arrived.
	condesc_setup(dict);
}

/// Return true if the given word can be found in the dictionary,
/// else return false.
bool as_boolean_lookup(Dictionary dict, const char *s)
{
	Local* local = (Local*) (dict->as_server);
	std::lock_guard<std::mutex> guard(local->dict_mutex);

	bool found = dict_node_exists_lookup(dict, s);
	if (found) return true;

	if (local->enable_unknown_word and 0 == strcmp(s, "<UNKNOWN-WORD>"))
		return true;

	if (0 == strcmp(s, LEFT_WALL_WORD))
		s = "###LEFT-WALL###";

	if (local->enable_sections)
		found = section_boolean_lookup(dict, s);

	if (0 < local->pair_disjuncts or 0 < local->extra_pairs)
	{
		bool have_pairs = pair_boolean_lookup(dict, s);
		found = found or have_pairs;
	}

	return found;
}

// ===============================================================

/// Add `item` to the linked list `orhead`, using a OR operator.
void or_enchain(Dictionary dict, Exp* &orhead, Exp* item)
{
	if (nullptr == item) return; // no-op

	if (nullptr == orhead)
	{
		orhead = item;
		return;
	}

	// Unary OR-nodes are not allowed; they will cause assorted
	// algos to croak. So the first OR node must have two.
	if (OR_type != orhead->type)
	{
		orhead = make_or_node(dict->Exp_pool, item, orhead);
		return;
	}

	/* Link new connectors to the head */
	item->operand_next = orhead->operand_first;
	orhead->operand_first = item;
}

/// Add `item` to the left end of the linked list `andhead`, using
/// an AND operator. The `andtail` is used to track the right side
/// of the list.
void and_enchain_left(Dictionary dict, Exp* &andhead, Exp* &andtail, Exp* item)
{
	if (nullptr == item) return; // no-op
	if (nullptr == andhead)
	{
		andhead = make_and_node(dict->Exp_pool, item, NULL);
		return;
	}

	/* Link new connectors to the head */
	item->operand_next = andhead->operand_first;
	andhead->operand_first = item;

	if (nullptr == andtail)
		andtail = item->operand_next;
}

/// Add `item` to the right end of the linked list `andhead`, using
/// an AND operator. The `andtail` is used to track the right side
/// of the list.
void and_enchain_right(Dictionary dict, Exp* &andhead, Exp* &andtail, Exp* item)
{
	if (nullptr == item) return; // no-op
	if (nullptr == andhead)
	{
		andhead = make_and_node(dict->Exp_pool, item, NULL);
		return;
	}

	/* Link new connectors to the tail */
	if (nullptr == andtail)
		andtail = andhead->operand_first;

	andtail->operand_next = item;
	andtail = item;
}

/// Build expressions for the dictionary word held by `germ`.
/// Exactly what is built is controlled by the configuration:
/// it will be some mixture of sections, word-pairs, and ANY
/// links.
Exp* make_exprs(Dictionary dict, const Handle& germ)
{
	Local* local = (Local*) (dict->as_server);

	Exp* orhead = nullptr;

	// Create disjuncts consisting entirely of "ANY" links.
	if (local->any_disjuncts)
	{
		Exp* any = make_any_exprs(dict);
		or_enchain(dict, orhead, any);
	}

	// Create disjuncts consisting entirely of word-pair links.
	if (0 < local->pair_disjuncts or 0 < local->extra_pairs)
	{
		Exp* cpr = make_cart_pairs(dict, germ, local->pair_disjuncts,
		                           local->pair_with_any);
		or_enchain(dict, orhead, cpr);
	}

	// Create disjuncts from Sections
	if (local->enable_sections)
	{
		Exp* sects = make_sect_exprs(dict, germ);
		or_enchain(dict, orhead, sects);
	}

	return orhead;
}

static void report_dict_usage(Dictionary dict)
{
	Dict_node* most_used = nullptr;
	Dict_node* least_used = nullptr;
	unsigned long most_cnt = 0;
	unsigned long least_cnt = 4000123123;
	for (Dict_node* dn = dict->root; dn; dn = dn->right)
	{
		if (dn->use_count > most_cnt)
		{
			most_cnt = dn->use_count;
			most_used = dn;
		}
		if (dn->use_count < least_cnt)
		{
			least_cnt = dn->use_count;
			least_used = dn;
		}
	}
	logger().info("LG Dict: %lu entries; most-used: %lu %s least-used: %lu %s",
		dict->num_entries, most_cnt, most_used->string,
		least_cnt, least_used->string);

	// Also report RAM usage.
	size_t psz = pool_size(dict->Exp_pool);
	size_t apparent = (psz * dict->Exp_pool->element_size) / (1024 * 1024);
	size_t actual = pool_bytes(dict->Exp_pool) / (1024 * 1024);
	logger().info("LG Dict: %lu of %lu pool elts in use. %lu/%lu MiBytes apparent/actual",
		pool_num_elements_issued(dict->Exp_pool), psz, apparent, actual);
}

/// Given an expression, wrap  it with a Dict_node and insert it into
/// the dictionary.
static Dict_node * make_dn(Dictionary dict, Exp* exp, const char* ssc)
{
	Dict_node* dn = dict_node_new();
	dn->string = ssc;
	dn->exp = exp;

	// Cache the result; avoid repeated lookups.
	dict->root = dict_node_insert(dict, dict->root, dn);
	dict->num_entries++;

	lgdebug(+D_SPEC+5, "as_lookup_list %d for >>%s<< nexpr=%d\n",
		dict->num_entries, ssc, size_of_expression(exp));

	// Rebalance the tree every now and then.
	if (0 == dict->num_entries%30)
	{
		dict->root = dsw_tree_to_vine(dict->root);
		report_dict_usage(dict);
		dict->root = dsw_vine_to_tree(dict->root, dict->num_entries);
	}

	// Perform the lookup. We cannot return the dn above, as the
	// dict_node_free_lookup() will delete it, leading to mem corruption.
	dn = dict_node_lookup(dict, ssc);
	return dn;
}

/// Given a word, return the collection of Dict_nodes holding the
/// expressions for that word.
Dict_node * as_lookup_list(Dictionary dict, const char *s)
{
	Local* local = (Local*) (dict->as_server);
	std::lock_guard<std::mutex> guard(local->dict_mutex);

	// Do we already have this word cached? If so, pull from
	// the cache.
	Dict_node* dn = dict_node_lookup(dict, s);

	if (dn) return dn;

	const char* ssc = string_set_add(s, dict->string_set);

	if (local->enable_unknown_word and 0 == strcmp(s, "<UNKNOWN-WORD>"))
	{
		Exp* exp = make_any_exprs(dict);
		return make_dn(dict, exp, ssc);
	}

	if (0 == strcmp(s, LEFT_WALL_WORD))
		s = "###LEFT-WALL###";

	Handle wrd = local->asp->get_node(WORD_NODE, s);
	if (nullptr == wrd) return nullptr;

	// Get expressions, where the word itself is the germ.
	Exp* exp = make_exprs(dict, wrd);

	// Get expressions, where the word is in some class.
	for (const Handle& memb : wrd->getIncomingSetByType(MEMBER_LINK))
	{
		const Handle& wcl = memb->getOutgoingAtom(1);
		if (WORD_CLASS_NODE != wcl->get_type()) continue;

		Exp* clexp = make_exprs(dict, wcl);
		if (nullptr == clexp) continue;

		lgdebug(+D_SPEC+5, "as_lookup_list class for >>%s<< nexpr=%d\n",
			ssc, size_of_expression(clexp));

		if (nullptr == exp)
			exp = clexp;
		else
			exp = make_or_node(dict->Exp_pool, exp, clexp);
	}

	if (nullptr == exp)
		return nullptr;

	return make_dn(dict, exp, ssc);
}

// This is supposed to provide a wild-card lookup.  However,
// There is currently no way to support a wild-card lookup in the
// atomspace: there is no way to ask for all WordNodes that match
// a given regex.  There's no regex predicate... this can be hacked
// around in various elegant and inelegant ways, e.g. adding a
// regex predicate to the AtomSpace. Punt for now. This is used
// only for the !! command in the parser command-line tool.
// XXX FIXME. But low priority.
Dict_node * as_lookup_wild(Dictionary dict, const char *s)
{
	Dict_node * dn = dict_node_wild_lookup(dict, s);
	if (dn) return dn;

	as_lookup_list(dict, s);
	return dict_node_wild_lookup(dict, s);
}

// Zap all the Dict_nodes that we've added earlier.
// This clears out everything hanging on dict->root
// as well as the expression pool.
// And also the local AtomSpace.
//
void as_clear_cache(Dictionary dict)
{
	Local* local = (Local*) (dict->as_server);
	printf("Prior to clear, dict has %d entries, Atomspace has %lu Atoms\n",
		dict->num_entries, local->asp->get_size());

	dict->Exp_pool = pool_new(__func__, "Exp", /*num_elements*/4096,
	                             sizeof(Exp), /*zero_out*/false,
	                             /*align*/false, /*exact*/false);

	// Clear the local AtomSpace too.
	// Easiest way to do this is to just close and reopen
	// the connection.

	AtomSpacePtr savea = external_atomspace;
	StorageNodePtr saves = external_storage;
	if (local->using_external_as)
	{
		external_atomspace = local->asp;
		external_storage = local->stnp;
	}

	as_close(dict);
	as_open(dict);
	external_atomspace = savea;
	external_storage = saves;
	as_boolean_lookup(dict, LEFT_WALL_WORD);
}
#endif /* HAVE_ATOMESE */
