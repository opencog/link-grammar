/*****************************************************************************/
/* Copyright (c) 2005                                                        */
/* Sampo Pyysalo                                                             */
/*                                                                           */
/* Use of the link grammar parsing system is subject to the terms of the     */
/* license set forth in the LICENSE file included with this software,        */
/* and also available at http://www.link.cs.cmu.edu/link/license.html        */
/* This license allows free redistribution and use in source and binary      */
/* forms, with or without modification, subject to certain conditions.       */
/*                                                                           */
/*****************************************************************************/

#include "link-includes.h"

/*
  Function for reading regular expression name:pattern combinations
  into the Dictionary from a given file.

  The format of the regex file is as follows:
  
  Lines starting with "%" are comments and are ignored.
  All other nonempty lines must follow the following format:
  
      REGEX_NAME:  /pattern/
  
  here REGEX_NAME is an identifying unique name for the regex.
  This name is used to determine the disjuncts that will be assigned to
  tokens matching the pattern, so in the dictionary file (e.g. 4.0.dict) 
  you must have something like

     REGEX_NAME:  (({@MX+} & (JG- or <noun-main-s>)) or YS+)) or AN+ or G+);

  using the same name. The pattern itself must be surrounded by slashes.
  Extra whitespace is ignored.
*/

#define MAX_REGEX_NAME_LENGTH 50
#define MAX_REGEX_LENGTH      255

int read_regex_file(char *regex_path_name, char *regex_file_name,
		    Dictionary dict) {
  Regex_node **tail = &dict->regex_root; /* Last Regex_node * in list */
  Regex_node *new_re;
  char name[MAX_REGEX_NAME_LENGTH];
  char regex[MAX_REGEX_LENGTH];
  int c,prev,i,line=1;
  FILE *fp;
  
  /* use dictopen to apply same path search as for the dictionary. */
  fp = dictopen(regex_path_name, regex_file_name, "r");
  if(fp == NULL) {
    lperror(NODICT, regex_file_name);
    return 0;
  }

  /* read in regexs. loop broken on EOF. */
  for(;;) {
    /* skip whitespace and comments. */
    do {
      do { 
	c=fgetc(fp);
	if(c == '\n') { line++; }
      } while(isspace(c));
      if(c == '%') {
	while((c != EOF) && (c != '\n')) { c=fgetc(fp); }
	line++;
      }
    } while(isspace(c));

    if(c == EOF) { break; } /* done. */

    /* read in the name of the regex. */
    i = 0;
    do {
      if(i > MAX_REGEX_NAME_LENGTH-1) {
	lperror(DICTPARSE, ". Regex name too long on line %d", line);
	return 0;
      }
      name[i++] = c;
      c=getc(fp);
    } while((!isspace(c)) && (c != ':') && (c != EOF));
    name[i] = '\0';
    
    /* skip possible whitespace after name, expect colon. */
    while(isspace(c)) { 
      if(c == '\n') { line++; }
      c=getc(fp); 
    }
    if(c != ':') {
      lperror(DICTPARSE, ". Regex  missing colon on line %d", line);
      return 0;
    }

    /* skip whitespace after colon, expect slash. */
    do {
      if(c == '\n') { line++; }
      c=getc(fp); 
    } while(isspace(c));
    if(c != '/') {
      lperror(DICTPARSE, ". Regex  missing slash on line %d\n", line);
      return 0;      
    }

    /* read in the regex. */
    prev = 0;
    i = 0;
    do {
      if(i > MAX_REGEX_LENGTH-1) {
	lperror(DICTPARSE, ". Regex too long on line %d", line);
	return 0;
      }
      prev=c;
      c=getc(fp);
      regex[i++] = c;
    } while((c != '/' || prev == '\\') && (c != EOF));
    regex[i-1] = '\0';

    /* expect termination by a slash. */
    if(c != '/') {
      lperror(DICTPARSE, ". Regex  missing slash on line %d", line);
      return 0;      
    }

    /* Create new Regex_node and add to dict list. */
    new_re = (Regex_node *)malloc(sizeof(Regex_node));
    new_re->name    = strdup(name);
    new_re->pattern = strdup(regex);
    new_re->re      = NULL;
    new_re->next    = NULL;
    *tail = new_re;
    tail  = &new_re->next;
  }
  fclose(fp);
  return 1;
}

