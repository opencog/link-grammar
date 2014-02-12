#
# Makefile for generating python bindings
#

SWIG_SOURCES = ../swig/link_grammar.i
BUILT_SOURCES = $(top_builddir)/bindings/python/lg_python_wrap.cc

pkgpython_PYTHON =                                 \
   linkgrammar.py                                  \
   $(top_builddir)/bindings/python/__init__.py     \
   $(top_builddir)/bindings/python/clinkgrammar.py

# The la MUST have the same name as the pm, 
# which MUST be the swig -module name!
# We're calling this clinkgrammar, since its a direct map of the C
# interfaces.
pkgpyexec_LTLIBRARIES = clinkgrammar.la

clinkgrammar_la_SOURCES = $(BUILT_SOURCES) $(SWIG_SOURCES)

clinkgrammar_la_CPPFLAGS =      \
   $(SWIG_PYTHON_CPPFLAGS)      \
   $(PYTHON_CPPFLAGS)           \
   -I$(top_srcdir)/link-grammar \
   $(ANSI_CFLAGS) 

clinkgrammar_la_LDFLAGS = -version-info @VERSION_INFO@ $(PYTHON_LDFLAGS) -module
clinkgrammar_la_LIBADD = $(top_builddir)/link-grammar/liblink-grammar.la

$(BUILT_SOURCES): $(SWIG_SOURCES)
	$(SWIG) $(SWIG_PYTHON_OPT) -module clinkgrammar -I$(top_srcdir)/link-grammar -o $@ $<

EXTRA_DIST =         \
   AUTHORS           \
   LICENSE           \
   README            \
   __init__.py.in    \
   linkgrammar.py    \
   tests.py          \
   setup.py

