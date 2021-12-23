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

#include "command-line.h"

char *expand_homedir(const char *fn);
void set_screen_width(Command_Options*);
void initialize_screen_width(Command_Options *);

#define MAX_INPUT_LINE 2048

#ifdef _WIN32
#ifndef __MINGW32__
/* There is no ssize_t definition in native Windows. */
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#define strcasecmp _stricmp
#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif

char *get_console_line(void);
char **ms_windows_setup(int);
int lg_isatty(int);
#define isatty lg_isatty
#else /* !_WIN32 */
#define ms_windows_setup(argc) (argv)
#endif /* _WIN32 */

char *get_utf8_line(char *, int, FILE *);
#endif // _PARSER_UTILITIES_
