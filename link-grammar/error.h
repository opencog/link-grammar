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
#include "utilities.h"                  // NORETURN, STRINGIFY

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
void debug_msg(int, int, char, const char[], const char[], const char *fmt, ...)
	GNUC_PRINTF(6,7);
bool verbosity_check(int, int, char, const char[], const char[], const char *);
const char *syserror_msg(int);
void lg_lib_failure(void);

/**
 * Print a debug messages according to their level.
 * Print the messages at levels <= the specified verbosity, with the
 * following restrictions:
 * - Level numbers 2 to D_USER_MAX are not printed on verbosity>D_USER_MAX,
 *   because they are designed only for extended user information.
 * - When verbosity > D_SPEC, print messages only when level==verbosity.
 * - The !debug variable can be set to a comma-separated list of functions
 *   and/or source filenames in order to restrict the debug messages to these
 *   functions and/or filenames only.
 *
 * Preceding the level number by a + (+level) adds printing of the
 * function name.
 *
 * Invoking lgdebug() with a level number preceded by a + (+level) adds
 * printing of the function name.
 * FIXME: The level is then Trace and if the message starts with a level
 * this level is ignored.
 */
#define lgdebug(level, ...) \
	do { \
		if (verbosity >= (level)) \
		    debug_msg(level, verbosity, STRINGIFY(level)[0], __func__, __FILE__, \
		              __VA_ARGS__); \
	} \
	while(0)

/**
 * Wrap-up a debug-messages block.
 * Return true if the debug-messages block should be executed, else false.
 *
 * Usage example, for debug messages at verbosity V:
 * if (verbosity_level(V))
 * {
 *    print_disjunct(d);
 * }
 *
 * A single optional argument can be used to add names for the "debug"
 * option (in addition to the current function and file names). (Several
 * names may be supplied using backslash-escaped comma separators - not
 * actually used for now.)
 */
#define verbosity_level(level, ...) \
	((verbosity >= (level)) && \
	verbosity_check(level, verbosity, STRINGIFY(level)[0], __func__, __FILE__, \
						 "" __VA_ARGS__))

/**
 * Return TRUE if the given feature (a string) is set in the !test variable
 * (a comma-separated feature list).
 */
#define test_enabled(feature) \
	(('\0' != test[0]) ? feature_enabled(test, feature, NULL) : NULL)

extern void (* assert_failure_trap)(void);
#define FILELINE __FILE__ ":" STRINGIFY(__LINE__)
NORETURN
void assert_failure(const char[], const char[], const char *, const char *, ...)
	GNUC_PRINTF(4,5);

/* Define a private version of assert() with a printf-like error
 * message. The C one is not used. */
#undef assert
#define assert(ex, ...) \
do { \
	if (!(ex)) assert_failure(#ex, __func__, FILELINE, __VA_ARGS__); \
} \
while(0)

/* Generally, our asserts should always remain in the code, even for
 * non-DEBUG images. However, some asserts may impose non-negligible
 * overhead and thus used only in DEBUG mode. */
#ifdef DEBUG
#define dassert assert
#else
#define dassert(...)
#endif

#endif
