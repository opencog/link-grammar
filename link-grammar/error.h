/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2009 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/
#ifndef _LINK_GRAMMAR_ERROR_H_
#define _LINK_GRAMMAR_ERROR_H_

#include "link-includes.h"
#include "externs.h"                    // verbosity
#include "utilities.h"                  // GNUC_NORETURN

/* User verbosity levels are 1-4, to be used for user info/debug.
 * For now hard-coded numbers are still used instead of D_USER_BASIC/TIMES. */
#define D_USER_BASIC 1   /* Basic verbosity level. */
#define D_USER_TIMES 2   /* Display step times. */
#define D_USER_INFO  3   /* Display some Info messages. */
#define D_USER_FILES 4   /* Display data file search and locale setup. */
#define D_USER_MAX   4   /* Maximum user verbosity level. */
#define D_DICT      10   /* Base of dictionary debug levels. */
#define D_SPEC     100   /* Base of special stand-alone levels. */

typedef struct
{
	Sentence sent;
} err_ctxt;

void err_msgc(err_ctxt *, lg_error_severity, const char *fmt, ...) GNUC_PRINTF(3,4);
#define err_msg(...) err_msgc(NULL, __VA_ARGS__)
const char *feature_enabled(const char *, ...);

/**
 * Print a debug message according to their level.
 * Print the messages at levels <= the specified verbosity, with the
 * following restrictions:
 * - Level numbers 2 to D_USER_MAX are not printed on verbosity>D_USER_MAX,
 *   because they are designed only for extended user information.
 * - When verbosity > D_SPEC, print messages only when level==verbosity.
 * - The !debug variable can be set to a comma-separated list of functions
 *   or source filenames in order to restrict the debug messages to these
 *   functions or filenames only.
 *
 * Invoking lgdebug() with a level number preceded by a + (+level) adds
 * printing of the function name.
 * FIXME: The level is then Trace and if the message starts with a level
 * it is ignored.
 */
#define lgdebug(level, ...) \
	(( \
	(((D_SPEC>=verbosity) && (verbosity>=(level))) || (verbosity==(level))) && \
	(((level)<=1) || !(((level)<=D_USER_MAX) && (verbosity>D_USER_MAX))) && \
	(('\0' == debug[0]) || \
	feature_enabled(debug, __func__, __FILE__, NULL))) ? \
	( \
		(STRINGIFY(level)[0] == '+' ? \
			(void)err_msg(lg_Trace, "%s: ", __func__) : \
			(void)0), \
		(void)err_msg(lg_Trace,  __VA_ARGS__) \
	) : \
	(void)0)

/**
 * Wrap-up a debug-messages block.
 * Preceding the level number by a + (+level) adds printing of the
 * function name.
 * The !debug variable can be set to a comma-separated list of functions
 * in order to restrict the debug messages to these functions only.
 *
 * Return true if the debug-messages block should be executed, else false.
 *
 * Usage example, for debug messages at verbosity V:
 * if (verbosity_level(V))
 * {
 *    print_disjunct(d);
 * }
 *
 * The optional printing of the function name is done here by prt_error()
 * and not err_msg(), in order to not specify the message severity.
 * Also note there is no trailing newline in that case. These things
 * ensured the message severity will be taken from a following message
 * which includes a newline. So verbosity_level(V) can be used for any
 * desired message severity.
 * The optional argument is used for additional names that can be used
 * in the "debug" option (in addition to the current function and file names).
 */
#define verbosity_level(level, ...) \
	(( \
	(((D_SPEC>=verbosity) && (verbosity>=(level))) || (verbosity==(level))) && \
	(((level)<=1) || !(((level)<=D_USER_MAX) && (verbosity>D_USER_MAX))) && \
	(('\0' == debug[0]) || \
	feature_enabled(debug, __func__, __FILE__, (__VA_ARGS__ ""), NULL))) \
	? ((STRINGIFY(level)[0] == '+' ? prt_error("%s: ", __func__) : 0), true) \
	: false)

/**
 * Return TRUE if the given feature (a string) is set in the !test variable
 * (a comma-separated feature list).
 */
#define test_enabled(feature) \
	(('\0' != test[0]) ? feature_enabled(test, feature, NULL) : NULL)

extern void (* assert_failure_trap)(void);
#define FILELINE __FILE__ ":" STRINGIFY(__LINE__)
void assert_failure(const char[], const char[], const char *, const char *, ...)
	GNUC_PRINTF(4,5) GNUC_NORETURN;

#undef assert
#define assert(ex, ...) \
do { \
	if (!(ex)) assert_failure(STRINGIFY(ex), __func__, FILELINE, __VA_ARGS__); }\
while(0)

#endif
