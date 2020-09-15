/*************************************************************************/
/* Copyright (C) 2019-2020 Amir Plivatsky                                     */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

/* Read "4.0.dialect". */

#include <string.h>
#include <errno.h>

#include "dict-common/dict-api.h"       // cost_stringify
#include "dict-common/dialect.h"
#include "dict-common/dict-common.h"
#include "dict-common/file-utils.h"
#include "dict-common/dialect.h"
#include "error.h"
#include "read-dialect.h"
#include "string-id.h"

#define SECTIONS_INIT_SZ 10
#define DIALECT_TABLE_INIT_SZ (SECTIONS_INIT_SZ * 10)

static void print_dialect_table(Dialect *di)
{
	if (di == NULL)
	{
		prt_error("Debug: No dialect table.\n");
		return;
	}
	bool is_dialect_table = (di->num_sections != 0);

	prt_error("\n\\");
	if (is_dialect_table)
		prt_error("Debug: Dialect table:\n");
	else
		prt_error("Debug: Dialect user setting:\n");

	/* First entry unused - tag ID 0 is invalid (for debug). */
	for (unsigned int i = 0; i < di->num_table_tags; i++)
	{
		if (is_dialect_table) prt_error("%3u: ", i);
		prt_error("%-15s %s\n\\",
		          di->table[i].name, cost_stringify(di->table[i].cost));
	}
	lg_error_flush();
}

static void print_dialect_components(Dictionary dict)
{
	expression_tag *dt = &dict->dialect_tag;
	Dialect *di = dict->dialect;

	prt_error("Debug: Dictionary dialect components:\n\\");
	prt_error("%3s  %-15s %s\n\\", "", "Component", "Dialect");

	for (unsigned int i = 1; i <= dt->num; i++)
	{
		const char *dialect_name = "#Internal error";
		bool comma_needed = false;

		prt_error("%3u: %-15s ", i, dt->name[i]);
		for (unsigned int t = 0; t < di->num_table_tags; t++)
		{
			if (cost_eq(di->table[t].cost, DIALECT_SECTION))
			{
				dialect_name = di->table[t].name;
			}
			else if (di->table[t].cost < DIALECT_COST_MAX)
			{
				prt_error("%s%s", comma_needed ? ", " : "", dialect_name);
				comma_needed = true;
			}
		}
		prt_error("\n\\");
	}
	lg_error_flush();
}

#define LINEBUF_SZ 16
static const char *suppress_0(unsigned int line, char *buf)
{
	if (line == 0) return "";
	snprintf(buf, LINEBUF_SZ, "%u:", line);
	return buf;
}

/**
 * Build the section array (index by section number).
 * Element 0 is reserved for a default entry table index, and is set later
 * (if a default section exists). The rest of the elements contain the
 * dialect table index of their section.
 */
static void section_add(Dialect *di, const char *token, unsigned int *size,
                        unsigned int table_index, unsigned int section_num)
{
	di->num_sections++;

	if (di->num_sections > *size)
	{
		if (di->num_sections == 1)
			*size = SECTIONS_INIT_SZ;
		else
			*size *= 2;

		di->section = realloc(di->section, *size * sizeof(*di->section));
	}
	di->section[di->num_sections] =
		(dialect_section_tag){ .name = token, .index = table_index };

	assert(di->num_sections == section_num, "Section number mismatch");
}

 /**
  * Add an entry to the dialect table di->table.
  * @return Table index of the added entry.
  */
static unsigned int dialect_table_add(Dialect *di, const char *token,
                                      unsigned int *size, double cost)
{
	if (di->num_table_tags == *size)
	{
		if (di->num_table_tags == 0)
			*size = DIALECT_TABLE_INIT_SZ;
		else
			*size *= 2;

		di->table = realloc(di->table, *size * sizeof(*di->table));
	}
	di->table[di->num_table_tags] = (dialect_tag){ .name = token, .cost = cost };

	return di->num_table_tags++;
}

typedef struct dialect_file_status
{
	const char *fname;
	char *pin;
	const char *delims;
	unsigned int line_number;
	char delim;
	bool eol;
} dialect_file_status;

static char *get_label(dialect_file_status *dfile)
{
	char *label = dfile->pin;
	char buf[LINEBUF_SZ];

	while ((*dfile->pin != '\0') && strchr(dfile->delims, *dfile->pin) == NULL)
		dfile->pin++;

	dfile->delim = *dfile->pin;
	if (*dfile->pin == '\n') dfile->eol = true;
	*dfile->pin = '\0';

	/* Trim spaces from end of label. */
	char *p;
	for (p = dfile->pin-1; p > label; p--)
		if (!lg_isspace(*p)) break;
	p[1] = '\0';

	const char *bad = valid_dialect_name(label);
	if (bad != NULL)
	{
		if (bad[0] == '\0')
		{
			prt_error("Error: %s:%s \"%s\": Missing name before a delimiter.\n",
		             dfile->fname, suppress_0(dfile->line_number, buf), label);
		}
		else
		{
			prt_error("Error: %s:%s \"%s\": Invalid character '%c' in dialect name.\n",
			          dfile->fname, suppress_0(dfile->line_number, buf), label,
			          bad[0]);
		}
		return NULL;
	}

	if (dfile->delim != '\0') /* Don't skip the final '\0' */
		dfile->pin++;
	return label;
}

static char *isolate_line(char *s)
{
	s[strcspn(s, "\n")] = '\0';
	return s;
}

static bool require_delimiter(dialect_file_status *dfile, char *s, char *buf)
{
	if (strchr(dfile->delims, *s) == NULL)
	{
		prt_error("Error: %s:%s Before \"%s\": Missing delimiter.\n",
		          dfile->fname, suppress_0(dfile->line_number, buf),
		          isolate_line(s));
		return false;
	}
	return true;
}

static void skip_space(dialect_file_status *dfile)
{
	while ((*dfile->pin != '\n') && (*dfile->pin != '\0') &&
	       lg_isspace(*dfile->pin))
	{
		dfile->pin++;
	}
}

/**
 * Build a dialect table from field \c pin in \p dfile.
 * @retrun \c true on success, \c false on failure.
 */

static bool dialect_read_from_str(Dictionary dict, Dialect *di,
                                  dialect_file_status *dfile)
{
	expression_tag *dt = &dict->dialect_tag;
	const char *token;
	char *end;
	unsigned int table_size = 0;
	unsigned int section_set_size = 0;
	char buf[LINEBUF_SZ];

	while (*dfile->pin != '\0')
	{
		skip_space(dfile);
		if (*dfile->pin == '[')
		{
			/* Get section header. */
			dfile->pin++;
			skip_space(dfile);
			token = get_label(dfile);
			if (token == NULL) return false;
			if (dfile->delim != ']')
			{
				prt_error("Error: %s:%s After \"%s\": Missing \"]\".\n",
				          dfile->fname, suppress_0(dfile->line_number, buf), token);
				return false;
			}

			unsigned int section_num = string_id_lookup(token, di->section_set);
			if (section_num != SID_NOTFOUND)
			{
				prt_error("Error: %s:%s Duplicate section \"%s\".\n",
				          dfile->fname, suppress_0(dfile->line_number, buf), token);
				return false;
			}
			section_num = string_id_add(token, di->section_set);
			unsigned int table_index =
				dialect_table_add(di, token, &table_size, DIALECT_SECTION);

			section_add(di, token, &section_set_size, table_index, section_num);
			skip_space(dfile);
		}
		else if ((*dfile->pin != '%') && (*dfile->pin != '\n'))
		{
			/* Get section (dialect vector name) or dialect component tag. */
			float cost;

			token = get_label(dfile);
			if (token == NULL) return false;
			if ((dfile->delim == '\n') || (dfile->delim == '\0') ||
			    (dfile->delim == ','))
			{
				dialect_table_add(di, token, &table_size, DIALECT_SUB);
				skip_space(dfile);
			}
			else if (dfile->delim == ':')
			{
				/* This is a dialect component definition. Cost is optional. */
				if (token == NULL)
				{
					prt_error("Error: %s:%s Missing label before ':'.\n",
					          dfile->fname, suppress_0(dfile->line_number, buf));
					return false;
				}
				if (string_id_lookup(token, dt->set) == SID_NOTFOUND)
				{
					prt_error("Error: %s:%s Expression tag \"%s\" not in "
					          "dict file.\n", dfile->fname,
					          suppress_0(dfile->line_number, buf), token);
					return false;
				}
				skip_space(dfile);
				end = dfile->pin + strspn(dfile->pin, "-0123456789.");
				if (!require_delimiter(dfile, end, buf)) return false;

				if (end == dfile->pin)
				{
					/* No cost supplied. */
					cost = DIALECT_COST_DISABLE;
				}
				else
				{
					char *next;
					if (*end == '\n') dfile->eol = true;
					if (*end == '\0')
					{
						next = end;
					}
					else
					{
						next = end + 1;
						*end = '\0';
					}
					if (!strtodC(dfile->pin, &cost))
					{
						prt_error("Error: %s:%s After \"%s\": Invalid cost \"%s\".\n",
						          dfile->fname, suppress_0(dfile->line_number, buf),
						          token, dfile->pin);
						return false;
					}
					dfile->pin = next;
				}

				dialect_table_add(di, token, &table_size, cost);
				skip_space(dfile);
			}
			else
			{
				prt_error("Error: %s:%s After \"%s\": Internal error char %02x.\n",
				          dfile->fname, suppress_0(dfile->line_number, buf),
				          token, (unsigned char)*dfile->pin);
			}
		}

		if (dfile->line_number == 0)
		{
			/* We read the dialect user setup. */
			if (dfile->delim == '\0') break;
		}
		else
		{
			/* We read the configuration file. */
			if (dfile->eol)
			{
				dfile->eol = false;
				dfile->line_number++;
			}
			else
			{
				end = strchr(dfile->pin, '\n');
				if (*dfile->pin == '%')
					dfile->pin = end;

				if (end == NULL)
				{
					prt_error("Error: %s:%s Missing newline at end of file.\n",
					          dfile->fname, suppress_0(dfile->line_number, buf));
					return false;
				}

				if (*dfile->pin != '\n')
				{
					*end = '\0';
					prt_error("Error: %s:%s Extra characters \"%s\".\n",
					          dfile->fname, suppress_0(dfile->line_number, buf),
					          dfile->pin);
					return false;
				}
			}

			while  (*dfile->pin == '\n')
			{
				dfile->pin++;
				dfile->line_number++;
			}
		}
	}

	return true;
}

const char * const delims_file = "]:%\n";
const char * const delims_string = "]:,";

bool dialect_file_read(Dictionary dict, const char *fname)
{
	char *input = get_file_contents(fname);
	if (input == NULL)
	{
		if (dict->dialect_tag.num != 0)
			prt_error("warning: No dialect file\n");
		return true; /* Not an error for now. */
	}
	if (dict->dialect_tag.num == 0)
	{
		prt_error("Warning: "
		          "File '%s' found but no dialects in the dictionary.\n", fname);
		return true;
	}

	Dialect *di = dict->dialect = dialect_alloc();

	/* Strings refer to kept_input instead of to a string_set, because
	 * there is no handy string_set to use for all of them (sub-dialects
	 * names are problematic). */
	di->kept_input = input;

	dialect_file_status dfile =
	{
		.fname = fname,
		.pin = input,
		.line_number = 1,
		.delims = delims_file,
		.eol = false,
	};

	bool rc = dialect_read_from_str(dict, di, &dfile);
	if (!rc) return false;

	if ((di->num_sections == 0) && verbosity_level(D_USER_FILES))
	{
		prt_error("Warning: Dialect file: No definitions found.\n");
		return true;
	}

	if (!cost_eq(di->table[0].cost, DIALECT_SECTION))
	{
		prt_error("Error: Dialect file: Must start with a section.\n");
		return false;
	}

	/* Validate that all the sub-dialects are defined as sections. */
	for (unsigned int i = 0; i < di->num_table_tags; i++)
	{
		if (cost_eq(di->table[i].cost, DIALECT_SUB) &&
		    (string_id_lookup(di->table[i].name, di->section_set) == SID_NOTFOUND))
		{
			prt_error("Error: Dialect file: sub-dialect \"%s\" doesn't "
			          "have a section.\n", di->table[i].name);
			return false;
		}
	}

	/* Set the first section entry to point to the default entry. */
	di->section[0].index = NO_INDEX; /* In case no default entry */
	for (unsigned int i = 1; i <= di->num_sections; i++)
	{
		if (strcmp("default", di->section[i].name) == 0)
		{
			di->section[0].index = di->section[i].index;
			break;
		}
	}
	if ((di->section[0].index == NO_INDEX) && verbosity_level(D_USER_FILES))
	{
		prt_error("Warning: Dialect file: No [default] section.\n");
	}

	if (verbosity_level(+D_DICT+1))
	{
		print_dialect_table(di);
		if (dict->dialect_tag.num == 0)
		{
			prt_error("Debug: No expression tags in the dict.\n");
		}
		else
		{
			print_dialect_components(dict);
		}
	}

	/* Validate that there are no loops in the dialect table. */
	dialect_info dinfo = { 0 };
	dinfo.cost_table =
		malloc((dict->dialect_tag.num + 1) * sizeof(*dinfo.cost_table));

	for (unsigned int i = 0; i < di->num_table_tags; i++)
	{
		if (cost_eq(di->table[i].cost, DIALECT_SECTION))
		{
			if (!apply_dialect(dict, di, di->section[0].index, di, &dinfo))
			{
				free(dinfo.cost_table);
				return false;
			}
		}
	}
	free(dinfo.cost_table);

	return true;
}

/* Denote link-parser's dialect variable or Parse_Options dialect option. */
#define DIALECT_OPTION "dialect option"

/**
 * Decode the !dialect user variable.
 */
bool dialect_read_from_one_line_str(Dictionary dict, Dialect *di,
                                    const char *user_setup)
{
	for (const char *c = user_setup; *c != '\0'; c++)
	{
		/* Check the user setup for characters that would cause problems. */
		if (*c == '[')
		{
			/* Section header specification is not allowed. */
			prt_error("Error: "DIALECT_OPTION": Invalid character \"[\".\n");
			return false;
		}
		else if (*c == '\n')
		{
			prt_error("Error: "DIALECT_OPTION": Newlines are not allowed.\n");
			return false;
		}
	}

	di->kept_input = strdup(user_setup);

	dialect_file_status dfile =
	{
		.fname = DIALECT_OPTION,
		.pin = di->kept_input,
		.line_number = 0, /* 0 denotes reading from a string. */
		.delims = delims_string,
		.eol = false,
	};

	bool rc = dialect_read_from_str(dict, di, &dfile);

	return rc;
}
