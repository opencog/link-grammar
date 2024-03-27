/*************************************************************************/
/* Copyright (c) 2021 Amir Plivatsky                                     */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "api-structures.h"

/*
 * Definitions for linkgrammar_get_configuration().
 */

#define MACVAL(macro) #macro lg_str(=macro) " "

#ifdef __STDC_VERSION__
#define LG_S1 MACVAL(__STDC_VERSION__)
#else
#define LG_S1
#endif

#ifdef _POSIX_C_SOURCE
#define LG_S2 MACVAL(_POSIX_C_SOURCE)
#else
#define LG_S2
#endif

#if !defined _POSIX_C_SOURCE || _POSIX_C_SOURCE == 0
 #ifdef _POSIX_SOURCE
 #define LG_S3 MACVAL(_POSIX_SOURCE)
 #else
 #define LG_S3
 #endif
#else
#define LG_S3
#endif

/* -DCC=$(CC) is added in the Makefile. */
#ifdef CC
#define LG_CC CC
#elif _MSC_VER
#define LG_CC "lc"
#else
#define LG_CC "(unknown)"
#endif

#ifdef __VERSION__
#define LG_V1 MACVAL(__VERSION__)
#else
#define LG_V1
#endif

#ifdef _MSC_FULL_VER
#define LG_V2 MACVAL(_MSC_FULL_VER)
#else
#define LG_V2
#endif

#define LG_COMP LG_CC " " LG_V1 " " LG_V2
#define LG_STD LG_S1 LG_S2 LG_S3

#ifdef __unix__
#define LG_unix "__unix__ "
#else
#define LG_unix
#endif

#ifdef _WIN32
#define LG_WIN32 "_WIN32 "
#else
#define LG_WIN32
#endif

#ifdef _WIN64
#define LG_WIN64 "_WIN64 "
#else
#define LG_WIN64
#endif

#ifdef __CYGWIN__
#define LG_CYGWIN "__CYGWIN__ "
#else
#define LG_CYGWIN
#endif

#ifdef __MINGW32__
#define LG_MINGW32 "__MINGW32__ "
#else
#define LG_MINGW32
#endif

#ifdef __MINGW64__
#define LG_MINGW64 "__MINGW64__ "
#else
#define LG_MINGW64
#endif

#ifdef __APPLE__
#define LG_APPLE "__APPLE__ "
#else
#define LG_APPLE
#endif

#ifdef __MACH__
#define LG_MACH "__MACH__ "
#else
#define LG_MACH
#endif

#ifndef DICTIONARY_DIR
#define DICTIONARY_DIR "None"
#endif

#define LG_windows LG_WIN32 LG_WIN64 LG_CYGWIN LG_MINGW32 LG_MINGW64
#define LG_mac LG_APPLE LG_MACH

/**
 * Return information about the configuration as a static string.
 */
const char *linkgrammar_get_configuration(void)
{
	return "Compiled with: " LG_COMP "\n"
		"OS: " LG_HOST_OS " " LG_unix LG_windows LG_mac "\n"
		"Standards: " LG_STD "\n"
		"Configuration (source code):\n\t"
			LG_CPPFLAGS "\n\t"
			LG_CFLAGS "\n"
	   "Configuration (features):\n\t"
			"DICTIONARY_DIR=" DICTIONARY_DIR "\n\t"
			LG_DEFS
		;
}
