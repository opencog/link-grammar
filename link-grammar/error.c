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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include "error.h"

static char   buf[1024];
#define CRLF  printf("\n")

int    lperrno;
char   lperrmsg[1024];

const char * msg_of_lperror(int lperr) {
    switch(lperr) {
    case NODICT:
	return "Could not open dictionary ";
	break;
    case DICTPARSE:
	return "Error parsing dictionary ";
	break;
    case WORDFILE:
	return "Error opening word file ";
	break;
    case SEPARATE:
	return "Error separating sentence ";
	break;
    case NOTINDICT:
	return "Sentence not in dictionary ";
	break;
    case BUILDEXPR:
	return "Could not build sentence expressions ";
	break;
    case INTERNALERROR:
	return "Internal error.  Send mail to link@juno.com ";
	break;
    default:
	return "";
	break;
    }
    return NULL;
}


void lperror(int lperr, char *fmt, ...) {
    char temp[1024];
    va_list args;

    va_start(args, fmt);
    sprintf(lperrmsg, "PARSER-API: %s", msg_of_lperror(lperr));
    vsprintf(temp, fmt, args); 
    strcat(lperrmsg, temp);
    va_end(args);
    lperrno = lperr;
    fflush(stderr);
}

void error(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args); CRLF;
    va_end(args);
    fprintf(stderr, "\n");
    if (errno > 0) {
	perror(buf);
	fprintf(stderr, "errno=%d\n", errno);
	fprintf(stderr, buf);
	fprintf(stderr, "\n");
    }
    fflush(stderr);
    fflush(stdout);
    fprintf(stderr, "Parser quitting.\n");
    exit(1); /* Always fail and print out this file name */
}
