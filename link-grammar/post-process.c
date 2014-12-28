/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2014 Linas Vepstas                                      */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

 /* see bottom of file for comments on post processing */

#include <memory.h>
#include <stdint.h>

#include "post-process.h"

#include "api-structures.h"
#include "error.h"
#include "externs.h"
#include "pp_knowledge.h"
#include "pp_linkset.h"
#include "structures.h"
#include "string-set.h"

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

bool post_process_match(const char *s, const char *t)
{
	if (NULL == t) return false;
	if (islower((int)*t)) t++; /* Skip head-dependent indicator */
	while (isupper((int)*s) || isupper((int)*t))
	{
		if (*s != *t) return false;
		s++;
		t++;
	}
	while (*s != '\0')
	{
		if (*s != '#')
		{
			char c;
			if (*t == '\0') c = '*'; else c = *t;
			if (*s != c) return false;
		}
		s++;
		if (*t != '\0') t++;
	}
	return true;
}

/***************** utility routines (not exported) ***********************/

static int string_in_list(const char * s, const char * a[])
{
	/* returns false if the string s does not match anything in
		 the array.	The array elements are post-processing symbols */
	int i;
	for (i=0; a[i] != NULL; i++)
		if (post_process_match(a[i], s)) return true;
	return false;
}

/**
 * Return the name of the domain associated with the provided starting
 * link. Return -1 if link isn't associated with a domain.
 */
static size_t find_domain_name(Postprocessor *pp, const char *link)
{
	size_t i, domain;
	StartingLinkAndDomain *sllt = pp->knowledge->starting_link_lookup_table;
	for (i=0;;i++)
	{
		domain = sllt[i].domain;
		if (domain == SIZE_MAX) return SIZE_MAX;  /* hit the end-of-list sentinel */
		if (post_process_match(sllt[i].starting_link, link)) return domain;
	}
}

/** Returns true if domain d1 is contained in domain d2 */
static int contained_in(const Domain * d1, const Domain * d2,
                        const Linkage sublinkage)
{
	bool *mark = alloca(sublinkage->num_links*sizeof(bool));
	List_o_links * lol;
	memset(mark, 0, sublinkage->num_links*(sizeof(bool)));
	for (lol=d2->lol; lol != NULL; lol = lol->next)
		mark[lol->link] = true;
	for (lol=d1->lol; lol != NULL; lol = lol->next)
		if (!mark[lol->link]) return false;
	return true;
}

/** Returns the predicate "the given link is in the given domain" */
static bool link_in_domain(size_t link, const Domain * d)
{
	List_o_links * lol;
	for (lol = d->lol; lol != NULL; lol = lol->next)
		if (lol->link == link) return true;
	return false;
}

/* #define CHECK_DOMAIN_NESTING */

#if defined(CHECK_DOMAIN_NESTING)
/* Although this is no longer used, I'm leaving the code here for future reference --DS 3/98 */

/* Returns true if the domains actually form a properly nested structure */
static bool check_domain_nesting(Postprocessor *pp, int num_links)
{
	Domain * d1, * d2;
	int counts[4];
	char mark[MAX_NUM_LINKS];
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
		return false;
		}
	}
	return true;
}
#endif

/**
 * Free the list of links pointed to by lol
 * (does not free any strings)
 */
static void free_List_o_links(List_o_links *lol)
{
	List_o_links * xlol;
	while (lol != NULL)
	{
		xlol = lol->next;
		free(lol);
		lol = xlol;
	}
}

static void free_D_tree_leaves(DTreeLeaf *dtl)
{
	DTreeLeaf * xdtl;
	while (dtl != NULL)
	{
		xdtl = dtl->next;
		free(dtl);
		dtl = xdtl;
	}
}

/**
 * Gets called after every invocation of post_process()
 */
void post_process_free_data(PP_data * ppd)
{
	size_t w, d;
	for (w = 0; w < ppd->wowlen; w++)
	{
		free_List_o_links(ppd->word_links[w]);
		ppd->word_links[w] = NULL;
	}

	for (d = 0; d < ppd->domlen; d++)
	{
		free_List_o_links(ppd->domain_array[d].lol);
		ppd->domain_array[d].lol = NULL;
		free_D_tree_leaves(ppd->domain_array[d].child);
		ppd->domain_array[d].child = NULL;
	}
	free_List_o_links(ppd->links_to_ignore);
	ppd->links_to_ignore = NULL;
	ppd->length = 0;
	ppd->N_domains = 0;
}

#ifdef THIS_FUNCTION_IS_NOT_CURRENTLY_USED
static void connectivity_dfs(Postprocessor *pp, Linkage sublinkage,
                             int w, pp_linkset *ls)
{
	List_o_links *lol;
	assert(w < pp->pp_data.length, "Bad word index");
	pp->visited[w] = true;
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next)
	{
		if (!pp->visited[lol->word] &&
				!pp_linkset_match(ls, sublinkage->link[lol->link]->name))
			connectivity_dfs(pp, sublinkage, lol->word, ls);
	}
}
#endif /* THIS_FUNCTION_IS_NOT_CURRENTLY_USED */

static void chk_d_type(PP_node* ppn, size_t idx)
{
	if (ppn->dtsz <= idx)
	{
		ppn->dtsz += idx + 5;
		ppn->d_type_array = realloc(ppn->d_type_array,
			ppn->dtsz * sizeof(D_type_list*));
	}
}

static void build_type_array(Postprocessor *pp)
{
	D_type_list * dtl;
	size_t d;
	List_o_links * lol;
	for (d = 0; d < pp->pp_data.N_domains; d++)
	{
		for (lol = pp->pp_data.domain_array[d].lol; lol != NULL; lol = lol->next)
		{
			chk_d_type(pp->pp_node, lol->link);
			dtl = (D_type_list *) malloc(sizeof(D_type_list));
			dtl->next = pp->pp_node->d_type_array[lol->link];
			dtl->type = pp->pp_data.domain_array[d].type;
			pp->pp_node->d_type_array[lol->link] = dtl;
		}
	}
}

static void free_d_type(D_type_list * dtl)
{
	D_type_list * dtlx;
	for (; dtl != NULL; dtl = dtlx)
	{
		dtlx = dtl->next;
		free((void*) dtl);
	}
}

/** free the pp node from last time */
static void free_pp_node(Postprocessor *pp)
{
	size_t i;
	PP_node *ppn = pp->pp_node;
	pp->pp_node = NULL;
	if (ppn == NULL) return;

	for (i=0; i<ppn->dtsz; i++)
	{
		free_d_type(ppn->d_type_array[i]);
	}
	free(ppn->d_type_array);
	free((void*) ppn);
}

/** set up a fresh pp_node for later use */
static void alloc_pp_node(Postprocessor *pp)
{
	size_t dz;
	PP_node *ppn = (PP_node *) malloc(sizeof(PP_node));

	/* highly unlikely that the number of links will ever exceed this */
	ppn->dtsz = 2 * pp->pp_data.length;
	dz = ppn->dtsz * sizeof(D_type_list*);
	ppn->d_type_array = (D_type_list **) malloc (dz);
	memset(ppn->d_type_array, 0, dz);

	pp->pp_node = ppn;
}

static void clear_pp_node(Postprocessor *pp)
{
	size_t i;
	PP_node *ppn = pp->pp_node;
	if (NULL == ppn) { alloc_pp_node(pp); ppn = pp->pp_node; }

	ppn->violation = NULL;
	for (i=0; i<ppn->dtsz; i++)
	{
		free_d_type(ppn->d_type_array[i]);
		ppn->d_type_array[i] = NULL;
	}
}


/************************ rule application *******************************/

static bool apply_rules(Postprocessor *pp,
                        bool (applyfn) (Postprocessor *, Linkage, pp_rule *),
                        Linkage sublinkage,
                        pp_rule *rule_array,
                        const char **msg)
{
	int i;
	for (i = 0; (*msg = rule_array[i].msg) != NULL; i++)
	{
		if (!applyfn(pp, sublinkage, &(rule_array[i])))
		{
			rule_array[i].use_count ++;
			return false;
		}
	}
	return true;
}

static bool
apply_relevant_rules(Postprocessor *pp,
                     bool (applyfn)(Postprocessor *, Linkage, pp_rule *),
                     Linkage sublinkage,
                     pp_rule *rule_array,
                     int *relevant_rules,
                     const char **msg)
{
	int i, idx;

	/* if we didn't accumulate link names for this sentence, we need to apply
		 all rules */
	if (pp_linkset_population(pp->set_of_links_of_sentence) == 0) {
		return apply_rules(pp, applyfn, sublinkage, rule_array, msg);
	}

	/* we did, and we don't */
	for (i = 0; (idx = relevant_rules[i]) != -1; i++)
	{
		*msg = rule_array[idx].msg;
		if (!applyfn(pp, sublinkage, &(rule_array[idx]))) return false;
	}
	return true;
}

/**
 * returns true if and only if all groups containing the specified link
 * contain at least one from the required list.	(as determined by exact
 * string matching)
 */
static bool
apply_contains_one(Postprocessor *pp, Linkage sublinkage, pp_rule *rule)
{
	DTreeLeaf * dtl;
	size_t d, count;
	for (d=0; d<pp->pp_data.N_domains; d++)
	{
		for (dtl = pp->pp_data.domain_array[d].child;
		     dtl != NULL &&
		        !post_process_match(rule->selector,
		           sublinkage->link_array[dtl->link].link_name);
		     dtl = dtl->next) {}
		if (dtl != NULL)
		{
			/* selector link of rule appears in this domain */
			count=0;
			for (dtl = pp->pp_data.domain_array[d].child; dtl != NULL; dtl = dtl->next)
			{
				if (string_in_list(sublinkage->link_array[dtl->link].link_name,
									 rule->link_array))
				{
					count=1;
					break;
				}
			}
			if (count == 0) return false;
		}
	}
	return true;
}


/**
 * Returns true if and only if:
 * all groups containing the selector link do not contain anything
 * from the link_array contained in the rule. Uses exact string matching.
 */
static bool
apply_contains_none(Postprocessor *pp, Linkage sublinkage, pp_rule *rule)
{
	size_t d;
	for (d=0; d<pp->pp_data.N_domains; d++)
	{
		DTreeLeaf * dtl;
		for (dtl = pp->pp_data.domain_array[d].child;
		     dtl != NULL &&
		         !post_process_match(rule->selector,
		                  sublinkage->link_array[dtl->link].link_name);
		     dtl = dtl->next) {}
		if (dtl != NULL)
		{
			/* selector link of rule appears in this domain */
			for (dtl = pp->pp_data.domain_array[d].child; dtl != NULL; dtl = dtl->next)
			{
				if (string_in_list(sublinkage->link_array[dtl->link].link_name,
				                   rule->link_array))
					return false;
			}
		}
	}
	return true;
}

/**
 * Returns true if and only if
 * (1) the sentence doesn't contain the selector link for the rule, or
 * (2) it does, and it also contains one or more from the rule's link set
 */
static bool
apply_contains_one_globally(Postprocessor *pp, Linkage sublinkage, pp_rule *rule)
{
	size_t i, j, count;
	for (i = 0; i < sublinkage->num_links; i++)
	{
		assert(sublinkage->link_array[i].lw != SIZE_MAX);
		if (post_process_match(rule->selector, sublinkage->link_array[i].link_name)) break;
	}
	if (i == sublinkage->num_links) return true;

	/* selector link of rule appears in sentence */
	count = 0;
	for (j = 0; j < sublinkage->num_links && count == 0; j++)
	{
		assert(sublinkage->link_array[j].lw != SIZE_MAX);
		if (string_in_list(sublinkage->link_array[j].link_name, rule->link_array))
		{
			count = 1;
			break;
		}
	}
	if (count == 0) return false; else return true;
}

/**
 * For each link in the linkage that is in the must_form_a_cycle list,
 * we want to make sure that that link is in a cycle.  We do this
 * simply by deleting the link, then seeing if the end points of that
 * link are still connected.
 */
static void reachable_without_dfs(Postprocessor *pp,
                    Linkage sublinkage, size_t a, size_t b, size_t w)
{
	/* This is a depth first search of words reachable from w, excluding
	 * any direct edge between word a and word b. */
	List_o_links *lol;
	assert(w < pp->pp_data.length, "Bad word index");
	pp->visited[w] = true;
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next)
	{
		assert(lol->word < pp->pp_data.length, "Bad word index");
		if (!pp->visited[lol->word] &&
		    !(w == a && lol->word == b) &&
		    !(w == b && lol->word == a))
		{
				reachable_without_dfs(pp, sublinkage, a, b, lol->word);
		}
	}
}

/**
 * Returns true if the linkage is connected when ignoring the links
 * whose names are in the given list of link names.
 * Actually, what it does is this: it returns false if the connectivity
 * of the subgraph reachable from word 0 changes as a result of deleting
 * these links.
 */
static bool
apply_must_form_a_cycle(Postprocessor *pp, Linkage sublinkage, pp_rule *rule)
{
	List_o_links *lol;
	size_t w;
	for (w = 0; w < pp->pp_data.length; w++)
	{
		for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next)
		{
			if (w > lol->word) continue;	/* only consider each edge once */
			if (!pp_linkset_match(rule->link_set, sublinkage->link_array[lol->link].link_name)) continue;

			memset(pp->visited, 0, pp->pp_data.length*(sizeof pp->visited[0]));
			reachable_without_dfs(pp, sublinkage, w, lol->word, w);
			if (!pp->visited[lol->word]) return false;
		}
	}

	for (lol = pp->pp_data.links_to_ignore; lol != NULL; lol = lol->next)
	{
		w = sublinkage->link_array[lol->link].lw;
		/* (w, lol->word) are the left and right ends of the edge we're considering */
		if (!pp_linkset_match(rule->link_set, sublinkage->link_array[lol->link].link_name)) continue;

		memset(pp->visited, 0, pp->pp_data.length*(sizeof pp->visited[0]));
		reachable_without_dfs(pp, sublinkage, w, lol->word, w);

		assert(lol->word < pp->pp_data.length, "Bad word index");
		if (!pp->visited[lol->word]) return false;
	}

	return true;
}

/**
 * Checks to see that all domains with this name have the property that
 * all of the words that touch a link in the domain are not to the left
 * of the root word of the domain.
 */
static bool
apply_bounded(Postprocessor *pp, Linkage sublinkage, pp_rule *rule)
{
	size_t d, lw;
	char d_type;
	List_o_links * lol;
	d_type = rule->domain;
	for (d = 0; d < pp->pp_data.N_domains; d++)
	{
		if (pp->pp_data.domain_array[d].type != d_type) continue;
		lw = sublinkage->link_array[pp->pp_data.domain_array[d].start_link].lw;
		for (lol = pp->pp_data.domain_array[d].lol; lol != NULL; lol = lol->next)
		{
			if (sublinkage->link_array[lol->link].lw < lw) return false;
		}
	}
	return true;
}

/**
 * fill in the pp->pp_data.word_links array with a list of words
 * neighboring each word (actually a list of links).	The dir fields
 * are not set, since this is an undirected graph.
 */
static void build_graph(Postprocessor *pp, Linkage sublinkage)
{
	size_t link;
	List_o_links * lol;

	/* Get more size, if needed */
	if (pp->pp_data.wowlen <= pp->pp_data.length)
	{
		size_t newsz;
		pp->pp_data.wowlen += pp->pp_data.length;
		newsz = pp->pp_data.wowlen * sizeof(List_o_links *);
		pp->pp_data.word_links = (List_o_links **) realloc(
			pp->pp_data.word_links, newsz);
	}
	memset(pp->pp_data.word_links, 0, pp->pp_data.wowlen * sizeof(List_o_links *));

	for (link = 0; link < sublinkage->num_links; link++)
	{
		assert (sublinkage->link_array[link].lw != SIZE_MAX);
		if (NULL == sublinkage->link_array[link].link_name) continue;
		if (pp_linkset_match(pp->knowledge->ignore_these_links,
		                     sublinkage->link_array[link].link_name))
		{
			lol = (List_o_links *) malloc(sizeof(List_o_links));
			lol->next = pp->pp_data.links_to_ignore;
			pp->pp_data.links_to_ignore = lol;
			lol->link = link;
			lol->word = sublinkage->link_array[link].rw;
			continue;
		}

		lol = (List_o_links *) malloc(sizeof(List_o_links));
		lol->next = pp->pp_data.word_links[sublinkage->link_array[link].lw];
		pp->pp_data.word_links[sublinkage->link_array[link].lw] = lol;
		lol->link = link;
		lol->word = sublinkage->link_array[link].rw;

		lol = (List_o_links *) malloc(sizeof(List_o_links));
		lol->next = pp->pp_data.word_links[sublinkage->link_array[link].rw];
		pp->pp_data.word_links[sublinkage->link_array[link].rw] = lol;
		lol->link = link;
		lol->word = sublinkage->link_array[link].lw;
	}
}

static void setup_domain_array(Postprocessor *pp, size_t n,
                               const char *string, int start_link)
{
	/* set pp->visited[i] to false */
	/* Grab more memory if needed */
	if (pp->vlength <= pp->pp_data.length)
	{
		size_t newsz;
		pp->vlength += pp->pp_data.length;
		newsz = pp->vlength * sizeof(bool);
		pp->visited = (bool *) realloc(pp->visited, newsz);
	}
	memset(pp->visited, 0, pp->pp_data.length*(sizeof pp->visited[0]));

	/* Grab more memory if needed */
	if (pp->pp_data.domlen <= n)
	{
		size_t newsz, incsz;
#define DOMINC 16
		pp->pp_data.domlen += DOMINC;
		newsz = pp->pp_data.domlen * sizeof(Domain);
		incsz = DOMINC * sizeof(Domain);
		pp->pp_data.domain_array = (Domain *) realloc(pp->pp_data.domain_array,
			newsz);
		memset(&pp->pp_data.domain_array[pp->pp_data.domlen], 0, incsz);
	}

	pp->pp_data.domain_array[n].string = string;
	pp->pp_data.domain_array[n].lol    = NULL;
	pp->pp_data.domain_array[n].size   = 0;
	pp->pp_data.domain_array[n].start_link = start_link;
}

static void add_link_to_domain(Postprocessor *pp, int link)
{
	List_o_links *lol;
	lol = (List_o_links *) malloc(sizeof(List_o_links));
	lol->next = pp->pp_data.domain_array[pp->pp_data.N_domains].lol;
	pp->pp_data.domain_array[pp->pp_data.N_domains].lol = lol;
	pp->pp_data.domain_array[pp->pp_data.N_domains].size++;
	lol->link = link;
}

static void depth_first_search(Postprocessor *pp, Linkage sublinkage,
                               size_t w, size_t root, size_t start_link)
{
	List_o_links *lol;
	assert(w < pp->pp_data.length, "Bad word index");
	pp->visited[w] = true;
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next)
	{
		if (lol->word < w && lol->link != start_link)
		{
			add_link_to_domain(pp, lol->link);
		}
	}
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next)
	{
		if (!pp->visited[lol->word] && (lol->word != root) &&
		       !(lol->word < root && lol->word < w &&
		       pp_linkset_match(pp->knowledge->restricted_links,
		                sublinkage->link_array[lol->link].link_name)))
		{
			depth_first_search(pp, sublinkage, lol->word, root, start_link);
		}
	}
}

static void bad_depth_first_search(Postprocessor *pp, Linkage sublinkage,
                                   size_t w, size_t root, size_t start_link)
{
	List_o_links * lol;
	assert(w < pp->pp_data.length, "Bad word index");
	pp->visited[w] = true;
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next)
	{
		if ((lol->word < w)	&& (lol->link != start_link) && (w != root))
		{
			add_link_to_domain(pp, lol->link);
		}
	}
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next)
	{
		assert(lol->word < pp->pp_data.length, "Bad word index");
		if ((!pp->visited[lol->word]) && !(w == root && lol->word < w) &&
		     !(lol->word < root && lol->word < w &&
		          pp_linkset_match(pp->knowledge->restricted_links,
		                sublinkage->link_array[lol->link].link_name)))
		{
			bad_depth_first_search(pp, sublinkage, lol->word, root, start_link);
		}
	}
}

static void d_depth_first_search(Postprocessor *pp, Linkage sublinkage,
                    size_t w, size_t root, size_t right, size_t start_link)
{
	List_o_links * lol;
	assert(w < pp->pp_data.length, "Bad word index");
	pp->visited[w] = true;
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next)
	{
		if ((lol->word < w) && (lol->link != start_link) && (w != root))
		{
			add_link_to_domain(pp, lol->link);
		}
	}
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next)
	{
		assert(lol->word < pp->pp_data.length, "Bad word index");
		if (!pp->visited[lol->word] && !(w == root && lol->word >= right) &&
		    !(w == root && lol->word < root) &&
		       !(lol->word < root && lol->word < w &&
		          pp_linkset_match(pp->knowledge->restricted_links,
		                   sublinkage->link_array[lol->link].link_name)))
		{
			d_depth_first_search(pp,sublinkage,lol->word,root,right,start_link);
		}
	}
}

static void left_depth_first_search(Postprocessor *pp, Linkage sublinkage,
                                    size_t w, size_t right, size_t start_link)
{
	List_o_links *lol;
	assert(w < pp->pp_data.length, "Bad word index");
	pp->visited[w] = true;
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next)
	{
		if (lol->word < w && lol->link != start_link)
		{
			add_link_to_domain(pp, lol->link);
		}
	}
	for (lol = pp->pp_data.word_links[w]; lol != NULL; lol = lol->next)
	{
		assert(lol->word < pp->pp_data.length, "Bad word index");
		if (!pp->visited[lol->word] && (lol->word != right))
		{
			depth_first_search(pp, sublinkage, lol->word, right, start_link);
		}
	}
}

static int domain_compare(const Domain * d1, const Domain * d2)
{
	return (d1->size - d2->size); /* for sorting the domains by size */
}

static void build_domains(Postprocessor *pp, Linkage sublinkage)
{
	size_t link, i, d;
	const char *s;
	pp->pp_data.N_domains = 0;

	for (link = 0; link<sublinkage->num_links; link++)
	{
		assert (sublinkage->link_array[link].lw != SIZE_MAX);
		if (NULL == sublinkage->link_array[link].link_name) continue;
		s = sublinkage->link_array[link].link_name;

		if (pp_linkset_match(pp->knowledge->ignore_these_links, s)) continue;
		if (pp_linkset_match(pp->knowledge->domain_starter_links, s))
		{
			setup_domain_array(pp, pp->pp_data.N_domains, s, link);
			if (pp_linkset_match(pp->knowledge->domain_contains_links, s))
				add_link_to_domain(pp, link);
			depth_first_search(pp, sublinkage, sublinkage->link_array[link].rw,
							 sublinkage->link_array[link].lw, link);

			pp->pp_data.N_domains++;
			assert(pp->pp_data.N_domains<PP_MAX_DOMAINS, "raise value of PP_MAX_DOMAINS");
		}
		else
		if (pp_linkset_match(pp->knowledge->urfl_domain_starter_links, s))
		{
			setup_domain_array(pp, pp->pp_data.N_domains, s, link);
			/* always add the starter link to its urfl domain */
			add_link_to_domain(pp, link);
			bad_depth_first_search(pp, sublinkage,sublinkage->link_array[link].rw,
			                       sublinkage->link_array[link].lw, link);
			pp->pp_data.N_domains++;
			assert(pp->pp_data.N_domains<PP_MAX_DOMAINS,"raise PP_MAX_DOMAINS value");
		}
		else
		if (pp_linkset_match(pp->knowledge->urfl_only_domain_starter_links, s))
		{
			setup_domain_array(pp, pp->pp_data.N_domains, s, link);
			/* do not add the starter link to its urfl_only domain */
			d_depth_first_search(pp, sublinkage, sublinkage->link_array[link].lw,
			                     sublinkage->link_array[link].lw,
			                     sublinkage->link_array[link].rw, link);
			pp->pp_data.N_domains++;
			assert(pp->pp_data.N_domains<PP_MAX_DOMAINS,"raise PP_MAX_DOMAINS value");
		}
		else
		if (pp_linkset_match(pp->knowledge->left_domain_starter_links, s))
		{
			setup_domain_array(pp, pp->pp_data.N_domains, s, link);
			/* do not add the starter link to a left domain */
			left_depth_first_search(pp, sublinkage, sublinkage->link_array[link].lw,
			                        sublinkage->link_array[link].rw, link);
			pp->pp_data.N_domains++;
			assert(pp->pp_data.N_domains<PP_MAX_DOMAINS,"raise PP_MAX_DOMAINS value");
		}
	}

	/* sort the domains by size */
	qsort((void *) pp->pp_data.domain_array,
		pp->pp_data.N_domains,
		sizeof(Domain),
		(int (*)(const void *, const void *)) domain_compare);

	/* sanity check: all links in all domains have a legal domain name */
	for (d = 0; d < pp->pp_data.N_domains; d++)
	{
		i = find_domain_name(pp, pp->pp_data.domain_array[d].string);
		if (i == SIZE_MAX)
			 prt_error("Error: post_process(): Need an entry for %s in LINK_TYPE_TABLE",
					 pp->pp_data.domain_array[d].string);
		pp->pp_data.domain_array[d].type = i;
	}
}

static void build_domain_forest(Postprocessor *pp, Linkage sublinkage)
{
	size_t d, d1, link;
	DTreeLeaf * dtl;
	if (pp->pp_data.N_domains > 0)
	{
		pp->pp_data.domain_array[pp->pp_data.N_domains-1].parent = NULL;
		for (d=0; d < pp->pp_data.N_domains-1; d++)
		{
			for (d1 = d+1; d1 < pp->pp_data.N_domains; d1++)
			{
				if (contained_in(&pp->pp_data.domain_array[d], &pp->pp_data.domain_array[d1], sublinkage))
				{
					pp->pp_data.domain_array[d].parent = &pp->pp_data.domain_array[d1];
					break;
				}
			}
			if (d1 == pp->pp_data.N_domains)
			{
				/* we know this domain is a root of a new tree */
				pp->pp_data.domain_array[d].parent = NULL;
			}
		}
	}
	/* the parent links of domain nodes have been established.
		 now do the leaves */
	for (d = 0; d < pp->pp_data.N_domains; d++)
	{
		pp->pp_data.domain_array[d].child = NULL;
	}
	for (link=0; link < sublinkage->num_links; link++)
	{
		assert (sublinkage->link_array[link].lw != SIZE_MAX);
		for (d=0; d<pp->pp_data.N_domains; d++)
		{
			if (link_in_domain(link, &pp->pp_data.domain_array[d]))
			{
				dtl = (DTreeLeaf *) malloc(sizeof(DTreeLeaf));
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
internal_process(Postprocessor *pp, Linkage sublinkage, const char **msg)
{
	size_t i;
	/* quick test: try applying just the relevant global rules */
	if (!apply_relevant_rules(pp, apply_contains_one_globally,
	                          sublinkage,
	                          pp->knowledge->contains_one_rules,
	                          pp->relevant_contains_one_rules, msg))
	{
		for (i = 0; i < pp->pp_data.wowlen; i++)
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
	if (!check_domain_nesting(pp, sublinkage->num_links))
			printf("WARNING: The domains are not nested.\n");
#endif

	/* The order below should be optimal for most cases */
	if (!apply_relevant_rules(pp, apply_contains_one, sublinkage,
								pp->knowledge->contains_one_rules,
								pp->relevant_contains_one_rules, msg)) return 1;
	if (!apply_relevant_rules(pp, apply_contains_none, sublinkage,
								pp->knowledge->contains_none_rules,
									pp->relevant_contains_none_rules, msg)) return 1;
	if (!apply_rules(pp, apply_must_form_a_cycle, sublinkage,
					 pp->knowledge->form_a_cycle_rules,msg)) return 1;
	if (!apply_rules(pp, apply_bounded, sublinkage,
					 pp->knowledge->bounded_rules, msg)) return 1;
	return 0; /* This linkage satisfied all the rules */
}


/**
 * Call this (a) after having called post_process_scan_linkage() on all
 * generated linkages, but (b) before calling post_process() on any
 * particular linkage. Here we mark all rules which we know (from having
 * accumulated a set of link names appearing in *any* linkage) that won't
 * ever be needed.
 */
static void prune_irrelevant_rules(Postprocessor *pp)
{
	pp_rule *rule;
	int coIDX, cnIDX, rcoIDX = 0, rcnIDX = 0;

	/* If we didn't scan any linkages, there's no pruning to be done. */
	if (pp_linkset_population(pp->set_of_links_of_sentence) == 0) return;

	for (coIDX = 0; ; coIDX++)
	{
		rule = &(pp->knowledge->contains_one_rules[coIDX]);
		if (rule->msg == NULL) break;
		if (pp_linkset_match_bw(pp->set_of_links_of_sentence, rule->selector))
		{
			/* mark rule as being relevant to this sentence */
			pp->relevant_contains_one_rules[rcoIDX++] = coIDX;
			pp_linkset_add(pp->set_of_links_in_an_active_rule, rule->selector);
		}
	}
	pp->relevant_contains_one_rules[rcoIDX] = -1;	/* end sentinel */

	for (cnIDX = 0; ; cnIDX++)
	{
		rule = &(pp->knowledge->contains_none_rules[cnIDX]);
		if (rule->msg == NULL) break;
		if (pp_linkset_match_bw(pp->set_of_links_of_sentence, rule->selector))
		{
			pp->relevant_contains_none_rules[rcnIDX++] = cnIDX;
			pp_linkset_add(pp->set_of_links_in_an_active_rule, rule->selector);
		}
	}
	pp->relevant_contains_none_rules[rcnIDX] = -1;

	if (verbosity > 1)
	{
		printf("PP: Saw %zd unique link names in all linkages.\n",
				pp_linkset_population(pp->set_of_links_of_sentence));
		printf("PP: Using %i 'contains one' rules and %i 'contains none' rules\n",
			   rcoIDX, rcnIDX);
	}
}


/***************** definitions of exported functions ***********************/

PostProcessor * post_process_open(const char *path)
{
	pp_knowledge *kno = pp_knowledge_open(path);
	return post_process_new(kno);
}

void post_process_close(PostProcessor *pp)
{
	pp_knowledge_close(pp->knowledge);
	post_process_free(pp);
}

/**
 * read rules from path and initialize the appropriate fields in
 * a postprocessor structure, a pointer to which is returned.
 */
Postprocessor * post_process_new(pp_knowledge * kno)
{
	Postprocessor *pp;

	pp = (Postprocessor *) malloc (sizeof(Postprocessor));
	pp->knowledge = kno;
	pp->string_set = string_set_create();
	pp->set_of_links_of_sentence = pp_linkset_open(1024);
	pp->set_of_links_in_an_active_rule = pp_linkset_open(1024);
	pp->relevant_contains_one_rules =
	      (int *) malloc ((pp->knowledge->n_contains_one_rules + 1)
	                      *(sizeof pp->relevant_contains_one_rules[0]));
	pp->relevant_contains_none_rules =
	      (int *) malloc ((pp->knowledge->n_contains_none_rules + 1)
	                      *(sizeof pp->relevant_contains_none_rules[0]));
	pp->relevant_contains_one_rules[0]	= -1;
	pp->relevant_contains_none_rules[0] = -1;
	pp->pp_node = NULL;
	pp->n_local_rules_firing	= 0;
	pp->n_global_rules_firing = 0;

	pp->q_pruned_rules = false;

	/* 60 is just starting size, these are expanded if needed */
	pp->vlength = 60;
	pp->visited = (bool*) malloc(pp->vlength * sizeof(bool));

	pp->pp_data.links_to_ignore = NULL;
	pp->pp_data.domlen = 60;
	pp->pp_data.domain_array = (Domain*) malloc(pp->pp_data.domlen * sizeof(Domain));
	memset(pp->pp_data.domain_array, 0, pp->pp_data.domlen * sizeof(Domain));

	pp->pp_data.wowlen = 60;
	pp->pp_data.word_links = (List_o_links **) malloc(pp->pp_data.wowlen * sizeof(List_o_links*));
	memset(pp->pp_data.word_links, 0, pp->pp_data.wowlen * sizeof(List_o_links *));

	return pp;
}

void post_process_free(Postprocessor *pp)
{
	/* frees up memory associated with pp, previously allocated by open */
	if (pp == NULL) return;
	string_set_delete(pp->string_set);
	pp_linkset_close(pp->set_of_links_of_sentence);
	pp_linkset_close(pp->set_of_links_in_an_active_rule);
	free(pp->relevant_contains_one_rules);
	free(pp->relevant_contains_none_rules);
	pp->knowledge = NULL;
	free_pp_node(pp);

	free(pp->visited);

	post_process_free_data(&pp->pp_data);
	free(pp->pp_data.domain_array);
	free(pp->pp_data.word_links);
	free(pp);
}

/**
 * During a first pass (prior to actual post-processing of the linkages
 * of a sentence), call this once for every generated linkage. Here we
 * simply maintain a set of "seen" link names for rule pruning, later on.
 */
void post_process_scan_linkage(Postprocessor *pp, Linkage linkage)
{
	size_t i;
	if (pp == NULL) return;
	for (i = 0; i < linkage->num_links; i++)
	{
		assert(linkage->link_array[i].lw != SIZE_MAX);

		pp_linkset_add(pp->set_of_links_of_sentence,
			linkage->link_array[i].link_name);
	}
}

static size_t report_rule_use(pp_rule *set)
{
	size_t cnt = 0;
	size_t i;
	for (i=0; set[i].msg != NULL; i++)
	{
		printf("usage: %d rule: %s\n", set[i].use_count, set[i].msg);
		cnt++;
	}
	return cnt;
}

static size_t report_unused_rule(pp_rule *set)
{
	size_t i;
	size_t cnt = 0;
	for (i=0; set[i].msg != NULL; i++)
	{
		if (0 == set[i].use_count)
		{
			printf("Unsued rule: %s\n", set[i].msg);
			cnt++;
		}
	}
	return cnt;
}

static void report_pp_stats(Postprocessor *pp)
{
	size_t rule_cnt = 0;
	size_t unused_cnt = 0;
	pp_knowledge * kno;
	if (verbosity < 3) return;

	printf("PP stats: local_rules_firing=%d\n", pp->n_local_rules_firing);
	kno = pp->knowledge;

	printf("\nPP stats: form_a_cycle_rules\n");
	rule_cnt += report_rule_use(kno->form_a_cycle_rules);

	printf("\nPP stats: contains_one_rules\n");
	rule_cnt += report_rule_use(kno->contains_one_rules);

	printf("\nPP stats: contains_none_rules\n");
	rule_cnt += report_rule_use(kno->contains_none_rules);

	printf("\nPP stats: bounded_rules\n");
	rule_cnt += report_rule_use(kno->bounded_rules);

	printf("\nPP stats: Rules that were not used:\n");
	unused_cnt += report_unused_rule(kno->form_a_cycle_rules);
	unused_cnt += report_unused_rule(kno->contains_one_rules);
	unused_cnt += report_unused_rule(kno->contains_none_rules);
	unused_cnt += report_unused_rule(kno->bounded_rules);

	printf("\nPP stats: %zd of %zd rules unused\n", unused_cnt, rule_cnt);
}

/**
 * Takes a linkage and returns:
 *  . for each link, the domain structure of that link
 *  . a list of the violation strings
 * NB: linkage->link[i]->l=-1 means that this connector is to be ignored.
 */
PP_node *do_post_process(Postprocessor *pp, Linkage sublinkage, bool is_long)
{
	const char *msg;

	if (pp == NULL) return NULL;

	// XXX wtf .. why is this not leaking memory ?
	pp->pp_data.links_to_ignore = NULL;

	pp->pp_data.length = sublinkage->num_words;

	/* In the name of responsible memory management, we retain a copy of the
	 * returned data structure pp_node as a field in pp, so that we can clear
	 * it out after every call, without relying on the user to do so. */
	clear_pp_node(pp);

	/* For long sentences, we can save some time by pruning the rules
	 * which can't possibly be used during postprocessing the linkages
	 * of this sentence. For short sentences, this is pointless. */
	if (is_long && pp->q_pruned_rules == false)
	{
		prune_irrelevant_rules(pp);
	}
	pp->q_pruned_rules = true;

	switch (internal_process(pp, sublinkage, &msg))
	{
		case -1:
			/* some global test failed even before we had to build the domains */
			pp->n_global_rules_firing++;
			pp->pp_node->violation = msg;
			report_pp_stats(pp);
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

	report_pp_stats(pp);
	build_type_array(pp);

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
