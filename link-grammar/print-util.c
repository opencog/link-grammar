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

#include <stdarg.h>
#include <link-grammar/api.h>

/* This is a "safe" append function, used here to build up a link diagram
   incrementally.  Because the diagram is built up a few characters at
   a time, we keep around a pointer to the end of string to prevent
   the algorithm from being quadratic. */

String * String_create(void) {
    String * string;
    string = (String *) exalloc(sizeof(String));
    string->allocated = 1;
    string->p = (char *) exalloc(sizeof(char));
    string->p[0] = '\0';
    string->eos = string->p;
    return string;
}

int append_string(String * string, const char *fmt, ...) {
    char temp_string[1024];
    char * p;
    int new_size;
    va_list args;

    va_start(args, fmt);
    vsprintf(temp_string, fmt, args); 
    va_end(args);

    if (string->allocated <= strlen(string->p)+strlen(temp_string)) {
	new_size = 2*string->allocated+strlen(temp_string)+1;
	p = exalloc(sizeof(char)*new_size);
	strcpy(p, string->p);
	strcat(p, temp_string);
	exfree(string->p, sizeof(char)*string->allocated);
	string->p = p;
	string->eos = strchr(p,'\0');
	string->allocated = new_size;
    }
    else {
	strcat(string->eos, temp_string);
	string->eos += strlen(temp_string);
    }

    return 0;
}

