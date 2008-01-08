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

char *safe_strdup(char *u) {
  if(u)
    return strdup(u);
  return NULL;
}

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
static E_list * copy_E_list(E_list * l);
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

static E_list * copy_E_list(E_list * l) {
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

#ifdef _WIN32
/* borrowed from glib */
/* Used only for Windows builds */
static char*
path_get_dirname (const char *file_name)
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

	  return safe_strdup (drive_colon_dot);
	}
#endif
    return safe_strdup (".");
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
#endif /* _WIN32 */

static char * get_datadir(void)
{
  char * data_dir = NULL;

#ifdef ENABLE_BINRELOC
  data_dir = safe_strdup (BR_DATADIR("/link-grammar"));
#elif defined(_WIN32)
  /* Dynamically locate library and return containing directory */
  HINSTANCE hInstance = GetModuleHandle(NULL);
  if(hInstance != NULL)
    {
      char dll_path[MAX_PATH];

      if(GetModuleFileName(hInstance,dll_path,MAX_PATH)) {
	data_dir = path_get_dirname(dll_path);
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
	fp = fopen(filename, how);  /* If the file does not exist NULL is returned */
	if(fp) return fp;
    }

    {
      char * data_dir = get_datadir();
      if(data_dir) {
	sprintf(fulldictpath, "%s%c%s%c", data_dir, PATH_SEPARATOR, DEFAULTPATH, PATH_SEPARATOR);
	free(data_dir);
      }
      else {
	/* always make sure that it ends with a path separator char 
	   for the below while() loop. */
	sprintf(fulldictpath, "%s%c", DEFAULTPATH, PATH_SEPARATOR);
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

static int step_generator(int d) {
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

void my_random_finalize(void) {
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

static int connector_set_hash(Connector_set *conset, char * s, int d) {
/* This hash function only looks at the leading upper case letters of
   the string, and the direction, '+' or '-'.
*/
    int i;
    for(i=d; isupper((int)*s); s++) i = i + (i<<1) + randtable[(*s + i) & (RTSIZE-1)];
    return (i & (conset->table_size-1));
}

static void build_connector_set_from_expression(Connector_set * conset, Exp * e) {
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

static int easy_match(char * s, char * t) {

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

static char *
win32_getlocale (void)
{
  LCID lcid;
  LANGID langid;
  char *ev;
  int primary, sub;
  char bfr[64];
  char iso639[10];
  char iso3166[10];
  const char *script = NULL;

  /* Let the user override the system settings through environment
   * variables, as on POSIX systems. Note that in GTK+ applications
   * since GTK+ 2.10.7 setting either LC_ALL or LANG also sets the
   * Win32 locale and C library locale through code in gtkmain.c.
   */
  if (((ev = getenv ("LC_ALL")) != NULL && ev[0] != '\0')
      || ((ev = getenv ("LC_MESSAGES")) != NULL && ev[0] != '\0')
      || ((ev = getenv ("LANG")) != NULL && ev[0] != '\0'))
    return safe_strdup (ev);

  lcid = GetThreadLocale ();

  if (!GetLocaleInfo (lcid, LOCALE_SISO639LANGNAME, iso639, sizeof (iso639)) ||
      !GetLocaleInfo (lcid, LOCALE_SISO3166CTRYNAME, iso3166, sizeof (iso3166)))
    return safe_strdup ("C");
  
  /* Strip off the sorting rules, keep only the language part.  */
  langid = LANGIDFROMLCID (lcid);

  /* Split into language and territory part.  */
  primary = PRIMARYLANGID (langid);
  sub = SUBLANGID (langid);

  /* Handle special cases */
  switch (primary)
    {
    case LANG_AZERI:
      switch (sub)
	{
	case SUBLANG_AZERI_LATIN:
	  script = "@Latn";
	  break;
	case SUBLANG_AZERI_CYRILLIC:
	  script = "@Cyrl";
	  break;
	}
      break;
    case LANG_SERBIAN:		/* LANG_CROATIAN == LANG_SERBIAN */
      switch (sub)
	{
	case SUBLANG_SERBIAN_LATIN:
	case 0x06: /* Serbian (Latin) - Bosnia and Herzegovina */
	  script = "@Latn";
	  break;
	}
      break;
    case LANG_UZBEK:
      switch (sub)
	{
	case SUBLANG_UZBEK_LATIN:
	  script = "@Latn";
	  break;
	case SUBLANG_UZBEK_CYRILLIC:
	  script = "@Cyrl";
	  break;
	}
      break;
    }

  strcat (bfr, iso639);
  strcat (bfr, "_");
  strcat (bfr, iso3166);

  if (script)
    strcat (bfr, script);

  return safe_strdup (bfr);
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
    locale = safe_strdup (getenv ("LANG"));

#if defined(HAVE_LC_MESSAGES)
  if(!locale)
    locale = safe_strdup (setlocale (LC_MESSAGES, NULL));
#endif

  if(!locale)
    locale = safe_strdup (setlocale (LC_ALL, NULL));

  if(!locale || strcmp(locale, "C") == 0) {
    free(locale);
    locale = safe_strdup("en");
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
