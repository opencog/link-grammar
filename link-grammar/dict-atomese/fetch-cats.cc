/*
 * fetch-cats.cc
 *
 * Fetch everything; needed for generation.
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
	dict->category = (Category*) malloc(dict->num_categories_alloced *
	                        sizeof(*dict->category));

printf("duude got %lu cats\n", ncat);

	HandleSeq allcl;
	local->asp->get_handles_by_type(allcl, WORD_CLASS_NODE);

	for (size_t i=0; i<allcl.size(); i++)
	{
		const Handle& wcl = allcl[i];

		size_t j = i + 1;
		dict->category[j].name =
			string_set_add(wcl->get_name().c_str(), dict->string_set);
printf("duude %lu catcl=%s\n", j, dict->category[j].name);

		dict->category[j].exp = make_exprs(dict, wcl);

		size_t nwo = wcl->getIncomingSetSizeByType(MEMBER_LINK);
		dict->category[j].word = (const char**)
			malloc(nwo * sizeof(*dict->category[0].word));

		nwo = 0;
		for (const Handle& memb : wcl->getIncomingSetByType(MEMBER_LINK))
		{
			const Handle& wrd = memb->getOutgoingAtom(0);
			if (WORD_NODE != wrd->get_type()) continue;

			dict->category[j].word[nwo] =
				string_set_add(wrd->get_name().c_str(), dict->string_set);
printf("duude %lu catl= %s %lu %s\n", j, dict->category[j].name, nwo,
dict->category[j].word[nwo]);
			nwo++;
		}
		dict->category[j].num_words = nwo;
	}

	HandleSeq allwo;
	local->asp->get_handles_by_type(allwo, WORD_NODE);

	for (size_t i=0; i<allwo.size(); i++)
	{
		size_t j = i + nclasses + 1;
		dict->category[j].name =
			string_set_add(allwo[i]->get_name().c_str(), dict->string_set);
printf("duude %lu catwo=%s\n", j, dict->category[j].name);

		dict->category[j].exp = make_exprs(dict, allwo[i]);
		dict->category[j].num_words = 1;
		dict->category[j].word =
			(const char**) malloc(sizeof(*dict->category[0].word));
		dict->category[j].word[0] = dict->category[j].name;
	}

	/* Set the termination entry. */
	dict->category[dict->num_categories + 1].num_words = 0;
}

#endif // HAVE_ATOMESE
