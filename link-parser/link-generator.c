/*
 * link-generator.c
 *
 * Generate random corpora from dictionaries.
 * February 2021
 */

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

#include <getopt.h>
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>                      // errno

#include "generator-utilities.h"
#include "parser-utilities.h"

#include <link-includes.h>
#include <dict-api.h>
#include <dict-defines.h>               // WILDCARD_WORD

#ifdef _MSC_VER
#ifndef getopt_long
#define getopt_long(...) _getopt_internal(__VA_ARGS__, false)
#endif /* !getopt_long */
#endif /* _MSC_VER */

int verbosity_level;

/* Argument parsing for the generator */
typedef struct
{
	const char* language;
	unsigned int sentence_length;
	unsigned int corpus_size; /* Limited to INT_MAX. */
	bool display_disjuncts;
	bool display_unused_disjuncts;
	bool leave_subscripts;
	bool unrepeatable_random;
	bool explode;
	bool walls;

	Parse_Options opts;
} gen_parameters;

/* Originally, this program used argp, but now it uses getopt in
 * order to make the porting to MS Windows easy. The original
 * definitions are still being used here because they are more readable
 * and the also allow easy a dynamic generation of an help message.
 * They are converted to getopt options.  Only the minimal needed
 * conversion is done (e.g. flags are not supported).
 */
static struct argp_option
{
	const char *name;
	int key;
	const char *arg;
	int flags;
	const char *doc;
	int group;
} options[] =
{
	{"language", 'l', "language", 0, "Directory containing language definition."},
	{"length", 's', "length", 0, "Sentence length. If 0 - get sentence template from stdin."},
	{"count", 'c', "count", 0, "Count of number of sentences to generate."},
	{"explode", 'x', 0, 0, "Generate all wording samples per linkage."},
	{"disjuncts", 'd', 0, 0, "Display linkage disjuncts."},
	{"unused", 'u', 0, 0, "Display unused disjuncts."},
	{"leave-subscripts", '.', 0, 0, "Don't remove word subscripts."},
	{"random", 'r', 0, 0, "Use unrepeatable random numbers."},
	{"no-walls", 'w'+128, 0, 0, "Don't use walls in wildcard words."},
	{0, 0, 0, 0, "Library options:", 1},
	{"cost-max", 4, "float"},
	{"dialect", 5, "dialect_list"},
	{0, 0, 0, 0, "Library debug options:", 2},
	{"debug", 1, "debug_specification", 0, 0},
	{"verbosity", 2, "level"},
	{"test", 3, "test_list"},
	{0, 0, 0, 0, "Standard options:", -1},
	/* These options are default in argparse but are needed for getopt. */
	{"help", '?', 0, 0, "Give this help list."},
	{"usage",'u'+128, 0, 0, "Give a short usage message."},
	{"version", 'v', 0, 0, "Print version and exit."},
	{ 0 }
};

static bool is_group_header(const struct argp_option *a_opt)
{
	return (a_opt->name == NULL) && (a_opt->doc != NULL);
}

static bool end_of_argp_options(const struct argp_option *a_opt)
{
	return (a_opt->name == NULL) && (a_opt->doc == NULL);
}

static bool is_short_option(const struct argp_option *a_opt)
{
	return (a_opt->key > 32) && (a_opt->key < 127);
}

#define ARGP_OPTION_ARRAY_SIZE (sizeof(options)/sizeof(options[0]))

static struct option long_options[ARGP_OPTION_ARRAY_SIZE];
static char short_options[2*ARGP_OPTION_ARRAY_SIZE +2]; /* :...\0 */

/*
 * Convert the argp-style options to getopt.
 * Only the minimal needed conversion is done.
 */
static void argp2getopt(struct argp_option *a_opt, char *g_optstring,
                        struct option *g_opt)
{
	*g_optstring++ = ':';

	for (struct argp_option *a = a_opt; !end_of_argp_options(a); a++)
	{
		if ((a->group == 0) && (a != a_opt))
			a->group = a[-1].group + ((a->name == NULL) ? 1 : 0);
		if (a->name == NULL) continue;

		/* Generate long options. */
		g_opt->name = (char *)a->name; /* For (char *) see commit comment. */
		g_opt->has_arg = (a->arg == NULL) ? no_argument : required_argument;
		g_opt->flag = NULL;
		g_opt->val = a->key;

		/* Generate short options. */
		if (is_short_option(a))
		{
			*g_optstring++ = (char)a->key;
			if (a->arg) *g_optstring++ = ':';
		}

		g_opt++;
	}
}

static const char *program_basename(const char *path)
{
	if (path == NULL) return "(null)";

	const char *fs = strrchr(path, '/');
	if (fs != NULL) return fs + 1;

	const char *rs = strrchr(path, '\\');
	if (rs != NULL) return rs + 1;

	return path;
}

/*
 * Print the given string "s" between columns "start_col" and "end_col",
 * folding after a word end if possible (so very long words are not
 * folded). "current_col" is the current screen column. Column number
 * starts at 1.
 *
 * - Only ASCII is supported.
 * - The line folding itself is counted as one blank.
 * - The rest of the blanks are preserved (but not at end of string).
 * - Tabs ('\t') are not supported.
 * - A single newline ('\n') in "s" folds the printout when encountered.
 */
static void print_folded_line(const char *s, int current_col, int start_col,
                              int end_col)
{
	int col = 0;
	const char *line_start = s;
	bool last_is_nonblank = true;
	const char *end_blank = NULL;


	int pad; /* Number of blanks for printout alignment. */
	if (current_col > start_col)
	{
		printf("\n");
		pad = start_col;
	}
	else
	{
		pad = start_col - current_col;
	}

	for (const char *p = s; ; p++)
	{
		if (line_start != s) pad = start_col;
		if (*p == '\n')
		{
			end_blank = p;
			col = (end_col - start_col) + 1; /* Force output. */
		}
		else if ((*p == ' ') || (*p == '\0'))
		{
			if (last_is_nonblank)
			{
				last_is_nonblank = false;
				end_blank = p;
			}
			if (*p == '\0') col = (end_col - start_col) + 1; /* Force output. */
		}
		else
		{
			last_is_nonblank = true;
		}

		if ((col > (end_col - start_col)) && (end_blank != NULL))
		{
			printf("%*s", pad + 1, ""); /* At least 1 blank separation. */
			printf("%.*s\n", (int)(end_blank - line_start), line_start);

			/* Find the start of the next line. Terminate if no more to print. */
			if (*end_blank == '\0') break;        /* Nothing remained. */
			p = line_start = end_blank + 1;
			if (p[strspn(p, " ")] == '\0') break; /* Nothing or only blanks. */

			end_blank = NULL;
			last_is_nonblank = true;
			col = 0;
			continue;
		}
		if (*p == '\0') break;
		col++;
	}
}

#define MAXCOL 79
#define SHORT_OPT_WIDTH 6
#define LONG_OPT_WIDTH 22

/*
 * Print a help message (mimicking argp).
 */
static void print_option_help(const struct argp_option *a_opt)
{
	if (a_opt->name == NULL)
	{
		printf("\n %s\n", a_opt->doc);
		return;
	}

	int col;
	if (is_short_option(a_opt))
		col = printf("  -%c, ", a_opt->key);
	else
		col = printf("      ");
	assert(col == SHORT_OPT_WIDTH);

	col += printf("--%s", a_opt->name);
	if (a_opt->arg) col += printf("=%s", a_opt->arg);

	if (a_opt->doc)
		print_folded_line(a_opt->doc, col, SHORT_OPT_WIDTH+LONG_OPT_WIDTH, MAXCOL);
	else
		printf("\n");
}

static void help(const char *path, const struct argp_option *a_opt)
{
	printf("Usage: %s [OPTIONS...]\n\n", program_basename(path));

	for (const struct argp_option *a = a_opt; !end_of_argp_options(a); a++)
		print_option_help(a);
}

#define USAGE_INDENT 12
/*
 * Print a usage message in a style as follows (mimicking argp):
 * Usage: link-generator [-.drux?v] [-c count] [-l language] [-s length]
 *             [--leave-subscripts] [--count=count] [--disjuncts]
 *             ...
 *
 * Only ASCII is supported.
 */
static void usage(const char *path, const struct argp_option *a_opt,
                  const char *g_short_options)
{
	int col = printf("Usage: %s", program_basename(path));

	if (*g_short_options == ':') g_short_options++;
	if (*g_short_options != ':')
	{
		col += printf(" [-");
		for (const struct argp_option *a = a_opt; !end_of_argp_options(a); a++)
		{
			if (is_group_header(a)) continue;
			{
				if ((a->arg == NULL) && is_short_option(a))
					col += printf("%c", a->key);
			}
		}
		col += printf("]");
	}

	char optbuff[128];
	for (const struct argp_option *a = a_opt; !end_of_argp_options(a); a++)
	{
		if (is_group_header(a)) continue;
		col += snprintf(optbuff, sizeof(optbuff), " [--%s%s%s]", a->name,
		      (a->arg == NULL) ? "" : "=", (a->arg == NULL) ? "" : a->arg);
		if (col <= MAXCOL)
		{
			printf("%s", optbuff);
		}
		else
		{
			printf("\n");
			col = printf("%*s%s", USAGE_INDENT-1, "", optbuff);
		}
	}
	printf("\n");
}

static void try_help(char *path)
{
	/* Flag errors are printed to stderr. Arrange that the following
	 * prt_error will be printed to stderr too. */
	void *default_handler = lg_error_set_handler(NULL, NULL);
	lg_error_set_handler(default_handler, (int []){0});

	prt_error("Try \"%s --help\" or \"%s --usage\" for more information.\n",
	          program_basename(path), program_basename(path));

	/* Restore stdout for message levels <= lg_Debug. */
	lg_error_set_handler(default_handler, (int []){lg_Debug});
}

static int strtoi_errexit(const char *flag, const char *strval,
                           int min_v, int max_v)
{
	char *end;

	errno = 0;
	long val = strtol(strval, &end, 10);

	int retval = (int)val;
	if (*end != '\0')
	{
		prt_error("Fatal error: %s: Bad character '%c' in argument \"%s\".\n",
		          flag, end[0], strval);
		exit(-1);
	}

	if ((val < min_v) || (val > max_v) || (errno != 0) || (retval != val))
	{
		prt_error("Fatal error: %s: Argument \"%s\" out of range [%d, %d].\n",
		          flag, strval, min_v, max_v);
		exit(-1);
	}

	return val;
}

static int option_cmp(const void *a, const void *b)
{
	const struct argp_option *oa = a;
	const struct argp_option *ob = b;

	int key_a = is_short_option(a) ? oa->key : (unsigned char)oa->name[0];
	int key_b = is_short_option(b) ? ob->key : (unsigned char)ob->name[0];

	return key_a - key_b;
}

/*
 * Note: This function only supports group numbers that fit inside
 * a signed char. (unsigned char) is used in the comparison to support
 * an order as follows: 0, 1, 2, ..., n, -m, ..., -2, -1.
 */
static int group_cmp(const void *a, const void *b)
{
	const struct argp_option *oa = a;
	const struct argp_option *ob = b;

	return (unsigned char)oa->group - (unsigned char)ob->group;
}

static int help_entry_cmp(const void *a, const void *b)
{
	if (is_group_header(a) || is_group_header(b)) return 0;
	if (group_cmp(a, b) != 0) return group_cmp(a, b);
	return option_cmp(a, b);
}

static void getopt_parse(int ac, char *av[], struct argp_option *a_opt,
                         const char *g_short_options,
                         const struct option *g_long_options,
                         gen_parameters* gp)
{
	argp2getopt(options, short_options, long_options);
	qsort(a_opt, ARGP_OPTION_ARRAY_SIZE-1, sizeof(options[0]), help_entry_cmp);

	while (true)
	{
		int key = getopt_long(ac, av, g_short_options, g_long_options, NULL);
		switch (key)
		{
			case 'l': gp->language = optarg; break;
			case 's': gp->sentence_length = (int)
			             strtoi_errexit(av[optind-2], optarg, 0, MAX_SENTENCE-2);
			          break;
			case 'c': gp->corpus_size =
			             strtoi_errexit(av[optind-2], optarg, 1, INT_MAX);
			          break;
			case 'x': gp->explode = true; break;
			case 'd': gp->display_disjuncts = true; break;
			case 'u': gp->display_unused_disjuncts = true; break;
			case '.': gp->leave_subscripts = true; break;
			case 'r': gp->unrepeatable_random = true; break;
			case 'w'+128:
			          gp->walls = false; break;

			// Library options.
			case 1:   parse_options_set_debug(gp->opts, optarg); break;
			case 2:   verbosity_level =
			             strtoi_errexit(av[optind-2], optarg, 0, 999);
			          parse_options_set_verbosity(gp->opts, verbosity_level); break;
			case 3:   parse_options_set_test(gp->opts, optarg); break;
			case 4:   parse_options_set_disjunct_cost(gp->opts, atof(optarg)); break;
			case 5:   parse_options_set_dialect(gp->opts, optarg); break;

			// Standard GNU options.
			case 'u'+128:
			          usage(av[0], a_opt, g_short_options);
			          exit(0);
			case 'v': printf("Version: %s\n", linkgrammar_get_version());
			          printf("%s\n", linkgrammar_get_configuration());
			          exit(0);
			case '?': if ((strncmp(av[optind-1], "--help",
			                      strlen(av[optind-1])) == 0) ||
			              (strcmp(av[optind-1], "-?") == 0))
			          {
				          help(av[0], options);
				          exit(0);
			          }

			// Error handling.
			          prt_error("Fatal error: Unknown %s option \"%s\"\n",
			                    program_basename(av[0]), av[optind-1]);
			          try_help(av[0]);
			          exit(-1);
			case ':': prt_error("Fatal error: %s: "
			                    "Missing argument to option \"%s\"\n",
			                    program_basename(av[0]), av[optind-1]);
			          try_help(av[0]);
						 exit(-1);
			case -1:  if (optind < ac)
			          {
				          prt_error("Fatal error: "
				                    "%s doesn't accept non-option arguments:",
				                    program_basename(av[0]));
							 while (optind < ac)
								 prt_error(" %s", av[optind++]);
							 prt_error("\n");
							 try_help(av[0]);
				          exit(-1);
			          }
			          break;

			default:  prt_error("Fatal error: %s encountered an internal error:"
			                    "getopt() returned an unexpected value %d\n",
			                    program_basename(av[0]), key);
			          exit(-1);
		}
		if (key == -1) break;
	}
}

int main (int argc, char* argv[])
{
	Dictionary      dict;
	Parse_Options   opts = parse_options_create();
	Sentence        sent = NULL;

	argv = ms_windows_setup(argc);

	gen_parameters parms;
	parms.language = "lt";
	parms.sentence_length = 6;
	parms.corpus_size = 50;
	parms.explode = false;
	parms.display_disjuncts = false;
	parms.display_unused_disjuncts = false;
	parms.leave_subscripts = false;
	parms.unrepeatable_random = false;
	parms.walls = true;
	parms.opts = opts;
	getopt_parse(argc, argv, options, short_options, long_options, &parms);
	if (!parms.unrepeatable_random)
	{
		srand(0);
	}
	else
	{
		srand(getpid());
	}

	printf("#\n# Corpus for language: \"%s\"\n", parms.language);
	if (parms.sentence_length != 0)
		printf("# Sentence length: %u\n", parms.sentence_length);
	printf("# Requested corpus size: %u\n", parms.corpus_size);

	// Force the system into generation mode by setting the "test"
	// parse-option to "generate".
	const char *old_test = parse_options_get_test(opts);
	char tbuf[256];
	snprintf(tbuf, sizeof(tbuf), "generate%s,%s",
	         parms.walls ? ":walls" : "", old_test);
	parse_options_set_test(opts, tbuf);

	dict = dictionary_create_lang(parms.language);
	if (dict == NULL)
	{
		prt_error("Fatal error: Unable to open dictionary.\n");
		exit(-1);
	}
	printf("# Dictionary version %s\n", linkgrammar_get_dict_version(dict));

	const Category* catlist = dictionary_get_categories(dict);
	if (catlist == NULL)
	{
		prt_error("Fatal error: dictionary_get_categories() failed\n");
		exit(-1);
	}
	unsigned int num_categories = 0;
	for (const Category* c = catlist; c->num_words != 0; c++)
		num_categories++;
	printf("# Number of categories: %u\n", num_categories);

	if (verbosity_level == 200)
	{
		dump_categories(dict, catlist);
		exit(0);
	}

	parse_options_set_linkage_limit(opts, parms.corpus_size);

	if (parms.sentence_length > 0)
	{
		// Set a sentence template for the requested sentence length.
		char *stmp = malloc(4 * parms.sentence_length + 1);
		stmp[0] = '\0';
		for (unsigned int i = 0; i < parms.sentence_length; i++)
			strcat(stmp, WILDCARD_WORD " ");

		sent = sentence_create(stmp, dict);
		free(stmp);
	}
	else
	{
		char sbuf[1024] = { [sizeof(sbuf)-1] = 'x' };
		char *retbuf = fgets(sbuf, sizeof(sbuf), stdin);
		if (NULL == retbuf || sbuf[sizeof(sbuf)-1] != 'x')
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

	Disjunct **unused_disjuncts = sentence_unused_disjuncts(sent);
	unsigned int num_unused = 0;
	for (Disjunct** d = unused_disjuncts; *d != NULL; d++)
		num_unused++;
	printf("# Number of unused disjuncts: %u\n", num_unused);
	printf("#\n");

	if (parms.display_unused_disjuncts)
	{
		// Display disjunct expressions with one word example.
		for (Disjunct **d = unused_disjuncts; *d != NULL; d++)
		{
			char* exp = disjunct_expression(*d);
			const Category_cost *cc = disjunct_categories(*d);
			const char *word = catlist[cc[0].num-1].word[0];
			printf("%5d: %20s  %s\n", (int)(d - unused_disjuncts), word, exp);
			free(exp);
		}
		exit(0);
	}

	int linkages_valid = sentence_num_valid_linkages(sent);
	assert(linkages_valid == num_linkages /* "unexpected linkages! */);

	// How many sentences to print per linkage.
	// Print more than one only if explode flag set.
	double samples = parms.explode ?
		((double) parms.corpus_size) / ((double) num_linkages)
		: 1.0;

	unsigned int num_printed = 0;
	for (int i=0; i<num_linkages; i++)
	{
		Linkage linkage;
		linkage = linkage_create(i, sent, opts);

		size_t nwords = linkage_get_num_words(linkage);
		const char **words = linkage_get_words(linkage);

		if (verbosity_level >= 5) printf("%d: ", i);
		num_printed += print_sentences(catlist, linkage, nwords, words,
		                               parms.leave_subscripts, samples);

		if (parms.display_disjuncts)
		{
			char *disjuncts = linkage_print_disjuncts(linkage);
			printf("%s\n", disjuncts);
			free(disjuncts);
		}

		linkage_delete(linkage);

		if (num_printed >= parms.corpus_size) break;
	}

	free(unused_disjuncts);
	parse_options_delete(opts);
	sentence_delete(sent);
	dictionary_delete(dict);
	printf ("# Bye.\n");
	return 0;
}
