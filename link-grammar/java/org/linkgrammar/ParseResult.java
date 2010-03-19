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
 * 
 * <p>
 * Represents the result of parsing a piece of text. The result
 * consists of some global meta information about the whole parse
 * and a list of <code>Linkage</code>s returned by the parser. The
 * original parsed text is available as the <code>text</code> attribute
 * and a tokenized version as the <code>String[] words</code> attribute.  
 * </p>
 *
 * @author Borislav Iordanov
 *
 */
public class ParseResult implements Iterable<Linkage>
{
	String parserVersion;
	String dictVersion;
	String text;
	String [] words;
	boolean [] entityFlags;
	boolean [] pastTenseFlags;
	List<Linkage> linkages = new ArrayList<Linkage>();
	int numSkippedWords;
	
	public String wordAt(int i)
	{
		return words[i];
	}
	
	/**
	 * past-tense verbs have a usbscript of .v-d, .w-d or .q-d
	 * look at the subscript instead, to see if a verb is past-tense.
	 * @deprecated
	 */
	@Deprecated public boolean isPastTenseForm(int i)
	{
		return pastTenseFlags[i];
	}
	
	public boolean isEntity(int i)
	{
		return entityFlags[i];
	}
	
	public Iterator<Linkage> iterator()
	{
		return linkages.iterator();
	}

	public List<Linkage> getLinkages()
	{
		return linkages;
	}
	
	public String getText()
	{
		return text;
	}

	public void setText(String text)
	{
		this.text = text;
	}

	public String[] getWords()
	{
		return words;
	}

	public void setWords(String[] words)
	{
		this.words = words;
	}

	public boolean[] getEntityFlags()
	{
		return entityFlags;
	}

	public void setEntityFlags(boolean[] entityFlags)
	{
		this.entityFlags = entityFlags;
	}

	/**
	 * past-tense verbs have a usbscript of .v-d, .w-d or .q-d
	 * look at the subscript instead, to see if a verb is past-tense.
	 * @deprecated
	 */
	@Deprecated
	public boolean[] getPastTenseFlags()
	{
		return pastTenseFlags;
	}

	/**
	 * past-tense verbs have a usbscript of .v-d, .w-d or .q-d
	 * look at the subscript instead, to see if a verb is past-tense.
	 * @deprecated
	 */
	@Deprecated
	public void setPastTenseFlags(boolean[] pastTenseFlags)
	{
		this.pastTenseFlags = pastTenseFlags;
	}

	public int getNumSkippedWords()
	{
		return numSkippedWords;
	}

	public void setNumSkippedWords(int numSkippedWords)
	{
		this.numSkippedWords = numSkippedWords;
	}

	public String getParserVersion()
	{
		return parserVersion;
	}

	public void setParserVersion(String parserVersion)
	{
		this.parserVersion = parserVersion;
	}	

	public String getDictVersion()
	{
		return dictVersion;
	}

	public void setDictVersion(String dictVersion)
	{
		this.dictVersion = dictVersion;
	}	
}
