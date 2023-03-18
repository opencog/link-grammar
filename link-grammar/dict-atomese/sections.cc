/*
 * sections.cc
 *
 * Translate AtomSpace Sections to LG Exp's.
 *
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 */
#ifdef HAVE_ATOMESE

#include <cstdlib>
#include <opencog/util/oc_assert.h>
#include <opencog/atomspace/AtomSpace.h>
#include <opencog/nlp/types/atom_types.h>

#undef STRINGIFY

extern "C" {
#include "../link-includes.h"              // For Dictionary
#include "../api-structures.h"             // For Sentence_s
#include "../dict-common/dict-common.h"    // for Dictionary_s
#include "../dict-common/dict-internals.h" // for dict_node_new()
#include "../dict-common/dict-utils.h"     // for size_of_expression()
#include "../dict-ram/dict-ram.h"
#include "lookup-atomese.h"
};

#include "local-as.h"

using namespace opencog;

// ===============================================================

/// Count the number of times that the `germ` appears in some
/// Section.
static size_t count_sections(Local* local, const Handle& germ)
{
	// Are there any Sections in the local atomspace?
	size_t nsects = germ->getIncomingSetSizeByType(SECTION);
	if (0 < nsects or nullptr == local->stnp) return nsects;

	local->stnp->fetch_incoming_by_type(germ, SECTION);
	local->stnp->barrier();
	nsects = germ->getIncomingSetSizeByType(SECTION);

	return nsects;
}

/// Return true if the given word can be found as the germ of some
/// Section, else return false. As a side-effect, Sections are loaded
/// from storage.
bool section_boolean_lookup(Dictionary dict, const char *s)
{
	double start = 0.0;
	if (D_USER_TIMES <= verbosity) start = total_usage_time();

	Local* local = (Local*) (dict->as_server);
	Handle wrd = local->asp->add_node(WORD_NODE, s);

	// Are there any Sections for this word in the local atomspace?
	size_t nwrdsects = count_sections(local, wrd);

	// Does this word belong to any classes?
	size_t nclass = wrd->getIncomingSetSizeByType(MEMBER_LINK);
	if (0 == nclass and local->stnp)
	{
		local->stnp->fetch_incoming_by_type(wrd, MEMBER_LINK);
		local->stnp->barrier();
	}

	size_t nclssects = 0;
	for (const Handle& memb : wrd->getIncomingSetByType(MEMBER_LINK))
	{
		const Handle& wcl = memb->getOutgoingAtom(1);
		if (WORD_CLASS_NODE != wcl->get_type()) continue;
		nclssects += count_sections(local, wcl);
	}

#define RES_COL_WIDTH 37
	if (D_USER_TIMES <= verbosity)
	{
		double now = total_usage_time();
		char s[128] = "";
		snprintf(s, sizeof(s),
			"Classes: %lu Sections: %lu %lu for >>%s<<",
			nclass, nwrdsects, nclssects, s);
		prt_error("Atomese: %-*s %6.2f seconds\n",
			RES_COL_WIDTH, s, now - start);
	}

	return 0 != (nwrdsects + nclssects);
}

// ===============================================================

/// Return a cached LG-compatible link string.
/// Assigns a new name, if one does not exist.
/// `germ` is the germ of the section, `ctcr` is one of the connectors.
static std::string get_linkname(Local* local, const Handle& germ,
                                const Handle& ctcr)
{
	// The connection target is the first Atom in the connector
	const Handle& tgt = ctcr->getOutgoingAtom(0);
	const Handle& dir = ctcr->getOutgoingAtom(1);
	const std::string& sdir = dir->get_name();

	// The link is the connection of both of these.
	if ('+' == sdir[0])
	{
		const Handle& lnk = local->asp->add_link(LIST_LINK, germ, tgt);
		return cached_linkname(local, lnk);
	}

	// Else direction is '-' and tgt comes before germ.
	const Handle& lnk = local->asp->add_link(LIST_LINK, tgt, germ);
	return cached_linkname(local, lnk);
}

// Debugging utility
void print_section(Dictionary dict, const Handle& sect)
{
	Local* local = (Local*) (dict->as_server);

	const Handle& germ = sect->getOutgoingAtom(0);
	printf("(%s: ", germ->get_name().c_str());

	const Handle& conseq = sect->getOutgoingAtom(1);
	for (const Handle& ctcr : conseq->getOutgoingSet())
	{
		// The connection target is the first Atom in the connector
		const Handle& tgt = ctcr->getOutgoingAtom(0);
		const Handle& dir = ctcr->getOutgoingAtom(1);
		const std::string& sdir = dir->get_name();
		printf("%s%c ", tgt->get_name().c_str(), sdir[0]);
	}
	printf(") == ");

	bool first = true;
	for (const Handle& ctcr : conseq->getOutgoingSet())
	{
		// Assign an upper-case name to the link.
		std::string slnk = get_linkname(local, germ, ctcr);

		const Handle& dir = ctcr->getOutgoingAtom(1);
		const std::string& sdir = dir->get_name();

		if (not first) printf("& ");
		first = false;
		printf("%s%c ", slnk.c_str(), sdir[0]);
	}
	printf("\n");
	fflush(stdout);
}

// ===============================================================

extern thread_local Sentence sentlo;
extern thread_local HandleSeq sent_words;

Exp* make_sect_exprs(Dictionary dict, const Handle& germ)
{
	Local* local = (Local*) (dict->as_server);
	Exp* orhead = nullptr;
	Exp* extras = nullptr;

	// Create some optional word-pair links; this may be nullptr.
	if (0 < local->extra_pairs)
	{
		extras = make_cart_pairs(dict, germ, sentlo->Exp_pool,
		                         sent_words,
		                         local->extra_pairs,
		                         local->extra_any);
	}

	// Loop over all Sections on the word.
	HandleSeq sects = germ->getIncomingSetByType(SECTION);
	for (const Handle& sect: sects)
	{
		// Apparently, some sections are missing MI's. This is
		// most likey due to some data-processing bug, where the
		// MI's were not recomputed. For now, we will silently
		// ignore this issue, and assign a default to them.
		double mi = local->cost_default;

		const ValuePtr& mivp = sect->getValue(local->miks);
		if (mivp)
		{
			// MI is the second entry in the vector.
			const FloatValuePtr& fmivp = FloatValueCast(mivp);
			mi = fmivp->value()[local->cost_index];
		}

		// If the MI is too low, just skip this.
		if (mi < local->cost_cutoff)
			continue;

		Exp* andhead = nullptr;
		Exp* andtail = nullptr;

		// The connector sequence the second Atom.
		// Loop over the connectors in the connector sequence.
		// The atomspace stores connectors in word-order, while LG
		// stores them as distance-from given word.  Thus, we have
		// to reverse the order when building the expression.
		const Handle& conseq = sect->getOutgoingAtom(1);
		for (const Handle& ctcr : conseq->getOutgoingSet())
		{
			// The direction is the second Atom in the connector
			const Handle& dir = ctcr->getOutgoingAtom(1);
			char cdir = dir->get_name().c_str()[0];

			/* Assign an upper-case name to the link. */
			const std::string& slnk = get_linkname(local, germ, ctcr);

			std::lock_guard<std::mutex> guard(local->dict_mutex);
			Exp* eee = make_connector_node(dict,
				dict->Exp_pool, slnk.c_str(), cdir, false);

			if ('+' == cdir)
				and_enchain_right(dict->Exp_pool, andhead, andtail, eee);
			else if ('-' == cdir)
				and_enchain_left(dict->Exp_pool, andhead, andtail, eee);
		}

		// Sanity-check the Section - this should never-ever happen!
		if (nullptr == andhead)
		{
			prt_error("Warning: Empty connector sequence for %s\n",
				sect->to_short_string().c_str());
			continue;
		}

		std::lock_guard<std::mutex> guard(local->dict_mutex);

		// Tack on extra connectors, as configured.
		if (extras)
		{
			Exp* optex = make_optional_node(dict->Exp_pool, extras);
			and_enchain_left(dict->Exp_pool, andhead, andtail, optex);
			optex = make_optional_node(dict->Exp_pool, extras);
			and_enchain_right(dict->Exp_pool, andhead, andtail, optex);
		}

		// Optional: shorten the expression,
		// if there's only one connector in it.
		if (nullptr == andhead->operand_first->operand_next)
		{
			Exp *tmp = andhead->operand_first;
			andhead->operand_first = NULL;
			// pool_free(dict->Exp_pool, andhead); // not needed!?
			andhead = tmp;
		}

		double cost = (local->cost_scale * mi) + local->cost_offset;
		andhead->cost = cost;

		// Save the exp-section pairing in the AtomSpace.
		// XXX Why did we think this was needed ??
		// What were we going to do with it?
		// cache_disjunct_string(local, sect, andhead);

#if DEBUG
		print_section(dict, sect);
		const char* wrd = germ->get_name().c_str();
		printf("Word: '%s'  Exp: %s\n", wrd, lg_exp_stringify(andhead));
#endif

		// Add it to the linked list.
		or_enchain(dict->Exp_pool, orhead, andhead);
	}

	return orhead;
}

#define ENCHAIN(ORHEAD,EXP) \
	if (nullptr == ORHEAD)   \
		ORHEAD = (EXP);       \
	else {                   \
		std::lock_guard<std::mutex> guard(local->dict_mutex); \
		ORHEAD = make_or_node(dict->Exp_pool, ORHEAD, (EXP)); \
	}

/// Given a word, return the collection of Dict_nodes holding the
/// expressions for that word. Valid only when sections are enabled.
/// Returns the "plain" node, without any word-pair decorations.
static Dict_node * lookup_plain_section(Dictionary dict, const char *s)
{
	Local* local = (Local*) (dict->as_server);

	// Do we already have this word cached? If so, pull from
	// the cache.
	{
		std::lock_guard<std::mutex> guard(local->dict_mutex);
		Dict_node* dn = dict_node_lookup(dict, s);
		if (dn) return dn;
	}

	const char* ssc = ss_add(s, dict);
	Handle germ = local->asp->get_node(WORD_NODE, ssc);
	if (nullptr == germ) return nullptr;

	Exp* exp = nullptr;

	// Create disjuncts consisting entirely of "ANY" links.
	if (local->any_disjuncts)
	{
		std::lock_guard<std::mutex> guard(local->dict_mutex);
		Exp* any = make_any_exprs(dict, dict->Exp_pool);
		or_enchain(dict->Exp_pool, exp, any);
	}

	// Create expressions from Sections
	Exp* sects = make_sect_exprs(dict, germ);
	ENCHAIN(exp, sects);

	// Get expressions, where the word is in some class.
	for (const Handle& memb : germ->getIncomingSetByType(MEMBER_LINK))
	{
		const Handle& wcl = memb->getOutgoingAtom(1);
		if (WORD_CLASS_NODE != wcl->get_type()) continue;

		Exp* clexp = make_sect_exprs(dict, wcl);
		if (nullptr == clexp) continue;

		lgdebug(+D_SPEC+5, "as_lookup_list class for >>%s<< nexpr=%d\n",
			ssc, size_of_expression(clexp));

		ENCHAIN(exp, clexp);
	}

	if (nullptr == exp)
		return nullptr;

	std::lock_guard<std::mutex> guard(local->dict_mutex);
	lgdebug(D_USER_INFO, "Atomese: Created plain expressions for >>%s<<\n", ssc);
	make_dn(dict, exp, ssc);
	return dict_node_lookup(dict, ssc);
}

/// Given a word, return the collection of Dict_nodes holding the
/// expressions for that word. Valid only when sections are enabled.
/// This is the "plain" expression, plus extra word-pair decorations,
/// if requested.
Dict_node * lookup_section(Dictionary dict, const char *s)
{
	Local* local = (Local*) (dict->as_server);

	// Get the "plain" expressions, without any addtional word-pair
	// decorations.
	Dict_node* dn = lookup_plain_section(dict, s);
	if (0 >= local->extra_pairs) return dn;

	// If we are here, then we have to tack on extra connectors,
	// created from word-pairs. Doing this naively results in
	// a combinatorial explosion, and so instead, we only add the
	// the word-pairs for words occurring in the sentence. These
	// must not be cached, and so they are rebuilt every time.
	// A per-thread Sentence::Exp_pool is used.

	if (0 == strcmp(s, LEFT_WALL_WORD)) s = "###LEFT-WALL###";
	const char* ssc = ss_add(s, dict);
	Handle germ = local->asp->get_node(WORD_NODE, ssc);
	Exp* extras = make_cart_pairs(dict, germ, sentlo->Exp_pool,
	                              sent_words,
	                              local->extra_pairs,
	                              local->extra_any);

	// No word-pairs found! Very unexpected, but OK.
	// Return the plain expression.
	if (nullptr == extras)
		return dn;

	Exp* andhead = nullptr;
	Exp* andtail = nullptr;

	// We realy expect dn to not be null here, but ... perhaps it
	// is, if its just not been observed before.
	Exp* eee = nullptr;
	if (dn)
	{
		eee = dn->exp;
		dict_node_free_lookup(dict, dn);
	}

	// Place the dictionary expression in the middle;
	// wrap on the left and right with extra word-pairs.
	and_enchain_right(sentlo->Exp_pool, andhead, andtail, eee);

	Exp* optex = make_optional_node(sentlo->Exp_pool, extras);
	and_enchain_left(sentlo->Exp_pool, andhead, andtail, optex);
	optex = make_optional_node(sentlo->Exp_pool, extras);
	and_enchain_right(sentlo->Exp_pool, andhead, andtail, optex);

	Dict_node * dsn = dict_node_new();
	dsn->string = ssc;
	dsn->exp = andhead;
	return dsn;
}

#endif /* HAVE_ATOMESE */
