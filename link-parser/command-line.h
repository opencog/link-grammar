/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2014 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <link-grammar/link-features.h>
#include <link-grammar/link-includes.h>

LINK_BEGIN_DECLS  /* Needed to keep MSVC6 happy */

typedef struct {
	Parse_Options popts;
	Parse_Options panic_opts;
	bool display_senses;
} Command_Options;

link_public_api(int)
     issue_special_command(const char * line, Command_Options *opts, Dictionary dict);

LINK_END_DECLS

Command_Options* command_options_create(void);
void command_options_delete(Command_Options* co);


