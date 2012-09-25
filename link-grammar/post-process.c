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

 /* see bottom of file for comments on post processing */

#include <stdarg.h>
#include <memory.h>
#include "api.h"
#include "post-process.h"
#include "error.h"

#define PP_MAX_DOMAINS 128

/**
 * post_process_match -- compare two link-types.
 *
 * string comparison in postprocessing. The first parameter is a
 * post-processing symbol. The second one is a connector name from a
 * link. The upper case parts must match. We imagine that the first
 * arg is padded with an infinite sequence of "#" and that the 2nd one
 * is padded with "*". "#" matches anything, but "*" is just like an
 * ordinary char for matching purposes. 
 */

int post_process_match(const char *s, const char *t)
{
	char c;
	while(isupper((int)*s) || isupper((int)*t))
	{
		if (*s != *t) return FALSE;
		s++;
		t++;
	}
	while (*s != '\0')
	{
		if (*s != '#')
		{
			if (*t == '\0') c = '*'; else c = *t;
			if (*s != c) return FALSE;
		}
		s++;
		if (*t != '\0') t++;
	}
	return TRUE;
}

/***************** utility routines (not exported) ***********************/

static int string_in_list(const char * s, const char * a[])
{
	/* returns FALSE if the string s does not match anything in
		 the array.	The array elements are post-processing symbols */
	int i;
	for (i=0; a[i] != NULL; i++)
		if (post_process_match(a[i], s)) return TRUE;
	return FALSE;
}

/** 
 * Return the name of the domain associated with the provided starting
 * link. Return -1 if link isn't associated with a domain.
 */
static int find_domain_name(Postprocessor *pp, const char *link)
{
	int i,domain;
	StartingLinkAndDomain *sllt = pp->knowledge->starting_link_lookup_table;
	for (i=0;;i++)
	{
		domain = sllt[i].domain;
		if (domain==-1) return -1;					/* hit the end-of-list sentinel */
		if (post_process_match(sllt[i].starting_link, link)) return domain;
	}
}

/** Returns TRUE if domain d1 is contained in domain d2 */
static int contained_in(const Domain * d1, const Domain * d2, 
                        const Sublinkage *sublinkage)
{
	char mark[MAX_LINKS];
	List_o_links * lol;
	memset(mark, 0, sublinkage->num_links*(sizeof mark[0]));
	for (lol=d2->lol; lol != NULL; lol = lol->next)
		mark[lol->link] = TRUE;
	for (lol=d1->lol; lol != NULL; lol = lol->next)
		if (!mark[lol->link]) return FALSE;
	return TRUE;
}

/** Returns the predicate "the given link is in the given domain" */
static int link_in_domain(int link, const Domain * d)
{
	List_o_links * lol;
	for (lol = d->lol; lol != NULL; lol = lol->next)
		if (lol->link == link) return TRUE;
	return FALSE;
}

/* #define CHECK_DOMAIN_NESTING */

#if defined(CHECK_DOMAIN_NESTING)
/* Although this is no longer used, I'm leaving the code here for future reference --DS 3/98 */

/* Returns TRUE if the domains actually form a properly nested structure */
static int check_domain_nesting(Postprocessor *pp, int num_links)
{
	Domain * d1, * d2;
	int counts[4];
	char mark[MAX_LINKS];
	List_o_links * lol;
	int i;
	for (d1=pp->pp_data.domain_array; d1 < pp->pp_data.domain_array + pp->pp_data.N_domains; d1++) {
		for (d2=d1+1; d2 < pp->pp_data.domain_array + pp->pp_data.N_domains; d2++) {
			memset(mark, 0, num_links*(sizeof mark[0]));
			for (lol=d2->lol; lol != NULL; lol = lol->next) {
		mark[lol->link] = 1;
			}
			for (lol=d1->lol; lol != NULL; lol = lol->next) {
		mark[lol->link] += 2;
			}
			counts[0] = counts[1] = counts[2] = counts[3] = 0;
			for (i=0; i<num_links; i++)
		counts[(int)mark[i]]++;/* (int) cast avoids compiler warning DS 7/97 */
			if ((counts[1] > 0) && (counts[2] > 0) && (counts[3] > 0))
		return FALSE;
		}
	}
	return TRUE;
}
#endif

/** 
 * Free the list of links pointed to by lol
 * (does not free any strings)
 */
static void free_List_o_links(List_o_links *lol)
{
	List_o_links * xlol;
	while(lol != NULL) {
		xlol = lol->next;
		xfree(lol, sizeof(List_o_links));
		lol = xlol;
	}
}

static void free_D_tree_leaves(DTreeLeaf *dtl)
{
	DTreeLeaf * xdtl;
	while(dtl != NULL) {
		xdtl = dtl->next;
		xfree(dtl, sizeof(DTreeLeaf));
		dtl = xdtl;
	}
}

/** 
 * Gets called after every invocation of post_process()
 */
void post_process_free_data(PP_data * ppd)
{
	int w, d;
	for (w = 0; w < ppd->length; w++)
	{
		free_List_o_links(ppd->word_links[w]);
		ppd->word_links[w] = NULL;
	}
	for (d = 0; d < ppd->N_domains; d++)
	{
		free_List_o_links(ppd->domain_array[d].lol);
		ppd->domain_array[d].lol = NULL;
		free_D_tree_leaves(ppd->domain_array[d].child);
		ppd->domain_array[d].child = NULL;
	}
	free_List_o_links(ppd->links_to_ignore);
	ppd->links_to_ignore = NULL;
}

#ifdef THIS_FUNCTION_IS_NOT_CURRENTLY_USED
static void connectivity_dfs(Postprocessor *pp, Sublinkage *sublinkage,
                             int w, pp_linkset *ls)
{
	List_o_links *lol;
	pp->visited[w] = TRUE;
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next)
	{
		if (!pp->visited[lol->word] &&
				!pp_linkset_match(ls, sublinkage->link[lol->link]->name))
			connectivity_dfs(pp, sublinkage, lol->word, ls);
	}
}
#endif /* THIS_FUNCTION_IS_NOT_CURRENTLY_USED */

static void mark_reachable_words(Postprocessor *pp, int w)
{
	List_o_links *lol;
	if (pp->visited[w]) return;
	pp->visited[w] = TRUE;
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next)
		mark_reachable_words(pp, lol->word);
}

/**
 * Returns true if the linkage is connected, considering words
 * that have at least one edge....this allows conjunctive sentences
 * not to be thrown out. */
static int is_connected(Postprocessor *pp)
{
	int i;
	for (i=0; i<pp->pp_data.length; i++)
		pp->visited[i] = (pp->pp_data.word_links[i] == NULL);
	mark_reachable_words(pp, 0);
	for (i=0; i<pp->pp_data.length; i++)
		if (!pp->visited[i]) return FALSE;
	return TRUE;
}


static void build_type_array(Postprocessor *pp)
{
	D_type_list * dtl;
	int d;
	List_o_links * lol;
	for (d=0; d<pp->pp_data.N_domains; d++)
	{
		for (lol=pp->pp_data.domain_array[d].lol; lol != NULL; lol = lol->next)
		{
			dtl = (D_type_list *) xalloc(sizeof(D_type_list));
			dtl->next = pp->pp_node->d_type_array[lol->link];
			pp->pp_node->d_type_array[lol->link] = dtl;
			dtl->type = pp->pp_data.domain_array[d].type;
		}
	}
}

void free_d_type(D_type_list * dtl)
{
	D_type_list * dtlx;
	for (; dtl!=NULL; dtl=dtlx) {
		dtlx = dtl->next;
		xfree((void*) dtl, sizeof(D_type_list));
	}
}

D_type_list * copy_d_type(const D_type_list * dtl)
{
	D_type_list *dtlx, *dtlcurr=NULL, *dtlhead=NULL;
	for (; dtl!=NULL; dtl=dtl->next)
	{
		dtlx = (D_type_list *) xalloc(sizeof(D_type_list));
		*dtlx = *dtl;
		if (dtlhead == NULL)
		{
			dtlhead = dtlx;
			dtlcurr = dtlx;
		}
		else
		{
			dtlcurr->next = dtlx;
			dtlcurr = dtlx;
		}
	}
	return dtlhead;
}

/** free the pp node from last time */
static void free_pp_node(Postprocessor *pp)
{
	int i;
	PP_node *ppn = pp->pp_node;
	pp->pp_node = NULL;
	if (ppn == NULL) return;

	for (i=0; i<MAX_LINKS; i++)
	{
		free_d_type(ppn->d_type_array[i]);
	}
	xfree((void*) ppn, sizeof(PP_node));
}


/** set up a fresh pp_node for later use */
static void alloc_pp_node(Postprocessor *pp)
{
	int i;
	pp->pp_node=(PP_node *) xalloc(sizeof(PP_node));
	pp->pp_node->violation = NULL;
	for (i=0; i<MAX_LINKS; i++)
		pp->pp_node->d_type_array[i] = NULL;
}

static void reset_pp_node(Postprocessor *pp)
{
	free_pp_node(pp);
	alloc_pp_node(pp);
}

/************************ rule application *******************************/

static int apply_rules(Postprocessor *pp,
							 int (applyfn) (Postprocessor *,Sublinkage *,pp_rule *),
							 Sublinkage *sublinkage,
							 pp_rule *rule_array,	
							 const char **msg)
{
	int i;
	for (i=0; (*msg=rule_array[i].msg)!=NULL; i++)
		if (!applyfn(pp, sublinkage, &(rule_array[i]))) return 0;
	return 1;
}

static int
apply_relevant_rules(Postprocessor *pp,
						 int(applyfn)(Postprocessor *pp,Sublinkage*,pp_rule *rule),
						 Sublinkage *sublinkage,
						 pp_rule *rule_array,	
						 int *relevant_rules,
						 const char **msg)
{
	int i, idx;

	/* if we didn't accumulate link names for this sentence, we need to apply
		 all rules */
	if (pp_linkset_population(pp->set_of_links_of_sentence)==0) {
			return apply_rules(pp, applyfn, sublinkage, rule_array, msg);
	}

	/* we did, and we don't */
	for (i=0; (idx=relevant_rules[i])!=-1; i++) {
			*msg = rule_array[idx].msg;	 /* Adam had forgotten this -- DS 4/9/98 */
			if (!applyfn(pp, sublinkage, &(rule_array[idx]))) return 0;
	}
	return 1;
}

/**
 * returns TRUE if and only if all groups containing the specified link
 * contain at least one from the required list.	(as determined by exact
 * string matching)
 */
static int
apply_contains_one(Postprocessor *pp, Sublinkage *sublinkage, pp_rule *rule)
{
	DTreeLeaf * dtl;
	int d, count;
	for (d=0; d<pp->pp_data.N_domains; d++)
		{
			for (dtl = pp->pp_data.domain_array[d].child;
			 dtl != NULL &&
				 !post_process_match(rule->selector,
								 sublinkage->link[dtl->link]->name);
			 dtl = dtl->next) {}
			if (dtl != NULL)
		{
			/* selector link of rule appears in this domain */
			count=0;
			for (dtl = pp->pp_data.domain_array[d].child; dtl != NULL; dtl = dtl->next)
				if (string_in_list(sublinkage->link[dtl->link]->name,
									 rule->link_array))
					{
				count=1;
				break;
					}
			if (count == 0) return FALSE;
		}
		}
	return TRUE;
}


/**
 * Returns TRUE if and only if:
 * all groups containing the selector link do not contain anything
 * from the link_array contained in the rule. Uses exact string matching.
 */
static int
apply_contains_none(Postprocessor *pp,Sublinkage *sublinkage,pp_rule *rule)
{
	DTreeLeaf * dtl;
	int d;
	for (d=0; d<pp->pp_data.N_domains; d++)
		{
			for (dtl = pp->pp_data.domain_array[d].child;
			 dtl != NULL &&
				 !post_process_match(rule->selector,
								 sublinkage->link[dtl->link]->name);
			 dtl = dtl->next) {}
			if (dtl != NULL)
		{
			/* selector link of rule appears in this domain */
			for (dtl = pp->pp_data.domain_array[d].child; dtl != NULL; dtl = dtl->next)
				if (string_in_list(sublinkage->link[dtl->link]->name,
									 rule->link_array))
					return FALSE;
		}
		}
	return TRUE;
}

/** 
 * Returns TRUE if and only if
 * (1) the sentence doesn't contain the selector link for the rule, or
 * (2) it does, and it also contains one or more from the rule's link set
 */
static int
apply_contains_one_globally(Postprocessor *pp,Sublinkage *sublinkage,pp_rule *rule)
{
	int i,j,count;
	for (i=0; i<sublinkage->num_links; i++) {
		if (sublinkage->link[i]->l == -1) continue;
		if (post_process_match(rule->selector,sublinkage->link[i]->name)) break;
	}
	if (i==sublinkage->num_links) return TRUE;

	/* selector link of rule appears in sentence */
	count=0;
	for (j=0; j<sublinkage->num_links && count==0; j++) {
		if (sublinkage->link[j]->l == -1) continue;
		if (string_in_list(sublinkage->link[j]->name, rule->link_array))
			{
		count=1;
		break;
			}
	}
	if (count==0) return FALSE; else return TRUE;
}

static int
apply_connected(Postprocessor *pp, Sublinkage *sublinkage, pp_rule *rule)
{
	/* There is actually just one (or none, if user didn't specify it)
		 rule asserting that linkage is connected. */
	if (!is_connected(pp)) return 0;
	return 1;										
}

#if 0
/* replaced in 3/98 with a slightly different algorithm shown below	---DS*/
static int
apply_connected_without(Postprocessor *pp,Sublinkage *sublinkage,pp_rule *rule)
{
	/* Returns true if the linkage is connected when ignoring the links
		 whose names are in the given list of link names.
		 Actually, what it does is this: it returns FALSE if the connectivity
		 of the subgraph reachable from word 0 changes as a result of deleting
		 these links. */
	int i;
	memset(pp->visited, 0, pp->pp_data.length*(sizeof pp->visited[0]));
	mark_reachable_words(pp, 0);
	for (i=0; i<pp->pp_data.length; i++)
		pp->visited[i] = !pp->visited[i];
	connectivity_dfs(pp, sublinkage, 0, rule->link_set);
	for (i=0; i<pp->pp_data.length; i++)
		if (pp->visited[i] == FALSE) return FALSE;
	return TRUE;
}
#else

/* Here's the new algorithm: For each link in the linkage that is in the
	 must_form_a_cycle list, we want to make sure that that link
	 is in a cycle.	We do this simply by deleting the link, then seeing if the
	 end points of that link are still connected.
*/

static void reachable_without_dfs(Postprocessor *pp, Sublinkage *sublinkage, int a, int b, int w) {
		 /* This is a depth first search of words reachable from w, excluding any direct edge
		between word a and word b. */
		List_o_links *lol;
		pp->visited[w] = TRUE;
		for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next) {
		if (!pp->visited[lol->word] && !(w == a && lol->word == b) && ! (w == b && lol->word == a)) {
				reachable_without_dfs(pp, sublinkage, a, b, lol->word);
		}
		}
}

/**
 * Returns TRUE if the linkage is connected when ignoring the links
 * whose names are in the given list of link names.
 * Actually, what it does is this: it returns FALSE if the connectivity
 * of the subgraph reachable from word 0 changes as a result of deleting
 * these links.
 */
static int
apply_must_form_a_cycle(Postprocessor *pp,Sublinkage *sublinkage,pp_rule *rule)
{
	List_o_links *lol;
	int w;
	for (w=0; w<pp->pp_data.length; w++) {
		for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next) {
				if (w > lol->word) continue;	/* only consider each edge once */
				if (!pp_linkset_match(rule->link_set, sublinkage->link[lol->link]->name)) continue;
				memset(pp->visited, 0, pp->pp_data.length*(sizeof pp->visited[0]));
				reachable_without_dfs(pp, sublinkage, w, lol->word, w);
				if (!pp->visited[lol->word]) return FALSE;
		}
	}

	for (lol = pp->pp_data.links_to_ignore; lol != NULL; lol = lol->next) {
		w = sublinkage->link[lol->link]->l;
		/* (w, lol->word) are the left and right ends of the edge we're considering */
		if (!pp_linkset_match(rule->link_set, sublinkage->link[lol->link]->name)) continue;
		memset(pp->visited, 0, pp->pp_data.length*(sizeof pp->visited[0]));
		reachable_without_dfs(pp, sublinkage, w, lol->word, w);
		if (!pp->visited[lol->word]) return FALSE;
	}

	return TRUE;
}

#endif

/**
 * Checks to see that all domains with this name have the property that
 * all of the words that touch a link in the domain are not to the left
 * of the root word of the domain.
 */
static int
apply_bounded(Postprocessor *pp, Sublinkage *sublinkage, pp_rule *rule)
{
	int d, lw, d_type;
	List_o_links * lol;
	d_type = rule->domain;
	for (d=0; d<pp->pp_data.N_domains; d++) {
		if (pp->pp_data.domain_array[d].type != d_type) continue;
		lw = sublinkage->link[pp->pp_data.domain_array[d].start_link]->l;
		for (lol = pp->pp_data.domain_array[d].lol; lol != NULL; lol = lol->next) {
			if (sublinkage->link[lol->link]->l < lw) return FALSE;
		}
	}
	return TRUE;
}

/**
 * fill in the pp->pp_data.word_links array with a list of words
 * neighboring each word (actually a list of links).	The dir fields
 * are not set, since this (after fat-link-extraction) is an
 * undirected graph.
 */
static void build_graph(Postprocessor *pp, Sublinkage *sublinkage)
{
	int i, link;
	List_o_links * lol;

	for (i=0; i<pp->pp_data.length; i++)
		pp->pp_data.word_links[i] = NULL;

	for (link=0; link<sublinkage->num_links; link++)
		{
			if (sublinkage->link[link]->l == -1) continue;
			if (pp_linkset_match(pp->knowledge->ignore_these_links, sublinkage->link[link]->name)) {
			lol = (List_o_links *) xalloc(sizeof(List_o_links));
			lol->next = pp->pp_data.links_to_ignore;
			pp->pp_data.links_to_ignore = lol;
			lol->link = link;
			lol->word = sublinkage->link[link]->r;
			continue;
			}

			lol = (List_o_links *) xalloc(sizeof(List_o_links));
			lol->next = pp->pp_data.word_links[sublinkage->link[link]->l];
			pp->pp_data.word_links[sublinkage->link[link]->l] = lol;
			lol->link = link;
			lol->word = sublinkage->link[link]->r;

			lol = (List_o_links *) xalloc(sizeof(List_o_links));
			lol->next = pp->pp_data.word_links[sublinkage->link[link]->r];
			pp->pp_data.word_links[sublinkage->link[link]->r] = lol;
			lol->link = link;
			lol->word = sublinkage->link[link]->l;
		}
}

static void setup_domain_array(Postprocessor *pp,
									 int n, const char *string, int start_link)
{
	/* set pp->visited[i] to FALSE */
	memset(pp->visited, 0, pp->pp_data.length*(sizeof pp->visited[0]));
	pp->pp_data.domain_array[n].string = string;
	pp->pp_data.domain_array[n].lol    = NULL;
	pp->pp_data.domain_array[n].size   = 0;
	pp->pp_data.domain_array[n].start_link = start_link;
}

static void add_link_to_domain(Postprocessor *pp, int link)
{
	List_o_links *lol;
	lol = (List_o_links *) xalloc(sizeof(List_o_links));
	lol->next = pp->pp_data.domain_array[pp->pp_data.N_domains].lol;
	pp->pp_data.domain_array[pp->pp_data.N_domains].lol = lol;
	pp->pp_data.domain_array[pp->pp_data.N_domains].size++;
	lol->link = link;
}

static void depth_first_search(Postprocessor *pp, Sublinkage *sublinkage,
									 int w, int root,int start_link)
{
	List_o_links *lol;
	pp->visited[w] = TRUE;
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next) {
		if (lol->word < w && lol->link != start_link) {
			add_link_to_domain(pp, lol->link);
		}
	}
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next) {
		if (!pp->visited[lol->word] && (lol->word != root) &&
		!(lol->word < root && lol->word < w &&
					pp_linkset_match(pp->knowledge->restricted_links,
							 sublinkage->link[lol->link]->name)))
			depth_first_search(pp, sublinkage, lol->word, root, start_link);
	}
}

static void bad_depth_first_search(Postprocessor *pp, Sublinkage *sublinkage,
									 int w, int root, int start_link)
{
	List_o_links * lol;
	pp->visited[w] = TRUE;
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next) {
		if ((lol->word < w)	&& (lol->link != start_link) && (w != root)) {
			add_link_to_domain(pp, lol->link);
		}
	}
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next) {
		if ((!pp->visited[lol->word]) && !(w == root && lol->word < w) &&
		!(lol->word < root && lol->word < w &&
					pp_linkset_match(pp->knowledge->restricted_links,
							 sublinkage->link[lol->link]->name)))
			bad_depth_first_search(pp, sublinkage, lol->word, root, start_link);
	}
}

static void d_depth_first_search(Postprocessor *pp, Sublinkage *sublinkage,
								 int w, int root, int right, int start_link)
{
	List_o_links * lol;
	pp->visited[w] = TRUE;
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next) {
		if ((lol->word < w) && (lol->link != start_link) && (w != root)) {
			add_link_to_domain(pp, lol->link);
		}
	}
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next) {
		if (!pp->visited[lol->word] && !(w == root && lol->word >= right) &&
		!(w == root && lol->word < root) &&
		!(lol->word < root && lol->word < w &&
					pp_linkset_match(pp->knowledge->restricted_links,
							 sublinkage->link[lol->link]->name)))
			d_depth_first_search(pp,sublinkage,lol->word,root,right,start_link);
	}
}

static void left_depth_first_search(Postprocessor *pp, Sublinkage *sublinkage,
															 int w, int right,int start_link)
{
	List_o_links *lol;
	pp->visited[w] = TRUE;
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next) {
		if (lol->word < w && lol->link != start_link) {
			add_link_to_domain(pp, lol->link);
		}
	}
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next) {
		if (!pp->visited[lol->word] && (lol->word != right))
			depth_first_search(pp, sublinkage, lol->word, right, start_link);
	}
}

static int domain_compare(const Domain * d1, const Domain * d2)
{ return (d1->size-d2->size); /* for sorting the domains by size */ }

static void build_domains(Postprocessor *pp, Sublinkage *sublinkage)
{
	int link, i, d;
	const char *s;
	pp->pp_data.N_domains = 0;

	for (link = 0; link<sublinkage->num_links; link++) {
		if (sublinkage->link[link]->l == -1) continue;
		s = sublinkage->link[link]->name;

		if (pp_linkset_match(pp->knowledge->ignore_these_links, s)) continue;
		if (pp_linkset_match(pp->knowledge->domain_starter_links, s))
			{
		setup_domain_array(pp, pp->pp_data.N_domains, s, link);
				if (pp_linkset_match(pp->knowledge->domain_contains_links, s))
			add_link_to_domain(pp, link);
		depth_first_search(pp,sublinkage,sublinkage->link[link]->r,
							 sublinkage->link[link]->l, link);

		pp->pp_data.N_domains++;
				assert(pp->pp_data.N_domains<PP_MAX_DOMAINS, "raise value of PP_MAX_DOMAINS");
			}
		else {
			if (pp_linkset_match(pp->knowledge->urfl_domain_starter_links,s))
		{
			setup_domain_array(pp, pp->pp_data.N_domains, s, link);
			/* always add the starter link to its urfl domain */
			add_link_to_domain(pp, link);
			bad_depth_first_search(pp,sublinkage,sublinkage->link[link]->r,
								 sublinkage->link[link]->l,link);
			pp->pp_data.N_domains++;
			assert(pp->pp_data.N_domains<PP_MAX_DOMAINS,"raise PP_MAX_DOMAINS value");
		}
			else
		if (pp_linkset_match(pp->knowledge->urfl_only_domain_starter_links,s))
			{
				setup_domain_array(pp, pp->pp_data.N_domains, s, link);
				/* do not add the starter link to its urfl_only domain */
				d_depth_first_search(pp,sublinkage, sublinkage->link[link]->l,
								 sublinkage->link[link]->l,
								 sublinkage->link[link]->r,link);
				pp->pp_data.N_domains++;
				assert(pp->pp_data.N_domains<PP_MAX_DOMAINS,"raise PP_MAX_DOMAINS value");
			}
		else
			if (pp_linkset_match(pp->knowledge->left_domain_starter_links,s))
				{
					setup_domain_array(pp, pp->pp_data.N_domains, s, link);
					/* do not add the starter link to a left domain */
					left_depth_first_search(pp,sublinkage, sublinkage->link[link]->l,
									 sublinkage->link[link]->r,link);
					pp->pp_data.N_domains++;
					assert(pp->pp_data.N_domains<PP_MAX_DOMAINS,"raise PP_MAX_DOMAINS value");
				}
		}
	}

	/* sort the domains by size */
	qsort((void *) pp->pp_data.domain_array,
		pp->pp_data.N_domains,
		sizeof(Domain),
		(int (*)(const void *, const void *)) domain_compare);

	/* sanity check: all links in all domains have a legal domain name */
	for (d=0; d<pp->pp_data.N_domains; d++) {
		i = find_domain_name(pp, pp->pp_data.domain_array[d].string);
		if (i == -1)
			 prt_error("Error: post_process(): Need an entry for %s in LINK_TYPE_TABLE",
					 pp->pp_data.domain_array[d].string);
		pp->pp_data.domain_array[d].type = i;
	}
}

static void build_domain_forest(Postprocessor *pp, Sublinkage *sublinkage)
{
	int d, d1, link;
	DTreeLeaf * dtl;
	if (pp->pp_data.N_domains > 0)
		pp->pp_data.domain_array[pp->pp_data.N_domains-1].parent = NULL;
	for (d=0; d < pp->pp_data.N_domains-1; d++) {
		for (d1 = d+1; d1 < pp->pp_data.N_domains; d1++) {
			if (contained_in(&pp->pp_data.domain_array[d],&pp->pp_data.domain_array[d1],sublinkage))
			{
				pp->pp_data.domain_array[d].parent = &pp->pp_data.domain_array[d1];
				break;
			}
		}
		if (d1 == pp->pp_data.N_domains) {
			/* we know this domain is a root of a new tree */
			pp->pp_data.domain_array[d].parent = NULL;
			/* It's now ok for this to happen.	It used to do:
		 printf("I can't find a parent domain for this domain\n");
		 print_domain(d);
		 exit(1); */
		}
	}
	/* the parent links of domain nodes have been established.
		 now do the leaves */
	for (d=0; d < pp->pp_data.N_domains; d++) {
		pp->pp_data.domain_array[d].child = NULL;
	}
	for (link=0; link < sublinkage->num_links; link++) {
		if (sublinkage->link[link]->l == -1) continue; /* probably not necessary */
		for (d=0; d<pp->pp_data.N_domains; d++) {
			if (link_in_domain(link, &pp->pp_data.domain_array[d])) {
				dtl = (DTreeLeaf *) xalloc(sizeof(DTreeLeaf));
				dtl->link = link;
				dtl->parent = &pp->pp_data.domain_array[d];
				dtl->next = pp->pp_data.domain_array[d].child;
				pp->pp_data.domain_array[d].child = dtl;
				break;
			}
		}
	}
}

static int
internal_process(Postprocessor *pp, Sublinkage *sublinkage, const char **msg)
{
	int i;
	/* quick test: try applying just the relevant global rules */
	if (!apply_relevant_rules(pp,apply_contains_one_globally,
								sublinkage,
								pp->knowledge->contains_one_rules,
								pp->relevant_contains_one_rules, msg)) {
		for (i=0; i<pp->pp_data.length; i++)
			pp->pp_data.word_links[i] = NULL;
		pp->pp_data.N_domains = 0;
		return -1;
	}

	/* build graph; confirm that it's legally connected */
	build_graph(pp, sublinkage);
	build_domains(pp, sublinkage);
	build_domain_forest(pp, sublinkage);

#if defined(CHECK_DOMAIN_NESTING)
	/* These messages were deemed to not be useful, so
		 this code is commented out.	See comment above. */
	if(!check_domain_nesting(pp, sublinkage->num_links))
			printf("WARNING: The domains are not nested.\n");
#endif

	/* The order below should be optimal for most cases */
	if (!apply_relevant_rules(pp,apply_contains_one, sublinkage,
								pp->knowledge->contains_one_rules,
								pp->relevant_contains_one_rules, msg))	return 1;
	if (!apply_relevant_rules(pp,apply_contains_none, sublinkage,
								pp->knowledge->contains_none_rules,
									pp->relevant_contains_none_rules, msg)) return 1;
	if (!apply_rules(pp,apply_must_form_a_cycle, sublinkage,
					 pp->knowledge->form_a_cycle_rules,msg))		 return 1;
	if (!apply_rules(pp,apply_connected, sublinkage,
					 pp->knowledge->connected_rules, msg))						return 1;
	if (!apply_rules(pp,apply_bounded, sublinkage,
					 pp->knowledge->bounded_rules, msg))							return 1;
	return 0; /* This linkage satisfied all the rules */
}


/**
 * Call this (a) after having called post_process_scan_linkage() on all
 * generated linkages, but (b) before calling post_process() on any
 * particular linkage. Here we mark all rules which we know (from having
 * accumulated a set of link names appearing in *any* linkage) won't
 * ever be needed.
 */
static void prune_irrelevant_rules(Postprocessor *pp)
{
	 pp_rule *rule;
	 int coIDX, cnIDX, rcoIDX=0, rcnIDX=0;

	/* If we didn't scan any linkages, there's no pruning to be done. */
	if (pp_linkset_population(pp->set_of_links_of_sentence)==0) return;

	for (coIDX=0;;coIDX++)
	{
		rule = &(pp->knowledge->contains_one_rules[coIDX]);
		if (rule->msg==NULL) break;
		if (pp_linkset_match_bw(pp->set_of_links_of_sentence, rule->selector))
		{
			/* mark rule as being relevant to this sentence */
			pp->relevant_contains_one_rules[rcoIDX++] = coIDX;
			pp_linkset_add(pp->set_of_links_in_an_active_rule, rule->selector);
		}
	}
	pp->relevant_contains_one_rules[rcoIDX] = -1;	/* end sentinel */
	
	for (cnIDX=0;;cnIDX++)
	{
		rule = &(pp->knowledge->contains_none_rules[cnIDX]);
		if (rule->msg==NULL) break;
		if (pp_linkset_match_bw(pp->set_of_links_of_sentence, rule->selector))
		{
			pp->relevant_contains_none_rules[rcnIDX++] = cnIDX;
			pp_linkset_add(pp->set_of_links_in_an_active_rule, rule->selector);
		}
	}
	pp->relevant_contains_none_rules[rcnIDX] = -1;

	if (verbosity > 1)
	{
		printf("Saw %i unique link names in all linkages.\n",
				pp_linkset_population(pp->set_of_links_of_sentence));
		printf("Using %i 'contains one' rules and %i 'contains none' rules\n",
			   rcoIDX, rcnIDX);
	}
}


/***************** definitions of exported functions ***********************/

/**
 * read rules from path and initialize the appropriate fields in
 * a postprocessor structure, a pointer to which is returned.
 */
Postprocessor * post_process_open(const char *path)
{
	Postprocessor *pp;
	if (path==NULL) return NULL;

	pp = (Postprocessor *) xalloc (sizeof(Postprocessor));
	pp->knowledge	= pp_knowledge_open(path);
	pp->sentence_link_name_set = string_set_create();
	pp->set_of_links_of_sentence = pp_linkset_open(1024);
	pp->set_of_links_in_an_active_rule = pp_linkset_open(1024);
	pp->relevant_contains_one_rules =
			(int *) xalloc ((pp->knowledge->n_contains_one_rules+1)
							*(sizeof pp->relevant_contains_one_rules[0]));
	pp->relevant_contains_none_rules =
			(int *) xalloc ((pp->knowledge->n_contains_none_rules+1)
							*(sizeof pp->relevant_contains_none_rules[0]));
	pp->relevant_contains_one_rules[0]	= -1;
	pp->relevant_contains_none_rules[0] = -1;	
	pp->pp_node = NULL;
	pp->pp_data.links_to_ignore = NULL;
	pp->n_local_rules_firing	= 0;
	pp->n_global_rules_firing = 0;
	return pp;
}

void post_process_close(Postprocessor *pp)
{
	/* frees up memory associated with pp, previously allocated by open */
	if (pp==NULL) return;
	string_set_delete(pp->sentence_link_name_set);
	pp_linkset_close(pp->set_of_links_of_sentence);
	pp_linkset_close(pp->set_of_links_in_an_active_rule);
	xfree(pp->relevant_contains_one_rules,
		(1+pp->knowledge->n_contains_one_rules)
		*(sizeof pp->relevant_contains_one_rules[0]));
	xfree(pp->relevant_contains_none_rules,
		(1+pp->knowledge->n_contains_none_rules)
		*(sizeof pp->relevant_contains_none_rules[0]));
	pp_knowledge_close(pp->knowledge);
	free_pp_node(pp);
	xfree(pp, sizeof(Postprocessor));
}

void post_process_close_sentence(Postprocessor *pp)
{
	if (pp==NULL) return;
	pp_linkset_clear(pp->set_of_links_of_sentence);
	pp_linkset_clear(pp->set_of_links_in_an_active_rule);
	string_set_delete(pp->sentence_link_name_set);
	pp->sentence_link_name_set = string_set_create();
	pp->n_local_rules_firing	= 0;
	pp->n_global_rules_firing = 0;
	pp->relevant_contains_one_rules[0]	= -1;
	pp->relevant_contains_none_rules[0] = -1;	
	free_pp_node(pp);
}

/**
 * During a first pass (prior to actual post-processing of the linkages
 * of a sentence), call this once for every generated linkage. Here we
 * simply maintain a set of "seen" link names for rule pruning later on
 */
void post_process_scan_linkage(Postprocessor *pp, Parse_Options opts,
									 Sentence sent, Sublinkage *sublinkage)
{
	const char *p;
	int i;
	if (pp == NULL) return;
	if (sent->length < opts->twopass_length) return;
	for (i=0; i<sublinkage->num_links; i++)
	{
		if (sublinkage->link[i]->l == -1) continue;
		p = string_set_add(sublinkage->link[i]->name, pp->sentence_link_name_set);
		pp_linkset_add(pp->set_of_links_of_sentence, p);
	}
}

/**
 * Takes a sublinkage and returns:
 *  . for each link, the domain structure of that link
 *  . a list of the violation strings
 * NB: sublinkage->link[i]->l=-1 means that this connector
 * is to be ignored
 */
PP_node *post_process(Postprocessor *pp, Parse_Options opts,
							Sentence sent, Sublinkage *sublinkage, int cleanup)
{
	const char *msg;

	if (pp==NULL) return NULL;

	pp->pp_data.links_to_ignore = NULL;
	pp->pp_data.length = sent->length;

	/* In the name of responsible memory management, we retain a copy of the
	 * returned data structure pp_node as a field in pp, so that we can clear
	 * it out after every call, without relying on the user to do so. */
	reset_pp_node(pp);

	/* The first time we see a sentence, prune the rules which we won't be
	 * needing during postprocessing the linkages of this sentence */
	if (sent->q_pruned_rules==FALSE && sent->length >= opts->twopass_length)
		prune_irrelevant_rules(pp);
	sent->q_pruned_rules=TRUE;

	switch(internal_process(pp, sublinkage, &msg))
	{
		case -1:
			/* some global test failed even before we had to build the domains */
			pp->n_global_rules_firing++;
			pp->pp_node->violation = msg;
			return pp->pp_node;
			break;
		case 1:
			/* one of the "normal" post processing tests failed */
			pp->n_local_rules_firing++;
			pp->pp_node->violation = msg;
			break;
		case 0:
			/* This linkage is legal according to the post processing rules */
			pp->pp_node->violation = NULL;
			break;
	}

	build_type_array(pp);
	if (cleanup)
	{
		post_process_free_data(&pp->pp_data);
	}
	return pp->pp_node;
}

/* OLD COMMENTS (OUT OF DATE):
  This file does the post-processing.
  The main routine is "post_process()".  It uses the link names only,
  and not the connectors.

  A domain is a set of links.  Each domain has a defining link.
  Only certain types of links serve to define a domain.  These
  parameters are set by the lists of link names in a separate,
  human-readable file referred to herein as the 'knowledge file.'

  The domains are nested: given two domains, either they're disjoint,
  or one contains the other, i.e. they're tree structured.  The set of links
  in a domain (but in no smaller domain) are called the "group" of the
  domain.  Data structures are built to store all this stuff.
  The tree structured property is not mathematically guaranteed by
  the domain construction algorithm.  Davy simply claims that because
  of how he built the dictionary, the domains will always be so
  structured.  The program checks this and gives an error message
  if it's violated.

  Define the "root word" of a link (or domain) to be the word at the
  left end of the link.  The other end of the defining link is called
  the "right word".

  The domain corresponding to a link is defined to be the set of links
  reachable by starting from the right word, following links and never
  using the root word or any word to its left.

  There are some minor exceptions to this.  The "restricted_link" lists
  those connectors that, even if they point back before the root word,
  are included in the domain.  Some of the starting links are included
  in their domain, these are listed in the "domain_contains_links" list.

  Such was the way it was.  Now Davy tells me there should be another type
  of domain that's quite different.  Let's call these "urfl" domains.
  Certain type of connectors start urfl domains.  They're listed below.
  In a urfl domain, the search includes the root word.  It does a separate
  search to find urfl domains.

  Restricted links should work just as they do with ordinary domains. If they
  come out of the right word, or anything to the right of it (that's
  in the domain), they should be included but should not be traced
  further. If they come out of the root word, they should not be
  included.
  */

/*
  I also, unfortunately, want to propose a new type of domain. These
  would include everything that can be reached from the root word of the
  link, to the right, that is closer than the right word of the link.
  (They would not include the link itself.)

  In the following sentence, then, the "Urfl_Only Domain" of the G link
  would include only the "O" link:

  +-----G----+
  +---O--+   +-AI+
  |      |   |   |
  hitting dogs is fun.a

  In the following sentence it would include the "O", the "TT", the "I",
  the second "O", and the "A".

  +----------------G---------------+
  +-----TT-----+  +-----O-----+    |
  +---O---+    +-I+    +---A--+    +-AI+
  |       |    |  |    |      |    |   |
  telling people to do stupid things is fun.a

  This would allow us to judge the following:

  kicking dogs bores me
  *kicking dogs kicks dogs
  explaining the program is easy
  *explaining the program is running

  (These are distinctions that I thought we would never be able to make,
  so I told myself they were semantic rather than syntactic. But with
  domains, they should be easy.)
  */

  /* Modifications, 6/96 ALB:
     1) Rules and link sets are relegated to a separate, user-written
        file(s), herein referred to as the 'knowledge file'
     2) This information is read by a lexer, in pp_lexer.l (lex code)
        whose exported routines are all prefixed by 'pp_lexer'
     3) when postprocessing a sentence, the links of each domain are
        placed in a set for quick lookup, ('contains one' and 'contains none')
     4) Functions which were never called have been eliminated:
        link_inhabits(), match_in_list(), group_type_contains(),
        group_type_contains_one(),  group_type_contains_all()
     5) Some 'one-by-one' initializations have been replaced by faster
        block memory operations (memset etc.)
     6) The above comments are correct but incomplete! (1/97)
     7) observation: the 'contains one' is, empirically, by far the most
        violated rule, so it should come first in applying the rules.

    Modifications, 9/97 ALB:
     Deglobalization. Made code constistent with api.
   */
