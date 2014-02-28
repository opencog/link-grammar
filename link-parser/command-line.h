/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
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
link_public_api(int)
     issue_special_command(const char * line, Parse_Options opts, Dictionary dict);
LINK_END_DECLS
