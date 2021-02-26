/*
 * link-generator.c
 *
 * Generate random corpora from dictionaries.
 * February 2021
 */

#include <argp.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../link-grammar/link-includes.h"
#include "../link-grammar/error.h"

/* Argument parsing for the generator */
typedef struct
{
	const char* language;
	int sentence_length;
	int corpus_size;
} gen_parameters;

static struct argp_option options[] =
{
	{"length", 'l', "length", 0, "Sentence length."},
	{"size", 's', "size", 0, "Corpus size."},
	{"version", 'v', 0, 0, "Print version and exit."},
	{ 0 }
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	gen_parameters* gp = state->input;
	switch (key)
	{
		case 'l': gp->sentence_length = atoi(arg); break;
		case 's': gp->corpus_size = atoi(arg); break;

		case 'v':
		{
			printf("Version: %s\n", linkgrammar_get_version());
			printf("%s\n", linkgrammar_get_configuration());
			exit(0);
		}

		case ARGP_KEY_ARG: return 0;
		default: return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static char args_doc[] = "";
static struct argp argp = { options, parse_opt, args_doc, 0, 0, 0 };

int main (int argc, char* argv[])
{
	Dictionary      dict;
	Parse_Options   opts;
	Sentence        sent = NULL;

	/* Process options used by GNU programs. */
	gen_parameters parms;
	parms.language = "lt";
	parms.sentence_length = 6;
	parms.corpus_size = 50;
	argp_parse(&argp, argc, argv, 0, 0, &parms);

	printf("#\n# Corpus for language: \"%s\"\n", parms.language);
	printf("# Sentence length: %d\n", parms.sentence_length);
	printf("# Requested corpus size: %d\n", parms.corpus_size);

	// Force the system into generation mode by appending "-generate"
	// to the langauge. XXX this seems hacky, need a better API.
	char lan[100]; // buffer overflow
	lan[0] = 0;
	strcat(lan, parms.language);
	strcat(lan, "-generate");

	dict = dictionary_create_lang(lan);
	if (dict == NULL)
	{
		prt_error("Fatal error: Unable to open dictionary.\n");
		exit(-1);
	}

	opts = parse_options_create();
	parse_options_set_linkage_limit(opts, parms.corpus_size);

	// Set the number of words in the sentence.
	// XXX this is a hacky API. Fix it.
	char slen[30];
	snprintf(slen, 30, "%d", parms.sentence_length);
	sent = sentence_create(slen, dict);

	// sentence_split(sent, opts);
	int num_linkages = sentence_parse(sent, opts);

	// In general, there will be more linkages found, than the
	// requested corpus size. We make used of the random sampler
	// that is already built into the linkage code to sample them
	// randomly, with some "uniform distribution". XXX At this time,
	// the precise meaning of "uniform distribution" is somewhat
	// ill-defined. It needs to be described and documented.
	int linkages_found = sentence_num_linkages_found(sent);
	printf("# Linkages found: %d\n", linkages_found);
	printf("# Linkages generated: %d\n", num_linkages);
	printf("#\n");

	int linkages_valid = sentence_num_valid_linkages(sent);
	assert(linkages_valid == num_linkages, "unexpected linkages!");

	for (int i=0; i<num_linkages; i++)
	{
		Linkage linkage;
		linkage = linkage_create(i, sent, opts);

		size_t nwords = linkage_get_num_words(linkage);
		const char **words = linkage_get_words(linkage);

		// printf("%d", i);
		for (unsigned int w=0; w<nwords; w++)
		{
			printf(" %s", words[w]);
		}
		printf("\n");

		linkage_delete(linkage);
	}

	sentence_delete(sent);
	dictionary_delete(dict);
	printf ("# Bye.\n");
	return 0;
}
