/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "api.h"

#ifdef USE_FAT_LINKAGES

#include "disjunct-utils.h"

/*
                             Notes about AND

  A large fraction of the code of this parser seems to deal with handling
  conjunctions.  This comment (combined with reading the paper) should
  give an idea of how it works.

  First of all, we need a more detailed discussion of strings, what they
  match, etc.  (This entire discussion ignores the labels, which are
  semantically the same as the leading upper case letters of the
  connector.)

  We'll deal with infinite strings from an alphabet of three types of
  characters: "*". "^" and ordinary characters (denoted "a" and "b").
  (The end of a string should be thought of as an infinite sequence of
  "*"s).

  Let match(s) be the set of strings that will match the string s.  This
  is defined as follows. A string t is in match(s) if (1) its leading
  upper case letters exactly match those of s.  (2) traversing through
  both strings, from left to right in step, no missmatch is found
  between corresponding letters.  A missmatch is a pair of differing
  ordinary characters, or a "^" and any ordinary letter or two "^"s.
  In other words, a match is exactly a "*" and anything, or two
  identical ordinary letters.

  Alternative definition of the set match(s):
  {t | t is obtained from s by replacing each "^" and any other characters
  by "*"s, and replacing any original "*" in s by any other character
  (or "^").}

  Theorem: if t in match(s) then s in match(t).

  It is also a theorem that given any two strings s and t, there exists a
  unique new string u with the property that:

	      match(u) = match(s) intersect match(t)

  This string is called the GCD of s and t.  Here are some examples.

		  GCD(N*a,Nb) = Nba
		  GCD(Na, Nb) = N^
		  GCD(Nab,Nb) = N^b
		  GCD(N^,N*a) = N^a
		  GCD(N^,  N) = N^
		  GCD(N^^,N^) = N^^

  We need an algorithm for computing the GCD of two strings.  Here is
  one.

  First get by the upper case letters (which must be equal, otherwise
  there is no intersection), issuing them.  Traverse the rest of the
  characters of s and t in lockstep until there is nothing left but
  "*"s.  If the two characters are:

		 "a" and "a", issue "a"
		 "a" and "b", issue "^"
		 "a" and "*", issue "a"
		 "*" and "*", issue "*"
		 "*" and "^", issue "^"
		 "a" and "^", issue "^"
		 "^" and "^", issue "^"

  A simple case analysis suffices to show that any string that matches
  the right side, must match both of the left sides, and any string not
  matching the right side must not match at least one of the left sides.

  This proves that the GCD operator is associative and commutative.
  (There must be a name for a mathematical structure with these properties.)

  To elaborate further on this theory, define the notion of two strings
  matching in the dual sense as follows: s and t dual-match if
  match(s) is contained in match(t) or vice versa---

  Full development of this theory could lead to a more efficient
  algorithm for this problem.  I'll defer this until such time as it
  appears necessary.


  We need a data structure that stores a set of fat links.  Each fat
  link has a number (called its label).  The fat link operates in liu of
  a collection of links.  The particular stuff it is a substitute for is
  defined by a disjunct.  This disjunct is stored in the data structure.

  The type of a disjunct is defined by the sequence of connector types
  (defined by their upper case letters) that comprises it.  Each entry
  of the label_table[] points to a list of disjuncts that have the same
  type (a hash table is uses so that, given a disjunct, we can efficiently
  compute the element of the label table in which it belongs).

  We begin by loading up the label table with all of the possible
  fat links that occur through the words of the sentence.  These are
  obtained by taking every sub-range of the connectors of each disjunct
  (containing the center).  We also compute the closure (under the GCD
  operator) of these disjuncts and store also store these in the
  label_table.  Each disjunct in this table has a string which represents
  the subscripts of all of its connectors (and their multi-connector bits).

  It is possible to generate a fat connector for any one of the
  disjuncts in the label_table.  This connector's label field is given
  the label from the disjunct from which it arose.  It's string field
  is taken from the string of the disjunct (mentioned above).  It will be
  given a priority with a value of UP_priority or DOWN_priority (depending
  on how it will be used).  A connector of UP_priority can match one of
  DOWN_priority, but neither of these can match any other priority.
  (Of course, a fat connector can match only another fat connector with
  the same label.)

  The paper describes in some detail how disjuncts are given to words
  and to "and" and ",", etc.  Each word in the sentence gets many more
  new disjuncts.  For each contiguous set of connectors containing (or
  adjacent to) the center of the disjunct, we generate a fat link, and
  replace these connector in the word by a fat link.  (Actually we do
  this twice.  Once pointing to the right, once to the left.)  These fat
  links have priority UP_priority.

  What do we generate for ","?  For each type of fat link (each label)
  we make a disjunct that has two down connectors (to the right and left)
  and one up connector (to the right).  There will be a unique way of
  hooking together a comma-separated and-list.

  The disjuncts on "and" are more complicated.  Here we have to do just what
  we did for comma (but also include the up link to the left), then
  we also have to allow the process to terminate.  So, there is a disjunct
  with two down fat links, and between them are the original thin links.
  These are said to "blossom" out.  However, this is not all that is
  necessary.  It's possible for an and-list to be part of another and list
  with a different labeled fat connector.  To make this possible, we
  regroup the just blossomed disjuncts (in all possible ways about the center)
  and install them as fat links.  If this sounds like a lot of disjuncts --
  it is!  The program is currently fairly slow on long sentence with and.

  It is slightly non-obvious that the fat-links in a linkage constructed
  from disjuncts defined in this way form a binary tree.  Naturally,
  connectors with UP_priority point up the tree, and those with DOWN_priority
  point down the tree.

  Think of the string x on the connector as representing a set X of strings.
  X = match(x).  So, for example, if x="S^" then match(x) = {"S", "S*a",
  "S*b", etc}.  The matching rules for UP and DOWN priority connectors
  are such that as you go up (the tree of ands) the X sets get no larger.
  So, for example, a "Sb" pointing up can match an "S^" pointing down.
  (Because more stuff can match "Sb" than can match "S^".)
  This guarantees that whatever connector ultimately gets used after the
  fat connector blossoms out (see below), it is a powerful enough connector
  to be able to match to any of the connectors associated with it.

  One problem with the scheme just descibed is that it sometimes generates
  essentially the same linkage several times.  This happens if there is
  a gap in the connective power, and the mismatch can be moved around in
  different ways.  Here is an example of how this happens.

  (Left is DOWN, right is UP)

     Sa <---> S^ <---> S            or             Sa <---> Sa <---> S
	 fat      thin                                 fat      thin

  Here two of the disjunct types are given by "S^" and "Sa".  Notice that
  the criterion of shrinking the matching set is satisfied by the the fat
  link (traversing from left to right).  How do I eliminate one of these?

  I use the technique of canonization.  I generate all the linkages.  There
  is then a procedure that can check to see of a linkage is canonical.
  If it is, it's used, otherwise it's ignored.  It's claimed that exactly
  one canonical one of each equivalence class will be generated.
  We basically insist that the intermediate fat disjuncts (ones that
  have a fat link pointing down) are all minimal -- that is, that they
  cannot be replaced by by another (with a strictly) smaller match set.
  If one is not minimal, then the linkage is rejected.

  Here's a proof that this is correct.  Consider the set of equivalent
  linkages that are generated.  These Pick a disjunct that is the root of
  its tree.  Consider the set of all disjuncts which occur in that positon
  among the equivalent linkages.  The GCD of all of these can fit in that
  position (it matches down the tree, since its match set has gotten
  smaller, and it also matches to the THIN links.)  Since the GCD is put
  on "and" this particular one will be generated.  Therefore rejecting
  a linkage in which a root fat disjunct can be replaced by a smaller one
  is ok (since the smaller one will be generated separately).  What about
  a fat disjunct that is not the root.  We consider the set of linkages in
  which the root is minimal (the ones for which it's not have already been
  eliminated).  Now, consider one of the children of the root in precisely
  the way we just considered the root.  The same argument holds.  The only
  difference is that the root node gives another constraint on how small
  you can make the disjunct -- so, within these constraints, if we can go
  smaller, we reject.

  The code to do all of this is fairly ugly, but I think it works.


Problems with this stuff:

  1) There is obviously a combinatorial explosion that takes place.
     As the number of disjuncts (and the number of their subscripts
     increase) the number of disjuncts that get put onto "and" will
     increase tremendously.  When we made the transcript for the tech
     report (Around August 1991) most of the sentence were processed
     in well under 10 seconds.  Now (Jan 1992), some of these sentences
     take ten times longer.  As of this writing I don't really know the
     reason, other than just the fact that the dictionary entries are
     more complex than they used to be.   The number of linkages has also
     increased significantly.

  2) Each element of an and list must be attached through only one word.
     This disallows "there is time enough and space enough for both of us",
     and many other reasonable sounding things.  The combinatorial
     explosion that would occur if you allowed two different connection
     points would be tremendous, and the number of solutions would also
     probably go up by another order of magnitude.  Perhaps if there
     were strong constraints on the type of connectors in which this
     would be allowed, then this would be a conceivable prospect.

  3) A multi-connector must be either all "outside" or all "inside" the and.
     For example, "the big black dog and cat ran" has only two ways to
     linkages (instead of three).

Possible bug: It seems that the following two linkages should be the
same under the canonical linkage test.  Could this have to do with the
pluralization system?

> I am big and the bike and the car were broken
Accepted (4 linkages, 4 with no P.P. violations) at stage 1
  Linkage 1, cost vector = (0, 0, 18)

                                   +------Spx-----+      
       +-----CC-----+------Wd------+-d^^*i^-+     |      
  +-Wd-+Spi+-Pa+    |   +--Ds-+d^^*+   +-Ds-+     +--Pv-+
  |    |   |   |    |   |     |    |   |    |     |     |
///// I.p am big.a and the bike.n and the car.n were broken

       /////          RW      <---RW---->  RW        /////
       /////          Wd      <---Wd---->  Wd        I.p
       I.p            CC      <---CC---->  CC        and
       I.p            Sp*i    <---Spii-->  Spi       am
       am             Pa      <---Pa---->  Pa        big.a
       and            Wd      <---Wd---->  Wd        and
       bike.n         d^s**  6<---d^^*i->  d^^*i  6  and
       the            D       <---Ds---->  Ds        bike.n
       and            Sp      <---Spx--->  Spx       were
       and            d^^*i  6<---d^^*i->  d^s**  6  car.n
       the            D       <---Ds---->  Ds        car.n
       were           Pv      <---Pv---->  Pv        broken

(press return for another)
>
  Linkage 2, cost vector = (0, 0, 18)

                                   +------Spx-----+      
       +-----CC-----+------Wd------+-d^s**^-+     |      
  +-Wd-+Spi+-Pa+    |   +--Ds-+d^s*+   +-Ds-+     +--Pv-+
  |    |   |   |    |   |     |    |   |    |     |     |
///// I.p am big.a and the bike.n and the car.n were broken 

       /////          RW      <---RW---->  RW        /////
       /////          Wd      <---Wd---->  Wd        I.p
       I.p            CC      <---CC---->  CC        and
       I.p            Sp*i    <---Spii-->  Spi       am
       am             Pa      <---Pa---->  Pa        big.a
       and            Wd      <---Wd---->  Wd        and
       bike.n         d^s**  6<---d^s**->  d^s**  6  and
       the            D       <---Ds---->  Ds        bike.n
       and            Sp      <---Spx--->  Spx       were
       and            d^s**  6<---d^s**->  d^s**  6  car.n
       the            D       <---Ds---->  Ds        car.n
       were           Pv      <---Pv---->  Pv        broken

*/

static void init_LT(Sentence sent)
{
	sent->and_data.LT_bound = 20;
	sent->and_data.LT_size = 0;
	sent->and_data.label_table =
		(Disjunct **) xalloc(sent->and_data.LT_bound * sizeof(Disjunct *));
}

static void grow_LT(Sentence sent)
{
	size_t oldsize = sent->and_data.LT_bound * sizeof(Disjunct *);
	sent->and_data.LT_bound = (3*sent->and_data.LT_bound)/2;
	sent->and_data.label_table =
		(Disjunct **) xrealloc(sent->and_data.label_table,
	                     oldsize,
		                  sent->and_data.LT_bound * sizeof(Disjunct *));
}

static void init_HT(Sentence sent)
{
	memset(sent->and_data.hash_table, 0, HT_SIZE*sizeof(Label_node *));
}

static void free_HT(Sentence sent)
{
	int i;
	Label_node * la, * la1;
	for (i=0; i<HT_SIZE; i++) {
		for (la=sent->and_data.hash_table[i]; la != NULL; la = la1) {
			la1 = la->next;
			xfree((char *)la, sizeof(Label_node));
		}
		sent->and_data.hash_table[i] = NULL;
	}
}

static void free_LT(Sentence sent)
{
	int i;
	for (i=0; i<sent->and_data.LT_size; i++) {
		free_disjuncts(sent->and_data.label_table[i]);
	}
	xfree((char *) sent->and_data.label_table,
		  sent->and_data.LT_bound * sizeof(Disjunct*));
	sent->and_data.LT_bound = 0;
	sent->and_data.LT_size = 0;
	sent->and_data.label_table = NULL;
}

void free_AND_tables(Sentence sent)
{
	free_LT(sent);
	free_HT(sent);
}

void initialize_conjunction_tables(Sentence sent)
{
	int i;
	sent->and_data.LT_bound = 0;
	sent->and_data.LT_size = 0;
	sent->and_data.label_table = NULL;
	for (i=0; i<HT_SIZE; i++) {
		sent->and_data.hash_table[i] = NULL;
	}
}

/**
 * This is a hash function for disjuncts
 */
static inline int and_hash_disjunct(Disjunct *d)
{
	unsigned int i;
	Connector *e;
	i = 0;
	for (e = d->left ; e != NULL; e = e->next) {
		i += connector_hash(e);
	}
	i += (i<<5);
	for (e = d->right ; e != NULL; e = e->next) {
		i += connector_hash(e);
	}
	return (i & (HT_SIZE-1));
}

/**
 * Returns TRUE if the disjunct is appropriate to be made into fat links.
 * Check here that the connectors are from some small set.
 * This will disallow, for example "the and their dog ran".
 */
static int is_appropriate(Sentence sent, Disjunct * d)
{
	Connector * c;

	if (sent->dict->andable_connector_set == NULL) return TRUE;
	/* if no set, then everything is considered andable */
	for (c = d->right; c!=NULL; c=c->next) {
		if (!match_in_connector_set(sent, sent->dict->andable_connector_set, c, '+')) return FALSE;
	}
	for (c = d->left; c!=NULL; c=c->next) {
		if (!match_in_connector_set(sent, sent->dict->andable_connector_set, c, '-')) return FALSE;
	}
	return TRUE;
}

/**
 * Two connectors are said to be of the same type if they have
 * the same label, and the initial upper case letters of their
 * strings match.
 */
static int connector_types_equal(Connector * c1, Connector * c2)
{
	if (c1->label != c2->label) return FALSE;
	return utf8_upper_match(c1->string, c2->string);
}

/**
 * Two disjuncts are said to be the same type if they're the same
 * ignoring the multi fields, the priority fields, and the subscripts
 * of the connectors (and the string field of the disjunct of course).
 * Disjuncts of the same type are located in the same label_table list.
 *
 * This returns TRUE if they are of the same type.
 */
static int disjunct_types_equal(Disjunct * d1, Disjunct * d2)
{
	Connector *e1, *e2;

	e1 = d1->left;
	e2 = d2->left;
	while((e1!=NULL) && (e2!=NULL)) {
		if (!connector_types_equal(e1,e2)) break;
		e1 = e1->next;
		e2 = e2->next;
	}
	if ((e1!=NULL) || (e2!=NULL)) return FALSE;
	e1 = d1->right;
	e2 = d2->right;
	while((e1!=NULL) && (e2!=NULL)) {
		if (!connector_types_equal(e1,e2)) break;
		e1 = e1->next;
		e2 = e2->next;
	}
	if ((e1!=NULL) || (e2!=NULL)) return FALSE;
	return TRUE;
}
#endif /* USE_FAT_LINKAGES */

/**
 * This returns a string that is the the GCD of the two given strings.
 * If the GCD is equal to one of them, a pointer to it is returned.
 * Otherwise a new string for the GCD is xalloced and put on the
 * "free later" list.
 */
const char * intersect_strings(Sentence sent, const char * s, const char * t)
{
	int i, j, d;
	const char *w, *s0;
	char u0[MAX_TOKEN_LENGTH]; /* Links are *always* less than 10 chars long */
	char *u;
	if (strcmp(s,t)==0) return s;  /* would work without this */
	i = strlen(s);
	j = strlen(t);
	if (j > i) {
		w = s; s = t; t = w;
	}
	/* s is now the longer (at least not the shorter) string */
	u = u0;
	d = 0;
	s0 = s;
	while (*t != '\0') {
		if ((*s == *t) || (*t == '*')) {
			*u = *s;
		} else {
			d++;
			if (*s == '*') *u = *t;
			else *u = '^';
		}
		s++; t++; u++;
	}
	if (d==0) {
		return s0;
	} else {
		strcpy(u, s);   /* get the remainder of s */
		return string_set_add(u0, sent->string_set);
	}
}

#ifdef USE_FAT_LINKAGES
/**
 * Two connectors are said to be equal if they are of the same type
 * (defined above), they have the same multi field, and they have
 *  exactly the same connectors (including lower case chars).
 * (priorities ignored).
 */
static int connectors_equal_AND(Connector *c1, Connector *c2)
{
	return (c1->label == c2->label) &&
		   (c1->multi == c2->multi) &&
		   (strcmp(c1->string, c2->string) == 0);
}

/**
 * Return true if the disjuncts are equal (ignoring priority fields)
 * and the string of the disjunct.
 */
static int disjuncts_equal_AND(Sentence sent, Disjunct * d1, Disjunct * d2)
{
	Connector *e1, *e2;
	sent->and_data.STAT_calls_to_equality_test++;
	e1 = d1->left;
	e2 = d2->left;
	while((e1!=NULL) && (e2!=NULL)) {
		if (!connectors_equal_AND(e1, e2)) break;
		e1 = e1->next;
		e2 = e2->next;
	}
	if ((e1!=NULL) || (e2!=NULL)) return FALSE;
	e1 = d1->right;
	e2 = d2->right;
	while((e1!=NULL) && (e2!=NULL)) {
		if (!connectors_equal_AND(e1, e2)) break;
		e1 = e1->next;
		e2 = e2->next;
	}
	if ((e1!=NULL) || (e2!=NULL)) return FALSE;
	return TRUE;
}

/**
 * Create a new disjunct that is the GCD of d1 and d2.
 * It assumes that the disjuncts are of the same type, so the
 * GCD will not be empty.
 */
static Disjunct * intersect_disjuncts(Sentence sent, Disjunct * d1, Disjunct * d2)
{
	Disjunct * d;
	Connector *c1, *c2, *c;
	d = copy_disjunct(d1);
	c = d->left;
	c1 = d1->left;
	c2 = d2->left;
	while (c1!=NULL) {
		connector_set_string (c, intersect_strings(sent, c1->string, c2->string));
		c->multi = (c1->multi) && (c2->multi);
		c = c->next; c1 = c1->next; c2 = c2->next;
	}
	c = d->right;
	c1 = d1->right;
	c2 = d2->right;
	while (c1!=NULL) {
		connector_set_string (c, intersect_strings(sent, c1->string, c2->string));
		c->multi = (c1->multi) && (c2->multi);
		c = c->next; c1 = c1->next; c2 = c2->next;
	}
	return d;
}

/**
 * (1) look for the given disjunct in the table structures
 *     if it's already in the table structures, do nothing
 * (2) otherwise make a copy of it, and put it into the table structures
 * (3) also put all of the GCDs of this disjunct with all of the
 *     other matching disjuncts into the table.
 *
 * The costs are set to zero.
 * Note that this has no effect on disjunct d.
 */
static void put_disjunct_into_table(Sentence sent, Disjunct *d)
{
	Disjunct *d1=NULL, *d2, *di, *d_copy;
	Label_node * lp;
	int h, k;

	h = and_hash_disjunct(d);

	for (lp = sent->and_data.hash_table[h]; lp != NULL; lp = lp->next)
	{
		d1 = sent->and_data.label_table[lp->label];
		if (disjunct_types_equal(d,d1)) break;
	}
	if (lp != NULL)
	{
		/* there is already a label for disjuncts of this type */
		/* d1 points to the list of disjuncts of this type already there */
		while(d1 != NULL)
		{
			if (disjuncts_equal_AND(sent, d1, d)) return;
			d1 = d1->next;
		}
		/* now we must put the d disjunct in there, and all of the GCDs of
		   it with the ones already there.

		   This is done as follows.  We scan through the list of disjuncts
		   computing the gcd of the new one with each of the others, putting
		   the resulting disjuncts onto another list rooted at d2.
		   Now insert d into the the list already there.  Now for each
		   one on the d2 list, put it in if it isn't already there.

		   Here we're making use of the following theorem: Given a
		   collection of sets s1, s2 ... sn closed under intersection,
		   to if we add a new set s to the collection and also add
		   all the intersections between s and s1...sn to the collection,
		   then the collection is still closed under intersection.

		   Use a Venn diagram to prove this theorem.

		*/
		d_copy = copy_disjunct(d);
		d_copy->cost = 0;
		k = lp->label;
		d2 = NULL;
		for (d1=sent->and_data.label_table[k]; d1!=NULL; d1 = d1->next) {
			di = intersect_disjuncts(sent, d_copy, d1);
			di->next = d2;
			d2 = di;
		}
		d_copy->next = sent->and_data.label_table[k];
		sent->and_data.label_table[k] = d_copy;
		for (;d2 != NULL; d2 = di) {
			di = d2->next;
			for (d1 = sent->and_data.label_table[k]; d1 != NULL; d1 = d1->next) {
				if (disjuncts_equal_AND(sent, d1, d2)) break;
			}
			if (d1 == NULL) {
				sent->and_data.STAT_N_disjuncts++;
				d2->next = sent->and_data.label_table[k];
				sent->and_data.label_table[k] = d2;
			} else {
				d2->next = NULL;
				free_disjuncts(d2);
			}
		}
	} else {
		/* create a new label for disjuncts of this type */
		d_copy = copy_disjunct(d);
		d_copy->cost = 0;
		d_copy->next = NULL;
		if (sent->and_data.LT_size == sent->and_data.LT_bound) grow_LT(sent);
		lp = (Label_node *) xalloc(sizeof(Label_node));
		lp->next = sent->and_data.hash_table[h];
		sent->and_data.hash_table[h] = lp;
		lp->label = sent->and_data.LT_size;
		sent->and_data.label_table[sent->and_data.LT_size] = d_copy;
		sent->and_data.LT_size++;
		sent->and_data.STAT_N_disjuncts++;
	}
}

/**
 * A sub disjuct of d is any disjunct obtained by killing the tail
 *  of either connector list at any point.
 *  Here we go through each sub-disjunct of d, and put it into our
 *  table data structure.
 *
 *  The function has no side effects on d.
 */
static void extract_all_fat_links(Sentence sent, Disjunct * d)
{
	Connector * cl, * cr, *tl, *tr;
	tl = d->left;
	d->left = NULL;
	for (cr = d->right; cr!=NULL; cr = cr->next) {
		tr = cr->next;
		cr->next = NULL;
		if (is_appropriate(sent, d)) put_disjunct_into_table(sent, d);
		cr->next = tr;
	}
	d->left = tl;

	tr = d->right;
	d->right = NULL;
	for (cl = d->left; cl!=NULL; cl = cl->next) {
		tl = cl->next;
		cl->next = NULL;
		if (is_appropriate(sent, d)) put_disjunct_into_table(sent, d);
		cl->next = tl;
	}
	d->right = tr;

	for (cl = d->left; cl!=NULL; cl = cl->next) {
		for (cr = d->right; cr!=NULL; cr = cr->next) {
			tl = cl->next;
			tr = cr->next;
			cl->next = cr->next = NULL;

			if (is_appropriate(sent, d)) put_disjunct_into_table(sent, d);

			cl->next = tl;
			cr->next = tr;
		}
	}
}

/**
 * put the next len characters from c->string (skipping upper
 * case ones) into s.  If there are fewer than this, pad with '*'s.
 * Then put in a character for the multi match bit of c.
 * Then put in a '\0', and return a pointer to this place.
 */
static char * stick_in_one_connector(char *s, Connector *c, int len)
{
	const char * t;

	t = skip_utf8_upper(c->string);

	while (*t != '\0') {
		*s++ = *t++;
		len--;
	}
	while (len > 0) {
		*s++ = '*';
		len--;
	}
	if (c->multi) *s++ = '*'; else *s++ = '^';  /* check this sometime */
	*s = '\0';
	return s;
}

/**
 * This takes a label k, modifies the list of disjuncts with that
 * label.  For each such disjunct, it computes the string that
 * will be used in the fat connector that represents it.
 *
 * The only hard part is finding the length of each of the strings
 * so that "*" can be put in.  A better explanation will have to wait.
 */
static void compute_matchers_for_a_label(Sentence sent, int k)
{
	char buff[2*MAX_WORD];
	int lengths[MAX_LINKS];
	int N_connectors, i, j;
	Connector * c;
	Disjunct * d;
	const char *cs;
	char *s;

	d = sent->and_data.label_table[k];

	N_connectors = 0;
	for (c=d->left; c != NULL; c = c->next) N_connectors ++;
	for (c=d->right; c != NULL; c = c->next) N_connectors ++;

	for (i=0; i<N_connectors; i++) lengths[i] = 0;
	while(d != NULL) {
		i = 0;
		for (c=d->left; c != NULL; c = c->next) {
         cs = skip_utf8_upper(c->string);
			j = strlen(cs);
			if (j > lengths[i]) lengths[i] = j;
			i++;
		}
		for (c=d->right; c != NULL; c = c->next) {
			cs = c->string;
			cs = skip_utf8_upper(cs);
			j = strlen(cs);
			if (j > lengths[i]) lengths[i] = j;
			i++;
		}
		d = d->next;
	}

	for (d = sent->and_data.label_table[k]; d!= NULL; d = d->next)
	{
		i=0;
		s = buff;
		for (c=d->left; c != NULL; c = c->next) {
			s = stick_in_one_connector(s, c, lengths[i]);
			i++;
		}
		for (c=d->right; c != NULL; c = c->next) {
			s = stick_in_one_connector(s, c, lengths[i]);
			i++;
		}
		d->string = string_set_add(buff, sent->string_set);
	}
}

/**
 * Goes through the entire sentence and builds the fat link tables
 * for all the disjuncts of all the words.
 */
void build_conjunction_tables(Sentence sent)
{
	int w;
	int k;
	Disjunct * d;

	init_HT(sent);
	init_LT(sent);
	sent->and_data.STAT_N_disjuncts = 0;
	sent->and_data.STAT_calls_to_equality_test = 0;

	for (w=0; w<sent->length; w++) {
		for (d=sent->word[w].d; d!=NULL; d=d->next) {
			extract_all_fat_links(sent, d);
		}
	}

	for (k=0; k<sent->and_data.LT_size; k++) {
		compute_matchers_for_a_label(sent, k);
	}
}

void print_AND_statistics(Sentence sent)
{
	printf("Number of disjunct types (labels): %d\n", sent->and_data.LT_size);
	printf("Number of disjuncts in the table: %d\n", sent->and_data.STAT_N_disjuncts);
	if (sent->and_data.LT_size != 0) {
	  printf("average list length: %f\n",
			 (float)sent->and_data.STAT_N_disjuncts/sent->and_data.LT_size);
	}
	printf("Number of equality tests: %d\n", sent->and_data.STAT_calls_to_equality_test);
}

/**
 * Fill in the fields of c for the disjunct.  This must be in
 * the table data structures.  The label field and the string field
 * are filled in appropriately.  Priority is set to UP_priority.
 */
static void connector_for_disjunct(Sentence sent, Disjunct * d, Connector * c)
{
	int h;
	Disjunct * d1 = NULL;
	Label_node * lp;

	h = and_hash_disjunct(d);

	for (lp = sent->and_data.hash_table[h]; lp != NULL; lp = lp->next) {
		d1 = sent->and_data.label_table[lp->label];
		if (disjunct_types_equal(d,d1)) break;
	}
	assert(lp != NULL, "A disjunct I inserted was not there. (1)");

	while(d1 != NULL) {
		if (disjuncts_equal_AND(sent, d1, d)) break;
		d1 = d1->next;
	}

	assert(d1 != NULL, "A disjunct I inserted was not there. (2)");

	c->label = lp->label;
	connector_set_string(c, d1->string);
	c->priority = UP_priority;
	c->multi = FALSE;
}


/**
 * This function allocates and returns a list of disjuncts.
 * This is the one obtained by substituting each contiguous
 * non-empty subrange of d (incident on the center) by an appropriate
 * fat link, in two possible positions.  Does not effect d.
 * The cost of d is inherited by all of the disjuncts in the result.
 */
static Disjunct * build_fat_link_substitutions(Sentence sent, Disjunct *d)
{
	Connector * cl, * cr, *tl, *tr, *wc, work_connector;
	Disjunct *d1, *wd, work_disjunct, *d_list;
	if (d==NULL) return NULL;
	wd = &work_disjunct;
	wc = init_connector(&work_connector);
	d_list = NULL;
	*wd = *d;
	tl = d->left;
	d->left = NULL;
	for (cr = d->right; cr!=NULL; cr = cr->next) {
		tr = cr->next;
		cr->next = NULL;
		if (is_appropriate(sent, d)) {
			connector_for_disjunct(sent, d, wc);
			wd->left = tl;
			wd->right = wc;
			wc->next = tr;
			d1 = copy_disjunct(wd);
			d1->next = d_list;
			d_list = d1;
			wd->left = wc;
			wc->next = tl;
			wd->right = tr;
			d1 = copy_disjunct(wd);
			d1->next = d_list;
			d_list = d1;
		}
		cr->next = tr;
	}
	d->left = tl;

	tr = d->right;
	d->right = NULL;
	for (cl = d->left; cl!=NULL; cl = cl->next) {
		tl = cl->next;
		cl->next = NULL;
		if (is_appropriate(sent, d)) {
			connector_for_disjunct(sent, d, wc);
			wd->left = tl;
			wd->right = wc;
			wc->next = tr;
			d1 = copy_disjunct(wd);
			d1->next = d_list;
			d_list = d1;
			wd->left = wc;
			wc->next = tl;
			wd->right = tr;
			d1 = copy_disjunct(wd);
			d1->next = d_list;
			d_list = d1;
		}
		cl->next = tl;
	}
	d->right = tr;

	for (cl = d->left; cl!=NULL; cl = cl->next) {
		for (cr = d->right; cr!=NULL; cr = cr->next) {
			tl = cl->next;
			tr = cr->next;
			cl->next = cr->next = NULL;
			if (is_appropriate(sent, d)) {
				connector_for_disjunct(sent, d, wc);
				wd->left = tl;
				wd->right = wc;
				wc->next = tr;
				d1 = copy_disjunct(wd);
				d1->next = d_list;
				d_list = d1;
				wd->left = wc;
				wc->next = tl;
				wd->right = tr;
				d1 = copy_disjunct(wd);
				d1->next = d_list;
				d_list = d1;
			}
			cl->next = tl;
			cr->next = tr;
		}
	}
	return d_list;
}

/**
 * This is basically a "map" function for build_fat_link_substitutions.
 * It's applied to the disjuncts for all regular words of the sentence.
 */
Disjunct * explode_disjunct_list(Sentence sent, Disjunct *d)
{
   Disjunct *d1;

   d1 = NULL;

   for (; d!=NULL; d = d->next) {
	   d1 = catenate_disjuncts(d1, build_fat_link_substitutions(sent, d));
   }
   return d1;
}

/**
 * Builds and returns a disjunct list for the comma.  These are the
 * disjuncts that are used when "," operates in conjunction with "and".
 * Does not deal with the ", and" issue, nor the other uses
 * of comma.
 */
Disjunct * build_COMMA_disjunct_list(Sentence sent)
{
	int lab;
	Disjunct *d1, *d2, *d, work_disjunct, *wd;
	Connector work_connector1, work_connector2, *c1, *c2;
	Connector work_connector3, *c3;
	c1 = init_connector(&work_connector1);
	c2 = init_connector(&work_connector2);
	c3 = init_connector(&work_connector3);
	wd = &work_disjunct;

	d1 = NULL;  /* where we put the list we're building */

	c1->next = NULL;
	c2->next = c3;
	c3->next = NULL;
	c1->priority = c3->priority = DOWN_priority;
	c2->priority = UP_priority;
	c1->multi = c2->multi = c3->multi = FALSE;
	wd->left = c1;
	wd->right = c2;
	wd->string = ",";  /* *** fix this later?? */
	wd->next = NULL;
	wd->cost = 0;
	for (lab = 0; lab < sent->and_data.LT_size; lab++) {
		for (d = sent->and_data.label_table[lab]; d!=NULL; d=d->next) {
			c1->string = c2->string = c3->string = d->string;
			c1->label = c2->label = c3->label = lab;
			d2 = copy_disjunct(wd);
			d2->next = d1;
			d1 = d2;
		}
	}
	return d1;
}

/**
 * Builds and returns a disjunct list for "and", "or" and "nor"
 * for each disjunct in the label_table, we build three disjuncts
 * this means that "Danny and Tycho and Billy" will be parsable in
 * two ways.  I don't know an easy way to avoid this
 * the string is either "and", or "or", or "nor" at the moment.
 */
Disjunct * build_AND_disjunct_list(Sentence sent, char * s)
{
	int lab;
	Disjunct *d_list, *d1, *d3, *d, *d_copy;
	Connector *c1, *c2, *c3;

	d_list = NULL;  /* where we put the list we're building */

	for (lab = 0; lab < sent->and_data.LT_size; lab++) {
		for (d = sent->and_data.label_table[lab]; d!=NULL; d=d->next) {
			d1 = build_fat_link_substitutions(sent, d);
			d_copy = copy_disjunct(d);  /* also include the thing itself! */
			d_copy->next = d1;
			d1 = d_copy;
			for(;d1 != NULL; d1 = d3) {
				d3 = d1->next;

				c1 = connector_new();
				c2 = connector_new();
				c1->priority = c2->priority = DOWN_priority;
				connector_set_string(c1, d->string);
				connector_set_string(c2, d->string);
				c1->label = c2->label = lab;

				d1->string = s;

				if (d1->right == NULL) {
					d1->right = c2;
				} else {
					for (c3=d1->right; c3->next != NULL; c3 = c3->next)
					  ;
					c3->next = c2;
				}
				if (d1->left == NULL) {
					d1->left = c1;
				} else {
					for (c3=d1->left; c3->next != NULL; c3 = c3->next)
					  ;
					c3->next = c1;
				}
				d1->next = d_list;
				d_list = d1;
			}
		}
	}
#if defined(PLURALIZATION)
	/* here is where "and" makes singular into plural. */
	/* must accommodate "he and I are good", "Davy and I are good"
	   "Danny and Davy are good", and reject all of these with "is"
	   instead of "are".

	   The SI connectors must also be modified to accommodate "are John
	   and Dave here", but kill "is John and Dave here"
	*/
	if (strcmp(s, "and") == 0)
	{
		for (d1 = d_list; d1 != NULL; d1 = d1->next)
		{
			for (c1 = d1->right; c1 != NULL; c1 = c1->next)
			{
				if ((c1->string[0] == 'S') &&
					((c1->string[1] == '^') ||
					 (c1->string[1] == 's') ||
					 (c1->string[1] == 'p') ||
					 (c1->string[1] == '\0')))
				{
					connector_set_string(c1, "Sp");
				}
			}
			for (c1 = d1->left; c1 != NULL; c1 = c1->next)
			{
				if ((c1->string[0] == 'S') && (c1->string[1] == 'I') &&
					((c1->string[2] == '^') ||
					 (c1->string[2] == 's') ||
					 (c1->string[2] == 'p') ||
					 (c1->string[2] == '\0')))
				{
					connector_set_string(c1, "SIp");
				}
			}
		}
	}
/*
  "a cat or a dog is here"  vs  "a cat or a dog are here"
  The first seems right, the second seems wrong.  I'll stick with this.

  That is, "or" has the property that if both parts are the same in
  number,  we use that but if they differ, we use plural.

  The connectors on "I" must be handled specially.  We accept
  "I or the dogs are here" but reject "I or the dogs is here"
*/

/* the code here still does now work "right", rejecting "is John or I invited"
   and accepting "I or my friend know what happened"

   The more generous code for "nor" has been used instead
*/
/*
	else if (strcmp(s, "or") == 0) {
		for (d1 = d_list; d1!=NULL; d1=d1->next) {
			for (c1=d1->right; c1!=NULL; c1=c1->next) {
				if (c1->string[0] == 'S') {
					if (c1->string[1]=='^') {
						if (c1->string[2]=='a') {
							connector_set_string(c1, "Ss");
						} else {
							connector_set_string(c1, "Sp");
						}
					} else if ((c1->string[1]=='p') && (c1->string[2]=='a')){
						connector_set_string(c1, "Sp");
					}
				}
			}
			for (c1=d1->left; c1!=NULL; c1=c1->next) {
				if ((c1->string[0] == 'S') && (c1->string[1] == 'I')) {
					if (c1->string[2]=='^') {
						if (c1->string[3]=='a') {
							connector_set_string(c1, "Ss");
						} else {
							connector_set_string(c1, "Sp");
						}
					} else if ((c1->string[2]=='p') && (c1->string[3]=='a')){
						connector_set_string(c1, "Sp");
					}
				}
			}
		}
	}
*/
/*
	It appears that the "nor" of two things can be either singular or
	plural.  "neither she nor John likes dogs"
             "neither she nor John like dogs"

*/
	else if ((strcmp(s,"nor")==0) || (strcmp(s,"or")==0)) {
		for (d1 = d_list; d1!=NULL; d1=d1->next) {
			for (c1=d1->right; c1!=NULL; c1=c1->next) {
				if ((c1->string[0] == 'S') &&
					((c1->string[1]=='^') ||
					 (c1->string[1]=='s') ||
					 (c1->string[1]=='p'))) {
					connector_set_string(c1, "S");
				}
			}
			for (c1=d1->left; c1!=NULL; c1=c1->next) {
				if ((c1->string[0] == 'S') && (c1->string[1] == 'I') &&
					((c1->string[2]=='^') ||
					 (c1->string[2]=='s') ||
					 (c1->string[2]=='p'))) {
					connector_set_string(c1, "SI");
				}
			}
		}
	}

#endif
	return d_list;
}


/* The following routines' purpose is to eliminate all but the
   canonical linkage (of a collection of linkages that are identical
   except for fat links).  An example of the problem is
   "I went to a talk and ate lunch".  Without the canonical checker
   this has two linkages with identical structure.

   We restrict our attention to a collection of linkages that are all
   isomorphic.  Consider the set of all disjuncts that are used on one
   word (over the collection of linkages).  This set is closed under GCD,
   since two linkages could both be used in that position, then so could
   their GCD.  The GCD has been constructed and put in the label table.

   The canonical linkage is the one in which the minimal disjunct that
   ever occurrs in a position is used in that position.  It is easy to
   prove that a disjunct is not canonical -- just find one of it's fat
   disjuncts that can be replaced by a smaller one.  If this can not be
   done, then the linkage is canonical.

   The algorithm uses link_array[] and chosen_disjuncts[] as input to
   describe the linkage, and also uses the label_table.

   (1) find all the words with fat disjuncts
   (2) scan all links and build, for each fat disjucnt used,
        an "image" structure that contains what this disjunct must
        connect to in the rest of the linkage.
   (3) For each fat disjunct, run through the label_table for disjuncts
        with the same label, considering only those with strictly more
        restricted match sets (this uses the string fields of the disjuncts
        from the table).
   (4) For each that passes this test, we see if it can replace the chosen
        disjunct.  This is performed by examining how this disjunct
        compares with the image structure for this word.
*/

struct Image_node_struct {
	Image_node * next;
	Connector * c;  /* the connector the place on the disjunct must match */
	int place;	  /* Indicates the place in the fat disjunct where this
					   connector must connect.  If 0 then this is a fat
					   connector.  If >0 then go place to the right, if
					   <0 then go -place to the left. */
};

/**
 * Fill in the has_fat_down array.  Uses link_array[].
 * Returns TRUE if there exists at least one word with a
 * fat down label.
 */
int set_has_fat_down(Sentence sent)
{
	int link, w, N_fat;
	Parse_info pi = sent->parse_info;

	N_fat = 0;

	for (w = 0; w < pi->N_words; w++)
	{
		pi->has_fat_down[w] = FALSE;
	}

	for (link = 0; link < pi->N_links; link++)
	{
		if (pi->link_array[link].lc->priority == DOWN_priority)
		{
			N_fat ++;
			pi->has_fat_down[pi->link_array[link].l] = TRUE;
		}
		else if (pi->link_array[link].rc->priority == DOWN_priority)
		{
			N_fat ++;
			pi->has_fat_down[pi->link_array[link].r] = TRUE;
		}
	}
	return (N_fat > 0);
}

static void free_image_array(Parse_info pi)
{
	int w;
	Image_node * in, * inx;
	for (w = 0; w < pi->N_words; w++)
	{
		for (in = pi->image_array[w]; in != NULL; in = inx)
		{
			inx = in->next;
			xfree((char *)in, sizeof(Image_node));
		}
		pi->image_array[w] = NULL;
	}
}

/**
 * Uses link_array, chosen_disjuncts, and down_label to construct
 * image_array
 */
static void build_image_array(Sentence sent)
{
	int link, end, word;
	Connector * this_end_con, *other_end_con, * upcon, * updiscon, *clist;
	Disjunct * dis, * updis;
	Image_node * in;
	Parse_info pi = sent->parse_info;

	for (word=0; word<pi->N_words; word++)
	{
		pi->image_array[word] = NULL;
	}

	for (end = -1; end <= 1; end += 2)
	{
		for (link = 0; link < pi->N_links; link++)
		{
			if (end < 0)
			{
				word = pi->link_array[link].l;
				if (!pi->has_fat_down[word]) continue;
				this_end_con = pi->link_array[link].lc;
				other_end_con = pi->link_array[link].rc;
				dis = pi->chosen_disjuncts[word];
				clist = dis->right;
			}
			else
			{
				word = pi->link_array[link].r;
				if (!pi->has_fat_down[word]) continue;
				this_end_con = pi->link_array[link].rc;
				other_end_con = pi->link_array[link].lc;
				dis = pi->chosen_disjuncts[word];
				clist = dis->left;
			}

			if (this_end_con->priority == DOWN_priority) continue;
			if ((this_end_con->label != NORMAL_LABEL) &&
				(this_end_con->label < 0)) continue;
			/* no need to construct an image node for down links,
			   or commas links or either/neither links */

			in = (Image_node *) xalloc(sizeof(Image_node));
			in->next = pi->image_array[word];
			pi->image_array[word] = in;
			in->c = other_end_con;

			/* the rest of this code is for computing in->place */
			if (this_end_con->priority == UP_priority)
			{
				in->place = 0;
			}
			else
			{
				in->place = 1;
				if ((dis->left != NULL) &&
					(dis->left->priority == UP_priority))
				{
					upcon = dis->left;
				}
				else if ((dis->right != NULL) &&
						   (dis->right->priority == UP_priority))
				{
					upcon = dis->right;
				}
				else
				{
					upcon = NULL;
				}
				if (upcon != NULL)
				{
					/* add on extra for a fat up link */
					updis = sent->and_data.label_table[upcon->label];
					if (end > 0)
					{
						updiscon = updis->left;
					}
					else
					{
						updiscon = updis->right;
					}
					for (;updiscon != NULL; updiscon = updiscon->next)
					{
						in->place ++;
					}
				}
				for (; clist != this_end_con; clist = clist->next)
				{
					if (clist->label < 0) in->place++;
				}
				in->place = in->place * (-end);
			}
		}
	}
}

/**
 * returns TRUE if string s represents a strictly smaller match set
 * than does t
 */
static int strictly_smaller(const char * s, const char * t)
{
	int strictness;
	strictness = 0;
	for (;(*s!='\0') && (*t!='\0'); s++,t++) {
		if (*s == *t) continue;
		if ((*t == '*') || (*s == '^')) {
			strictness++;
		} else {
			return FALSE;
		}
	}
	assert(! ((*s!='\0') || (*t!='\0')), "s and t should be the same length!");
	return (strictness > 0);
}

/**
 * dis points to a disjunct in the label_table.  label is the label
 * of a different set of disjuncts.  These can be derived from the label
 * of dis.  Find the specific disjunct of in label_table[label]
 * which corresponds to dis.
 */
static Disjunct * find_subdisjunct(Sentence sent, Disjunct * dis, int label)
{
	Disjunct * d;
	Connector * cx, *cy;
	for (d=sent->and_data.label_table[label]; d!=NULL; d=d->next)
	{
		for (cx=d->left, cy=dis->left; cx!=NULL; cx=cx->next,cy=cy->next)
		{
/*			if ((cx->string != cy->string) || */
			if ((strcmp(connector_get_string(cx),
			            connector_get_string(cy)) != 0) ||
				(cx->multi != cy->multi)) break;/* have to check multi? */
		}
		if (cx!=NULL) continue;
		for (cx=d->right, cy=dis->right; cx!=NULL; cx=cx->next,cy=cy->next)
		{
/*			if ((cx->string != cy->string) || */
			if ((strcmp(connector_get_string(cx),
			            connector_get_string(cy)) != 0) ||
				(cx->multi != cy->multi)) break;
		}
		if (cx==NULL) break;
	}
	assert(d!=NULL, "Never found subdisjunct");
	return d;
}

/**
 * is_canonical_linkage --
 * This uses link_array[], chosen_disjuncts[], has_fat_down[].
 * It assumes that there is a fat link in the current linkage.
 * See the comments above for more information about how it works
 */
int is_canonical_linkage(Sentence sent)
{
	int w, d_label=0, place;
	Connector *d_c, *c, dummy_connector, *upcon;
	Disjunct *dis, *chosen_d;
	Image_node * in;
	Parse_info pi = sent->parse_info;

	init_connector(&dummy_connector);
	dummy_connector.priority = UP_priority;

	build_image_array(sent);

	for (w=0; w<pi->N_words; w++)
	{
		if (!pi->has_fat_down[w]) continue;
		chosen_d = pi->chosen_disjuncts[w];

		/* there must be a down connector in both the left and right list */
		for (d_c = chosen_d->left; d_c!=NULL; d_c=d_c->next)
		{
			if (d_c->priority == DOWN_priority)
			{
				d_label = d_c->label;
				break;
			}
		}
		assert(d_c != NULL, "Should have found the down link.");

		if ((chosen_d->left != NULL) &&
			(chosen_d->left->priority == UP_priority)) {
			upcon = chosen_d->left;
		} else if ((chosen_d->right != NULL) &&
				   (chosen_d->right->priority == UP_priority)) {
			upcon = chosen_d->right;
		} else {
			upcon = NULL;
		}

		/* check that the disjunct on w is minimal (canonical) */

		for (dis=sent->and_data.label_table[d_label]; dis!=NULL; dis=dis->next)
		{
			/* now, reject a disjunct if it's not strictly below the old */
			if(!strictly_smaller(dis->string, 
			                     connector_get_string(d_c))) continue;

			/* Now, it has to match the image connectors */
			for (in = pi->image_array[w]; in != NULL; in = in->next)
			{
				place = in->place;
				if (place == 0)
				{
					assert(upcon != NULL, "Should have found an up link");
					dummy_connector.label = upcon->label;

					/* now we have to compute the string of the
					   disjunct with upcon->label that corresponds
					   to dis  */
					if (upcon->label == d_label)
					{
						connector_set_string(&dummy_connector, dis->string);
					} else {
						connector_set_string(&dummy_connector,
						  find_subdisjunct(sent, dis, upcon->label)->string);
					}

					/* I hope using x_match here is right */
					if (!x_match(sent, &dummy_connector, in->c)) break;  
				} else if (place > 0) {
					for (c=dis->right; place > 1; place--) {
						c = c->next;
					}
					if (!x_match(sent, c, in->c)) break;	/* Ditto above comment  --DS 07/97*/
				} else {
					for (c=dis->left; place < -1; place++) {
						c = c->next;
					}
					if (!x_match(sent, c, in->c)) break;  /* Ditto Ditto */
				}
			}

			if (in == NULL) break;
		}
		if (dis != NULL) break;
		/* there is a better disjunct that the one we're using, so this
		   word is bad, so we're done */
	}
	free_image_array(pi);
	return (w == pi->N_words);
}

/**
 * This takes as input link_array[], sublinkage->link[]->l and
 * sublinkage->link[]->r (and also has_fat_down[word], which has been
 * computed in a prior call to is_canonical()), and from these
 * computes sublinkage->link[].lc and .rc.  We assume these have
 * been initialized with the values from link_array.  We also assume
 * that there are fat links.
 */
void compute_pp_link_array_connectors(Sentence sent, Sublinkage *sublinkage)
{
	int link, end, word, place;
	Connector * this_end_con, * upcon, * updiscon, *clist, *con, *mycon;
	Disjunct * dis, * updis, *mydis;
	Parse_info pi = sent->parse_info;

	for (end = -1; end <= 1; end += 2)
	{
		for (link=0; link<pi->N_links; link++)
		{
			if (sublinkage->link[link]->l == -1) continue;
			if (end < 0)
			{
				word = pi->link_array[link].l;
				if (!pi->has_fat_down[word]) continue;
				this_end_con = pi->link_array[link].lc;
				dis = pi->chosen_disjuncts[word];
				mydis = pi->chosen_disjuncts[sublinkage->link[link]->l];
				clist = dis->right;
			}
			else
			{
				word = pi->link_array[link].r;
				if (!pi->has_fat_down[word]) continue;
				this_end_con = pi->link_array[link].rc;
				dis = pi->chosen_disjuncts[word];
				mydis = pi->chosen_disjuncts[sublinkage->link[link]->r];
				clist = dis->left;
			}

			if (this_end_con->label != NORMAL_LABEL) continue;
			/* no need to construct a connector for up links,
			   or commas links or either/neither links */

			/* Now compute the place */
			place = 0;
			if ((dis->left != NULL) &&
				(dis->left->priority == UP_priority)) {
				upcon = dis->left;
			} else if ((dis->right != NULL) &&
					   (dis->right->priority == UP_priority)) {
				upcon = dis->right;
			} else {
				upcon = NULL;
			}
			if (upcon != NULL) { /* add on extra for a fat up link */
				updis = sent->and_data.label_table[upcon->label];
				if (end > 0) {
					updiscon = updis->left;
				} else {
					updiscon = updis->right;
				}
				for (;updiscon != NULL; updiscon = updiscon->next) {
					place ++;
				}
			}
			for (; clist != this_end_con; clist = clist->next) {
				if (clist->label < 0) place++;
			}
			/* place has just been computed */

			/* now find the right disjunct in the table */
			if ((mydis->left != NULL) &&
				(mydis->left->priority == UP_priority)) {
				mycon = mydis->left;
			} else if ((mydis->right != NULL) &&
					   (mydis->right->priority == UP_priority)) {
				mycon = mydis->right;
			} else {
				printf("word = %d\n", word);
				printf("fat link: [%d, %d]\n",
					   pi->link_array[link].l, pi->link_array[link].r);
				printf("thin link: [%d, %d]\n",
					   sublinkage->link[link]->l, sublinkage->link[link]->r);
				assert(FALSE, "There should be a fat UP link here");
			}

			for (dis=sent->and_data.label_table[mycon->label];
				 dis != NULL; dis=dis->next) {
				if (dis->string == connector_get_string(mycon)) break;
			}
			assert(dis!=NULL, "Should have found this connector string");
			/* the disjunct in the table has just been found */

			if (end < 0)
			{
			  for (con = dis->right; place > 0; place--, con=con->next) {}
				/* sublinkage->link[link]->lc = con; OLD CODE */
			  exfree_connectors(sublinkage->link[link]->lc);
			  sublinkage->link[link]->lc = excopy_connectors(con);
			}
			else
			{
				for (con = dis->left; place > 0; place--, con=con->next) {}
				/* sublinkage->link[link]->rc = con; OLD CODE */
				exfree_connectors(sublinkage->link[link]->rc);
				sublinkage->link[link]->rc = excopy_connectors(con);
			}
		}
	}
}
#endif /* USE_FAT_LINKAGES */

