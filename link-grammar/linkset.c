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
#include <memory.h>

static LINKSET_SET ss[LINKSET_MAX_SETS];
static char q_unit_is_used[LINKSET_MAX_SETS];

/* delcarations of non-exported functions */
static void clear_hash_table(const int unit);
static void initialize_unit(const int unit, const int size);
static LINKSET_NODE *linkset_add_internal(const int unit, char *str);
static int  take_a_unit(void);
static int  compute_hash(const int unit, const char *str);
static char *local_alloc (int nbytes);
 
int linkset_open(const int size)
{
  int unit = take_a_unit();
  initialize_unit(unit, size);
  return unit;
}

void linkset_close(const int unit)
{
  if (!q_unit_is_used[unit]) return;
  linkset_clear(unit);
  free (ss[unit].hash_table);
  q_unit_is_used[unit] = 0;
}

void linkset_clear(const int unit)
{
  int i;
  if (!q_unit_is_used[unit]) return;
  for (i=0; i<ss[unit].hash_table_size; i++)
    {
      LINKSET_NODE *p=ss[unit].hash_table[i];
      while (p!=0)
	{
	  LINKSET_NODE *q = p;
	  p=p->next;
	  if (q->solid) free (q->str);
	  free(q);
	}
    }
  clear_hash_table(unit);
}

int linkset_add(const int unit, char *str)
{
    /* returns 0 if already there, 1 if new. Stores only the pointer. */
    LINKSET_NODE *sn = linkset_add_internal(unit, str);
    if (sn==NULL) return 0;
    sn->solid = 0;
    return 1;
}

int linkset_add_solid(const int unit, char *str)
{
    /* returns 0 if already there, 1 if new. Copies string. */
    LINKSET_NODE *sn = linkset_add_internal(unit, str);
    if (sn==NULL) return 0;
    sn->str = (char *) malloc ((1+strlen(str))*sizeof(char));
    if (!sn->str) error("linkset: out of memory!");
    strcpy(sn->str,str);
    sn->solid = 1;
    return 1;
}

int linkset_remove(const int unit, char *str) 
{
  /* returns 1 if removed, 0 if not found */
  int hashval;
  LINKSET_NODE *p, *last;
  hashval = compute_hash(unit, str);
  last = ss[unit].hash_table[hashval];
  if (!last) return 0;
  if (!strcmp(last->str,str)) 
    {
	ss[unit].hash_table[hashval] = last->next;
	if (last->solid) free(last->str);
	free(last);
	return 1;
    }
  p = last->next;
  while (p)
    {
	if (!strcmp(p->str,str)) 
	  {
	      last->next = p->next;
	      if (last->solid) free(last->str);
	      free(p);
	      return 1;
	  }
	p=p->next;
	last = last->next;
    }
  return 0;
}


int linkset_match(const int unit, char *str) {
    int hashval;
    LINKSET_NODE *p;
    hashval = compute_hash(unit, str);
    p = ss[unit].hash_table[hashval];
    while(p!=0) 
      {
	  if (post_process_match(p->str,str)) return 1;
	  p=p->next;
      }
    return 0;
}

int linkset_match_bw(const int unit, char *str) {
    int hashval;
    LINKSET_NODE *p;
    hashval = compute_hash(unit, str);
    p = ss[unit].hash_table[hashval];
    while(p!=0)
      {
	  if (post_process_match(str,p->str)) return 1;
	  p=p->next;
      }
    return 0;
}

/***********************************************************************/
static void clear_hash_table(const int unit)
{
  memset(ss[unit].hash_table, 0, 
	 ss[unit].hash_table_size*sizeof(LINKSET_NODE *));
}

static void initialize_unit(const int unit, const int size) {
  if(size<=0) {
     printf("size too small!");
     abort();
  }
  ss[unit].hash_table_size = (int) ((float) size*LINKSET_SPARSENESS);
  ss[unit].hash_table = (LINKSET_NODE**) 
	local_alloc (ss[unit].hash_table_size*sizeof(LINKSET_NODE *));
  clear_hash_table(unit);
}

static LINKSET_NODE *linkset_add_internal(const int unit, char *str)
{
  LINKSET_NODE *p, *n;
  int hashval;

  /* look for str in set */
  hashval = compute_hash(unit, str);
  for (p=ss[unit].hash_table[hashval]; p!=0; p=p->next)
    if (!strcmp(p->str,str)) return NULL;  /* already present */
  
  /* create a new node for u; stick it at head of linked list */
  n = (LINKSET_NODE *) local_alloc (sizeof(LINKSET_NODE));      
  n->next = ss[unit].hash_table[hashval];
  n->str = str;
  ss[unit].hash_table[hashval] = n;
  return n;
}

static int compute_hash(const int unit, const char *str)
 {
   /* hash is computed from capitalized prefix only */
  int i, hashval;
  hashval=LINKSET_DEFAULT_SEED;
  for (i=0; isupper((int)str[i]); i++)
    hashval = str[i] + 31*hashval;
  hashval = hashval % ss[unit].hash_table_size;
  if (hashval<0) hashval*=-1;
  return hashval;
}

static int take_a_unit(void) {
  /* hands out free units */
  int i;
  static int q_first = 1;
  if (q_first) {
    memset(q_unit_is_used, 0, LINKSET_MAX_SETS*sizeof(char));
    q_first = 0;
  }
  for (i=0; i<LINKSET_MAX_SETS; i++)
    if (!q_unit_is_used[i]) break;
  if (i==LINKSET_MAX_SETS) {
    printf("linkset.h: No more free units");   
    abort();
  }
  q_unit_is_used[i] = 1;
  return i;
}

static char *local_alloc (int nbytes)  {
   char * p;
   p = (char *) malloc (nbytes);
   if (!p) { 
        printf("linkset: out of memory");
       abort();
    }
   return p;
}

