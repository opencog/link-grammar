/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/
#ifndef LG_PRINT_UTIL_H_
#define LG_PRINT_UTIL_H_

#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define GNUC_PRINTF( format_idx, arg_idx )    \
  __attribute__((__format__ (__printf__, format_idx, arg_idx)))
#else
#define GNUC_PRINTF( format_idx, arg_idx )
#endif

#include <stdlib.h>

typedef struct String_s String;

String * string_new(void);
void string_delete(String *);
char * string_copy(String *);
void append_string(String * string, const char *fmt, ...) GNUC_PRINTF(2,3);
size_t append_utf8_char(String * string, const char * mbs);

#endif


