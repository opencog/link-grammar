/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2008, 2009, 2011 Linas Vepstas                          */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <wchar.h>
#include <wctype.h>

#include "link-includes.h"
#include "read-dict.h"  /* For non-public dict_display_word_info */
#include "utilities.h"  /* For MSWindows portability */

static struct
{
	int verbosity;
	int timeout;
	int memory;
	int linkage_limit;
	int null_block;
	int islands_ok;
	int spell_guess;
	int short_length;
	int batch_mode;
	int panic_mode;
	int allow_null;
	int use_cluster_disjuncts;
#ifdef USE_FAT_LINKAGES
	int use_fat_links;
	int display_union;
#endif /* USE_FAT_LINKAGES */
	int use_sat_solver;
	int use_viterbi;
	int echo_on;
	int cost_model;
	float max_cost;
	int screen_width;
	int display_on;
	int display_constituents;
	int max_sentence_length;
	int display_postscript;
	int display_bad;
	int display_links;
	int display_walls;
	int display_disjuncts;
	int display_senses;
} local;

typedef enum
{
	Int,
	Bool,
	Float,
} ParamType;

typedef struct
{
	const char *string;
	ParamType param_type;
	const char *description;
	void *ptr;
} Switch;

static Switch default_switches[] =
{
   {"bad",        Bool, "Display of bad linkages",         &local.display_bad},
   {"batch",      Bool, "Batch mode",                      &local.batch_mode},
   {"cluster",    Bool, "Use clusters to loosen parsing",  &local.use_cluster_disjuncts},
   {"constituents", Int,  "Generate constituent output",   &local.display_constituents},
   {"cost-model", Int,  "Cost model used for ranking",     &local.cost_model},
   {"cost-max",   Float, "Largest cost to be considered",  &local.max_cost},
   {"disjuncts",  Bool, "Display of disjuncts used",       &local.display_disjuncts},
   {"echo",       Bool, "Echoing of input sentence",       &local.echo_on},
   {"graphics",   Bool, "Graphical display of linkage",    &local.display_on},
   {"islands-ok", Bool, "Use of null-linked islands",      &local.islands_ok},
   {"limit",      Int,  "The maximum linkages processed",  &local.linkage_limit},
   {"links",      Bool, "Display of complete link data",   &local.display_links},
   {"max-length", Int,  "Maximum sentence length",         &local.max_sentence_length},
   {"memory",     Int,  "Max memory allowed",              &local.memory},
   {"null",       Bool, "Allow null links",                &local.allow_null},
   {"null-block", Int,  "Size of blocks with null count 1", &local.null_block},
   {"panic",      Bool, "Use of \"panic mode\"",           &local.panic_mode},
   {"postscript", Bool, "Generate postscript output",      &local.display_postscript},
   {"senses",     Bool, "Display of word senses",          &local.display_senses},
   {"short",      Int,  "Max length of short links",       &local.short_length},
#if defined HAVE_HUNSPELL || defined HAVE_ASPELL
   {"spell",      Bool, "Use spell-guesser for unknown words",  &local.spell_guess},
#endif /* HAVE_HUNSPELL */
   {"timeout",    Int,  "Abort parsing after this many seconds", &local.timeout},
#ifdef USE_FAT_LINKAGES
   {"union",      Bool, "Display of 'union' linkage",      &local.display_union},
   {"use-fat",    Bool, "Use fat links when parsing",      &local.use_fat_links},
#endif /* USE_FAT_LINKAGES */
#ifdef USE_SAT_SOLVER
   {"use-sat",    Bool, "Use Boolean SAT-based parser",    &local.use_sat_solver},
#endif /* USE_SAT_SOLVER */
   {"verbosity",  Int,  "Level of detail in output",       &local.verbosity},
#ifdef USE_VITERBI
   {"vit",        Bool, "Use Viterbit-based parser",       &local.use_viterbi},
#endif
   {"walls",      Bool, "Display wall words",              &local.display_walls},
   {"width",      Int,  "The width of the display",        &local.screen_width},
   {NULL,         Bool,  NULL,                             NULL}
};

struct {const char * s; const char * str;} user_command[] =
{
	{"variables",	"List user-settable variables and their functions"},
	{"help",	      "List the commands and what they do"},
	{"file",       "Read input from the specified filename"},
	{NULL,		   NULL}
};

/**
 *  Gets rid of all the white space in the string s.  Changes s
 */
static void clean_up_string(char * s)
{
	char * x, * y;
	wchar_t p;
	size_t len, w;
	mbstate_t state;
	memset (&state, 0, sizeof(mbstate_t));

	len = strlen(s);
	y = x = s;
	while(*x != '\0')
	{
		w = mbrtowc(&p, x, len, &state); 
		if (0 == w) break;
		len -= w;

		if (!iswspace(p)) {
			while(w) { *y = *x; x++; y++; w--; }
		} else {
			x += w;
		}
	}
	*y = '\0';
}

/** 
 * Return TRUE if s points to a number:
 * optional + or - followed by 1 or more
 *	digits.
 */
static int is_numerical_rhs(char *s)
{
	wchar_t p;
	size_t len, w;
	mbstate_t state;
	memset (&state, 0, sizeof(mbstate_t));

	len = strlen(s);

	if (*s=='+' || *s == '-') s++;
	if (*s == '\0') return FALSE;

	for (; *s != '\0'; s+=w)
	{
		w = mbrtowc(&p, s, len, &state); 
		if (0 == w) break;
		len -= w;
		if(!iswdigit(p)) return FALSE;
	}
	return TRUE;
}

static inline int ival(Switch s)
{
	return *((int *) s.ptr);
}

static inline void setival(Switch s, int val)
{
	*((int *) s.ptr) = val;
}

static int x_issue_special_command(const char * line, Parse_Options opts, Dictionary dict)
{
	char *s, myline[1000], *x, *y;
	int i, count, j, k;
	Switch * as = default_switches;

	strncpy(myline, line, sizeof(myline));
	myline[sizeof(myline)-1] = '\0';
	clean_up_string(myline);

	s = myline;
	j = k = -1;
	count = 0;

	/* Look for boolean flippers */
	for (i=0; as[i].string != NULL; i++)
	{
		if ((Bool == as[i].param_type) && 
		    strncasecmp(s, as[i].string, strlen(s)) == 0)
		{
			count++;
			j = i;
		}
	}

	/* Look for abbreviations */
	for (i=0; user_command[i].s != NULL; i++)
	{
		if (strncasecmp(s, user_command[i].s, strlen(s)) == 0)
		{
			count++;
			k = i;
		}
	}

	if (count > 1)
	{
		printf("Ambiguous command.  Type \"!help\" or \"!variables\"\n");
		return -1;
	}
	else if (count == 1)
	{
		/* flip boolean value */
		if (j >= 0)
		{
			setival(as[j], (0 == ival(as[j])));
			printf("%s turned %s.\n", as[j].description, (ival(as[j]))? "on" : "off");
			return 0;
		}
		else
		{
			/* Found an abbreviated command, but it wasn't a boolean */
			/* Replace the abbreviated command by the full one */
			/* Basically, this just fixes !v and !h for use below. */
			strcpy(s, user_command[k].s);
		}
	}

	if (strcmp(s, "variables") == 0)
	{
        printf(" Variable     Controls                                      Value\n");
        printf(" --------     --------                                      -----\n");
		for (i = 0; as[i].string != NULL; i++)
		{
			printf(" ");
            left_print_string(stdout, as[i].string, "             ");
            left_print_string(stdout, as[i].description,
                   "                                              ");
			if (Float == as[i].param_type)
			{
				/* Float point print! */
				printf("%5.2f", (double) (*((float *)as[i].ptr)));
			}
			else
			{
				printf("%5d", ival(as[i]));
			}
			if (Bool == as[i].param_type)
			{
				if (ival(as[i])) printf(" (On)"); else printf(" (Off)");
			}
			printf("\n");
		}
		printf("\n");
		printf("Toggle a boolean variable as in \"!batch\"; ");
		printf("set a variable as in \"!width=100\".\n");
		return 0;
	}

	if (strcmp(s, "help") == 0)
	{
		printf("Special commands always begin with \"!\".  Command and variable names\n");
		printf("can be abbreviated.  Here is a list of the commands:\n\n");
		for (i=0; user_command[i].s != NULL; i++) {
			printf(" !");
            left_print_string(stdout, user_command[i].s, "               ");
            left_print_string(stdout, user_command[i].str, "                                                    ");
			printf("\n");
		}
		printf(" !!<string>      Print all the dictionary words matching <string>.\n");
		printf("                 Also print the number of disjuncts of each.\n");
		printf("\n");
		printf(" !<var>          Toggle the specified boolean variable.\n");
		printf(" !<var>=<val>    Assign that value to that variable.\n");
		return 0;
	}

	if (s[0] == '!')
	{
		dict_display_word_info(dict, s+1);
		dict_display_word_expr(dict, s+1);
		return 0;
	}

	/* Test here for an equation i.e. does the command line hold an equals sign? */
	for (x=s; (*x != '=') && (*x != '\0') ; x++)
	  ;
	if (*x == '=')
	{
		*x = '\0';
		y = x+1;
		x = s;
		/* now x is the first word and y is the rest */

		/* Figure out which command it is .. it'll be the j'th one */
		j = -1;
		for (i=0; as[i].string != NULL; i++)
		{
			if (strncasecmp(x, as[i].string, strlen(x)) == 0)
			{
				j = i;
				count ++;
			}
		}

		if (j<0)
		{
			printf("There is no user variable called \"%s\".\n", x);
			return -1;
		}

		if (count > 1)
		{
			printf("Ambiguous variable.  Type \"!help\" or \"!variables\"\n");
			return -1;
		}

		if (as[j].param_type != Float)
		{
			int val = -1;
			if (is_numerical_rhs(y)) val = atoi(y);

			if ((0 == strcasecmp(y, "true")) || (0 == strcasecmp(y, "t"))) val = 1;
			if ((0 == strcasecmp(y, "false")) || (0 == strcasecmp(y, "f"))) val = 0;

			if (val < 0)
			{
				printf("Invalid value %s for variable %s Type \"!help\" or \"!variables\"\n", y, as[j].string);
				return -1;
			}

			setival(as[j], val);
			printf("%s set to %d\n", as[j].string, val);
			return 0;
		}
		else
		{
			float val = -1.0;
			val = atof(y);
			if (val < 0.0f)
			{
				printf("Invalid value %s for variable %s Type \"!help\" or \"!variables\"\n", y, as[j].string);
				return -1;
			}

			*((float *) as[j].ptr) = val;
			printf("%s set to %5.2f\n", as[j].string, val);
			return 0;
		}
	}

	/* Look for valid commands, but ones that needed an argument */
	j = -1;
	count = 0;
	for (i = 0; as[i].string != NULL; i++)
	{
		if ((Bool != as[i].param_type) && 
		    strncasecmp(s, as[i].string, strlen(s)) == 0)
		{
			j = i;
			count++;
		}
	}

	if (0 < count)
	{
		printf("Variable \"%s\" requires a value.  Try \"!help\".\n", as[j].string);
		return -1;
	}

	printf("I can't interpret \"%s\" as a command.  Try \"!help\".\n", myline);
	return -1;
}

static void put_opts_in_local_vars(Parse_Options opts)
{
	local.verbosity = parse_options_get_verbosity(opts);
	local.timeout = parse_options_get_max_parse_time(opts);;
	local.memory = parse_options_get_max_memory(opts);;
	local.linkage_limit = parse_options_get_linkage_limit(opts);
	local.null_block = parse_options_get_null_block(opts);
	local.islands_ok = parse_options_get_islands_ok(opts);
	local.spell_guess = parse_options_get_spell_guess(opts);
	local.short_length = parse_options_get_short_length(opts);
	local.cost_model = parse_options_get_cost_model_type(opts);
	local.max_cost = parse_options_get_disjunct_costf(opts);
	local.echo_on = parse_options_get_echo_on(opts);
	local.batch_mode = parse_options_get_batch_mode(opts);
	local.panic_mode = parse_options_get_panic_mode(opts);
	local.screen_width = parse_options_get_screen_width(opts);
	local.allow_null = parse_options_get_allow_null(opts);
	local.use_cluster_disjuncts = parse_options_get_use_cluster_disjuncts(opts);
#ifdef USE_FAT_LINKAGES
	local.use_fat_links = parse_options_get_use_fat_links(opts);
	local.display_union = parse_options_get_display_union(opts);
#endif /* USE_FAT_LINKAGES */
	local.use_sat_solver = parse_options_get_use_sat_parser(opts);
	local.use_viterbi = parse_options_get_use_viterbi(opts);
	local.screen_width = parse_options_get_screen_width(opts);
	local.display_on = parse_options_get_display_on(opts);
	local.display_postscript = parse_options_get_display_postscript(opts);
	local.display_constituents = parse_options_get_display_constituents(opts);
	local.max_sentence_length = parse_options_get_max_sentence_length(opts);
	local.display_bad = parse_options_get_display_bad(opts);
	local.display_disjuncts = parse_options_get_display_disjuncts(opts);
	local.display_links = parse_options_get_display_links(opts);
	local.display_senses = parse_options_get_display_senses(opts);
	local.display_walls = parse_options_get_display_walls(opts);
}

static void put_local_vars_in_opts(Parse_Options opts)
{
	parse_options_set_verbosity(opts, local.verbosity);
	parse_options_set_max_parse_time(opts, local.timeout);
	parse_options_set_max_memory(opts, local.memory);
	parse_options_set_linkage_limit(opts, local.linkage_limit);
	parse_options_set_null_block(opts, local.null_block);
	parse_options_set_islands_ok(opts, local.islands_ok);
	parse_options_set_spell_guess(opts, local.spell_guess);
	parse_options_set_short_length(opts, local.short_length);
	parse_options_set_echo_on(opts, local.echo_on);
	parse_options_set_cost_model_type(opts, local.cost_model);
	parse_options_set_disjunct_costf(opts, local.max_cost);
	parse_options_set_batch_mode(opts, local.batch_mode);
	parse_options_set_panic_mode(opts, local.panic_mode);
	parse_options_set_screen_width(opts, local.screen_width);
	parse_options_set_allow_null(opts, local.allow_null);
	parse_options_set_use_cluster_disjuncts(opts, local.use_cluster_disjuncts);
#ifdef USE_FAT_LINKAGES
	parse_options_set_use_fat_links(opts, local.use_fat_links);
	parse_options_set_display_union(opts, local.display_union);
#endif /* USE_FAT_LINKAGES */
#ifdef USE_SAT_SOLVER
	parse_options_set_use_sat_parser(opts, local.use_sat_solver);
#endif
	parse_options_set_use_viterbi(opts, local.use_viterbi);
	parse_options_set_screen_width(opts, local.screen_width);
	parse_options_set_display_on(opts, local.display_on);
	parse_options_set_display_postscript(opts, local.display_postscript);
	parse_options_set_display_constituents(opts, local.display_constituents);
	parse_options_set_max_sentence_length(opts, local.max_sentence_length);
	parse_options_set_display_bad(opts, local.display_bad);
	parse_options_set_display_disjuncts(opts, local.display_disjuncts);
	parse_options_set_display_links(opts, local.display_links);
	parse_options_set_display_senses(opts, local.display_senses);
	parse_options_set_display_walls(opts, local.display_walls);
}

int issue_special_command(const char * line, Parse_Options opts, Dictionary dict)
{
	int rc;
	put_opts_in_local_vars(opts);
	rc = x_issue_special_command(line, opts, dict);
	put_local_vars_in_opts(opts);
	return rc;
}


