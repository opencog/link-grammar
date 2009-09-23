/*************************************************************************/
/* Copyright (c) 2005  Sampo Pyysalo                                     */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "api-structures.h"

int compile_regexs(Dictionary);
const char *match_regex(Dictionary, const char *);
void free_regexs(Dictionary dict);
