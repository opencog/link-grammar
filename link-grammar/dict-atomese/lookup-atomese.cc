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
#include <opencog/util/oc_assert.h>
#include <opencog/util/Logger.h>

#undef STRINGIFY

extern "C" {
#include "../link-includes.h"              // For Dictionary
#include "../api-structures.h"             // For Sentence_s
#include "../dict-common/dict-common.h"    // for Dictionary_s
#include "../dict-common/dict-internals.h" // for dict_node_new()
#include "../dict-common/dict-utils.h"     // for size_of_expression()
#include "../dict-ram/dict-ram.h"
#include "../tokenize/word-structures.h"   // for Word_struct
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
	local->pair_dict = nullptr;

	// If an external atomspace is specified, then use that.
	if (external_atomspace)
	{
		lgdebug(D_USER_BASIC, "Atomese: Attach external AtomSpace: `%s`\n",
			external_atomspace->get_name().c_str());
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
			lgdebug(D_USER_BASIC, "Atomese: Using storage %s\n",
				hsn->to_short_string().c_str());
		}
		else
			lgdebug(D_USER_BASIC, "Atomese: No external storage specified.\n");
	}
	else
	{
		lgdebug(D_USER_BASIC, "Atomese: Using private Atomspace\n");

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
			prt_error(
				"Warning: Using a private AtomSpace with no StorageNode!\n"
				"Are you sure?\n"
				"All parses will be random planar parses using the ANY link.\n"
				"This is probably not what you wanted!\n");
	}

	// Create the connector predicate.
	// This will be used to avoid accidental aliasing.
	local->bany = local->asp->add_node(BOND_NODE, "ANY");

	// Internal-use only. Do we have this pair yet?
	local->prk = local->asp->add_node(PREDICATE_NODE, "*-fetched-pair-*");

	// Internal-use only. What is the last ID we've issued?
	local->idanch = local->asp->add_node(ANCHOR_NODE, "*-LG-issued-link-id-*");
	local->last_id = 0;

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
	else
	{
		local->extra_pairs = 0;
	}

	// -----------------------------------------------------
	// Pair stuff.

	local->pair_disjuncts = atoi(LDEF(PAIR_DISJUNCTS_STRING, "4"));

	// Setting pair-disjuncts to zero disables pair prococessing
	if (0 < local->pair_disjuncts or 0 < local->extra_pairs)
	{
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

		local->pair_with_any = atoi(LDEF(PAIR_WITH_ANY_STRING, "1"));

		local->pair_dict = create_pair_cache_dict(dict);
	}

	local->any_disjuncts = atoi(LDEF(ANY_DISJUNCTS_STRING, "0"));

	// The any_default is used to set the costs on UNKNOWN-WORD
	// XXX FIXME, maybe there should be a different parameter for this?
	local->any_default = atof(LDEF(ANY_DEFAULT_STRING, "2.6"));

	dict->as_server = (void*) local;

	local->any_expr = make_any_exprs(dict, dict->Exp_pool);

	local->enable_unknown_word = atoi(LDEF(ENABLE_UNKNOWN_WORD_STRING, "1"));
	if (local->enable_unknown_word)
	{
		const char* ukw = string_set_add(UNKNOWN_WORD, dict->string_set);
		make_dn(dict, local->any_expr, ukw);
	}

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

	// Get last issued link ID.
	fetch_link_id(local);

	return true;
}

/// Close the connection to the StorageNode (e.g. CogServer.)
/// To be used only if the everything has been fetched, and the
/// dict is now in local RAM. The dict remains usable, after
/// closing the connection. Only local StorageNodes are closed.
/// External storage nodes will remain open, but will no longer
/// be used.
void as_storage_close(Dictionary dict)
{
	if (nullptr == dict->as_server) return;
	Local* local = (Local*) (dict->as_server);

	// Record the last unissued id.
	store_link_id(local);

	if (not local->using_external_as and local->stnp)
		local->stnp->close();

	local->stnp = nullptr;
}

/// Close the connection to the AtomSpace. This will also empty out
/// the local dictionary, and so the dictionary will no longer be
/// usable after a close.
void as_close(Dictionary dict)
{
	logger().info("Atomese: Close dict `%s`", dict->name);

	as_storage_close(dict);

	if (nullptr == dict->as_server) return;
	Local* local = (Local*) (dict->as_server);
	if (local->pair_dict)
	{
		free_dict_node_recursive(local->pair_dict->root);
		free(local->pair_dict);
	}

	delete local;
	dict->as_server = nullptr;

	// Clear the cache as well
	free_dictionary_root(dict);
	dict->num_entries = 0;
}

// ===============================================================

/**
 * Specialized version of `condesc_setup()`, tuned for Atomese.
 * When new expressions are created (even temporary ones, e.g. for MST)
 * the connectors in those expressions are placed into a connector
 * hash table. The connectors are then compiled into a form that is
 * used to speed up matching. For a one-time dictionary load, this is
 * done by `condesc_setup()`.
 *
 * For Atomese, new connectors dribble in with each new sentance.
 * As there are millions grand-total, calling `condesc_setup()` is
 * stunningly inefficient; only the new ones need to be processed.
 * That's what this routine does.
 *
 * Limitations:
 * ++ All connectors are marked UNLIMITED_LEN.
 * ++ All connectors are assumed to be upper-case only, with no
 *    lower-case parts. (That means that there will never be two or
 *    more connectors with the same upper-case part, thus shortening
 *    the qsort, below.)
 */
static void update_condesc(Dictionary dict)
{
	ConTable *ct = &dict->contable;

	// How many new ones are there?
	size_t num_new = ct->num_con - ct->last_num;
	if (0 == num_new) return;

	condesc_t **sdesc = (condesc_t **) malloc(num_new * sizeof(condesc_t *));
	size_t i = 0;
	for (size_t n = 0; n < ct->size; n++)
	{
		condesc_t *condesc = ct->hdesc[n].desc;

		if (NULL == condesc) continue;
		if (UINT32_MAX != condesc->uc_num) continue;

		calculate_connector_info(&ct->hdesc[n]);
		condesc->more->length_limit = UNLIMITED_LEN;
		sdesc[i++] = condesc;
	}

	qsort(sdesc, num_new, sizeof(condesc_t *),
	      condesc_by_uc_constring);

	/* Enumerate the connectors according to their UC part. */
	int uc_num = ct->num_uc;

	sdesc[0]->uc_num = uc_num;
	for (size_t n=1; n < num_new; n++)
	{
		condesc_t **condesc = &sdesc[n];

		if (condesc[0]->more->uc_length != condesc[-1]->more->uc_length)
		{
			/* We know that the UC part has been changed. */
			uc_num++;
		}
		else
		{
			const char *uc1 = &condesc[0]->more->string[condesc[0]->more->uc_start];
			const char *uc2 = &condesc[-1]->more->string[condesc[-1]->more->uc_start];
			if (0 != strncmp(uc1, uc2, condesc[0]->more->uc_length))
			{
				uc_num++;
			}
		}

		//printf("%5d constring=%s\n", uc_num, condesc[0]->string);
		condesc[0]->uc_num = uc_num;
	}

	lgdebug(+11, "Dictionary %s: added %zu different connectors "
	        "(%d with a different UC part)\n",
	        dict->name, ct->num_con, uc_num+1);

	ct->num_uc = uc_num + 1;
	ct->last_num = ct->num_con;

	free(sdesc);
}

// ===============================================================

// The sentence that this thread is working with. Assume one thread
// per Sentence, which should be a perfectly valid assumption.
thread_local Sentence sentlo = nullptr;
thread_local HandleSeq sent_words;

/// Make a note of all of the words in the sentence. We need this,
/// to pre-prune MST word-pairs, as well as disjuncts decorated with
/// extra pairs.
///
/// XXX A future enhancement would be to pre-prune disjuncts as well,
/// although it is not obvious that we could do any better here, than
/// the generic expression pruning code could do.
void as_start_lookup(Dictionary dict, Sentence sent)
{
	Local* local = (Local*) (dict->as_server);
	sentlo = sent;

	lgdebug(D_USER_INFO, "Atomese: Start dictionary lookup for >>%s<<\n",
		sent->orig_sentence);

	if (0 < local->pair_disjuncts or 0 < local->extra_pairs)
	{
		for(size_t i=0; i<sent->length; i++)
		{
			const char* wstr = sent->word[i].unsplit_word;
			if (wstr)
			{
				if (0 == i && 0 == strcmp(wstr, LEFT_WALL_WORD))
					wstr = "###LEFT-WALL###";

				Handle wrd = local->asp->get_node(WORD_NODE, wstr);
				if (wrd)
					sent_words.push_back(wrd);
			}
			int j = 0;
			const char* astr = sent->word[i].alternatives[j];
			while (astr)
			{
				Handle wrd = local->asp->get_node(WORD_NODE, astr);
				if (wrd)
					sent_words.push_back(wrd);
				j++;
				astr = sent->word[i].alternatives[j];
			}
		}
	}
}

void as_end_lookup(Dictionary dict, Sentence sent)
{
	Local* local = (Local*) (dict->as_server);
	sentlo = nullptr;

	if (0 < local->pair_disjuncts or 0 < local->extra_pairs)
		sent_words.clear();

	lgdebug(D_USER_INFO, "Atomese: Finish dictionary lookup for >>%s<<\n",
		sent->orig_sentence);

	// Create connector descriptors for any new connectors.
	std::lock_guard<std::mutex> guard(local->dict_mutex);
	update_condesc(dict);
}

/// Thread-safe dict lookup.
static bool locked_dict_node_exists_lookup(Dictionary dict, const char *s)
{
	Local* local = (Local*) (dict->as_server);
	std::lock_guard<std::mutex> guard(local->dict_mutex);
	return dict_node_exists_lookup(dict, s);
}

/// Return true if the given word can be found in the dictionary,
/// else return false.
bool as_boolean_lookup(Dictionary dict, const char *s)
{
	if (locked_dict_node_exists_lookup(dict, s))
	{
		lgdebug(D_USER_INFO, "Atomese: Found in local dict: >>%s<<\n", s);
		return true;
	}

	Local* local = (Local*) (dict->as_server);
	if (local->enable_unknown_word and 0 == strcmp(s, "<UNKNOWN-WORD>"))
		return true;

	if (0 == strcmp(s, LEFT_WALL_WORD))
		s = "###LEFT-WALL###";

	if (local->enable_sections and section_boolean_lookup(dict, s))
	{
		if (0 < local->extra_pairs or 0 < local->pair_disjuncts)
			pair_boolean_lookup(dict, s);
		return true;
	}

	if ((0 < local->pair_disjuncts or 0 < local->extra_pairs) and
	    pair_boolean_lookup(dict, s))
	{
		return true;
	}

	return false;
}

// ===============================================================

/// Add `item` to the linked list `orhead`, using a OR operator.
void or_enchain(Pool_desc* pool, Exp* &orhead, Exp* item)
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
		orhead = make_or_node(pool, item, orhead);
		return;
	}

	/* Link new connectors to the head */
	item->operand_next = orhead->operand_first;
	orhead->operand_first = item;
}

/// Add `item` to the left end of the linked list `andhead`, using
/// an AND operator. The `andtail` is used to track the right side
/// of the list.
void and_enchain_left(Pool_desc* pool, Exp* &andhead, Exp* &andtail, Exp* item)
{
	if (nullptr == item) return; // no-op
	if (nullptr == andhead)
	{
		andhead = make_and_node(pool, item, NULL);
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
void and_enchain_right(Pool_desc* pool, Exp* &andhead, Exp* &andtail, Exp* item)
{
	if (nullptr == item) return; // no-op
	if (nullptr == andhead)
	{
		andhead = make_and_node(pool, item, NULL);
		return;
	}

	/* Link new connectors to the tail */
	if (nullptr == andtail)
		andtail = andhead->operand_first;

	andtail->operand_next = item;
	andtail = item;
}

// Report Number of entries in the dict, and also RAM usage.
static void report_dict_usage(Dictionary dict)
{
	// We could also print pool_num_elements_issued() but this is not
	// interesting; its slightly less than pool_size().
	logger().info("LG Dict: %lu entries; %lu Exp_pool elts; %lu MiBytes",
		dict->num_entries,
		pool_size(dict->Exp_pool),
		pool_bytes(dict->Exp_pool) / (1024 * 1024));
}

/// Given an expression, wrap it with a Dict_node and insert it into
/// the dictionary.
void make_dn(Dictionary dict, Exp* exp, const char* ssc)
{
	Dict_node* dn = dict_node_new();
	dn->string = ssc;
	dn->exp = exp;

	// Cache the result; avoid repeated lookups.
	dict->root = dict_node_insert(dict, dict->root, dn);
	dict->num_entries++;

	// Rebalance the tree every now and then.
	if (0 == dict->num_entries%60)
	{
		dict->root = dsw_tree_to_vine(dict->root);
		report_dict_usage(dict);
		dict->root = dsw_vine_to_tree(dict->root, dict->num_entries);
	}
}

const char* ss_add(const char *s, Dictionary dict)
{
	Local* local = (Local*) (dict->as_server);
	std::lock_guard<std::mutex> guard(local->dict_mutex);
	return string_set_add(s, dict->string_set);
}

/// Given a word, return the collection of Dict_nodes holding the
/// expressions for that word.
Dict_node * as_lookup_list(Dictionary dict, const char *s)
{
	// The tokenizer might call us, asking about COMMON_ENTITY_MARKER
	// in order to figure out if a word can be down-cased. This will
	// happen outside the sentence-begin-end expression context, so
	// sentlo will be null. Just ignore this; we don't have entity
	// markers. Anyway, handling of this kind of stuff needs to be
	// done differently, anyway. Meanwhile, avoid null ptr derefernce.
	if (nullptr == sentlo) return nullptr;

	Local* local = (Local*) (dict->as_server);

	if (0 == strcmp(s, LEFT_WALL_WORD)) s = "###LEFT-WALL###";
	const char* ssc = ss_add(s, dict);
	Handle wrd = local->asp->get_node(WORD_NODE, ssc);
	if (nullptr == wrd)
	{
		if (local->enable_unknown_word)
			return dict_node_lookup(dict, UNKNOWN_WORD);
		return nullptr;
	}

	// Create expressions consisting entirely of word-pair links.
	// These are "temporary", and always go into Sentence::Exp_pool.
	Exp* cpr = nullptr;
	if (0 < local->pair_disjuncts)
		cpr = make_cart_pairs(dict, wrd, sentlo->Exp_pool, sent_words,
		                      local->pair_disjuncts,
		                      local->pair_with_any);

	// If sections are enabled, get those. Or-chain them to pairs.
	if (local->enable_sections)
	{
		Dict_node* dn = lookup_section(dict, wrd);
		if (nullptr == cpr) return dn;

		// Splice in pairs.
		if (dn)
		{
			dn->exp = make_or_node(sentlo->Exp_pool, dn->exp, cpr);
			return dn;
		}
	}

	// If we are here, then there are only pairs, no sections.
	if (cpr)
	{
		Dict_node * dn = dict_node_new();
		dn->string = ssc;
		dn->exp = cpr;
		return dn;
	}

	// Create disjuncts consisting entirely of "ANY" links.
	if (local->any_disjuncts)
	{
		std::lock_guard<std::mutex> guard(local->dict_mutex);

		// If it's cached, just return that.
		Dict_node* dn = dict_node_lookup(dict, ssc);
		if (dn) return dn;

		// Make a new one.
		make_dn(dict, local->any_expr, ssc);
		return dict_node_lookup(dict, ssc);
	}

	OC_ASSERT(0, "Internal Error! Must have sections, pair or any!");
	return nullptr;
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
// This can be called by link-parser command line, with the !clear
// "special" command.
void as_clear_cache(Dictionary dict)
{
	Local* local = (Local*) (dict->as_server);
	printf("Prior to clear, dict has %d entries, Atomspace has %lu Atoms\n",
		dict->num_entries, local->asp->get_size());

	// Free the dict nodes. Free the pair-cache, too.
	// Reuse the existing Exp pool.
	free_dict_node_recursive(dict->root);
	free_dict_node_recursive(local->pair_dict->root);
	pool_reuse(dict->Exp_pool);

	local->have_pword.clear();

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
