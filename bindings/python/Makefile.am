#
# Makefile for generating python bindings
#

SWIG_SOURCES = ../swig/link_grammar.i
BUILT_SOURCES = $(top_builddir)/bindings/python/link_grammar_wrap.cc

pkgpython_PYTHON =    \
   pylinkgrammar/__init__.py     \
   pylinkgrammar/linkgrammar.py     \
   $(top_builddir)/bindings/python/clinkgrammar.py

pkgpyexec_LTLIBRARIES = libpylink_grammar.la

libpylink_grammar_la_SOURCES = $(BUILT_SOURCES) $(SWIG_SOURCES)

libpylink_grammar_la_CPPFLAGS = \
   $(SWIG_PYTHON_CPPFLAGS)      \
   $(PYTHON_CPPFLAGS)           \
   -I$(top_srcdir)/link-grammar \
   $(ANSI_CFLAGS) 

libpylink_grammar_la_LDFLAGS = -version-info @VERSION_INFO@ $(PYTHON_LDFLAGS) -module
libpylink_grammar_la_LIBADD = $(top_builddir)/link-grammar/liblink-grammar.la

$(BUILT_SOURCES): $(SWIG_SOURCES)
	$(SWIG) $(SWIG_PYTHON_OPT) -I$(top_srcdir)/link-grammar -o $@ $<

EXTRA_DIST =                       \
   AUTHORS                         \
   LICENSE                         \
   README                          \
   pylinkgrammar                   \
   pylinkgrammar/__init__.py       \
   pylinkgrammar/linkgrammar.py    \
   pylinkgrammar/tests.py          \
   setup.py

