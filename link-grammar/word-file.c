/***************************************************************************/
/* Copyright (c) 2004                                                      */
/* Daniel Sleator, David Temperley, and John Lafferty                      */
/* All rights reserved                                                     */
/*                                                                         */
/* Use of the link grammar parsing system is subject to the terms of the   */
/* license set forth in the LICENSE file included with this software,      */ 
/* and also available at http://www.link.cs.cmu.edu/link/license.html      */
/* This license allows free redistribution and use in source and binary    */
/* forms, with or without modification, subject to certain conditions.     */
/*                                                                         */
/***************************************************************************/

#include <wchar.h>
#include <wctype.h>
#include "api.h"
#include "error.h"

/**
 * Reads in one word from the file, allocates space for it,
 * and returns it.
 */
static const char * get_a_word(Dictionary dict, FILE * fp)
{
	char word[MAX_WORD+4]; /* allow for 4-byte wide chars */
	const char * s;
	wint_t c;
	mbstate_t mbss;
	int j;

	do {
		c = fgetwc(fp);
	} while ((c != WEOF) && iswspace(c));
	if (c == WEOF) return NULL;

	memset(&mbss, 0, sizeof(mbss));
	for (j=0; (j <= MAX_WORD-1) && (!iswspace(c)) && (c != WEOF);)
	{
		j += wctomb_check(&word[j], c, &mbss);
		c = fgetwc(fp);
	}

	if (j >= MAX_WORD) {
		word[MAX_WORD] = 0x0;
		prt_error("Fatal Error: The dictionary contains a word that "
		          "is too long. The word was: %s", word);
		exit(1);
	}
	word[j] = '\0';
	s = string_set_add(word, dict->string_set);
	return s;
}

/**
 *
 * (1) opens the word file and adds it to the word file list
 * (2) reads in the words
 * (3) puts each word in a Dict_node
 * (4) links these together by their left pointers at the
 *     front of the list pointed to by dn
 * (5) returns a pointer to the first of this list
 */
Dict_node * read_word_file(Dictionary dict, Dict_node * dn, char * filename)
{
	Dict_node * dn_new;
	Word_file * wf;
	FILE * fp;
	const char * s;
	char file_name_copy[MAX_PATH_NAME+1];

	safe_strcpy(file_name_copy, filename+1, sizeof(file_name_copy)); /* get rid of leading '/' */

	if ((fp = dictopen(file_name_copy, "r")) == NULL) {
		prt_error("Error opening word file %s\n", file_name_copy);
		return NULL;
	}

	/*printf("   Reading \"%s\"\n", file_name_copy);*/
	/*printf("*"); fflush(stdout);*/

	wf = (Word_file *) xalloc(sizeof (Word_file));
	safe_strcpy(wf->file, file_name_copy, sizeof(wf->file));
	wf->changed = FALSE;
	wf->next = dict->word_file_header;
	dict->word_file_header = wf;

	while ((s = get_a_word(dict, fp)) != NULL) {
		dn_new = (Dict_node *) xalloc(sizeof(Dict_node));
		dn_new->left = dn;
		dn = dn_new;
		dn->string = s;
		dn->file = wf;
	}
	fclose(fp);
	return dn;
}

void save_files(Dictionary dict)
{
	Word_file *wf;
	FILE *fp;
	for (wf = dict->word_file_header; wf != NULL; wf = wf->next) {
		if (wf->changed) {
			if ((fp = fopen(wf->file, "w")) == NULL) {
				printf("\nCannot open %s. Gee, this shouldn't happen.\n", wf->file);
				printf("file not saved\n");
				return;
			}
			printf("   saving file \"%s\"\n", wf->file);
			/*output_dictionary(dict_root, fp, wf);*/
			fclose(fp);
			wf->changed = FALSE;
		}
	}
}

int files_need_saving(Dictionary dict)
{
	Word_file *wf;
	for (wf = dict->word_file_header; wf != NULL; wf = wf->next) {
		if (wf->changed) return TRUE;
	}
	return FALSE;
}
