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

#ifndef _PRUNE_H
#define _PRUNE_H

#include "api-types.h"                  // Tracon_sharing
#include "link-includes.h"

unsigned int pp_and_power_prune(Sentence, Tracon_sharing *,  unsigned int,
                              Parse_Options, unsigned int *[2]);
bool optional_gap_collapse(Sentence, int, int);

#endif /* _PRUNE_H */
