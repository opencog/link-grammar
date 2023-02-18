/*
 * fetch-cats.cc
 *
 * Fetch the entire contents of the dict; needed for generation.
 *
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 */
#ifdef HAVE_ATOMESE

extern "C" {
#include "../link-includes.h"            // For Dictionary
#include "../dict-common/dict-common.h"  // for Dictionary_s
#include "lookup-atomese.h"
};

#include "local-as.h"
#include <opencog/nlp/types/atom_types.h>

using namespace opencog;

// This works only with sections; it won't handle pairs or MST things.
void as_add_categories(Dictionary dict)
{
	Local* local = (Local*) (dict->as_server);

	// Get all sections.
	if (local->stnp)
	{
		local->stnp->fetch_all_atoms_of_type(SECTION);
		local->stnp->barrier();
	}

	// Assume one category per word and word-class.
	// (i.e. that all words will have sections)
	size_t nclasses = local->asp->get_num_atoms_of_type(WORD_CLASS_NODE);
	size_t nwords = local->asp->get_num_atoms_of_type(WORD_NODE);
	size_t ncat = nclasses + nwords;

	dict->num_categories = ncat;
	dict->num_categories_alloced = ncat + 2;
	dict->category = (Category*) malloc(
		dict->num_categories_alloced * sizeof(*dict->category));

	// First, loop over all word-classes
	HandleSeq allcl;
	local->asp->get_handles_by_type(allcl, WORD_CLASS_NODE);

	size_t j = 1;
	for (size_t i=0; i<allcl.size(); i++)
	{
		const Handle& wcl = allcl[i];

		// Some word-classes might not be in any sections.
		dict->category[j].exp = make_sect_exprs(dict, wcl);
		if (nullptr == dict->category[j].exp)
			continue;

		// Crufty dicts can contain empty cats.
		size_t nwo = wcl->getIncomingSetSizeByType(MEMBER_LINK);
		if (0 == nwo)
			continue;

		dict->category[j].name =
			string_set_add(wcl->get_name().c_str(), dict->string_set);

		// Copy the words into the cateory.
		dict->category[j].word = (const char**)
			malloc(nwo * sizeof(*dict->category[0].word));

		nwo = 0;
		for (const Handle& memb : wcl->getIncomingSetByType(MEMBER_LINK))
		{
			const Handle& wrd = memb->getOutgoingAtom(0);
			if (WORD_NODE != wrd->get_type()) continue;

			dict->category[j].word[nwo] =
				string_set_add(wrd->get_name().c_str(), dict->string_set);
			nwo++;
		}
		dict->category[j].num_words = nwo;
		j++;
	}

	// Add individual words.
	HandleSeq allwo;
	local->asp->get_handles_by_type(allwo, WORD_NODE);

	for (size_t i=0; i<allwo.size(); i++)
	{
		dict->category[j].exp = make_sect_exprs(dict, allwo[i]);
		if (nullptr == dict->category[j].exp)
			continue;

		dict->category[j].name =
			string_set_add(allwo[i]->get_name().c_str(), dict->string_set);

		dict->category[j].num_words = 1;
		dict->category[j].word =
			(const char**) malloc(sizeof(*dict->category[0].word));
		dict->category[j].word[0] = dict->category[j].name;
		j++;
	}

	j--;  // First j was empty (on purpose)

	/* Free excess memory, if any */
	dict->num_categories_alloced = j + 2;
	dict->category = (Category*) realloc(dict->category,
		dict->num_categories_alloced * sizeof(*dict->category));

	/* Set the termination entry. */
	dict->num_categories = j;
	dict->category[j + 1].num_words = 0;
}

#endif // HAVE_ATOMESE
