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

using namespace opencog;

void as_add_categories(Dictionary dict)
{
printf ("hello world\n");

	Local* local = (Local*) (dict->as_server);

	dict->num_categories = 0;
	dict->num_categories_alloced = 3; // XXX
	dict->category = (Category*) malloc(dict->num_categories_alloced *
	                        sizeof(*dict->category));

#if 0
		dict->category[i].name =
			string_set_add(argv[0], dict->string_set);
		dict->category[i].exp
		dict->category[i].num_words
		dict->category[i].word =
			malloc(bs.count * sizeof(*dict->category[0].word));

		dict->num_categories = i;
#endif

	/* Set the termination entry. */
	dict->category[dict->num_categories + 1].num_words = 0;
}

#endif // HAVE_ATOMESE
