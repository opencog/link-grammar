Note: This document is obsolete.
Instead, see [BUILDING on Windows (MinGW/MSYS2)](README.MSYS2).

BUILDING on Windows (MinGW/MSYS)
--------------------------------
   MinGW/MSYS uses the Gnu toolset to compile Windows programs for
   Windows.  This is probably the easiest way to obtain workable Java
   bindings for Windows.  Download and install MinGW, MSYS and MSYS-DTK
   from http://mingw.org.

      Then build and install link-grammar with

          ./configure
          make
          make install

   If you used the standard installation paths, the directory /usr/ is
   mapped to C:\msys\1.0, so after 'make install', the libraries and
   executable will be found at C:\msys\1.0\local\bin and the dictionary
   files at C:\msys\1.0\local\share\link-grammar.

Running
-------
  See "RUNNING the program" in the main README.


Java bindings
-------------

   In order to use the Java bindings you'll need to build two extra
   DLLs, by running the following commands from the link-grammar base
   directory:

       cd link-grammar

       gcc -g -shared -Wall -D_JNI_IMPLEMENTATION_ -Wl,--kill-at \
       .libs/analyze-linkage.o .libs/and.o .libs/api.o \
       .libs/build-disjuncts.o .libs/constituents.o \
       .libs/count.o .libs/disjuncts.o .libs/disjunct-utils.o \
       .libs/error.o .libs/expand.o .libs/extract-links.o \
       .libs/fast-match.o .libs/idiom.o .libs/massage.o \
       .libs/post-process.o .libs/pp_knowledge.o .libs/pp_lexer.o \
       .libs/pp_linkset.o .libs/prefix.o .libs/preparation.o \
       .libs/print-util.o .libs/print.o .libs/prune.o \
       .libs/read-dict.o .libs/read-regex.o .libs/regex-morph.o \
       .libs/resources.o .libs/spellcheck-aspell.o \
       .libs/spellcheck-hun.o .libs/string-set.o .libs/tokenize.o \
       .libs/utilities.o .libs/word-file.o .libs/word-utils.o \
       -o /usr/local/bin/link-grammar.dll

       gcc -g -shared -Wall -D_JNI_IMPLEMENTATION_ -Wl,--kill-at \
       .libs/jni-client.o /usr/local/bin/link-grammar.dll \
       -o /usr/local/bin/link-grammar-java.dll

   This will create link-grammar.dll and link-grammar-java.dll in the
   directory c:\msys\1.0\local\bin . These files, together with
   link-grammar-*.jar, will be used by Java programs.

   Make sure that this directory is in the %PATH setting, as otherwise,
   the DLL's will not be found.
