/*
 * Miscellaneous utilities for dealing with word types.
 */

#include "link-includes.h"
#include "word-utils.h"
#include <stdio.h>

#define PAST_TENSE_FORM_MARKER "<marker-past>"
#define ENTITY_MARKER "<marker-entity>"

static Dict_node * lookup_list2 = NULL;

static void rdictionary_lookup2(Dict_node * dn, const char * s)
{
	/* see comment in dictionary_lookup below */
	int m;
	Dict_node * dn_new;
	if (dn == NULL) return;
	m = dict_match(s, dn->string);
	if (m >= 0) {
		rdictionary_lookup2(dn->right, s);
	}
	if (m == 0) {
		dn_new = (Dict_node*) xalloc(sizeof(Dict_node));
		*dn_new = *dn;
		dn_new->right = lookup_list2;
		lookup_list2 = dn_new;
	}
	if (m <= 0) {
		rdictionary_lookup2(dn->left, s);
	}
}

/** 
 * Returns a pointer to a lookup list of the words in the dictionary.
 * This list is made up of Dict_nodes, linked by their right pointers.
 * The node, file and string fields are copied from the dictionary.
 *
 * Freeing this list elsewhere is unnecessary, as long as the rest of
 * the program merely examines the list (doesn't change it)
 */

Dict_node * dictionary_lookup2(Dictionary dict, const char *s)
{
	free_lookup_list(lookup_list2);
	rdictionary_lookup2(dict->root, s);
	lookup_list2 = prune_lookup_list2(lookup_list2, s);
	return lookup_list2;
}


//1 for equal
//0 for unequal
static int exp_compare(Exp * e1, Exp * e2)
{
	E_list *el1, *el2;

	if ((e1 == NULL) && (e2 == NULL))
		return 1; //they are equal
	if ((e1 == NULL) || (e2 == NULL))
		return 0; //they are not equal
	if (e1->type != e2->type)
		return 0;
	if (e1->cost != e2->cost)
		return 0;
	if (e1->type == CONNECTOR_type) {
		if (e1->dir != e2->dir)
			return 0;
		//printf("%s %s\n",e1->u.string,e2->u.string);
		if (strcmp(e1->u.string,e2->u.string)!=0)
			return 0;
	} else {
		el1 = e1->u.l;
		el2 = e2->u.l;
		//while at least 1 is non-null
		for (;(el1!=NULL)||(el2!=NULL);) {
			//fail if 1 is null
			if ((el1==NULL)||(el2==NULL))
				return 0;
			//fail if they are not compared
			if (exp_compare(el1->e, el2->e) == 0)
				return 0;
			if (el1!=NULL)
				el1 = el1->next;
			if (el2!=NULL)
				el2 = el2->next;
		}
	}
	return 1; //if never returned 0, return 1
}

//1 if sub is non-NULL and contained in super
//0 otherwise
static int exp_contains(Exp * super, Exp * sub) {
	E_list * el;

	//printf("\nSUP:");
	//if (super!=NULL)
	//	print_expression(super);

	if (sub==NULL || super==NULL)
		return 0;
	if (exp_compare(sub,super)==1)
		return 1;
	if (super->type==CONNECTOR_type)
		return 0; //super is a leaf
	//proceed through supers children and return 1 if sub
	//is contained in any of them
	for(el = super->u.l; el!=NULL; el=el->next) {
		if (exp_contains(el->e, sub)==1)
			return 1;
	}
	return 0;
}

int dn_word_contains(Dict_node * w_dn, const char * macro, Dictionary dict)
{
	Dict_node *m_dn;
	m_dn = dictionary_lookup(dict, macro);

	if ((w_dn == NULL)||(m_dn == NULL))
		return 0;
	//printf("\n\nWORD:");
	//print_expression(w_dn->exp);
	//printf("\nMACR:\n");
	//print_expression(m_dn->exp);

	for (;w_dn != NULL; w_dn = w_dn->right) {
		if (exp_contains(w_dn->exp, m_dn->exp)==1)
			return 1;
	}

	return 0;
}

/** 
 * Return true if word's expression contains macro's expression, false otherwise.
 */
int word_contains(const char * word, const char * macro, Dictionary dict)
{
	Dict_node *w_dn;
	int ret;
	w_dn = dictionary_lookup2(dict, word);
	ret = dn_word_contains(w_dn, macro, dict);
	free_lookup_list(lookup_list2);
	return ret;
}

int is_past_tense_form(const char * str, Dictionary dict)
{
	if (word_contains(str, PAST_TENSE_FORM_MARKER, dict) == 1)
		return 1;
	return 0;
}

/**
 * is_entity - Return true if word is entity.
 * Entities are proper names (geographical names, 
 * names of people), street addresses, phone numbers,
 * etc.
 */
int is_entity(const char * str, Dictionary dict)
{
	if (word_contains(str, ENTITY_MARKER, dict) == 1)
		return 1;
	return 0;
}
