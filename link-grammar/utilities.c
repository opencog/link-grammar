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
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef ENABLE_BINRELOC
#include "prefix.h"
#endif /* BINRELOC */

#ifdef _WIN32
#  include <windows.h>
#  define DIR_SEPARATOR '\\'
#  define PATH_SEPARATOR ';'
#else
#  define DIR_SEPARATOR '/'
#  define PATH_SEPARATOR ':'
#endif

#define IS_DIR_SEPARATOR(ch) (DIR_SEPARATOR == (ch))
#define DEFAULTPATH DICTIONARY_DIR

/* This file contains certain general utilities. */

int   verbosity;

void safe_strcpy(char *u, char * v, int usize) {
/* Copies as much of v into u as it can assuming u is of size usize */
/* guaranteed to terminate u with a '\0'.                           */
    strncpy(u, v, usize-1);
    u[usize-1] = '\0';
}

void safe_strcat(char *u, char *v, int usize) {
/* catenates as much of v onto u as it can assuming u is of size usize           */
/* guaranteed to terminate u with a '\0'.  Assumes u and v are null terminated.  */
    strncat(u, v, usize-strlen(u)-1);
    u[usize-1] = '\0';
}

void free_connectors(Connector *e) {
/* free the list of connectors pointed to by e
   (does not free any strings)
*/
    Connector * n;
    for(;e != NULL; e = n) {
	n = e->next;
	xfree((char *)e, sizeof(Connector));
    }
}

void free_disjuncts(Disjunct *c) {
/* free the list of disjuncts pointed to by c
   (does not free any strings)
*/
    Disjunct *c1;
    for (;c != NULL; c = c1) {
	c1 = c->next;
	free_connectors(c->left);
	free_connectors(c->right);
	xfree((char *)c, sizeof(Disjunct));
    }
}

Connector * init_connector(Connector *c) {
    c->length_limit = UNLIMITED_LEN;
    /*    c->my_word = NO_WORD;  */       /* mark it unset, to make sure it gets set later */
    return c;
}

void free_X_nodes(X_node * x) {
/* frees the list of X_nodes pointed to by x, and all of the expressions */
    X_node * y;
    for (; x!= NULL; x = y) {
	y = x->next;
	free_Exp(x->exp);
	xfree((char *)x, sizeof(X_node));
    }
}

void free_E_list(E_list *);
void free_Exp(Exp * e) {
    if (e->type != CONNECTOR_type) {
	free_E_list(e->u.l);
    }
    xfree((char *)e, sizeof(Exp));
}

void free_E_list(E_list * l) {
    if (l == NULL) return;
    free_E_list(l->next);
    free_Exp(l->e);
    xfree((char *)l, sizeof(E_list));
}

int size_of_expression(Exp * e) {
/* Returns the number of connectors in the expression e */
    int size;
    E_list * l;
    if (e->type == CONNECTOR_type) return 1;
    size = 0;
    for (l=e->u.l; l!=NULL; l=l->next) {
	size += size_of_expression(l->e);
    }
    return size;
}

unsigned int randtable[RTSIZE];

/* There is a legitimate question of whether having the hash function    */
/* depend on a large array is a good idea.  It might not be fastest on   */
/* a machine that depends on caching for its efficiency.  On the other   */
/* hand, Phong Vo's hash (and probably other linear-congruential) is     */
/* pretty bad.  So, mine is a "competitive" hash function -- you can't   */
/* make it perform horribly.                                             */

void init_randtable(void) {
    int i;
    srand(10);
    for (i=0; i<RTSIZE; i++) {
	randtable[i] = rand();
    }
}

/* Build a copy of the given expression (don't copy strings, of course) */
E_list * copy_E_list(E_list * l);
Exp * copy_Exp(Exp * e) {
    Exp * n;
    if (e == NULL) return NULL;
    n = (Exp *) xalloc(sizeof(Exp));
    *n = *e;
    if (e->type != CONNECTOR_type) {
	n->u.l = copy_E_list(e->u.l);
    }
    return n;
}

E_list * copy_E_list(E_list * l) {
    E_list * nl;
    if (l == NULL) return NULL;
    nl = (E_list *) xalloc(sizeof(E_list));
    *nl = *l;    /* not necessary -- both fields will be built below */
    nl->next = copy_E_list(l->next);
    nl->e = copy_Exp(l->e);
    return nl;
}

Connector * copy_connectors(Connector * c) {
/* This builds a new copy of the connector list pointed to by c.
   Strings, as usual, are not copied.
*/
    Connector *c1;
    if (c == NULL) return NULL;
    c1 = init_connector((Connector *) xalloc(sizeof(Connector)));
    *c1 = *c;
    c1->next = copy_connectors(c->next);
    return c1;
}

Disjunct * copy_disjunct(Disjunct * d) {
/* This builds a new copy of the disjunct pointed to by d (except for the
   next field which is set to NULL).  Strings, as usual,
   are not copied.
*/
    Disjunct * d1;
    if (d == NULL) return NULL;
    d1 = (Disjunct *) xalloc(sizeof(Disjunct));
    *d1 = *d;
    d1->next = NULL;
    d1->left = copy_connectors(d->left);
    d1->right = copy_connectors(d->right);
    return d1;
}

int max_space_in_use;
int space_in_use;
int max_external_space_in_use;
int external_space_in_use;

void * xalloc(int size) {
/* To allow printing of a nice error message, and keep track of the
   space allocated.
*/
    char * p = (char *) malloc(size);
    space_in_use += size;
    if (space_in_use > max_space_in_use) max_space_in_use = space_in_use;
    if ((p == NULL) && (size != 0)){
        printf("Ran out of space.\n");
	abort();
        exit(1);
    }
    return (void *) p;
}

void xfree(void * p, int size) {
    space_in_use -= size;
    free(p);
}

void * exalloc(int size) {

    char * p = (char *) malloc(size);
    external_space_in_use += size;
    if (external_space_in_use > max_external_space_in_use) {
	max_external_space_in_use = external_space_in_use;
    }
    if ((p == NULL) && (size != 0)){
        printf("Ran out of space.\n");
	abort();
        exit(1);
    }
    return (void *) p;
}

void exfree(void * p, int size) {
    external_space_in_use -= size;
    free(p);
}

/* This is provided as part of the API */
void string_delete(char * p) {
    exfree(p, strlen(p)+1);
}

void exfree_connectors(Connector *e) {
    Connector * n;
    for(;e != NULL; e = n) {
	n = e->next;
	exfree(e->string, sizeof(char)*(strlen(e->string)+1));
	exfree(e, sizeof(Connector));
    }
}

Connector * excopy_connectors(Connector * c) {
    Connector *c1;

    if (c == NULL) return NULL;

    c1 = init_connector((Connector *) exalloc(sizeof(Connector)));
    *c1 = *c;
    c1->string = (char *) exalloc(sizeof(char)*(strlen(c->string)+1));
    strcpy(c1->string, c->string);
    c1->next = excopy_connectors(c->next);

    return c1;
}


Link excopy_link(Link l) {
     Link newl;

     if (l == NULL) return NULL;

     newl = (Link) exalloc(sizeof(struct Link_s));
     newl->name = (char *) exalloc(sizeof(char)*(strlen(l->name)+1));
     strcpy(newl->name, l->name);
     newl->l = l->l;
     newl->r = l->r;
     newl->lc = excopy_connectors(l->lc);
     newl->rc = excopy_connectors(l->rc);

     return newl;
}

void exfree_link(Link l) {
     exfree_connectors(l->rc);
     exfree_connectors(l->lc);
     exfree(l->name, sizeof(char)*(strlen(l->name)+1));
     exfree(l, sizeof(struct Link_s));
}


Disjunct * catenate_disjuncts(Disjunct *d1, Disjunct *d2) {
/* Destructively catenates the two disjunct lists d1 followed by d2. */
/* Doesn't change the contents of the disjuncts */
/* Traverses the first list, but not the second */
    Disjunct * dis = d1;

    if (d1 == NULL) return d2;
    if (d2 == NULL) return d1;
    while (dis->next != NULL) dis = dis->next;
    dis->next = d2;
    return d1;
}

X_node * catenate_X_nodes(X_node *d1, X_node *d2) {
/* Destructively catenates the two disjunct lists d1 followed by d2. */
/* Doesn't change the contents of the disjuncts */
/* Traverses the first list, but not the second */
    X_node * dis = d1;

    if (d1 == NULL) return d2;
    if (d2 == NULL) return d1;
    while (dis->next != NULL) dis = dis->next;
    dis->next = d2;
    return d1;
}

int next_power_of_two_up(int i) {
/* Returns the smallest power of two that is at least i and at least 1 */
    int j=1;
    while(j<i) j = j<<1;
    return j;
}

int upper_case_match(char *s, char *t) {
/* returns TRUE if the initial upper case letters of s and t match */
    while(isupper((int)*s) || isupper((int)*t)) {
	if (*s != *t) return FALSE;
	s++;
	t++;
    }
    return (!isupper((int)*s) && !isupper((int)*t));
}

void left_print_string(FILE * fp, char * s, char * t) {
/* prints s then prints the last |t|-|s| characters of t.
   if s is longer than t, it truncates s.
*/
    int i, j, k;
    j = strlen(t);
    k = strlen(s);
    for (i=0; i<j; i++) {
	if (i<k) {
	    fprintf(fp, "%c", s[i]);
	} else {
	    fprintf(fp, "%c", t[i]);
	}
    }
}

int sentence_contains(Sentence sent, char * s) {
/* Returns TRUE if one of the words in the sentence is s */
    int w;
    for (w=0; w<sent->length; w++) {
	if (strcmp(sent->word[w].string, s) == 0) return TRUE;
    }
    return FALSE;
}

void set_is_conjunction(Sentence sent) {
    int w;
    char * s;
    for (w=0; w<sent->length; w++) {
	s = sent->word[w].string;
	sent->is_conjunction[w] = ((strcmp(s, "and")==0) || (strcmp(s, "or" )==0) ||
				   (strcmp(s, "but")==0) || (strcmp(s, "nor")==0));
    }
}

int sentence_contains_conjunction(Sentence sent) {
/* Return true if the sentence contains a conjunction.  Assumes
   is_conjunction[] has been initialized.
*/
    int w;
    for (w=0; w<sent->length; w++) {
	if (sent->is_conjunction[w]) return TRUE;
    }
    return FALSE;
}

int conj_in_range(Sentence sent, int lw, int rw) {
/* Returns true if the range lw...rw inclusive contains a conjunction     */
    for (;lw <= rw; lw++) {
	if (sent->is_conjunction[lw]) return TRUE;
    }
    return FALSE;
}

/* borrowed from glib */
static char*
path_get_dirname (const char	   *file_name)
{
  register char *base;
  register int len;

  base = strrchr (file_name, DIR_SEPARATOR);
#ifdef _WIN32
  {
    char *q = strrchr (file_name, '/');
    if (base == NULL || (q != NULL && q > base))
	base = q;
  }
#endif
  if (!base)
    {
#ifdef _WIN32
      if (isalpha (file_name[0]) && file_name[1] == ':')
	{
	  char drive_colon_dot[4];

	  drive_colon_dot[0] = file_name[0];
	  drive_colon_dot[1] = ':';
	  drive_colon_dot[2] = '.';
	  drive_colon_dot[3] = '\0';

	  return strdup (drive_colon_dot);
	}
#endif
    return strdup (".");
    }

  while (base > file_name && IS_DIR_SEPARATOR (*base))
    base--;

#ifdef _WIN32
  /* base points to the char before the last slash.
   *
   * In case file_name is the root of a drive (X:\) or a child of the
   * root of a drive (X:\foo), include the slash.
   *
   * In case file_name is the root share of an UNC path
   * (\\server\share), add a slash, returning \\server\share\ .
   *
   * In case file_name is a direct child of a share in an UNC path
   * (\\server\share\foo), include the slash after the share name,
   * returning \\server\share\ .
   */
  if (base == file_name + 1 && isalpha (file_name[0]) && file_name[1] == ':')
    base++;
  else if (IS_DIR_SEPARATOR (file_name[0]) &&
	   IS_DIR_SEPARATOR (file_name[1]) &&
	   file_name[2] &&
	   !IS_DIR_SEPARATOR (file_name[2]) &&
	   base >= file_name + 2)
    {
      const char *p = file_name + 2;
      while (*p && !IS_DIR_SEPARATOR (*p))
	p++;
      if (p == base + 1)
	{
	  len = (int) strlen (file_name) + 1;
	  base = (char *)malloc(len + 1);
	  strcpy (base, file_name);
	  base[len-1] = DIR_SEPARATOR;
	  base[len] = 0;
	  return base;
	}
      if (IS_DIR_SEPARATOR (*p))
	{
	  p++;
	  while (*p && !IS_DIR_SEPARATOR (*p))
	    p++;
	  if (p == base + 1)
	    base++;
	}
    }
#endif

  len = (int) 1 + base - file_name;

  base = (char *)malloc(len + 1);
  memmove (base, file_name, len);
  base[len] = 0;

  return base;
}

static char * get_datadir(void)
{
  char * data_dir = NULL;

#ifdef ENABLE_BINRELOC
  data_dir = strdup (BR_DATADIR("/link-grammar"));
#elif defined(_WIN32)
  /* Dynamically locate library and return containing directory */
  HINSTANCE hInstance = GetModuleHandle(NULL);
  if(hInstance != NULL)
    {
      char dll_path[MAX_PATH];

      if(GetModuleFileName(hInstance,dll_path,MAX_PATH)) {
	char * prefix = path_get_dirname(dll_path);
	if(prefix) {
	  char * path = join_path(prefix, "share");
	  data_dir = join_path(path, "link-grammar");
	  free(path);
	  free(prefix);
	}
      }
    }
#endif

  return data_dir;
}

FILE *dictopen(char *dictname, char *filename, char *how) {

    /* This function is used to open a dictionary file or a word file,
       or any associated data file (like a post process knowledge file).

       It works as follows.  If the file name begins with a "/", then
       it's assumed to be an absolute file name and it tries to open
       that exact file.

       If the filename does not begin with a "/", then it uses the
       dictpath mechanism to find the right file to open.  This looks
       for the file in a sequence of directories until it finds it.  The
       sequence of directories is specified in a dictpath string, in
       which each directory is followed by a ":".

       The dictpath that it uses is constructed as follows.  If the
       dictname is non-null, and is an absolute path name (beginning
       with a "/", then the part after the last "/" is removed and this
       is the first directory on the dictpath.  After this comes the
       DICTPATH environment variable, followed by the DEFAULTPATH
    */

    char completename[MAX_PATH_NAME+1];
    char fulldictpath[MAX_PATH_NAME+1];
    char *pos, *oldpos;
    int filenamelen, len;
    FILE *fp;

    if (filename[0] == '/') {
	return fopen(filename, how);  /* If the file does not exist NULL is returned */
    }

    {
      char * data_dir = get_datadir();
      if(data_dir) {
	sprintf(fulldictpath, "%s%c%s%c", data_dir, PATH_SEPARATOR, DEFAULTPATH, PATH_SEPARATOR);
	free(data_dir);
      }
      else {
	strcpy(fulldictpath, DEFAULTPATH);
      }
    }

    /* now fulldictpath is our dictpath, where each entry is followed by a ":"
       including the last one */

    filenamelen = strlen(filename);
    len = strlen(fulldictpath)+ filenamelen + 1 + 1;
    oldpos = fulldictpath;
    while ((pos = strchr(oldpos, PATH_SEPARATOR)) != NULL) {
	strncpy(completename, oldpos, (pos-oldpos));
	*(completename+(pos-oldpos)) = DIR_SEPARATOR;
	strcpy(completename+(pos-oldpos)+1,filename);
	if ((fp = fopen(completename, how)) != NULL) {
	    return fp;
	}
	oldpos = pos+1;
    }
    return NULL;
}

static int random_state[2] = {0,0};
static int random_count = 0;
static int random_inited = FALSE;

int step_generator(int d) {
    /* no overflow should occur, so this is machine independent */
    random_state[0] = ((random_state[0] * 3) + d + 104729) % 179424673;
    random_state[1] = ((random_state[1] * 7) + d + 48611) % 86028121;
    return random_state[0] + random_state[1];;
}

void my_random_initialize(int seed) {
    assert(!random_inited, "Random number generator not finalized.");

    seed = (seed < 0) ? -seed : seed;
    seed = seed % (1 << 30);

    random_state[0] = seed % 3;
    random_state[1] = seed % 5;
    random_count = seed;
    random_inited = TRUE;
}

void my_random_finalize() {
    assert(random_inited, "Random number generator not initialized.");
    random_inited = FALSE;
}

int my_random(void) {
    random_count++;
    return (step_generator(random_count));
}


/* Here's the connector set data structure */

#if 0
/* this is defined in parser_api.h */
typedef struct {
    Connector ** hash_table;
    int          table_size;
    int          is_defined;  /* if 0 then there is no such set */
} Connector_set;
#endif

int connector_set_hash(Connector_set *conset, char * s, int d) {
/* This hash function only looks at the leading upper case letters of
   the string, and the direction, '+' or '-'.
*/
    int i;
    for(i=d; isupper((int)*s); s++) i = i + (i<<1) + randtable[(*s + i) & (RTSIZE-1)];
    return (i & (conset->table_size-1));
}

void build_connector_set_from_expression(Connector_set * conset, Exp * e) {
    E_list * l;
    Connector * c;
    int h;
    if (e->type == CONNECTOR_type) {
	c = init_connector((Connector *) xalloc(sizeof(Connector)));
	c->string = e->u.string;
	c->label = NORMAL_LABEL;        /* so we can use match() */
	c->priority = THIN_priority;
	c->word = e->dir;     /* just use the word field to give the dir */
	h = connector_set_hash(conset, c->string, c->word);
	c->next = conset->hash_table[h];
	conset->hash_table[h] = c;
    } else {
	for (l=e->u.l; l!=NULL; l=l->next) {
	    build_connector_set_from_expression(conset, l->e);
	}
    }
}

Connector_set * connector_set_create(Exp *e) {
    int i;
    Connector_set *conset;

    conset = (Connector_set *) xalloc(sizeof(Connector_set));
    conset->table_size = next_power_of_two_up(size_of_expression(e));
    conset->hash_table =
      (Connector **) xalloc(conset->table_size * sizeof(Connector *));
    for (i=0; i<conset->table_size; i++) conset->hash_table[i] = NULL;
    build_connector_set_from_expression(conset, e);
    return conset;
}

void connector_set_delete(Connector_set * conset) {
    int i;
    if (conset == NULL) return;
    for (i=0; i<conset->table_size; i++) free_connectors(conset->hash_table[i]);
    xfree(conset->hash_table, conset->table_size * sizeof(Connector *));
    xfree(conset, sizeof(Connector_set));
}

int match_in_connector_set(Connector_set *conset, Connector * c, int d) {
/* Returns TRUE the given connector is in this conset.  FALSE otherwise.
   d='+' means this connector is on the right side of the disjunct.
   d='-' means this connector is on the left side of the disjunct.
*/
    int h;
    Connector * c1;
    if (conset == NULL) return FALSE;
    h = connector_set_hash(conset, c->string, d);
    for (c1=conset->hash_table[h]; c1!=NULL; c1 = c1->next) {
	if (x_match(c1, c) && (d == c1->word)) return TRUE;
    }
    return FALSE;
}

Dict_node * list_whole_dictionary(Dict_node *root, Dict_node *dn) {
    Dict_node *c, *d;
    if (root == NULL) return dn;
    c = (Dict_node *) xalloc(sizeof(Dict_node));
    *c = *root;
    d = list_whole_dictionary(root->left, dn);
    c->right = list_whole_dictionary(root->right, d);
    return c;
}

void free_listed_dictionary(Dict_node *dn) {
    Dict_node * dn2;
    while(dn!=NULL) {
      dn2=dn->right;
      xfree(dn, sizeof(Dict_node));
      dn=dn2;
    }
}

int easy_match(char * s, char * t) {

    /* This is like the basic "match" function in count.c - the basic connector-matching
       function used in parsing - except it ignores "priority" (used to handle fat links) */

    while(isupper((int)*s) || isupper((int)*t)) {
	if (*s != *t) return FALSE;
	s++;
	t++;
    }

    while ((*s!='\0') && (*t!='\0')) {
      if ((*s == '*') || (*t == '*') ||
	  ((*s == *t) && (*s != '^'))) {
	s++;
	t++;
      } else return FALSE;
    }
    return TRUE;

}

int word_has_connector(Dict_node * dn, char * cs, int direction) {

  /* This function takes a dict_node (corresponding to an entry in a given dictionary), a
     string (representing a connector), and a direction (0 = right-pointing, 1 = left-pointing);
     it returns 1 if the dictionary expression for the word includes the connector, 0 otherwise.
     This can be used to see if a word is in a certain category (checking for a category
     connector in a table), or to see if a word has a connector in a normal dictionary. The
     connector check uses a "smart-match", the same kind used by the parser. */

    Connector * c2=NULL;
    Disjunct * d, *d0;
    if(dn == NULL) return -1;
    d0 = d = build_disjuncts_for_dict_node(dn);
    if(d == NULL) return 0;
    for(; d!=NULL; d=d->next) {
      if(direction==0) c2 = d->right;
      if(direction==1) c2 = d->left;
      for(; c2!=NULL; c2=c2->next) {
	if(easy_match(c2->string, cs)==1) {
	    free_disjuncts(d0);
	    return 1;
	}
      }
    }
    free_disjuncts(d0);
    return 0;
}

#ifdef _WIN32

/**
 * win32_getlocale:
 *
 * The setlocale() function in the Microsoft C library uses locale
 * names of the form "English_United States.1252" etc. We want the
 * UNIXish standard form "en_US", "zh_TW" etc. This function gets the
 * current thread locale from Windows - without any encoding info -
 * and returns it as a string of the above form for use in forming
 * file names etc. The returned string should be deallocated with
 * free().
 *
 * Returns: newly-allocated locale name.
 **/

/* Borrowed from GNU gettext 0.13.1: */
/* Mingw headers don't have latest language and sublanguage codes.  */
#ifndef LANG_AFRIKAANS
#define LANG_AFRIKAANS 0x36
#endif
#ifndef LANG_ALBANIAN
#define LANG_ALBANIAN 0x1c
#endif
#ifndef LANG_AMHARIC
#define LANG_AMHARIC 0x5e
#endif
#ifndef LANG_ARABIC
#define LANG_ARABIC 0x01
#endif
#ifndef LANG_ARMENIAN
#define LANG_ARMENIAN 0x2b
#endif
#ifndef LANG_ASSAMESE
#define LANG_ASSAMESE 0x4d
#endif
#ifndef LANG_AZERI
#define LANG_AZERI 0x2c
#endif
#ifndef LANG_BASQUE
#define LANG_BASQUE 0x2d
#endif
#ifndef LANG_BELARUSIAN
#define LANG_BELARUSIAN 0x23
#endif
#ifndef LANG_BENGALI
#define LANG_BENGALI 0x45
#endif
#ifndef LANG_BURMESE
#define LANG_BURMESE 0x55
#endif
#ifndef LANG_CAMBODIAN
#define LANG_CAMBODIAN 0x53
#endif
#ifndef LANG_CATALAN
#define LANG_CATALAN 0x03
#endif
#ifndef LANG_CHEROKEE
#define LANG_CHEROKEE 0x5c
#endif
#ifndef LANG_DIVEHI
#define LANG_DIVEHI 0x65
#endif
#ifndef LANG_EDO
#define LANG_EDO 0x66
#endif
#ifndef LANG_ESTONIAN
#define LANG_ESTONIAN 0x25
#endif
#ifndef LANG_FAEROESE
#define LANG_FAEROESE 0x38
#endif
#ifndef LANG_FARSI
#define LANG_FARSI 0x29
#endif
#ifndef LANG_FRISIAN
#define LANG_FRISIAN 0x62
#endif
#ifndef LANG_FULFULDE
#define LANG_FULFULDE 0x67
#endif
#ifndef LANG_GAELIC
#define LANG_GAELIC 0x3c
#endif
#ifndef LANG_GALICIAN
#define LANG_GALICIAN 0x56
#endif
#ifndef LANG_GEORGIAN
#define LANG_GEORGIAN 0x37
#endif
#ifndef LANG_GUARANI
#define LANG_GUARANI 0x74
#endif
#ifndef LANG_GUJARATI
#define LANG_GUJARATI 0x47
#endif
#ifndef LANG_HAUSA
#define LANG_HAUSA 0x68
#endif
#ifndef LANG_HAWAIIAN
#define LANG_HAWAIIAN 0x75
#endif
#ifndef LANG_HEBREW
#define LANG_HEBREW 0x0d
#endif
#ifndef LANG_HINDI
#define LANG_HINDI 0x39
#endif
#ifndef LANG_IBIBIO
#define LANG_IBIBIO 0x69
#endif
#ifndef LANG_IGBO
#define LANG_IGBO 0x70
#endif
#ifndef LANG_INDONESIAN
#define LANG_INDONESIAN 0x21
#endif
#ifndef LANG_INUKTITUT
#define LANG_INUKTITUT 0x5d
#endif
#ifndef LANG_KANNADA
#define LANG_KANNADA 0x4b
#endif
#ifndef LANG_KANURI
#define LANG_KANURI 0x71
#endif
#ifndef LANG_KASHMIRI
#define LANG_KASHMIRI 0x60
#endif
#ifndef LANG_KAZAK
#define LANG_KAZAK 0x3f
#endif
#ifndef LANG_KONKANI
#define LANG_KONKANI 0x57
#endif
#ifndef LANG_KYRGYZ
#define LANG_KYRGYZ 0x40
#endif
#ifndef LANG_LAO
#define LANG_LAO 0x54
#endif
#ifndef LANG_LATIN
#define LANG_LATIN 0x76
#endif
#ifndef LANG_LATVIAN
#define LANG_LATVIAN 0x26
#endif
#ifndef LANG_LITHUANIAN
#define LANG_LITHUANIAN 0x27
#endif
#ifndef LANG_MACEDONIAN
#define LANG_MACEDONIAN 0x2f
#endif
#ifndef LANG_MALAY
#define LANG_MALAY 0x3e
#endif
#ifndef LANG_MALAYALAM
#define LANG_MALAYALAM 0x4c
#endif
#ifndef LANG_MALTESE
#define LANG_MALTESE 0x3a
#endif
#ifndef LANG_MANIPURI
#define LANG_MANIPURI 0x58
#endif
#ifndef LANG_MARATHI
#define LANG_MARATHI 0x4e
#endif
#ifndef LANG_MONGOLIAN
#define LANG_MONGOLIAN 0x50
#endif
#ifndef LANG_NEPALI
#define LANG_NEPALI 0x61
#endif
#ifndef LANG_ORIYA
#define LANG_ORIYA 0x48
#endif
#ifndef LANG_OROMO
#define LANG_OROMO 0x72
#endif
#ifndef LANG_PAPIAMENTU
#define LANG_PAPIAMENTU 0x79
#endif
#ifndef LANG_PASHTO
#define LANG_PASHTO 0x63
#endif
#ifndef LANG_PUNJABI
#define LANG_PUNJABI 0x46
#endif
#ifndef LANG_RHAETO_ROMANCE
#define LANG_RHAETO_ROMANCE 0x17
#endif
#ifndef LANG_SAAMI
#define LANG_SAAMI 0x3b
#endif
#ifndef LANG_SANSKRIT
#define LANG_SANSKRIT 0x4f
#endif
#ifndef LANG_SERBIAN
#define LANG_SERBIAN 0x1a
#endif
#ifndef LANG_SINDHI
#define LANG_SINDHI 0x59
#endif
#ifndef LANG_SINHALESE
#define LANG_SINHALESE 0x5b
#endif
#ifndef LANG_SLOVAK
#define LANG_SLOVAK 0x1b
#endif
#ifndef LANG_SOMALI
#define LANG_SOMALI 0x77
#endif
#ifndef LANG_SORBIAN
#define LANG_SORBIAN 0x2e
#endif
#ifndef LANG_SUTU
#define LANG_SUTU 0x30
#endif
#ifndef LANG_SWAHILI
#define LANG_SWAHILI 0x41
#endif
#ifndef LANG_SYRIAC
#define LANG_SYRIAC 0x5a
#endif
#ifndef LANG_TAGALOG
#define LANG_TAGALOG 0x64
#endif
#ifndef LANG_TAJIK
#define LANG_TAJIK 0x28
#endif
#ifndef LANG_TAMAZIGHT
#define LANG_TAMAZIGHT 0x5f
#endif
#ifndef LANG_TAMIL
#define LANG_TAMIL 0x49
#endif
#ifndef LANG_TATAR
#define LANG_TATAR 0x44
#endif
#ifndef LANG_TELUGU
#define LANG_TELUGU 0x4a
#endif
#ifndef LANG_THAI
#define LANG_THAI 0x1e
#endif
#ifndef LANG_TIBETAN
#define LANG_TIBETAN 0x51
#endif
#ifndef LANG_TIGRINYA
#define LANG_TIGRINYA 0x73
#endif
#ifndef LANG_TSONGA
#define LANG_TSONGA 0x31
#endif
#ifndef LANG_TSWANA
#define LANG_TSWANA 0x32
#endif
#ifndef LANG_TURKMEN
#define LANG_TURKMEN 0x42
#endif
#ifndef LANG_UKRAINIAN
#define LANG_UKRAINIAN 0x22
#endif
#ifndef LANG_URDU
#define LANG_URDU 0x20
#endif
#ifndef LANG_UZBEK
#define LANG_UZBEK 0x43
#endif
#ifndef LANG_VENDA
#define LANG_VENDA 0x33
#endif
#ifndef LANG_VIETNAMESE
#define LANG_VIETNAMESE 0x2a
#endif
#ifndef LANG_WELSH
#define LANG_WELSH 0x52
#endif
#ifndef LANG_XHOSA
#define LANG_XHOSA 0x34
#endif
#ifndef LANG_YI
#define LANG_YI 0x78
#endif
#ifndef LANG_YIDDISH
#define LANG_YIDDISH 0x3d
#endif
#ifndef LANG_YORUBA
#define LANG_YORUBA 0x6a
#endif
#ifndef LANG_ZULU
#define LANG_ZULU 0x35
#endif
#ifndef SUBLANG_ARABIC_SAUDI_ARABIA
#define SUBLANG_ARABIC_SAUDI_ARABIA 0x01
#endif
#ifndef SUBLANG_ARABIC_IRAQ
#define SUBLANG_ARABIC_IRAQ 0x02
#endif
#ifndef SUBLANG_ARABIC_EGYPT
#define SUBLANG_ARABIC_EGYPT 0x03
#endif
#ifndef SUBLANG_ARABIC_LIBYA
#define SUBLANG_ARABIC_LIBYA 0x04
#endif
#ifndef SUBLANG_ARABIC_ALGERIA
#define SUBLANG_ARABIC_ALGERIA 0x05
#endif
#ifndef SUBLANG_ARABIC_MOROCCO
#define SUBLANG_ARABIC_MOROCCO 0x06
#endif
#ifndef SUBLANG_ARABIC_TUNISIA
#define SUBLANG_ARABIC_TUNISIA 0x07
#endif
#ifndef SUBLANG_ARABIC_OMAN
#define SUBLANG_ARABIC_OMAN 0x08
#endif
#ifndef SUBLANG_ARABIC_YEMEN
#define SUBLANG_ARABIC_YEMEN 0x09
#endif
#ifndef SUBLANG_ARABIC_SYRIA
#define SUBLANG_ARABIC_SYRIA 0x0a
#endif
#ifndef SUBLANG_ARABIC_JORDAN
#define SUBLANG_ARABIC_JORDAN 0x0b
#endif
#ifndef SUBLANG_ARABIC_LEBANON
#define SUBLANG_ARABIC_LEBANON 0x0c
#endif
#ifndef SUBLANG_ARABIC_KUWAIT
#define SUBLANG_ARABIC_KUWAIT 0x0d
#endif
#ifndef SUBLANG_ARABIC_UAE
#define SUBLANG_ARABIC_UAE 0x0e
#endif
#ifndef SUBLANG_ARABIC_BAHRAIN
#define SUBLANG_ARABIC_BAHRAIN 0x0f
#endif
#ifndef SUBLANG_ARABIC_QATAR
#define SUBLANG_ARABIC_QATAR 0x10
#endif
#ifndef SUBLANG_AZERI_LATIN
#define SUBLANG_AZERI_LATIN 0x01
#endif
#ifndef SUBLANG_AZERI_CYRILLIC
#define SUBLANG_AZERI_CYRILLIC 0x02
#endif
#ifndef SUBLANG_BENGALI_INDIA
#define SUBLANG_BENGALI_INDIA 0x00
#endif
#ifndef SUBLANG_BENGALI_BANGLADESH
#define SUBLANG_BENGALI_BANGLADESH 0x01
#endif
#ifndef SUBLANG_CHINESE_MACAU
#define SUBLANG_CHINESE_MACAU 0x05
#endif
#ifndef SUBLANG_ENGLISH_SOUTH_AFRICA
#define SUBLANG_ENGLISH_SOUTH_AFRICA 0x07
#endif
#ifndef SUBLANG_ENGLISH_JAMAICA
#define SUBLANG_ENGLISH_JAMAICA 0x08
#endif
#ifndef SUBLANG_ENGLISH_CARIBBEAN
#define SUBLANG_ENGLISH_CARIBBEAN 0x09
#endif
#ifndef SUBLANG_ENGLISH_BELIZE
#define SUBLANG_ENGLISH_BELIZE 0x0a
#endif
#ifndef SUBLANG_ENGLISH_TRINIDAD
#define SUBLANG_ENGLISH_TRINIDAD 0x0b
#endif
#ifndef SUBLANG_ENGLISH_ZIMBABWE
#define SUBLANG_ENGLISH_ZIMBABWE 0x0c
#endif
#ifndef SUBLANG_ENGLISH_PHILIPPINES
#define SUBLANG_ENGLISH_PHILIPPINES 0x0d
#endif
#ifndef SUBLANG_ENGLISH_INDONESIA
#define SUBLANG_ENGLISH_INDONESIA 0x0e
#endif
#ifndef SUBLANG_ENGLISH_HONGKONG
#define SUBLANG_ENGLISH_HONGKONG 0x0f
#endif
#ifndef SUBLANG_ENGLISH_INDIA
#define SUBLANG_ENGLISH_INDIA 0x10
#endif
#ifndef SUBLANG_ENGLISH_MALAYSIA
#define SUBLANG_ENGLISH_MALAYSIA 0x11
#endif
#ifndef SUBLANG_ENGLISH_SINGAPORE
#define SUBLANG_ENGLISH_SINGAPORE 0x12
#endif
#ifndef SUBLANG_FRENCH_LUXEMBOURG
#define SUBLANG_FRENCH_LUXEMBOURG 0x05
#endif
#ifndef SUBLANG_FRENCH_MONACO
#define SUBLANG_FRENCH_MONACO 0x06
#endif
#ifndef SUBLANG_FRENCH_WESTINDIES
#define SUBLANG_FRENCH_WESTINDIES 0x07
#endif
#ifndef SUBLANG_FRENCH_REUNION
#define SUBLANG_FRENCH_REUNION 0x08
#endif
#ifndef SUBLANG_FRENCH_CONGO
#define SUBLANG_FRENCH_CONGO 0x09
#endif
#ifndef SUBLANG_FRENCH_SENEGAL
#define SUBLANG_FRENCH_SENEGAL 0x0a
#endif
#ifndef SUBLANG_FRENCH_CAMEROON
#define SUBLANG_FRENCH_CAMEROON 0x0b
#endif
#ifndef SUBLANG_FRENCH_COTEDIVOIRE
#define SUBLANG_FRENCH_COTEDIVOIRE 0x0c
#endif
#ifndef SUBLANG_FRENCH_MALI
#define SUBLANG_FRENCH_MALI 0x0d
#endif
#ifndef SUBLANG_FRENCH_MOROCCO
#define SUBLANG_FRENCH_MOROCCO 0x0e
#endif
#ifndef SUBLANG_FRENCH_HAITI
#define SUBLANG_FRENCH_HAITI 0x0f
#endif
#ifndef SUBLANG_GERMAN_LUXEMBOURG
#define SUBLANG_GERMAN_LUXEMBOURG 0x04
#endif
#ifndef SUBLANG_GERMAN_LIECHTENSTEIN
#define SUBLANG_GERMAN_LIECHTENSTEIN 0x05
#endif
#ifndef SUBLANG_KASHMIRI_INDIA
#define SUBLANG_KASHMIRI_INDIA 0x02
#endif
#ifndef SUBLANG_MALAY_MALAYSIA
#define SUBLANG_MALAY_MALAYSIA 0x01
#endif
#ifndef SUBLANG_MALAY_BRUNEI_DARUSSALAM
#define SUBLANG_MALAY_BRUNEI_DARUSSALAM 0x02
#endif
#ifndef SUBLANG_NEPALI_INDIA
#define SUBLANG_NEPALI_INDIA 0x02
#endif
#ifndef SUBLANG_PUNJABI_INDIA
#define SUBLANG_PUNJABI_INDIA 0x00
#endif
#ifndef SUBLANG_PUNJABI_PAKISTAN
#define SUBLANG_PUNJABI_PAKISTAN 0x01
#endif
#ifndef SUBLANG_ROMANIAN_ROMANIA
#define SUBLANG_ROMANIAN_ROMANIA 0x00
#endif
#ifndef SUBLANG_ROMANIAN_MOLDOVA
#define SUBLANG_ROMANIAN_MOLDOVA 0x01
#endif
#ifndef SUBLANG_SERBIAN_LATIN
#define SUBLANG_SERBIAN_LATIN 0x02
#endif
#ifndef SUBLANG_SERBIAN_CYRILLIC
#define SUBLANG_SERBIAN_CYRILLIC 0x03
#endif
#ifndef SUBLANG_SINDHI_INDIA
#define SUBLANG_SINDHI_INDIA 0x00
#endif
#ifndef SUBLANG_SINDHI_PAKISTAN
#define SUBLANG_SINDHI_PAKISTAN 0x01
#endif
#ifndef SUBLANG_SPANISH_GUATEMALA
#define SUBLANG_SPANISH_GUATEMALA 0x04
#endif
#ifndef SUBLANG_SPANISH_COSTA_RICA
#define SUBLANG_SPANISH_COSTA_RICA 0x05
#endif
#ifndef SUBLANG_SPANISH_PANAMA
#define SUBLANG_SPANISH_PANAMA 0x06
#endif
#ifndef SUBLANG_SPANISH_DOMINICAN_REPUBLIC
#define SUBLANG_SPANISH_DOMINICAN_REPUBLIC 0x07
#endif
#ifndef SUBLANG_SPANISH_VENEZUELA
#define SUBLANG_SPANISH_VENEZUELA 0x08
#endif
#ifndef SUBLANG_SPANISH_COLOMBIA
#define SUBLANG_SPANISH_COLOMBIA 0x09
#endif
#ifndef SUBLANG_SPANISH_PERU
#define SUBLANG_SPANISH_PERU 0x0a
#endif
#ifndef SUBLANG_SPANISH_ARGENTINA
#define SUBLANG_SPANISH_ARGENTINA 0x0b
#endif
#ifndef SUBLANG_SPANISH_ECUADOR
#define SUBLANG_SPANISH_ECUADOR 0x0c
#endif
#ifndef SUBLANG_SPANISH_CHILE
#define SUBLANG_SPANISH_CHILE 0x0d
#endif
#ifndef SUBLANG_SPANISH_URUGUAY
#define SUBLANG_SPANISH_URUGUAY 0x0e
#endif
#ifndef SUBLANG_SPANISH_PARAGUAY
#define SUBLANG_SPANISH_PARAGUAY 0x0f
#endif
#ifndef SUBLANG_SPANISH_BOLIVIA
#define SUBLANG_SPANISH_BOLIVIA 0x10
#endif
#ifndef SUBLANG_SPANISH_EL_SALVADOR
#define SUBLANG_SPANISH_EL_SALVADOR 0x11
#endif
#ifndef SUBLANG_SPANISH_HONDURAS
#define SUBLANG_SPANISH_HONDURAS 0x12
#endif
#ifndef SUBLANG_SPANISH_NICARAGUA
#define SUBLANG_SPANISH_NICARAGUA 0x13
#endif
#ifndef SUBLANG_SPANISH_PUERTO_RICO
#define SUBLANG_SPANISH_PUERTO_RICO 0x14
#endif
#ifndef SUBLANG_SWEDISH_FINLAND
#define SUBLANG_SWEDISH_FINLAND 0x02
#endif
#ifndef SUBLANG_TAMAZIGHT_ARABIC
#define SUBLANG_TAMAZIGHT_ARABIC 0x01
#endif
#ifndef SUBLANG_TAMAZIGHT_LATIN
#define SUBLANG_TAMAZIGHT_LATIN 0x02
#endif
#ifndef SUBLANG_TIGRINYA_ETHIOPIA
#define SUBLANG_TIGRINYA_ETHIOPIA 0x00
#endif
#ifndef SUBLANG_TIGRINYA_ERITREA
#define SUBLANG_TIGRINYA_ERITREA 0x01
#endif
#ifndef SUBLANG_URDU_PAKISTAN
#define SUBLANG_URDU_PAKISTAN 0x01
#endif
#ifndef SUBLANG_URDU_INDIA
#define SUBLANG_URDU_INDIA 0x02
#endif
#ifndef SUBLANG_UZBEK_LATIN
#define SUBLANG_UZBEK_LATIN 0x01
#endif
#ifndef SUBLANG_UZBEK_CYRILLIC
#define SUBLANG_UZBEK_CYRILLIC 0x02
#endif

static char *
win32_getlocale (void)
{
  LCID lcid;
  LANGID langid;
  char *ev;
  int primary, sub;
  char *l = "C", *sl = NULL;
  char bfr[20];

  /* Let the user override the system settings through environment
     variables, as on POSIX systems.  */
  if (((ev = getenv ("LC_ALL")) != NULL && ev[0] != '\0')
      || ((ev = getenv ("LC_MESSAGES")) != NULL && ev[0] != '\0')
      || ((ev = getenv ("LANG")) != NULL && ev[0] != '\0'))
    return strdup (ev);

  /* Use native Win32 API locale ID.  */
  lcid = GetThreadLocale ();

  /* Strip off the sorting rules, keep only the language part.  */
  langid = LANGIDFROMLCID (lcid);

  /* Split into language and territory part.  */
  primary = PRIMARYLANGID (langid);
  sub = SUBLANGID (langid);
  switch (primary)
    {
    case LANG_AFRIKAANS: l = "af"; sl = "ZA"; break;
    case LANG_ALBANIAN: l = "sq"; sl = "AL"; break;
    case LANG_ARABIC:
      l = "ar";
      switch (sub)
	{
	case SUBLANG_ARABIC_SAUDI_ARABIA: sl = "SA"; break;
	case SUBLANG_ARABIC_IRAQ: sl = "IQ"; break;
	case SUBLANG_ARABIC_EGYPT: sl = "EG"; break;
	case SUBLANG_ARABIC_LIBYA: sl = "LY"; break;
	case SUBLANG_ARABIC_ALGERIA: sl = "DZ"; break;
	case SUBLANG_ARABIC_MOROCCO: sl = "MA"; break;
	case SUBLANG_ARABIC_TUNISIA: sl = "TN"; break;
	case SUBLANG_ARABIC_OMAN: sl = "OM"; break;
	case SUBLANG_ARABIC_YEMEN: sl = "YE"; break;
	case SUBLANG_ARABIC_SYRIA: sl = "SY"; break;
	case SUBLANG_ARABIC_JORDAN: sl = "JO"; break;
	case SUBLANG_ARABIC_LEBANON: sl = "LB"; break;
	case SUBLANG_ARABIC_KUWAIT: sl = "KW"; break;
	case SUBLANG_ARABIC_UAE: sl = "AE"; break;
	case SUBLANG_ARABIC_BAHRAIN: sl = "BH"; break;
	case SUBLANG_ARABIC_QATAR: sl = "QA"; break;
	}
      break;
    case LANG_ARMENIAN: l = "hy"; sl = "AM"; break;
    case LANG_ASSAMESE: l = "as"; sl = "IN"; break;
    case LANG_AZERI:
      l = "az";
      switch (sub)
	{
	/* FIXME: Adjust this when Azerbaijani locales appear on Unix.  */
	case SUBLANG_AZERI_LATIN: sl = "AZ@latin"; break;
	case SUBLANG_AZERI_CYRILLIC: sl = "AZ@cyrillic"; break;
	}
      break;
    case LANG_BASQUE:
      l = "eu"; /* sl could be "ES" or "FR".  */
      break;
    case LANG_BELARUSIAN: l = "be"; sl = "BY"; break;
    case LANG_BENGALI:
      l = "bn";
      switch (sub)
	{
	case SUBLANG_BENGALI_INDIA: sl = "IN"; break;
	case SUBLANG_BENGALI_BANGLADESH: sl = "BD"; break;
	}
      break;
    case LANG_BULGARIAN: l = "bg"; sl = "BG"; break;
    case LANG_BURMESE: l = "my"; sl = "MM"; break;
    case LANG_CAMBODIAN: l = "km"; sl = "KH"; break;
    case LANG_CATALAN: l = "ca"; sl = "ES"; break;
    case LANG_CHINESE:
      l = "zh";
      switch (sub)
	{
	case SUBLANG_CHINESE_TRADITIONAL: sl = "TW"; break;
	case SUBLANG_CHINESE_SIMPLIFIED: sl = "CN"; break;
	case SUBLANG_CHINESE_HONGKONG: sl = "HK"; break;
	case SUBLANG_CHINESE_SINGAPORE: sl = "SG"; break;
	case SUBLANG_CHINESE_MACAU: sl = "MO"; break;
	}
      break;
    case LANG_CROATIAN:		/* LANG_CROATIAN == LANG_SERBIAN */
      switch (sub)
	{
	/* FIXME: How to distinguish Croatian and Latin Serbian locales?  */
	case SUBLANG_SERBIAN_LATIN: l = "sr"; sl = "@Latn"; break;
	case SUBLANG_SERBIAN_CYRILLIC: l = "sr"; break;
	default: l = "hr"; sl = "HR";
	}
      break;
    case LANG_CZECH: l = "cs"; sl = "CZ"; break;
    case LANG_DANISH: l = "da"; sl = "DK"; break;
    case LANG_DIVEHI: l = "div"; sl = "MV"; break;
    case LANG_DUTCH:
      l = "nl";
      switch (sub)
	{
	case SUBLANG_DUTCH: sl = "NL"; break;
	case SUBLANG_DUTCH_BELGIAN: sl = "BE"; break;
	}
      break;
    case LANG_ENGLISH:
      l = "en";
      switch (sub)
	{
	case SUBLANG_ENGLISH_US: sl = "US"; break;
	case SUBLANG_ENGLISH_UK: sl = "GB"; break;
	case SUBLANG_ENGLISH_AUS: sl = "AU"; break;
	case SUBLANG_ENGLISH_CAN: sl = "CA"; break;
	case SUBLANG_ENGLISH_NZ: sl = "NZ"; break;
	case SUBLANG_ENGLISH_EIRE: sl = "IE"; break;
	case SUBLANG_ENGLISH_SOUTH_AFRICA: sl = "ZA"; break;
	case SUBLANG_ENGLISH_JAMAICA: sl = "JM"; break;
	case SUBLANG_ENGLISH_CARIBBEAN: sl = "GD"; break; /* Grenada? */
	case SUBLANG_ENGLISH_BELIZE: sl = "BZ"; break;
	case SUBLANG_ENGLISH_TRINIDAD: sl = "TT"; break;
	case SUBLANG_ENGLISH_ZIMBABWE: sl = "ZW"; break;
	case SUBLANG_ENGLISH_PHILIPPINES: sl = "PH"; break;
	case SUBLANG_ENGLISH_INDONESIA: sl = "ID"; break;
	case SUBLANG_ENGLISH_HONGKONG: sl = "HK"; break;
	case SUBLANG_ENGLISH_INDIA: sl = "IN"; break;
	case SUBLANG_ENGLISH_MALAYSIA: sl = "MY"; break;
	case SUBLANG_ENGLISH_SINGAPORE: sl = "SG"; break;
	}
      break;
    case LANG_ESTONIAN: l = "et"; sl = "EE"; break;
    case LANG_FAEROESE: l = "fo"; sl = "FO"; break;
    case LANG_FARSI: l = "fa"; sl = "IR"; break;
    case LANG_FINNISH: l = "fi"; sl = "FI"; break;
    case LANG_FRENCH:
      l = "fr";
      switch (sub)
	{
	case SUBLANG_FRENCH: sl = "FR"; break;
	case SUBLANG_FRENCH_BELGIAN: sl = "BE"; break;
	case SUBLANG_FRENCH_CANADIAN: sl = "CA"; break;
	case SUBLANG_FRENCH_SWISS: sl = "CH"; break;
	case SUBLANG_FRENCH_LUXEMBOURG: sl = "LU"; break;
	case SUBLANG_FRENCH_MONACO: sl = "MC"; break;
	case SUBLANG_FRENCH_WESTINDIES: break;
	case SUBLANG_FRENCH_REUNION: sl = "RE"; break;
	case SUBLANG_FRENCH_CONGO: sl = "CG"; break;
	case SUBLANG_FRENCH_SENEGAL: sl = "SN"; break;
	case SUBLANG_FRENCH_CAMEROON: sl = "CM"; break;
	case SUBLANG_FRENCH_COTEDIVOIRE: sl = "CI"; break;
	case SUBLANG_FRENCH_MALI: sl = "ML"; break;
	case SUBLANG_FRENCH_MOROCCO: sl = "MA"; break;
	case SUBLANG_FRENCH_HAITI: sl = "HT"; break;
	}
      break;
    case LANG_FRISIAN: l = "fy"; sl ="NL"; break;
    case LANG_FULFULDE: l = "ful"; sl = "NG"; break;
    case LANG_GAELIC:
      switch (sub)
	{
	case 0x01: /* SCOTTISH */ l = "gd"; sl = "GB"; break;
	case 0x02: /* IRISH */ l = "ga"; sl = "IE"; break;
	}
      break;
    case LANG_GALICIAN: l = "gl"; sl = "ES"; break;
    case LANG_GEORGIAN: l = "ka"; sl = "GE"; break;
    case LANG_GERMAN:
      l = "de";
      switch (sub)
	{
	case SUBLANG_GERMAN: sl = "DE"; break;
	case SUBLANG_GERMAN_SWISS: sl = "CH"; break;
	case SUBLANG_GERMAN_AUSTRIAN: sl = "AT"; break;
	case SUBLANG_GERMAN_LUXEMBOURG: sl = "LU"; break;
	case SUBLANG_GERMAN_LIECHTENSTEIN: sl = "LI"; break;
	}
      break;
    case LANG_GREEK: l = "el"; sl = "GR"; break;
    case LANG_GUARANI: l = "gn"; sl = "PY"; break;
    case LANG_GUJARATI: l = "gu"; sl = "IN"; break;
    case LANG_HAUSA: l = "ha"; sl = "NG"; break;
    case LANG_HAWAIIAN:
      /* FIXME: Do they mean Hawaiian ("haw_US", 1000 speakers)
       * or Hawaii Creole English ("cpe_US", 600000 speakers)?
       */
      l = "cpe";
      sl = "US";
      break;
    case LANG_HEBREW: l = "he"; sl = "IL"; break;
    case LANG_HINDI: l = "hi"; sl = "IN"; break;
    case LANG_HUNGARIAN: l = "hu"; sl = "HU"; break;
    case LANG_IBIBIO: l = "nic"; sl = "NG"; break;
    case LANG_ICELANDIC: l = "is"; sl = "IS"; break;
    case LANG_IGBO: l = "ibo"; sl = "NG"; break;
    case LANG_INDONESIAN: l = "id"; sl = "ID"; break;
    case LANG_INUKTITUT: l = "iu"; sl = "CA"; break;
    case LANG_ITALIAN:
      l = "it";
      switch (sub)
	{
	case SUBLANG_ITALIAN: sl = "IT"; break;
	case SUBLANG_ITALIAN_SWISS: sl = "CH"; break;
	}
      break;
    case LANG_JAPANESE: l = "ja"; sl = "JP"; break;
    case LANG_KANNADA: l = "kn"; sl = "IN"; break;
    case LANG_KANURI: l = "kau"; sl = "NG"; break;
    case LANG_KASHMIRI:
      l = "ks";
      switch (sub)
	{
	case SUBLANG_DEFAULT: sl = "PK"; break;
	case SUBLANG_KASHMIRI_INDIA: sl = "IN"; break;
	}
      break;
    case LANG_KAZAK: l = "kk"; sl = "KZ"; break;
    case LANG_KONKANI:
      /* FIXME: Adjust this when such locales appear on Unix.  */
      l = "kok";
      sl = "IN";
      break;
    case LANG_KOREAN: l = "ko"; sl = "KR"; break;
    case LANG_KYRGYZ: l = "ky"; sl = "KG"; break;
    case LANG_LAO: l = "lo"; sl = "LA"; break;
    case LANG_LATIN: l = "la"; sl = "VA"; break;
    case LANG_LATVIAN: l = "lv"; sl = "LV"; break;
    case LANG_LITHUANIAN: l = "lt"; sl = "LT"; break;
    case LANG_MACEDONIAN: l = "mk"; sl = "MK"; break;
    case LANG_MALAY:
      l = "ms";
      switch (sub)
	{
	case SUBLANG_MALAY_MALAYSIA: sl = "MY"; break;
	case SUBLANG_MALAY_BRUNEI_DARUSSALAM: sl = "BN"; break;
	}
      break;
    case LANG_MALAYALAM: l = "ml"; sl = "IN"; break;
    case LANG_MANIPURI:
      /* FIXME: Adjust this when such locales appear on Unix.  */
      l = "mni";
      sl = "IN";
      break;
    case LANG_MARATHI: l = "mr"; sl = "IN"; break;
    case LANG_MONGOLIAN:
      /* Ambiguous: could be "mn_CN" or "mn_MN".  */
      l = "mn";
      break;
    case LANG_NEPALI:
      l = "ne";
      switch (sub)
	{
	case SUBLANG_DEFAULT: sl = "NP"; break;
	case SUBLANG_NEPALI_INDIA: sl = "IN"; break;
	}
      break;
    case LANG_NORWEGIAN:
      l = "no";
      switch (sub)
	{
	case SUBLANG_NORWEGIAN_BOKMAL: sl = "NO"; break;
	case SUBLANG_NORWEGIAN_NYNORSK: l = "nn"; sl = "NO"; break;
	}
      break;
    case LANG_ORIYA: l = "or"; sl = "IN"; break;
    case LANG_OROMO: l = "om"; sl = "ET"; break;
    case LANG_PAPIAMENTU: l = "pap"; sl = "AN"; break;
    case LANG_PASHTO:
      /* Ambiguous: could be "ps_PK" or "ps_AF".  */
      l = "ps";
      break;
    case LANG_POLISH: l = "pl"; sl = "PL"; break;
    case LANG_PORTUGUESE:
      l = "pt";
      switch (sub)
	{
	case SUBLANG_PORTUGUESE: sl = "PT"; break;
	case SUBLANG_PORTUGUESE_BRAZILIAN: sl = "BR"; break;
	}
      break;
    case LANG_PUNJABI:
      l = "pa";
      switch (sub)
	{
	case SUBLANG_PUNJABI_INDIA: sl = "IN"; break; /* Gurmukhi script */
	case SUBLANG_PUNJABI_PAKISTAN: sl = "PK"; break; /* Arabic script */
	}
      break;
    case LANG_RHAETO_ROMANCE: l = "rm"; sl = "CH"; break;
    case LANG_ROMANIAN:
      l = "ro";
      switch (sub)
	{
	case SUBLANG_ROMANIAN_ROMANIA: sl = "RO"; break;
	case SUBLANG_ROMANIAN_MOLDOVA: sl = "MD"; break;
	}
      break;
    case LANG_RUSSIAN:
      l = "ru";/* Ambiguous: could be "ru_RU" or "ru_UA" or "ru_MD". */
      break;
    case LANG_SAAMI: /* actually Northern Sami */ l = "se"; sl = "NO"; break;
    case LANG_SANSKRIT: l = "sa"; sl = "IN"; break;
    case LANG_SINDHI: l = "sd";
      switch (sub)
	{
	case SUBLANG_SINDHI_INDIA: sl = "IN"; break;
	case SUBLANG_SINDHI_PAKISTAN: sl = "PK"; break;
	}
      break;
    case LANG_SINHALESE: l = "si"; sl = "LK"; break;
    case LANG_SLOVAK: l = "sk"; sl = "SK"; break;
    case LANG_SLOVENIAN: l = "sl"; sl = "SI"; break;
    case LANG_SOMALI: l = "so"; sl = "SO"; break;
    case LANG_SORBIAN:
      /* FIXME: Adjust this when such locales appear on Unix.  */
      l = "wen";
      sl = "DE";
      break;
    case LANG_SPANISH:
      l = "es";
      switch (sub)
	{
	case SUBLANG_SPANISH: sl = "ES"; break;
	case SUBLANG_SPANISH_MEXICAN: sl = "MX"; break;
	case SUBLANG_SPANISH_MODERN:
	  sl = "ES@modern"; break;	/* not seen on Unix */
	case SUBLANG_SPANISH_GUATEMALA: sl = "GT"; break;
	case SUBLANG_SPANISH_COSTA_RICA: sl = "CR"; break;
	case SUBLANG_SPANISH_PANAMA: sl = "PA"; break;
	case SUBLANG_SPANISH_DOMINICAN_REPUBLIC: sl = "DO"; break;
	case SUBLANG_SPANISH_VENEZUELA: sl = "VE"; break;
	case SUBLANG_SPANISH_COLOMBIA: sl = "CO"; break;
	case SUBLANG_SPANISH_PERU: sl = "PE"; break;
	case SUBLANG_SPANISH_ARGENTINA: sl = "AR"; break;
	case SUBLANG_SPANISH_ECUADOR: sl = "EC"; break;
	case SUBLANG_SPANISH_CHILE: sl = "CL"; break;
	case SUBLANG_SPANISH_URUGUAY: sl = "UY"; break;
	case SUBLANG_SPANISH_PARAGUAY: sl = "PY"; break;
	case SUBLANG_SPANISH_BOLIVIA: sl = "BO"; break;
	case SUBLANG_SPANISH_EL_SALVADOR: sl = "SV"; break;
	case SUBLANG_SPANISH_HONDURAS: sl = "HN"; break;
	case SUBLANG_SPANISH_NICARAGUA: sl = "NI"; break;
	case SUBLANG_SPANISH_PUERTO_RICO: sl = "PR"; break;
	}
      break;
    case LANG_SUTU: l = "bnt"; sl = "TZ"; break; /* or "st_LS" or "nso_ZA"? */
    case LANG_SWAHILI: l = "sw"; sl = "KE"; break;
    case LANG_SWEDISH:
      l = "sv";
      switch (sub)
	{
	case SUBLANG_DEFAULT: sl = "SE"; break;
	case SUBLANG_SWEDISH_FINLAND: sl = "FI"; break;
	}
      break;
    case LANG_SYRIAC: l = "syr"; sl = "TR"; break; /* An extinct language. */
    case LANG_TAGALOG: l = "tl"; sl = "PH"; break;
    case LANG_TAJIK: l = "tg"; sl = "TJ"; break;
    case LANG_TAMIL:
      l = "ta"; /* Ambiguous: could be "ta_IN" or "ta_LK" or "ta_SG". */
      break;
    case LANG_TATAR: l = "tt"; sl = "RU"; break;
    case LANG_TELUGU: l = "te"; sl = "IN"; break;
    case LANG_THAI: l = "th"; sl = "TH"; break;
    case LANG_TIBETAN: l = "bo"; sl = "CN"; break;
    case LANG_TIGRINYA:
      l = "ti";
      switch (sub)
	{
	case SUBLANG_TIGRINYA_ETHIOPIA: sl = "ET"; break;
	case SUBLANG_TIGRINYA_ERITREA: sl = "ER"; break;
	}
      break;
    case LANG_TSONGA: l = "ts"; sl = "ZA"; break;
    case LANG_TSWANA: l = "tn"; sl = "BW"; break;
    case LANG_TURKISH: l = "tr"; sl = "TR"; break;
    case LANG_TURKMEN: l = "tk"; sl = "TM"; break;
    case LANG_UKRAINIAN: l = "uk"; sl = "UA"; break;
    case LANG_URDU:
      l = "ur";
      switch (sub)
	{
	case SUBLANG_URDU_PAKISTAN: sl = "PK"; break;
	case SUBLANG_URDU_INDIA: sl = "IN"; break;
	}
      break;
    case LANG_UZBEK:
      l = "uz";
      switch (sub)
	{
	case SUBLANG_UZBEK_LATIN: sl = "UZ"; break;
	case SUBLANG_UZBEK_CYRILLIC: sl = "UZ@cyrillic"; break;
	}
      break;
    case LANG_VENDA:
      /* FIXME: It's not clear whether Venda has the ISO 639-2 two-letter code
	 "ve" or not.
	 http://www.loc.gov/standards/iso639-2/englangn.html has it, but
	 http://lcweb.loc.gov/standards/iso639-2/codechanges.html doesn't,  */
      l = "ven"; /* or "ve"? */
      sl = "ZA";
      break;
    case LANG_VIETNAMESE: l = "vi"; sl = "VN"; break;
    case LANG_WELSH: l = "cy"; sl = "GB"; break;
    case LANG_XHOSA: l = "xh"; sl = "ZA"; break;
    case LANG_YI: l = "sit"; sl = "CN"; break;
    case LANG_YIDDISH: l = "yi"; sl = "IL"; break;
    case LANG_YORUBA: l = "yo"; sl = "NG"; break;
    case LANG_ZULU: l = "zu"; sl = "ZA"; break;
    }
  strcpy (bfr, l);
  if (sl != NULL)
    {
      if (sl[0] != '@')
	strcat (bfr, "_");
      strcat (bfr, sl);
    }

  return strdup (bfr);
}
#endif

char * join_path(char * prefix, char * suffix)
{
  char * path;
  int path_len;

  path_len = strlen(prefix) + 1 /* len(DIR_SEPARATOR) */ + strlen(suffix);
  path = malloc(path_len + 1);

  strcpy(path, prefix);
  path[strlen(path)+1] = '\0';
  path[strlen(path)] = DIR_SEPARATOR;
  strcat(path, suffix);

  return path;
}

char * get_default_locale(void)
{
  char * locale, * needle;

  locale = NULL;

#ifdef _WIN32
  if(!locale)
    locale = win32_getlocale ();
#endif

  if(!locale)
    locale = strdup (getenv ("LANG"));

#if defined(HAVE_LC_MESSAGES)
  if(!locale)
    locale = strdup (setlocale (LC_MESSAGES, NULL));
#endif

  if(!locale)
    locale = strdup (setlocale (LC_ALL, NULL));

  if(!locale || strcmp(locale, "C") == 0) {
    free(locale);
    locale = strdup("en");
  }

  /* strip off "@euro" from en_GB@euro */
  if ((needle = strchr (locale, '@')) != NULL)
    *needle = '\0';

  /* strip off ".UTF-8" from en_GB.UTF-8 */
  if ((needle = strchr (locale, '.')) != NULL)
    *needle = '\0';

  /* strip off "_GB" from en_GB */
  if ((needle = strchr (locale, '_')) != NULL)
    *needle = '\0';

  return locale;
}
