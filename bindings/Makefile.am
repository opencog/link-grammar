#
# Automake file for programming language bindings to link-grammar
#

SUBDIRS=

if HAVE_JAVA
SUBDIRS += java-jni
endif

if HAVE_ANT
SUBDIRS += java
endif

if HAVE_SWIG
if HAVE_PERL
SUBDIRS += perl
endif
SUBDIRS += python
endif


EXTRA_DIST =                              \
   autoit/_LGTest.au3                     \
   autoit/_LinkGrammar.au3                \
   autoit/README                          \
   lisp/link-interface.cl                 \
   swig/link_grammar.i


