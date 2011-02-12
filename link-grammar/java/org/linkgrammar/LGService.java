/*************************************************************************/
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

package org.linkgrammar;

import java.io.File;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.io.Reader;
import java.net.ServerSocket;
import java.net.Socket;
import java.text.CharacterIterator;
import java.text.SimpleDateFormat;
import java.text.StringCharacterIterator;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

/**
 * A simple server implementation for running Link Grammar as a
 * standalone server. The server accepts parsing requests and returns
 * the result as a JSON formatted string (see this 
 * <a href="http://www.json.org">JSON</a> website for more information).
 * There is no session maintained between client and server, it's a
 * simple, stateless, single round-trip, request-response protocol.
 * 
 * Requests consist of a bag of parameters separated by the null '\0'
 * character. Each request must be terminated with the newline '\n' 
 * character. Each parameter has the form <em>name:value\0</em> where
 * <em>name</em> and <em>value</em> can contain any character except
 * '\0' and '\n'. The following parameters are recognized:
 * 
 * <ul>
 * <li><b>maxCost</b> - ??</li>
 * <li><b>storeConstituentString</b> - whether to return the constituent
 *      string for each Linkage as part of the result.</li> 
 * <li><b>maxLinkages</b> - maximum number of parses to return in the
 *      result. Note that this does not affect the parser behavior which
 *      computes all parses anyway.</li>
 * <li><b>maxParseSeconds</b> - ??</li>
 * <li><b>text</b> - The text to parse. Note that it must be stripped
 *      from newlines.</li>   
 * </ul>
 * 
 * The server maintains incoming requests in an unbounded queue and
 * handles them in thread pool whose size can be specified at the
 * command line. A thread pool of size > 1 will only work if the Link
 * Grammar version used is thread-safe. 
 * 
 * Execute this class as a main program to view a list of options.   
 *
 * @author Borislav Iordanov
 *
 */
public class LGService
{
	private static boolean verbose = false;
	private static SimpleDateFormat dateFormatter = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss");

	//
	// The current Java bindings require each concurrent thread to
	// initialize separately. This means that each thread has a
	// separate copy of the dictionaries, which in unfortunate design.
	// Nevertheless, we want to avoid initializing before every parse
	// activity so we maintain a thread local flag and initialize on
	// demand only. 
	//
	// The C library itself does not have this restriction; one
	// dictionary can be shared by many threads. However, the java
	// bindings do have this restriction; the java bindings were 
	// never designed to be run multi-threaded. XXX This needs to be
	// fixed.
	//
	// The main problem is to detect when a thread completes its work
	// and therefore LinkGrammar.close should be called to free
	// allocated memory. We leave that to the use of this class.
	//
	private static ThreadLocal<Boolean> initialized = new ThreadLocal<Boolean>()
	{ protected Boolean initialValue() { return Boolean.FALSE; } };
	
	/**
	 * <p>Return <code>true</code> if LinkGrammar is initialized for the current thread
	 * and <code>false</code> otherwise.
	 */
	public static boolean isInitialized()
	{
		return initialized.get();
	}
	
	/**
	 * <p>
	 * Initialize LinkGrammar for the current is this is not already done. Note that
	 * this method is called by all other methods in this class that invoke LinkGrammar
	 * so there's no really need to call it yourself. It is safe to call the method repeatedly.
	 * </p>
	 */
	public static void init()
	{
		if (!initialized.get())
		{
			LinkGrammar.init();
			initialized.set(Boolean.TRUE);
		}
	}

	/**
	 * <p>
	 * Cleanup allocated memory for use of LinkGrammar in the current thread. 
	 * </p>
	 */
	public static void close()
	{
		LinkGrammar.close();
		initialized.set(Boolean.FALSE);
	}


	private static void trace(String s) 
	{
		if (verbose)
			System.out.println("LG " + dateFormatter.format(new java.util.Date()) + " " + s); 
	}

	private static boolean getBool(String name, Map<String, String> msg, boolean def)
	{
		String x = msg.get(name);
		return x == null ? def : Boolean.valueOf(x);		
	}
	
	private static int getInt(String name, Map<String, String> msg, int def)
	{
		String x = msg.get(name);
		return x == null ? def : Integer.parseInt(x);		
	}

	/**
	 * <p>
	 * Apply configuration parameters to the parser. 
	 * </p>
	 */
	public static void configure(LGConfig config)
	{
		init();
		if (config.getMaxCost() > -1)
			LinkGrammar.setMaxCost(config.getMaxCost());
		if (config.getMaxParseSeconds() > -1)
			LinkGrammar.setMaxParseSeconds(config.getMaxParseSeconds());
		if (config.getDictionaryLocation() != null)
			LinkGrammar.setDictionariesPath(config.getDictionaryLocation());
	}

	/**
	 * Assuming <code>LinkGrammar.parse</code> has already been called,
	 * construct a full <code>ParseResult</code> given the passed in
	 * configuration. For example, no more that 
	 * <code>config.getMaxLinkages</code> are returned, etc.
	 * 
	 * @param config
	 * @return
	 */
	public static ParseResult getAsParseResult(LGConfig config)
	{
		LinkGrammar.makeLinkage(0); // need to call at least once, otherwise it crashes		
		ParseResult parseResult = new ParseResult();
		parseResult.setParserVersion(LinkGrammar.getVersion());
		parseResult.setDictVersion(LinkGrammar.getDictVersion());
		parseResult.numSkippedWords = LinkGrammar.getNumSkippedWords();
		parseResult.words = new String[LinkGrammar.getNumWords()];
		parseResult.entityFlags = new boolean[LinkGrammar.getNumWords()];
		parseResult.pastTenseFlags = new boolean[LinkGrammar.getNumWords()];
		for (int i = 0; i < parseResult.words.length; i++)
		{
			String word = parseResult.words[i] = LinkGrammar.getWord(i);
			parseResult.entityFlags[i] = LinkGrammar.isEntity(word);
			parseResult.pastTenseFlags[i] = LinkGrammar.isPastTenseForm(word);
		}
		int maxLinkages = Math.min(config.getMaxLinkages(), LinkGrammar.getNumLinkages());
		for (int li = 0; li < maxLinkages; li++)
		{
			LinkGrammar.makeLinkage(li);
			Linkage linkage = new Linkage();
			linkage.setAndCost(LinkGrammar.getLinkageAndCost());
			linkage.setDisjunctCost(LinkGrammar.getLinkageDisjunctCost());
			linkage.setLinkCost(LinkGrammar.getLinkageLinkCost());
			linkage.setLinkedWordCount(LinkGrammar.getNumWords());
			linkage.setNumViolations(LinkGrammar.getLinkageNumViolations());
			String [] disjuncts = new String[LinkGrammar.getNumWords()];
			String [] words = new String[LinkGrammar.getNumWords()];
			for (int i = 0; i < words.length; i++)
			{
				disjuncts[i] = LinkGrammar.getLinkageDisjunct(i);
				words[i] = LinkGrammar.getLinkageWord(i);
			}
			linkage.setWords(words);
			linkage.setDisjuncts(disjuncts);
			int numLinks = LinkGrammar.getNumLinks();
			for (int i = 0; i < numLinks; i++)
			{
				Link link = new Link();
				link.setLabel(LinkGrammar.getLinkLabel(i));
				link.setLeft(LinkGrammar.getLinkLWord(i));
				link.setRight(LinkGrammar.getLinkRWord(i));
				link.setLeftLabel(LinkGrammar.getLinkLLabel(i));
				link.setRightLabel(LinkGrammar.getLinkRLabel(i));
				linkage.getLinks().add(link);
			}
			if (config.isStoreConstituentString())
				linkage.setConstituentString(LinkGrammar.getConstituentString());
			parseResult.linkages.add(linkage);
		}
		return parseResult;		
	}
	
    /**
     * Construct a JSON formatted result for a parse which yielded 0 linkages. 
     */
    public static String getEmptyJSONResult(LGConfig config)
    {
        StringBuffer buf = new StringBuffer();      
        buf.append("{\"tokens\":[],"); 
        buf.append("\"numSkippedWords\":0,");
        buf.append("\"entity\":[],");
        buf.append("\"pastTense\":[],");
        buf.append("\"linkages\":[],");
        buf.append("\"version\":\"" + LinkGrammar.getVersion() + "\",");
        buf.append("\"dictVersion\":\"" + LinkGrammar.getDictVersion() + "\"}");
        return buf.toString();
    }
	
	static char[] hex = "0123456789ABCDEF".toCharArray();
	
	private static String jsonString(String s)
	{
	    if (s == null)
	        return null;
		StringBuffer b = new StringBuffer();
		b.append("\"");
        CharacterIterator it = new StringCharacterIterator(s);
        for (char c = it.first(); c != CharacterIterator.DONE; c = it.next()) 
        {
            if (c == '"') b.append("\\\"");
            else if (c == '\\') b.append("\\\\");
            else if (c == '/') b.append("\\/");
            else if (c == '\b') b.append("\\b");
            else if (c == '\f') b.append("\\f");
            else if (c == '\n') b.append("\\n");
            else if (c == '\r') b.append("\\r");
            else if (c == '\t') b.append("\\t");
            else if (Character.isISOControl(c)) 
            {
                int n = c;
                for (int i = 0; i < 4; ++i) {
                    int digit = (n & 0xf000) >> 12;
                	b.append(hex[digit]);
                    n <<= 4;
                }
            } 
            else 
            {
            	b.append(c);
            }
        }
        b.append("\"");
        return b.toString();
	}
	
	/**
	 * Format the current parsing result as a JSON string. This method
	 * assume that <code>LinkGrammar.parse</code> has been called before. 
	 */
	public static String getAsJSONFormat(LGConfig config)
	{
		LinkGrammar.makeLinkage(0); // need to call at least once, otherwise it crashes
		int numWords = LinkGrammar.getNumWords();
		int maxLinkages = Math.min(config.getMaxLinkages(), LinkGrammar.getNumLinkages());		
		StringBuffer buf = new StringBuffer();		
		buf.append("{\"tokens\":["); 
		for (int i = 0; i < numWords; i++)
		{
			buf.append(jsonString(LinkGrammar.getWord(i)));
			if (i + 1 < numWords)
				buf.append(",");
		}
		buf.append("],\"numSkippedWords\":" + LinkGrammar.getNumSkippedWords());
		buf.append(",\"entity\":[");
		boolean first = true;
		for (int i = 0; i < numWords; i++)
			if (LinkGrammar.isEntity(LinkGrammar.getWord(i)))
			{
				if (!first)
					buf.append(",");
				first = false;
				buf.append(Integer.toString(i));
			}
		buf.append("],\"pastTense\":[");
		first = true;
		for (int i = 0; i < numWords; i++)
			if (LinkGrammar.isPastTenseForm(LinkGrammar.getWord(i).toLowerCase()))
			{
				if (!first)
					buf.append(",");
				first = false;
				buf.append(Integer.toString(i));
			}
		buf.append("],\"linkages\":[");
		for (int li = 0; li < maxLinkages; li++)
		{
			LinkGrammar.makeLinkage(li);
			buf.append("{\"words\":["); 
			for (int i = 0; i < numWords; i++)
			{
				buf.append(jsonString(LinkGrammar.getLinkageWord(i)));
				if (i + 1 < numWords)
					buf.append(",");
			}
			buf.append("], \"disjuncts\":["); 
			for (int i = 0; i < numWords; i++)
			{			    
				buf.append(jsonString(LinkGrammar.getLinkageDisjunct(i)));
				if (i + 1 < numWords)
					buf.append(",");
			}
			buf.append("], \"andCost\":");
			buf.append(Integer.toString(LinkGrammar.getLinkageAndCost()));
			buf.append(", \"disjunctCost\":");
			buf.append(Integer.toString(LinkGrammar.getLinkageDisjunctCost()));
			buf.append(", \"linkageCost\":");
			buf.append(Integer.toString(LinkGrammar.getLinkageLinkCost()));
			buf.append(", \"numViolations\":");
			buf.append(Integer.toString(LinkGrammar.getLinkageNumViolations()));
			if (config.isStoreConstituentString())
			{
				buf.append(", \"constituentString\":");
				buf.append(jsonString(LinkGrammar.getConstituentString()));				
			}			
			buf.append(", \"links\":[");
			int numLinks = LinkGrammar.getNumLinks();
			for (int i = 0; i < numLinks; i++)
			{
				buf.append("{\"label\":" + jsonString(LinkGrammar.getLinkLabel(i)) + ",");
				buf.append("\"left\":" + LinkGrammar.getLinkLWord(i) + ",");
				buf.append("\"right\":" + LinkGrammar.getLinkRWord(i) + ",");
				buf.append("\"leftLabel\":" + jsonString(LinkGrammar.getLinkLLabel(i)) + ",");
				buf.append("\"rightLabel\":" + jsonString(LinkGrammar.getLinkRLabel(i)) + "}");
				if (i + 1 < numLinks)
					buf.append(",");
			}			
			buf.append("]");
			buf.append("}");	
			if (li < maxLinkages - 1)
			    buf.append(",");
		}
		buf.append("],\"version\":\"" + LinkGrammar.getVersion() + "\"");
		buf.append(",\"dictVersion\":\"" + LinkGrammar.getDictVersion() + "\"");
		buf.append("}");
		return buf.toString();
	}
	
	/**
	 * A stub method for now for implementing a compact binary format
	 * for parse results.
	 * 
	 * @param config
	 * @return
	 */
	public static byte [] getAsBinary(LGConfig config)
	{
		int size = 0;
		byte [] buf = new byte[1024];
		// TODO ..... grow buf as needed
		byte [] result = new byte[size];
		System.arraycopy(buf, 0, result, 0, size);
		return result;
	}
	
	private static Map<String, String> readMsg(Reader in) throws java.io.IOException
	{
		int length = 0;
		char [] buf = new char[1024];
		for (int count = in.read(buf, length, buf.length - length); 
			 count > -1; 
			 count = in.read(buf, length, buf.length - length))
		{
			length += count;
			if (length == buf.length)
			{
				char [] nbuf = new char[buf.length + 512];
				System.arraycopy(buf, 0, nbuf, 0, buf.length);
				buf = nbuf;
			}
			if (buf[length-1] == '\n')
				break;
		}
		Map<String, String> result = new HashMap<String, String>();

		int start = -1;
		int column = -1;
		for (int offset = 0; offset < length - 1; offset++)
		{
			char c = buf[offset];
			if (start == -1)
				start = offset;
			else if (c == ':' && column == -1)
				column = offset;
			else if (c == '\0')
			{
				if (start == -1 || column == -1)
					throw new RuntimeException("Malformat message:" + new String(buf, 0, length));
				String name = new String(buf, start, column - start);
				String value = new String(buf, column + 1, offset - column - 1);
				result.put(name, value);
				start = column = -1;
			}
		}
		if (start != -1 || column != -1)
			throw new RuntimeException("Malformat message:" + new String(buf, 0, length));		
		return result;		
	}
	
	private static void handleClient(Socket clientSocket)
	{
		init();
		Reader in = null;
		PrintWriter out = null;
		try
		{
			trace("Connection accepted from : " + clientSocket.getInetAddress());			
			in = new InputStreamReader(clientSocket.getInputStream());
			Map<String, String> msg = readMsg(in); 
			if (verbose)
				trace("Received msg '" + msg + "' from " + clientSocket.getInetAddress());
			String json = "{}";			
			if ("version".equals(msg.get("get"))) // special case msg 'get:version'
				json = "{\"version\":\"" + LinkGrammar.getVersion() + "\"}";
			else
			{
				LGConfig config = new LGConfig();
				config.setStoreConstituentString(getBool("storeConstituentString", msg, config.isStoreConstituentString()));
				config.setMaxCost(getInt("maxCost", msg, config.getMaxCost()));
				config.setMaxLinkages(getInt("maxLinkages", msg, config.getMaxLinkages()));
				config.setMaxParseSeconds(getInt("maxParseSeconds", msg, config.getMaxParseSeconds()));
				configure(config);
                String text = msg.get("text");
                if (text != null && text.trim().length() > 0)
                {
                    LinkGrammar.parse(text);
                    if (LinkGrammar.getNumLinkages() > 0)
                        json = getAsJSONFormat(config);
                    else
                        json = getEmptyJSONResult(config);
                }
                else
                    json = getEmptyJSONResult(config);
            }
            out = new PrintWriter(clientSocket.getOutputStream(), true);
            out.print(json.length() + 1);
            out.print('\n');
            out.print(json);
            out.print('\n');
            out.flush();				
			trace("Response written to " + clientSocket.getInetAddress() + ", closing client connection...");
		}
		catch (Throwable t)
		{
			t.printStackTrace(System.err);
		}
		finally
		{
			if (out != null) try { out.close(); } catch (Throwable t) { }
			if (in != null) try { in.close(); } catch (Throwable t) { }
			if (clientSocket != null) try { clientSocket.close(); } catch (Throwable t) { }
		}
	}
	
	/**
	 * <p>
	 * Parse a piece of text with the given configuration and return the <code>ParseResult</code>.
	 * </p>
	 * 
	 * @param config The configuration to be used. If this is the first time the <code>parse</code>
	 * method is called within the current thread, the dictionary location (if not <code>null</code>)
	 * of this parameter will be used to initialize the parser. Otherwise the dictionary location is
	 * ignored.
	 * @param text The text to parse, normally a single sentence.
	 * @return The <code>ParseResult</code>. Note that <code>null</code> is never returned. If parsing
	 * failed, there will be 0 linkages in the result.
	 */
	public static ParseResult parse(LGConfig config, String text)
	{
		if (!isInitialized() && 
			config.getDictionaryLocation() != null && 
			config.getDictionaryLocation().trim().length() > 0)
			LinkGrammar.setDictionariesPath(config.getDictionaryLocation());
		configure(config);
		LinkGrammar.parse(text);
		return getAsParseResult(config);
	}
	
	public static void main(String [] argv)
	{
		int threads = 1;
		int port = 0;
		String dictionaryPath = null;
		try
		{
			int argIdx = 0;
			if (argv[argIdx].equals("-verbose")) { verbose = true; argIdx++; }			
			if (argv[argIdx].equals("-threads")) { threads = Integer.parseInt(argv[++argIdx]); argIdx++; }
			port = Integer.parseInt(argv[argIdx++]);
			if (argv.length > argIdx)
				dictionaryPath = argv[argIdx];
		}
		catch (Throwable ex)
		{
			if (argv.length > 0)
				ex.printStackTrace(System.err);
			System.out.println("Syntax: java org.linkgrammar.LGService [-verbose] [-threads n] port [dictionaryPath]");
			System.out.println("\t where 'port' is the TCP port the service should listen to and");
			System.out.println("\t -verbose forces tracing of every message received and");
			System.out.println("\t -threads specifies the number of concurrent threads/clients allowed (default 1) and ");
			System.out.println("\t 'dictionaryPath' full path to the Link Grammar dictionaries (optional).");
			System.exit(-1);
		}	

		if (dictionaryPath != null)
		{
			File f = new File(dictionaryPath);
			if (!f.exists()) 
			{ System.err.println("Dictionary path " + dictionaryPath + " not found."); System.exit(-1); }
			else if (!f.isDirectory()) 
			{ System.err.println("Dictionary path " + dictionaryPath + " not a directory."); System.exit(-1); }
		}
		
		System.out.println("Starting Link Grammar Server at port " + port +
				", with " + threads + " available processing threads and " + 
				((dictionaryPath == null) ? " with default dictionary location." : 
					"with dictionary location '" + dictionaryPath + "'."));
		ThreadPoolExecutor threadPool = new ThreadPoolExecutor(threads, 
				  											   threads,
				  											   Long.MAX_VALUE, 
				  											   TimeUnit.SECONDS,
				  											   new LinkedBlockingQueue<Runnable>());
		try
		{
			if (dictionaryPath != null)
				LinkGrammar.setDictionariesPath(dictionaryPath);
			ServerSocket serverSocket = new ServerSocket(port);
			while (true)
			{
				trace("Waiting for client connections...");
				final Socket clientSocket = serverSocket.accept();				
				threadPool.submit(new Runnable() { public void run() { handleClient(clientSocket); } });
			}
		}
		catch (Throwable t)
		{
			t.printStackTrace(System.err);
			System.exit(-1);
		}
	}
}

