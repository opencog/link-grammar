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

#include "api-types.h"
#include "link-includes.h"

#ifdef USE_FAT_LINKAGES
void       fat_prune(Sentence);
#endif
int        power_prune(Sentence, int mode, Parse_Options);
void       pp_and_power_prune(Sentence, int mode, Parse_Options);
int        prune_match(int dist, Connector * left, Connector * right);
void       expression_prune(Sentence);
