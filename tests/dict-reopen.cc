/***************************************************************************/
/* Copyright (c) 2014 Linas Vepstas                                        */
/* All rights reserved                                                     */
/*                                                                         */
/* Use of the link grammar parsing system is subject to the terms of the   */
/* license set forth in the LICENSE file included with this software.      */
/* This license allows free redistribution and use in source and binary    */
/* forms, with or without modification, subject to certain conditions.     */
/*                                                                         */
/***************************************************************************/

// This implelements a simple check, opening and closing the dictionary
// repeatedly.

#include <locale.h>
#include <stdio.h>
#include "link-grammar/link-includes.h"

int main()
{
	const char * input_string[] = {
		"под броню боевого робота устремились потоки энергии.",
		"через четверть часа здесь будет полно полицейских." };

	setlocale(LC_ALL, "en_US.UTF-8");

	Parse_Options opts = parse_options_create();

	for (int i=0; i<20; i++)
	{
		printf("Opening the dictionary for the %d'th time\n", i+1);
		Dictionary dict = dictionary_create_lang("ru");
		if (!dict) {
			printf ("Fatal error: Unable to open the dictionary\n");
			return 1;
		}

		for (int j=0; j<2; j++) {
			Sentence sent = sentence_create(input_string[j], dict);
			sentence_split(sent, opts);
			int num_linkages = sentence_parse(sent, opts);
			if (num_linkages > 0) {
		   	Linkage linkage = linkage_create(0, sent, opts);
		   	linkage_delete(linkage);
			}
			sentence_delete(sent);
		}
		dictionary_delete(dict);
	}
	parse_options_delete(opts);
	return 0;
}
