/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2009 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.   */ 
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
bool feature_enabled(const char *, const char *);

#ifdef _WIN32
# define __func__ __FUNCTION__
#endif

/**
 * Print a debug message at verbosity >= level.
 * Preceding the level number by a + (+level) adds printing of the
 * function name.
 * The !debug variable can be set to a comma-separated list of functions
 * in order to restrict the debug messages to these functions only.
 */
#define lgdebug(level, ...) \
(((verbosity >= (level)) && \
	 (('\0' == debug[0]) || feature_enabled(debug, __func__))) \
	 ? ((STRINGIFY(level)[0] == '+' ? printf("%s: ", __func__) : (void)0), \
	 printf(__VA_ARGS__)) : (void)0)

/**
 * Return TRUE if the given feature (a string) is set in the !test variable
 * (a comma-separated feature list).
 */
#define test_enabled(feature) \
	(('\0' != test) && feature_enabled(test, feature))

#endif
