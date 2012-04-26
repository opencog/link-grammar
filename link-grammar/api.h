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

#include "structures.h"
#include "link-includes.h"
#include "api-structures.h"
#include "externs.h"

#include "analyze-linkage.h"
#include "and.h"
#include "build-disjuncts.h"
#include "constituents.h"
#include "count.h"
#include "error.h"
#include "extract-links.h"
#include "fast-match.h"
#include "idiom.h"
#include "massage.h"
#include "post-process.h"
#include "pp_knowledge.h"
#include "pp_lexer.h"
#include "pp_linkset.h"
#include "preparation.h"
#include "print.h"
#include "print-util.h"
#include "prune.h"
#include "read-dict.h"
#include "resources.h"
#include "string-set.h"
#include "tokenize.h"
#include "utilities.h"
#include "word-file.h"
#include "word-utils.h"

#endif
