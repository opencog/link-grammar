/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2009 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this softwares.   */ 
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/
#ifndef _LINK_GRAMMAR_ERROR_H_
#define _LINK_GRAMMAR_ERROR_H_

#include "link-includes.h"

typedef struct 
{
	Sentence sent;
} err_ctxt;

typedef enum 
{
	Fatal = 1,
	Error,
	Warn,
	Info,
	Debug
} severity;

void err_msg(err_ctxt *, severity, const char *fmt, ...) GNUC_PRINTF(3,4);

#endif

