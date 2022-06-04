/*************************************************************************/
/* Copyright (c) 2005 Sampo Pyysalo                                      */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <string.h>

#include "link-includes.h"
#include "dict-common/dict-common.h"
#include "dict-common/file-utils.h"
#include "error.h"
#include "utilities.h"
#include "read-regex.h"

#define D_REGEX 10 /* Verbosity level for this file */

/*
  Function for reading regular expression name:regex combinations
  into the Dictionary from a given file.

  The format of the regex file is as follows:

  Lines starting with "%" are comments and are ignored.
  All other nonempty lines must follow the following format:

      REGEX_NAME:  /pattern/

  Here REGEX_NAME is an identifying unique name for the regex.
  This name is used to determine the disjuncts that will be assigned to
  tokens matching the pattern, so in the dictionary file (e.g. 4.0.dict)
  you must have something like

     REGEX_NAME:  (({@MX+} & (JG- or <noun-main-s>)) or YS+)) or AN+ or G+);

  using the same name. The pattern itself must be surrounded by slashes.
  Extra whitespace is ignored.

  Regexs that are preceded by ! (i.e. !/pattern/) if they match a token,
  stop further match tries until a different regex name is encountered.
  Thus, they can serve as a kind of a negative look-ahead.

  To allow non-ASCII ranges in character classes for libraries other
  than recent PCRE2, such ranges are expanded by default. Since various
  regex libraries have different syntax features for character classes,
  it is hard to do such an expansion only inside character classes, so
  this expansion is always done. To avoid it, add:
  NO-EXPAND://
*/

#define MAX_REGEX_NAME_LENGTH 50
#define MAX_REGEX_LENGTH      10240

static bool expand_character_ranges(const char *file_name, int line,
                                    const char *name, char *regex)
{
	const char *orig_regex = strdupa(regex);
	const char *p = orig_regex;
	char *r = regex;
	int b_len, e_len; /* Range begin/end utf8 character number of bytes */
	bool expanded = false;

	do
	{
		const char *b_char = p;
		b_len = utf8_charlen(b_char);

		if (b_len < 0)
		{
			prt_error("Error: File \"%s\", line %d: \"%s\": "
			          "Bad utf8 in definition.\n", file_name, line, name);
			return false;
		}
		memcpy(r, b_char, b_len);

		if (b_len + 1 > (int)(MAX_REGEX_LENGTH - (regex - r)))
		{
			lgdebug(D_REGEX, "Warning: File \"%s\", line %d, position %d: "
			        "\"%s\": Expanded definition overflow (>%d chars)\n",
			        file_name, line, (int)(p - orig_regex), name,
			        MAX_REGEX_LENGTH-1);
			strcpy(regex, orig_regex);
			return false;
		}

		p += b_len;
		r += b_len;

		/* Expand ranges of non-ASCII characters. */
		if ((b_len > 1) && (p[0] == '-') && (p[-1] != '\\') &&
		    p[1] != '\0' && p[1] != '[' && p[1] != ']' && p[1] != '\\')
		{
			p++;
			e_len = utf8_charlen(p);
			if (e_len < 0)
			{
				prt_error("Error: File \"%s\", line %d: \"%s\": "
				          "Bad utf8 in definition.\n", file_name, line, name);
				return false;
			}

			if (b_len != e_len)
			{
				lgdebug(D_REGEX, "Warning: File \"%s\", line %d: \"%s\": "
				        "Range \"%.*s-%.*s\": "
				        "Characters with an unequal number of bytes.\n",
				        file_name, line, name, b_len, b_char, e_len, p);
				return true;
			}

			size_t prefix_len = b_len - 1;
			if (strncmp(b_char, p, prefix_len) != 0)
			{
				lgdebug(D_REGEX, "Warning: File \"%s\", line %d: \"%s\": "
				        "Range \"%.*s-%.*s\": "
				        "No common prefix before the last byte.\n",
				        file_name, line, name, b_len, b_char, e_len, p);
				return true;
			}
			if ((unsigned char)b_char[prefix_len] > (unsigned char)p[prefix_len])
			{
				lgdebug(D_REGEX, "Warning: File \"%s\", line %d: \"%s\": "
				        "Range \"%.*s-%.*s\": Decreasing order.\n",
				        file_name, line, name, b_len, b_char, e_len, p);
				return true;
			}

			for (unsigned char last_byte = b_char[prefix_len] + 1;
				  last_byte <= (unsigned char)p[prefix_len]; last_byte++)
			{
				if (b_len + 1 > (int)(MAX_REGEX_LENGTH - (r - regex)))
				{
					lgdebug(D_REGEX, "Warning: : File \"%s\", line %d: ,position %d: "
					        "\"%s\": Range \"%.*s-%.*s\": "
					        "Expanded definition overflow (>%d chars)\n",
					        file_name, line, (int)(r - regex), name,
					        b_len, b_char, e_len, p, MAX_REGEX_LENGTH-1);
					strcpy(regex, orig_regex);
					return true;
				}
				memcpy(r, b_char, prefix_len);
				r += prefix_len;
				*r++ = last_byte;
			}

			p += b_len;
			expanded = true;
		}
	} while (*p != '\0');

	*r = '\0';
	if (expanded)
		lgdebug(+D_REGEX, "%s: %s\n", name, regex);

	return true;
}

bool read_regex_file(Dictionary dict, const char *file_name)
{
	Regex_node **tail = &dict->regex_root; /* Last Regex_node * in list */
	Regex_node *new_re;
	int c,prev,i,line=1;
	FILE *fp;
	bool no_expand = false;
	char name[MAX_REGEX_NAME_LENGTH];
	char regex[MAX_REGEX_LENGTH];

	fp = dictopen(file_name, "r");
	if (fp == NULL)
	{
		prt_error("Error: Cannot open regex file %s.\n", file_name);
		return false;
	}

	/* read in regexs. loop broken on EOF. */
	while (1)
	{
		bool neg = false;

		/* skip whitespace and comments. */
		do
		{
			do
			{
				c = fgetc(fp);
				if (c == '\n') { line++; }
			}
			while (lg_isspace(c));

			if (c == '%')
			{
				while ((c != EOF) && (c != '\n')) { c = fgetc(fp); }
				line++;
			}
		}
		while (lg_isspace(c));

		if (c == EOF) { break; } /* done. */

		/* read in the name of the regex. */
		i = 0;
		do
		{
			if (i >= MAX_REGEX_NAME_LENGTH-1)
			{
				prt_error("Error: File \"%s\", line %d: "
				          "Regex name too long.\n", file_name, line);
				goto failure;
			}
			name[i++] = c;
			c = fgetc(fp);
		}
		while ((!lg_isspace(c)) && (c != ':') && (c != EOF));
		name[i] = '\0';

		/* Skip possible whitespace after name, expect colon. */
		while (lg_isspace(c))
		{
			if (c == '\n') { line++; }
			c = fgetc(fp);
		}
		if (c != ':')
		{
			prt_error("Error: File \"%s\", line %d: "
			          "Regex missing colon.\n", file_name, line);
			goto failure;
		}

		/* Skip whitespace after colon, expect slash. */
		do
		{
			if (c == '\n') { line++; }
			c = fgetc(fp);
		}
		while (lg_isspace(c));
		if (c == '!')
		{
			neg = true;
			do
			{
				if (c == '\n') { line++; }
				c = fgetc(fp);
			}
			while (lg_isspace(c));
		}
		if (c != '/')
		{
			prt_error("Error: File \"%s\", line %d: "
			          "Regex missing leading slash.\n", file_name, line);
			goto failure;
		}

		/* Read in the regex. */
		i = 0;
		do
		{
			if (i > MAX_REGEX_LENGTH-1)
			{
				prt_error("Error: File \"%s\", line %d: "
				          "Regex too long.\n", file_name, line);
				goto failure;
			}
			prev = c;
			c = fgetc(fp);
			if ((c == '/') && (prev == '\\'))
				regex[i-1] = '/'; /* \/ is undefined */
			else
				regex[i++] = c;
		}
		while ((c != '/' || prev == '\\') && (c != EOF));
		regex[i-1] = '\0';

		/* Expect termination by a slash. */
		if (c != '/')
		{
			prt_error("Error: File \"%s\", line %d: "
			          "Regex missing trailing slash.\n", file_name, line);
			goto failure;
		}

		lgdebug(+D_REGEX+1, "%s: %s\n", name, regex);

		if (strcmp(name, "NO-EXPAND") == 0)
		{
			no_expand = true;
			continue;
		}

		if (!no_expand && !expand_character_ranges(file_name, line, name, regex))
			goto failure;

		/* Create new Regex_node and add to dict list. */
		new_re = (Regex_node *) malloc(sizeof(Regex_node));
		new_re->name    = string_set_add(name, dict->string_set);
		new_re->pattern = strdup(regex);
		new_re->neg     = neg;
		new_re->re      = NULL;
		new_re->next    = NULL;
		*tail = new_re;
		tail = &new_re->next;
	}

	fclose(fp);
	return true;
failure:
	fclose(fp);
	return false;
}

