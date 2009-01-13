/*************************************************************************/
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

package org.linkgrammar;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * A <code>Linkage</code> represents one of possibly many parses
 * returned by the Link Grammar parser. Each <code>Linkage</code>
 * is defined by a list of <code>Link</code>s between the tokens
 * in a sentence.  A <code>Linkage</code> also has some metadata
 *  associated with it, e.g. for various cost measures. 
 *
 * @author Borislav Iordanov
 */
public class Linkage implements Iterable<Link>
{
	private List<Link> links = new ArrayList<Link>();
	private String [] disjuncts;
	private String [] words;
	private String constituentString;
	private int linkedWordCount;
	private int andCost;
	private int disjunctCost;
	private int linkCost;
	private int numViolations;
	
	public List<Link> getLinks()
	{
		return links;
	}
	
	public Iterator<Link> iterator()
	{
		return links.iterator();
	}
	
	public String disjunctAt(int i)
	{
		return disjuncts[i];
	}
 
	public String[] getDisjuncts()
	{
		return disjuncts;
	}

	public void setDisjuncts(String[] disjuncts)
	{
		this.disjuncts = disjuncts;
	}

	public String wordAt(int i)
	{
		return words[i];
	}
 
	public String[] getWords()
	{
		return words;
	}

	public void setWords(String[] words)
	{
		this.words = words;
	}

	public int getAndCost()
	{
		return andCost;
	}

	public void setAndCost(int andCost)
	{
		this.andCost = andCost;
	}

	public int getDisjunctCost()
	{
		return disjunctCost;
	}

	public void setDisjunctCost(int disjunctCost)
	{
		this.disjunctCost = disjunctCost;
	}

	public int getLinkCost()
	{
		return linkCost;
	}

	public void setLinkCost(int linkCost)
	{
		this.linkCost = linkCost;
	}

	public int getNumViolations()
	{
		return numViolations;
	}

	public void setNumViolations(int numViolations)
	{
		this.numViolations = numViolations;
	}

	public int getLinkedWordCount()
	{
		return linkedWordCount;
	}

	public void setLinkedWordCount(int linkedWordCount)
	{
		this.linkedWordCount = linkedWordCount;
	}

	public String getConstituentString()
	{
		return constituentString;
	}

	public void setConstituentString(String constituentString)
	{
		this.constituentString = constituentString;
	}	
}
