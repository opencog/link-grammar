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

#ifndef LINK_GRAMMAR_API_H
#define LINK_GRAMMAR_API_H

/* This file is somewhat misnamed, as everything here defines the
 * link-private, internal-use-only "api", which is subject to change
 * from revision to revision. No external code should link to this
 * stuff.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)) && defined(__ELF__)
#define link_private		__attribute__((__visibility__("hidden")))
#else
#define link_private
#endif

#include <link-grammar/structures.h>
#include <link-grammar/link-includes.h>
#include <link-grammar/api-structures.h>
#include <link-grammar/externs.h>

#include <link-grammar/analyze-linkage.h>
#include <link-grammar/and.h>
#include <link-grammar/build-disjuncts.h>
#include <link-grammar/constituents.h>
#include <link-grammar/count.h>
#include <link-grammar/error.h>
#include <link-grammar/extract-links.h>
#include <link-grammar/fast-match.h>
#include <link-grammar/idiom.h>
#include <link-grammar/massage.h>
#include <link-grammar/post-process.h>
#include <link-grammar/pp_knowledge.h>
#include <link-grammar/pp_lexer.h>
#include <link-grammar/pp_linkset.h>
#include <link-grammar/preparation.h>
#include <link-grammar/print.h>
#include <link-grammar/print-util.h>
#include <link-grammar/prune.h>
#include <link-grammar/read-dict.h>
#include <link-grammar/resources.h>
#include <link-grammar/string-set.h>
#include <link-grammar/tokenize.h>
#include <link-grammar/utilities.h>
#include <link-grammar/word-file.h>
#include <link-grammar/word-utils.h>

#endif
