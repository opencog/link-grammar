/*
 * link-generator.c
 *
 * Generate random corpora from dictionaries.
 * February 2021
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../link-grammar/link-includes.h"

int main (int argc, char* argv[])
{
	const char     *language = NULL;
	Dictionary      dict;
	Parse_Options   opts;
	Sentence        sent = NULL;

	/* Process options used by GNU programs. */
	for (int i = 1; i < argc; i++)
	{
		if (strcmp("--help", argv[i]) == 0)
		{
			printf("Usage: %s <language|dictionary>\n", argv[0]);
			exit(0);
		}

		if (strcmp("--version", argv[i]) == 0)
		{
			printf("Version: %s\n", linkgrammar_get_version());
			printf("%s\n", linkgrammar_get_configuration());
			exit(0);
		}
	}

	if ((argc > 1) && (argv[1][0] != '-')) {
		/* The dictionary is the first argument if it doesn't begin with "-" */
		language = argv[1];
	}

	if (language && *language)
	{
		dict = dictionary_create_lang(language);
		if (dict == NULL)
		{
			prt_error("Fatal error: Unable to open dictionary.\n");
			exit(-1);
		}
	}
	else
	{
		prt_error("Fatal error: A language must be specified!\n");
		exit(-1);
	}

	opts = parse_options_create();
	parse_options_set_linkage_limit(opts, 350);

	sent = sentence_create("6", dict);
	// sentence_split(sent, opts);
	int num_linkages = sentence_parse(sent, opts);
	printf("Linakges generated: %d\n", num_linkages);

	int linkages_found = sentence_num_linkages_found(sent);
	printf("Linakges found: %d\n", linkages_found);

	int linkages_valid = sentence_num_valid_linkages(sent);
	printf("Linakges valid: %d\n", linkages_valid);

	for (int i=0; i<num_linkages; i++)
	{
		Linkage linkage;
		linkage = linkage_create(i, sent, opts);

		size_t nwords = linkage_get_num_words(linkage);
		const char **words = linkage_get_words(linkage);

		printf("%d ", i);
		for (unsigned int w=0; w<nwords; w++)
		{
			printf(" %s", words[w]);
		}
		printf("\n");

		linkage_delete(linkage);
	}

	dictionary_delete(dict);
	printf ("Bye.\n");
	return 0;
}
