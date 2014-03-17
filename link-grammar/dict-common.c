/*************************************************************************/
/* Copyright (c) 2013, 2014 Linas Vepstas                                */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "dict-common.h"
#include "structures.h"
#include "utilities.h"

/* ======================================================================== */
/* Replace the right-most dot with SUBSCRIPT_MARK */
void patch_subscript(char * s)
{
	char *ds, *de;
	ds = strrchr(s, SUBSCRIPT_DOT);
	if (!ds) return;

	/* a dot at the end or a dot followed by a number is NOT
	 * considered a subscript */
	de = ds + 1;
	if (*de == '\0') return;
	if (isdigit((int)*de)) return;
	*ds = SUBSCRIPT_MARK;
}

