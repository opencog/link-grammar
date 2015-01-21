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

SUBDIRS += ocaml

if HAVE_PERL
SUBDIRS += perl
endif

if HAVE_PYTHON
SUBDIRS += python python-examples
endif


EXTRA_DIST =                              \
   README                                 \
   autoit/_LGTest.au3                     \
   autoit/_LinkGrammar.au3                \
   autoit/README                          \
   lisp/link-interface.cl                 \
   swig/link_grammar.i


