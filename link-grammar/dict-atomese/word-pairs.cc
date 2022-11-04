/*
 * word-pairs.cc
 *
 * Create disjuncts consisting of optionals, from word-pairs.
 *
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 */
#ifdef HAVE_ATOMESE

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/atoms/value/FloatValue.h>
#include <opencog/nlp/types/atom_types.h>
#undef STRINGIFY

extern "C" {
#include "../link-includes.h"            // For Dictionary
#include "../dict-common/dict-common.h"  // for Dictionary_s
#include "../dict-common/dict-utils.h"   // for size_of_expression()
#include "../dict-ram/dict-ram.h"
};

#include "local-as.h"

using namespace opencog;

/// Return true if word-pairs for `germ` need to be fetched.
static bool need_pair_fetch(Local* local, const Handle& germ)
{
	const ValuePtr& fpv = germ->getValue(local->prk);
	if (fpv) return false;

	// Just tag it. We could tag it with a date. That way, we'd know
	// how old it is, to re-fetch it !?
	local->asp->set_value(germ, local->prk, createFloatValue(1.0));
	return true;
}

/// Get the word-pairs for `germ` from storage.
/// Return zero, if there are none; else return non-zero.
static size_t fetch_pairs(Local* local, const Handle& germ)
{
	local->stnp->fetch_incoming_by_type(germ, LIST_LINK);
	local->stnp->barrier();

	size_t cnt = 0;
	HandleSeq rprs = germ->getIncomingSetByType(LIST_LINK);
	for (const Handle& rawpr : rprs)
	{
		local->stnp->fetch_incoming_by_type(rawpr, EVALUATION_LINK);
		cnt++;
	}
	return cnt;
}

/// Return true if there are any word-pairs for `germ`.
/// The will automatically fetch from storage, as needed.
static bool have_pairs(Local* local, const Handle& germ)
{
	if (need_pair_fetch(local, germ))
		fetch_pairs(local, germ);

	const AtomSpacePtr& asp = local->asp;
	const Handle& hpr = local->prp; // (Predicate "word-pair")

	// Are there any pairs in the local AtomSpace?
	// If there's at least one, just return `true`.
	HandleSeq rprs = germ->getIncomingSetByType(LIST_LINK);
	for (const Handle& rawpr : rprs)
	{
		Handle evpr = asp->get_link(EVALUATION_LINK, hpr, rawpr);
		if (evpr)
		{
			// Verify that its an actual word-pair.
			if (evpr->getOutgoingAtom(0) == local->prp)
				return true;
		}
	}

	return false;
}

/// Return true if the given word occurs in some word-pair, else return
/// false. As a side-effect, word-pairs are loaded from storage.
bool pair_boolean_lookup(Dictionary dict, const char *s)
{
	Local* local = (Local*) (dict->as_server);
	Handle wrd = local->asp->add_node(WORD_NODE, s);

	// Are there any pairs that contain this word?
	bool have_word = have_pairs(local, wrd);

	// Does this word belong to any classes?
	size_t nclass = wrd->getIncomingSetSizeByType(MEMBER_LINK);
	if (0 == nclass and local->stnp)
	{
		local->stnp->fetch_incoming_by_type(wrd, MEMBER_LINK);
		local->stnp->barrier();
	}

	for (const Handle& memb : wrd->getIncomingSetByType(MEMBER_LINK))
	{
		const Handle& wcl = memb->getOutgoingAtom(1);
		if (WORD_CLASS_NODE != wcl->get_type()) continue;

		// If there's at least one, return it.
		if (have_pairs(local, wcl)) return true;
	}

	return have_word;
}

/// Create a list of connectors, one for each available word pair
/// containing the word in the germ. These are simply OR'ed together.
Exp* make_pair_exprs(Dictionary dict, const Handle& germ)
{
	Local* local = (Local*) (dict->as_server);
	const AtomSpacePtr& asp = local->asp;
	Exp* orhead = nullptr;

	const Handle& hpr = local->prp; // (Predicate "pair")

	HandleSeq rprs = germ->getIncomingSetByType(LIST_LINK);
	for (const Handle& rawpr : rprs)
	{
		Handle evpr = asp->get_link(EVALUATION_LINK, hpr, rawpr);
		if (nullptr == evpr) continue;

		double cost = local->pair_default;
		const ValuePtr& mivp = evpr->getValue(local->mikp);
		if (mivp)
		{
			// MI is the second entry in the vector.
			const FloatValuePtr& fmivp = FloatValueCast(mivp);
			double mi = fmivp->value()[local->pair_index];
			cost = (local->pair_scale * mi) + local->pair_offset;
		}
		// If the cost is too high, just skip this.
		if (local->pair_cutoff <= cost)
			continue;

		// Get the cached link-name for this pair.
		const std::string& slnk = cached_linkname(local, rawpr);

		// Direction is easy to determine: its either left or right.
		char cdir = '+';
		if (rawpr->getOutgoingAtom(1) == germ) cdir  = '-';

		// Create the connector
		Exp* eee = make_connector_node(dict,
			dict->Exp_pool, slnk.c_str(), cdir, false);

		eee->cost = cost;

		or_enchain(dict, orhead, eee);
	}

	return orhead;
}

// ===============================================================

/// Create exprs that consist of a Cartesian product of pairs.
/// Given a word, a lookup is made to find all word-pairs holding
/// that word. This is done by `make_pair_exprs()`, above. Then
/// this is ANDED against itself N times, and the result is returned.
/// The N is the `arity` argument.
///
/// For example, if `make_pair_exprs()` returns `(A+ or B- or C+)`
/// and arity is 3, then this will return `(A+ or B- or C+ or ())
/// and (A+ or B- or C+ or ()) and (A+ or B- or C+ or ())`. When
/// this is exploded into disjuncts, any combination is possible,
/// from size zero to three. That's why its a Cartesian product.
///
/// FYI, this is a work-around for the lack of a commmutative
/// multi-product. What we really want to do here is to have the
/// expression `(@A+ com @B- com @C+)` where `com` is a commutative
/// product.  The `@` sign denotes a multi-connector, so that `@A+`
/// is the same things as `(() or A+ or (A+ & A+) or ...)` and the
/// commutative product allows any of these to commute, i.e. so that
/// disjuncts such as `(A+ & C+ & A+ & C+)` are possible. But we do
/// not have such a commutative multi-product, and so we fake it with
/// a plain cartesian product. The only issue is that this eats up
/// RAM. At least RAM use is linear: it goes as `O(arity)`.  More
/// precisely, as `O(npairs x arity)`.
Exp* make_cart_pairs(Dictionary dict, const Handle& germ,
                     int arity, bool with_any)
{
	if (0 >= arity) return nullptr;

	Exp* andhead = nullptr;
	Exp* andtail = nullptr;

	Exp* epr = make_pair_exprs(dict, germ);
	if (nullptr == epr) return nullptr;

	// Tack on ANY connectors, if requested.
	if (with_any)
	{
		Exp* ap = make_any_exprs(dict);
		epr = make_or_node(dict->Exp_pool, epr, ap);
	}
	Exp* optex = make_optional_node(dict->Exp_pool, epr);

	// If its 1-dimensional, we are done.
	if (1 == arity) return optex;

	and_enchain_right(dict, andhead, andtail, optex);

	for (int i=1; i< arity; i++)
	{
		Exp* opt = make_optional_node(dict->Exp_pool, epr);
		and_enchain_right(dict, andhead, andtail, opt);
	}

	// Could verify that it all multiplies out as expected.
	// lg_assert(arity * size_of_expression(epr) ==
	//           size_of_expression(andhead));

	return andhead;
}

// ===============================================================

/// Create exprs that are cartesian products of ANY links. The
/// corresponding disjuncts will have `arity` number of connectors.
/// If these are used all by themselves, the resulting parses will
/// be random planar graphs; i.e. will be equivalent to the `any`
/// language parses.
Exp* make_any_exprs(Dictionary dict)
{
	// Create a pair of ANY-links that can connect either left or right.
	Exp* aneg = make_connector_node(dict, dict->Exp_pool, "ANY", '-', true);
	Exp* apos = make_connector_node(dict, dict->Exp_pool, "ANY", '+', true);

	Local* local = (Local*) (dict->as_server);
	aneg->cost = local->any_default;
	apos->cost = local->any_default;

	Exp* any = make_or_node(dict->Exp_pool, aneg, apos);

	return any;
}

/// Much like make_part_pairs, except that this duplicates the
/// ANY connector. It creates the expression
/// {@ANY- or @ANY+} and {@ANY- or @ANY+} and ... and {@ANY- or @ANY+}
/// This cartesian allows multiple connectors to participate in loops.
/// However, the behavior is ... sruprising. See
///    https://github.com/opencog/link-grammar/issues/1351
/// for a discussion of what this is all about.
Exp* make_cart_any(Dictionary dict, int arity)
{
	if (0 >= arity) return nullptr;

	Exp* andhead = nullptr;
	Exp* andtail = nullptr;

	Exp* any = make_any_exprs(dict);

	Exp* optex = make_optional_node(dict->Exp_pool, any);

	// If its 1-dimensional, we are done.
	if (1 == arity) return optex;

	and_enchain_right(dict, andhead, andtail, optex);

	for (int i=1; i< arity; i++)
	{
		Exp* opt = make_optional_node(dict->Exp_pool, any);
		and_enchain_right(dict, andhead, andtail, opt);
	}

	return andhead;
}

// ===============================================================
#endif // HAVE_ATOMESE
