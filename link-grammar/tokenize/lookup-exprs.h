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

#ifndef _LOOKUP_EXPRS_H
#define _LOOKUP_EXPRS_H

#include "api-types.h"
#include "link-includes.h"

bool sentence_in_dictionary(Sentence);
bool build_sentence_expressions(Sentence, Parse_Options);

#endif /* _LOOKUP_EXPRS_H */
