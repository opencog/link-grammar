/********************************************************************************/
/* Copyright (c) 2004                                                           */
/* Daniel Sleator, David Temperley, and John Lafferty                           */
/* All rights reserved                                                          */
/*                                                                              */
/* Use of the link grammar parsing system is subject to the terms of the        */
/* license set forth in the LICENSE file included with this software,           */ 
/* and also available at http://www.link.cs.cmu.edu/link/license.html           */
/* This license allows free redistribution and use in source and binary         */
/* forms, with or without modification, subject to certain conditions.          */
/*                                                                              */
/********************************************************************************/

#include <link-grammar/api.h>

char * get_a_word(Dictionary dict, FILE * fp) {
/* Reads in one word from the file, allocates space for it,
   and returns it.
*/
    char word[MAX_WORD+1];
    char * s;
    int c, j;
    do {
	c = getc(fp);
    } while ((c != EOF) && isspace(c));
    if (c == EOF) return NULL;

    for (j=0; (j <= MAX_WORD-1) && (!isspace(c)) && (c != EOF); j++) {
	word[j] = c;
	c = getc(fp);
    }

    if (j == MAX_WORD) {
	error("The dictionary contains a word that is too long.");
    }
    word[j] = '\0';
    s = string_set_add(word, dict->string_set);
    return s;
}

Dict_node * read_word_file(Dictionary dict, Dict_node * dn, char * filename) {
/*

   (1) opens the word file and adds it to the word file list
   (2) reads in the words
   (3) puts each word in a Dict_node
   (4) links these together by their left pointers at the
       front of the list pointed to by dn
   (5) returns a pointer to the first of this list

*/
    Dict_node * dn_new;
    Word_file * wf;
    FILE * fp;
    char * s;
    char file_name_copy[MAX_PATH_NAME+1];

    safe_strcpy(file_name_copy, filename+1, sizeof(file_name_copy)); /* get rid of leading '/' */

    if ((fp = dictopen(dict->name, file_name_copy, "r")) == NULL) {
	lperror(WORDFILE, "%s\n", file_name_copy);
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

#define LINE_LIMIT 70
int xwhere_in_line;

void routput_dictionary(Dict_node * dn, FILE *fp, Word_file *wf) {
/* scan the entire dictionary rooted at dn and print into the file
   pf all of the words whose origin is the file wf.
*/
    if (dn == NULL) return;
    routput_dictionary(dn->left, fp, wf);
    if (dn->file == wf) {
	if (xwhere_in_line+strlen(dn->string) > LINE_LIMIT) {
	    xwhere_in_line = 0;
	    fprintf(fp,"\n");
	}
	xwhere_in_line += strlen(dn->string) + 1;
	fprintf(fp,"%s ", dn->string);
    }
    routput_dictionary(dn->right, fp, wf);
}

void output_dictionary(Dict_node * dn, FILE *fp, Word_file *wf) {
    xwhere_in_line = 0;
    routput_dictionary(dn, fp, wf);
    fprintf(fp,"\n");
}

void save_files(Dictionary dict) {
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

int files_need_saving(Dictionary dict) {
    Word_file *wf;
    for (wf = dict->word_file_header; wf != NULL; wf = wf->next) {
	if (wf->changed) return TRUE;
    }
    return FALSE;
}
