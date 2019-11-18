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

if HAVE_PYTHON2
SUBDIRS += python
endif

if HAVE_PYTHON3
SUBDIRS += python3
endif

# Add python-examples only once - else make enters it twice.
if HAVE_PYTHON2
SUBDIRS += python-examples
else !HAVE_PYTHON2
if HAVE_PYTHON3
SUBDIRS += python-examples
endif HAVE_PYTHON3
endif !HAVE_PYTHON2

EXTRA_DIST =                              \
   README                                 \
   autoit/_LGTest.au3                     \
   autoit/_LinkGrammar.au3                \
   autoit/README                          \
   lisp/link-interface.cl                 \
   node.js/.npmignore                     \
   node.js/README.md                      \
   node.js/package.json                   \
   node.js/src                            \
   node.js/src/test                       \
   node.js/src/test/build                 \
   node.js/src/test/build/spec.js         \
   node.js/src/test/spec.coffee           \
   node.js/src/index.litcoffee            \
   node.js/package-lock.json              \
   node.js/examples                       \
   node.js/examples/simple.js             \
   swig/link_grammar.i
