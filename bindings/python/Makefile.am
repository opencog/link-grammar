#
# Makefile for generating python bindings
#


# swig -python -o pylinkgrammar/link_grammar_wrap.c pylinkgrammar/link_grammar.i


SWIG_SOURCES = pylinkgrammar/link_grammar.i
BUILT_SOURCES = $(top_builddir)/bindings/python/link_grammar_wrap.cc

pkgpython_PYTHON = pylinkgrammar.py
pkgpyexec_LTLIBRARIES = libpylink_grammar.la
libpylink_grammar_la_SOURCES = $(BUILT_SOURCES) $(SWIG_SOURCES)
libpylink_grammar_la_CPPFLAGS = $(SWIG_PYTHON_CPPFLAGS) $(PYTHON_CPPFLAGS) -I$(top_srcdir)/link-grammar $(ANSI_CFLAGS) $(LINK_CFLAGS)
libpylink_grammar_la_LDFLAGS = $(PYTHON_LDFLAGS) -module
libpylink_grammar_la_LIBADD = ../../link-grammar/liblink_grammar.la

$(BUILT_SOURCES): $(SWIG_SOURCES)
	$(SWIG) $(SWIG_PYTHON_OPT) -I$(top_srcdir)/link-grammar -o $@ $<
