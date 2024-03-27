/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2013, 2014 Linas Vepstas                                */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

/* Private state, used only while file-backed dictionaries are being
 * read. Invalid soon after read is complete.
 */
struct File_Dictionary_s
{
	const char    * input;
	const char    * pin;
	bool            recursive_error;
	bool            is_special;
	int             already_got_it; /* For char, but needs to hold EOF */
	char            token[MAX_TOKEN_LENGTH];
};

typedef struct File_Dictionary_s * File_Dictionary;
