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
	for (size_t i=0; i<=ncat; i++)
	{
#if 0
		dict->category[i].name =
			string_set_add(argv[0], dict->string_set);
		dict->category[i].exp
		dict->category[i].num_words
		dict->category[i].word =
			malloc(bs.count * sizeof(*dict->category[0].word));

		dict->num_categories = i;
#endif
	}

	/* Set the termination entry. */
	dict->category[dict->num_categories + 1].num_words = 0;
}

#endif // HAVE_ATOMESE
