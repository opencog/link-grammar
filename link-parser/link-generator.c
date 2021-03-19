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

#define WILDCARDWORD "\\*"

static int verbosity_level; // Avoid using exposed library static variable.

/* Argument parsing for the generator */
typedef struct
{
	const char* language;
	int sentence_length;
	int corpus_size;
	bool display_disjuncts;

	Parse_Options opts;
} gen_parameters;

static struct argp_option options[] =
{
	{"language", 'l', "language", 0, "Directory containing language definition."},
	{"length", 's', "length", 0, "Sentence length. If 0 - get sentence template from stdin."},
	{"count", 'c', "count", 0, "Count of number of sentences to generate."},
	{"version", 'v', 0, 0, "Print version and exit."},
	{"disjuncts", 'd', 0, 0, "Display linkage disjuncts"},
	{0, 0, 0, 0, "Library options:", 1},
	{"cost-max", '\4', "float"},
	{"dialect", '\5', "dialect_list"},
	{0, 0, 0, 0, "Library debug options:", 2},
	{"debug", '\1', "debug_specification", 0, 0},
	{"verbosity", '\2', "level"},
	{"test", '\3', "test_list"},
	{ 0 }
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	gen_parameters* gp = state->input;
	switch (key)
	{
		case 'l': gp->language = arg; break;
		case 's': gp->sentence_length = atoi(arg); break;
		case 'c': gp->corpus_size = atoi(arg); break;
		case 'd': gp->display_disjuncts = true; break;

		case 'v':
		{
			printf("Version: %s\n", linkgrammar_get_version());
			printf("%s\n", linkgrammar_get_configuration());
			exit(0);
		}

		// Library options.
		case 1:
			parse_options_set_debug(gp->opts, arg); break;
		case 2:
			verbosity_level = atoi(arg);
			parse_options_set_verbosity(gp->opts, verbosity_level); break;
		case 3:
			parse_options_set_test(gp->opts, arg); break;
		case 4:
			parse_options_set_disjunct_cost(gp->opts, atof(arg)); break;
		case 5:
			parse_options_set_dialect(gp->opts, arg); break;

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
	Parse_Options   opts = parse_options_create();
	Sentence        sent = NULL;

	/* Process options used by GNU programs. */
	gen_parameters parms;
	parms.language = "lt";
	parms.sentence_length = 6;
	parms.corpus_size = 50;
	parms.display_disjuncts = false;
	parms.opts = opts;
	argp_parse(&argp, argc, argv, 0, 0, &parms);

	printf("#\n# Corpus for language: \"%s\"\n", parms.language);
	if (parms.sentence_length != 0)
		printf("# Sentence length: %d\n", parms.sentence_length);
	printf("# Requested corpus size: %d\n", parms.corpus_size);

	// Force the system into generation mode by setting the "test"
	// parse-option to "generate".
	const char *old_test = parse_options_get_test(opts);
	char tbuf[256];
	snprintf(tbuf, sizeof(tbuf), "generate,%s", old_test);
	parse_options_set_test(opts, tbuf);

	dict = dictionary_create_lang(parms.language);
	if (dict == NULL)
	{
		prt_error("Fatal error: Unable to open dictionary.\n");
		exit(-1);
	}
	printf("# Dictionary version %s\n", linkgrammar_get_dict_version(dict));

	parse_options_set_linkage_limit(opts, parms.corpus_size);

	if (parms.sentence_length < 0)
	{
		prt_error("Fatal error: Invalid sentence length \"%d\".\n",
		          parms.sentence_length);
		exit(-1);
	}

	if (parms.sentence_length > 0)
	{
		// Set a sentence template for the requested sentence length.
		char *stmp = malloc(4 * parms.sentence_length + 1);
		stmp[0] = '\0';
		for (int i = 0; i < parms.sentence_length; i++)
			strcat(stmp, WILDCARDWORD " ");

		sent = sentence_create(stmp, dict);
		free(stmp);
	}
	else
	{
		char sbuf[1024] = { [sizeof(sbuf)-1] = 'x' };
		fgets(sbuf, sizeof(sbuf), stdin);
		if (sbuf[sizeof(sbuf)-1] != 'x')
		{
			prt_error("Fatal error: Input line too long (>%zu)\n", sizeof(sbuf)-2);
			exit(-1);
		}
		sent = sentence_create(sbuf, dict);
		printf("# Sentence template: %s\n", sbuf);
	}

	// sentence_split(sent, opts);
	int num_linkages = sentence_parse(sent, opts);
	if (num_linkages < 0)
	{
		prt_error("Fatal error: Invalid sentence.\n");
		exit(-1);
	}

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

		if (verbosity_level >= 5) printf("%d: ", i);
		for (unsigned int w=0; w<nwords; w++)
		{
			printf(" %s", words[w]);
		}
		printf("\n");
		if (parms.display_disjuncts)
		{
			char *disjuncts = linkage_print_disjuncts(linkage);
			printf("%s\n", disjuncts);
			free(disjuncts);
		}

		linkage_delete(linkage);
	}

	parse_options_delete(opts);
	sentence_delete(sent);
	dictionary_delete(dict);
	printf ("# Bye.\n");
	return 0;
}
