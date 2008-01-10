package org.linkgrammar;

/**
 * This class serves as a wrapper to the C Link Grammar Parser library.
 * It provides a simple public Java API to the equivalent public C API.
 *
 * Unfortunately, this class is not at all OOP in style; it operates on 
 * the * single, current sentence and linkage.  This could be improved.
 */

public class LinkGrammar {

    static {
        // On a linux system, the actual name of the library is prefixed 
        // with "lib" and suffixed with ".so" -- e.g. "liblink-grammar.so"
        System.loadLibrary("link-grammar");
    }

    // C functions for changing linkparser options
    public static native void setMaxParseSeconds(int maxParseSeconds);

    public static native void setMaxCost(int maxCost);

    // C functions in the linkparser API
    public static native void init();

    public static native void parse(String sent);

    public static native void close();

    // C sentence access functions
    public static native int numWords();

    public static native String getWord(int i);

    public static native int numSkippedWords();

    // C linkage access functions
    public static native int numLinkages();

    public static native void makeLinkage(int index);

    public static native int linkageNumViolations();

    public static native int linkageDisjunctCost();

    public static native int numLinks();

    public static native int linkLWord(int link);

    public static native int linkRWord(int link);

    public static native String linkLLabel(int link);

    public static native String linkRLabel(int link);

    public static native String linkLabel(int link);

    public static native String constituentString();

    public static native String linkString();

    // OTHER UTILITY C FUNCTIONS
    public static native boolean isPastTenseForm(String word);

    public static native boolean isEntity(String word);
}

