//
// Demo program showing a bad aspell memleak.
// This will leak a Gigabyte of RSS every few minutes.
//
// See https://github.com/opencog/link-grammar/discussions/1373
// for details.
//
// Compile with
// cc -o aspell-memleak aspell-memleak.c -laspell
// Run as ./aspell-memleak
//
// ------
// Linas Vepstas 16 January 2023

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aspell.h>

int main()
{
	AspellConfig *config = new_aspell_config();
	aspell_config_replace(config, "lang", "en_US");

	AspellCanHaveError *spell_err = new_aspell_speller(config);
	AspellSpeller *speller = to_aspell_speller(spell_err);

	size_t k=0;
	char* word = "asdf";
	for (int l=0; l<1000000; l++)
	{
		/* Returns 1 if the word is in dict. */
		int found = aspell_speller_check(speller, word, -1);
		// printf("Found the word: %d\n", found);

		const AspellWordList *list = aspell_speller_suggest(speller, word, -1);
		AspellStringEnumeration *elem = aspell_word_list_elements(list);
		unsigned int size = aspell_word_list_size(list);

		const char *aword = NULL;
		while ((aword = aspell_string_enumeration_next(elem)) != NULL)
		{
			// printf("Spell suggesion: %s\n", aword);
			k++;
		}
		delete_aspell_string_enumeration(elem);

		if (0 == l%20000)
		{
			printf("\n");
			printf("Loop count= %d spell suggests= %lu\n", l, k);
			malloc_stats();
		}
	}

	delete_aspell_speller(speller);
	delete_aspell_config(config);
}
