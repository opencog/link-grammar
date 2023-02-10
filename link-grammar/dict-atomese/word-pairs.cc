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

// Create a mini-dict, used only for caching word-pair dict-nodes.
Dictionary create_pair_cache_dict(Dictionary dict)
{
	Dictionary prca = (Dictionary) malloc(sizeof(struct Dictionary_s));
	memset(prca, 0, sizeof(struct Dictionary_s));

	/* Shared string-set */
	prca->string_set = dict->string_set;
	prca->name = string_set_add("word-pair cache", dict->string_set);

	/* Shared Exp_pool, too */
	prca->Exp_pool = dict->Exp_pool;

	return prca;
}

/// Return true if word-pairs for `germ` need to be fetched.
static bool need_pair_fetch(Local* local, const Handle& germ)
{
	if (nullptr == local->stnp) return false;

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
	lgdebug(D_USER_INFO, "Atomese: Fetched %lu word-pairs for >>%s<<\n",
		cnt, germ->get_name().c_str());
	return cnt;
}

/// Get the word-pair cost. It's either a static cost, located at
/// `mikey`, or it's a dynamically-computed cost, built from a
/// formula at `miformula`.
static double pair_mi(Local* local, const Handle& evpr)
{
	double mi = local->pair_default;
	if (local->mikey)
	{
		const ValuePtr& mivp = evpr->getValue(local->mikey);
		if (mivp)
		{
			// MI is the second entry in the vector.
			const FloatValuePtr& fmivp = FloatValueCast(mivp);
			mi = fmivp->value()[local->pair_index];
		}
		return mi;
	}

	if (nullptr == local->miformula)
		return mi;

	// The formula expects a (ListLink left right) and that
	// is exactly the second Atom in the outgoing set of evpr.
	// TODO: perhaps in the future, the formula should take
	// evpr directly, instead?
	const AtomSpacePtr& asp = local->asp;
	Handle exout(asp->add_link(EXECUTION_OUTPUT_LINK,
		local->miformula, evpr->getOutgoingAtom(1)));
	ValuePtr midy = exout->execute(asp.get());

	// Same calculation as above.
	const FloatValuePtr& fmivp = FloatValueCast(midy);
	mi = fmivp->value()[local->pair_index];

	return mi;
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
		if (nullptr == evpr) continue;

		// The lookup will discard pairs with too low an MI.
		double mi = pair_mi(local, evpr);
		if (mi < local->pair_cutoff) continue;

		return true;
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
	if (have_pairs(local, wrd)) return true;

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

	return false;
}

/// Create a list of connectors, one for each available word pair
/// containing the word in the germ. These are simply OR'ed together.
static Exp* make_pair_exprs(Dictionary dict, const Handle& germ)
{
	Local* local = (Local*) (dict->as_server);
	const AtomSpacePtr& asp = local->asp;
	Exp* orhead = nullptr;

	const Handle& hpr = local->prp; // (Predicate "pair")

	size_t cnt = 0;
	HandleSeq rprs = germ->getIncomingSetByType(LIST_LINK);
	for (const Handle& rawpr : rprs)
	{
		Handle evpr = asp->get_link(EVALUATION_LINK, hpr, rawpr);
		if (nullptr == evpr) continue;

		double mi = pair_mi(local, evpr);

		// If the MI is too low, just skip this.
		if (mi < local->pair_cutoff)
			continue;

		// Get the cached link-name for this pair.
		const std::string& slnk = cached_linkname(local, rawpr);

		// Direction is easy to determine: its either left or right.
		char cdir = '+';
		if (rawpr->getOutgoingAtom(1) == germ) cdir  = '-';

		// Create the connector
		Exp* eee = make_connector_node(dict,
			dict->Exp_pool, slnk.c_str(), cdir, false);

		double cost = (local->pair_scale * mi) + local->pair_offset;
		eee->cost = cost;

		or_enchain(dict, orhead, eee);
		cnt ++;
	}

	lgdebug(D_USER_INFO, "Atomese: Created %lu of %lu pair exprs for >>%s<<\n",
		cnt, germ->getIncomingSetSizeByType(LIST_LINK),
		germ->get_name().c_str());

	return orhead;
}

/// Get a list of connectors, one for each available word pair
/// containing the word in the germ. These are simply OR'ed together.
/// Get them from the local dictionary cache, if they're already
/// there; create them from scratch, polling the AtomSpace.
Exp* get_pair_exprs(Dictionary dict, const Handle& germ)
{
	Local* local = (Local*) (dict->as_server);

	const char* wrd = germ->get_name().c_str();
	Dictionary prdct = local->pair_dict;
   Dict_node* dn = dict_node_lookup(prdct, wrd);

   if (dn)
   {
      lgdebug(D_USER_INFO, "Atomese: Found pairs in cache: >>%s<<\n", wrd);
		Exp* exp = dn->exp;
		dict_node_free_lookup(dn);
      return exp;
   }

	Exp* exp = make_pair_exprs(dict, germ);
	const char* ssc = string_set_add(wrd, dict->string_set);
	Dict_node* dn = make_dn(prdct, exp, ssc);

	// The make_dn made a copy, but we don't want it.
	dict_node_free_lookup(dn);
	return exp;
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

/// Create an expression having the form
///    @ANY- or @ANY+ or (@ANY- & @ANY+)
/// These are multi-connectors. The cost on each connector is the
/// default any-cost from the configuration.  Note that the use of
/// this expression will result in random planar parses between the
/// words having this expression. i.e. it will be the same as using
/// the `any` language to create random planar pares.
///
/// FYI, there is a minor issue for cost-accounting on multi-connectors;
/// see https://github.com/opencog/link-grammar/issues/1351 for details.
///
Exp* make_any_exprs(Dictionary dict)
{
	// Create a pair of ANY-links that can connect either left or right.
	Exp* aneg = make_connector_node(dict, dict->Exp_pool, "ANY", '-', true);
	Exp* apos = make_connector_node(dict, dict->Exp_pool, "ANY", '+', true);

	Local* local = (Local*) (dict->as_server);
	aneg->cost = local->any_default;
	apos->cost = local->any_default;

	Exp* any = make_or_node(dict->Exp_pool, aneg, apos);

	Exp* andy = make_and_node(dict->Exp_pool, aneg, apos);
	or_enchain(dict, any, andy);

	return any;
}

// ===============================================================
#endif // HAVE_ATOMESE
