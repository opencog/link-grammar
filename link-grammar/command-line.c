/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
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
#include <link-grammar/api.h>

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
	int echo_on;
	int cost_model;
	int screen_width;
	int display_on;
	int display_constituents;
	int max_sentence_length;
	int display_postscript;
	int display_bad;
	int display_links;
	int display_walls;
	int display_union;
	int display_disjuncts;
	int display_senses;
} local;

typedef struct
{
	const char * string;
	int	isboolean;
	const char * description;
	int  * p;
} Switch;

static Switch default_switches[] =
{
   {"bad",        1, "Display of bad linkages",         &local.display_bad},
   {"batch",      1, "Batch mode",                      &local.batch_mode},
   {"constituents", 0, "Generate constituent output",   &local.display_constituents},
   {"cost",       0, "Cost model used for ranking",     &local.cost_model},
   {"disjuncts",  1, "Showing of disjunct used",        &local.display_disjuncts},
   {"echo",       1, "Echoing of input sentence",       &local.echo_on},
   {"graphics",   1, "Graphical display of linkage",    &local.display_on},
   {"islands-ok", 1, "Use of null-linked islands",      &local.islands_ok},
   {"limit",      0, "The maximum linkages processed",  &local.linkage_limit},
   {"links",      1, "Showing of complete link data",   &local.display_links},
   {"max-length", 0, "Maximum sentence length",         &local.max_sentence_length},
   {"memory",     0, "Max memory allowed",              &local.memory},
   {"null",       1, "Null links",                      &local.allow_null},
   {"null-block", 0, "Size of blocks with null cost 1", &local.null_block},
   {"panic",      1, "Use of \"panic mode\"",           &local.panic_mode},
   {"postscript", 1, "Generate postscript output",      &local.display_postscript},
   {"senses",     1, "Showing of word senses",          &local.display_senses},
   {"short",      0, "Max length of short links",       &local.short_length},
   {"spell",      1, "Spell-guesser for unkown words",  &local.spell_guess},
   {"timeout",    0, "Abort parsing after this many seconds",   &local.timeout},
   {"union",      1, "Showing of 'union' linkage",      &local.display_union},
   {"verbosity",  0, "Level of detail in output",       &local.verbosity},
   {"walls",      1, "Showing of wall words",           &local.display_walls},
   {"width",      0, "The width of the display",        &local.screen_width},
   {NULL,         1,  NULL,                             NULL}
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

static void x_issue_special_command(char * line, Parse_Options opts, Dictionary dict)
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
		if (as[i].isboolean && strncasecmp(s, as[i].string, strlen(s)) == 0)
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
		return;
	}
	else if (count == 1)
	{
		/* flip boolean value */
		if (j >= 0)
		{
			*as[j].p = !(*as[j].p);
			printf("%s turned %s.\n", as[j].description, (*as[j].p)? "on" : "off");
			return;
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
		for (i=0; as[i].string != NULL; i++) {
			printf(" ");
            left_print_string(stdout, as[i].string, "             ");
            left_print_string(stdout, as[i].description, "                                              ");
			printf("%5d", *as[i].p);
			if (as[i].isboolean) {
				if (*as[i].p) printf(" (On)"); else printf(" (Off)");
			}
			printf("\n");
		}
		printf("\n");
		printf("Toggle a boolean variable as in \"!batch\"; ");
		printf("set a variable as in \"!width=100\".\n");
		return;
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
		return;
	}

	if(s[0] == '!')
	{
		dict_display_word_info(dict, s+1);
		dict_display_word_expr(dict, s+1);
		return;
	}

	/* Test here for an equation i.e. does the command line hold an equals sign? */
	for (x=s; (*x != '=') && (*x != '\0') ; x++)
	  ;
	if (*x == '=')
	{
		int val;

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
			return;
		}

		if (count > 1)
		{
			printf("Ambiguous variable.  Type \"!help\" or \"!variables\"\n");
			return;
		}

		val = -1;
		if (is_numerical_rhs(y)) val = atoi(y);

		if ((0 == strcasecmp(y, "true")) || (0 == strcasecmp(y, "t"))) val = 1;
		if ((0 == strcasecmp(y, "false")) || (0 == strcasecmp(y, "f"))) val = 0;

		if (val < 0)
		{
			printf("Invalid value %s for variable %s Type \"!help\" or \"!variables\"\n", y, as[j].string);
			return;
		}

		*(as[j].p) = val;
		printf("%s set to %d\n", as[j].string, val);
		return;
	}

	/* Look for valid commands, but ones that needed an argument */
	j = -1;
	count = 0;
	for (i=0; as[i].string != NULL; i++)
	{
		if (!as[i].isboolean && strncasecmp(s, as[i].string, strlen(s)) == 0)
		{
			j = i;
			count++;
		}
	}

	if (0 < count)
	{
		printf("Variable \"%s\" requires a value.  Try \"!help\".\n", as[j].string);
		return;
	}

	printf("I can't interpret \"%s\" as a command.  Try \"!help\".\n", myline);
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
	local.echo_on = parse_options_get_echo_on(opts);
	local.batch_mode = parse_options_get_batch_mode(opts);
	local.panic_mode = parse_options_get_panic_mode(opts);
	local.screen_width = parse_options_get_screen_width(opts);
	local.allow_null = parse_options_get_allow_null(opts);
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
	local.display_union = parse_options_get_display_union(opts);
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
	parse_options_set_batch_mode(opts, local.batch_mode);
	parse_options_set_panic_mode(opts, local.panic_mode);
	parse_options_set_screen_width(opts, local.screen_width);
	parse_options_set_allow_null(opts, local.allow_null);
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
	parse_options_set_display_union(opts, local.display_union);
}

void issue_special_command(char * line, Parse_Options opts, Dictionary dict)
{
	put_opts_in_local_vars(opts);
	x_issue_special_command(line, opts, dict);
	put_local_vars_in_opts(opts);
}


