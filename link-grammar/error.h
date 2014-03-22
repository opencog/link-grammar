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
Boolean feature_enabled(const char *, const char *);

#ifdef _WIN32
# define __func__ __FUNCTION__
#endif
#define lgdebug(level, ...) \
	{ if ((verbosity > level) && \
	 	(('\0' == debug[0]) || feature_enabled(debug, __func__))) \
	 	{ printf(__VA_ARGS__); } \
	}

#define test_enabled(feature) \
	(('\0' != test) && feature_enabled(test, feature))

#endif
