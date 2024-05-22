/***************************************************************************/
/* Copyright (c) 2012 Linas Vepstas                                        */
/* All rights reserved                                                     */
/*                                                                         */
/* Use of the link grammar parsing system is subject to the terms of the   */
/* license set forth in the LICENSE file included with this software.      */
/* This license allows free redistribution and use in source and binary    */
/* forms, with or without modification, subject to certain conditions.     */
/*                                                                         */
/***************************************************************************/

#if HAVE_EDITLINE
char *lg_readline(const char *mb_prompt);
#endif /* HAVE_EDITLINE */

#if HAVE_WIDECHAR_EDITLINE
void find_history_filepath(const char *, const char *, const char *);
#else
// Defining here an empty inline function causes a mysterious ld
// error on MinGW.
#define find_history_filepath(a, b, c)
#endif /* HAVE_WIDECHAR_EDITLINE */
