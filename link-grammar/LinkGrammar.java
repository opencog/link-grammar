package org.linkgrammar;

/**
 * This class serves as a wrapper to the C Link Grammar Parser library.
 * It provides a simple public Java API to the equivalent public C API.
 *
 * Unfortunately, this class is not at all OOP in style; it operates on 
 * the single, current sentence and linkage.  This could be improved.
 */

public class LinkGrammar
{
    static
    {
        // On a linux system, the actual name of the library is prefixed 
        // with "lib" and suffixed with ".so" -- e.g. "liblink-grammar-java.so"
        System.loadLibrary("link-grammar-java");
    }

    // C functions for changing linkparser options
    public static native void setMaxParseSeconds(int maxParseSeconds);

    public static native void setMaxCost(int maxCost);

    // C functions in the linkparser API
    public static native void init();

    public static native void parse(String sent);

    public static native void close();

    // C sentence access functions
    public static native int getNumWords();

    public static native String getWord(int i);

    public static native int getNumSkippedWords();

    // C linkage access functions
    public static native int getNumLinkages();

    public static native void makeLinkage(int index);

    public static native int getLinkageNumViolations();

    public static native int getLinkageDisjunctCost();

    public static native int getNumLinks();

    public static native int getLinkLWord(int link);

    public static native int getLinkRWord(int link);

    public static native String getLinkLLabel(int link);

    public static native String getLinkRLabel(int link);

    public static native String getLinkLabel(int link);

    public static native String getConstituentString();

    public static native String getLinkString();

    // OTHER UTILITY C FUNCTIONS
    public static native boolean isPastTenseForm(String word);

    public static native boolean isEntity(String word);
}

