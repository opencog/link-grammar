/***************************************************************************/
/* Copyright (c) 2016 Linas Vepstas                                        */
/* Copyright (c) 2016 Amir Plivatsky                                       */
/* All rights reserved                                                     */
/*                                                                         */
/* Use of the link grammar parsing system is subject to the terms of the   */
/* license set forth in the LICENSE file included with this software.      */
/* This license allows free redistribution and use in source and binary    */
/* forms, with or without modification, subject to certain conditions.     */
/*                                                                         */
/***************************************************************************/

#ifndef _PARSER_UTILITIES_
#define _PARSER_UTILITIES_

#include "../link-grammar/link-includes.h"

#if _WIN32
#include <io.h> /* for isatty() */
#endif

#define MAX_INPUT 1024

#if !defined(MIN)
#define MIN(X,Y)  ( ((X) < (Y)) ? (X) : (Y))
#endif

#ifdef _WIN32
char *get_console_line(void);
void win32_set_utf8_output(void);
#define strcasecmp _stricmp
#define strncasecmp(a,b,s) _strnicmp((a),(b),(s))
#endif /* _WIN32 */

#endif // _PARSER_UTILITIES_
