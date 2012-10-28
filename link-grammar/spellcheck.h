/*************************************************************************/
/* Copyright (c) 2009 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#if (defined HAVE_HUNSPELL) || (defined HAVE_ASPELL)
#include "api-types.h"

void * spellcheck_create(const char * lang);
void spellcheck_destroy(void *);
Boolean spellcheck_test(void *, const char * word);
int spellcheck_suggest(void * chk, char ***sug, const char * word);
void spellcheck_free_suggest(char **sug, int size);

#else

#include "utilities.h"  /* For MSVC inline portability */

static inline void * spellcheck_create(const char * lang) { return NULL; }
static inline void spellcheck_destroy(void * chk) {}
static inline Boolean spellcheck_test(void * chk, const char * word) { return 0; }
static inline int spellcheck_suggest(void * chk, char ***sug, const char * word) { return 0; }
static inline void spellcheck_free_suggest(char **sug, int size) {}

#endif 
