/*************************************************************************/
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

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
        // On a Linux system, the actual name of the library
        // is prefixed with "lib" and suffixed with ".so"
        // -- e.g. "liblink-grammar-java.so"
        // Windows looks for "link-grammar-java.dll"
        // MacOS looks for "liblink-grammar-java.dylib"
        //
        // On a Windows system, we also need to load the prequisite
        // libraries first. (Linux loaders do this automatically).
        // Actually, I guess Windows does this too, unless the user
        // failed to add the working directory to %PATH
        //
        String osname = System.getProperty("os.name");
        if (osname.indexOf("win") > -1 || osname.indexOf("Win") > -1)
        {
            System.loadLibrary("link-grammar");
        }
        // if (osname.indexOf("Mac OS X") > -1) {}
        System.loadLibrary("link-grammar-java");
    }

    //! Get the version string for the parser.
    public static native String getVersion();

    //! Get the version string for the dictionary.
    public static native String getDictVersion();

    // C functions for changing linkparser options
    public static native void setMaxParseSeconds(int maxParseSeconds);

    public static native void setMaxCost(int maxCost);

    // Defaults to /usr/local/share/link-grammar/
    public static native void setDictionariesPath(String path);

    // C functions in the linkparser API
    public static native void init();

    public static native void parse(String sent);

    public static native void close();

    // C sentence access functions
    public static native int getNumWords();

    public static native String getWord(int i);

    // Get the inflected form of the word.
    public static native String getLinkageWord(int i);

    // Get string representing the disjunct actually used.
    public static native String getLinkageDisjunct(int i);

    public static native int getNumSkippedWords();

    // C linkage access functions
    public static native int getNumLinkages();

    public static native void makeLinkage(int index);

    public static native int getLinkageNumViolations();

    public static native int getLinkageAndCost();

    public static native int getLinkageDisjunctCost();

    public static native int getLinkageLinkCost();

    public static native int getNumLinks();

    public static native int getLinkLWord(int link);

    public static native int getLinkRWord(int link);

    public static native String getLinkLLabel(int link);

    public static native String getLinkRLabel(int link);

    public static native String getLinkLabel(int link);

    public static native String getConstituentString();

    public static native String getLinkString();

    public static native String getLinkageSense(int word, int sense);

    public static native double getLinkageSenseScore(int word, int sense);

    // OTHER UTILITY C FUNCTIONS
    public static native boolean isPastTenseForm(String word);

    public static native boolean isEntity(String word);
}

