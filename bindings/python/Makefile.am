#
# Makefile for generating python bindings
#
# Created by Linas Vepstas February 2014
#

SWIG_SOURCES = ../swig/link_grammar.i
# BUILT_SOURCES = $(top_builddir)/bindings/python/lg_python_wrap.cc
BUILT_SOURCES = ../../bindings/python/lg_python_wrap.cc

# Over-ride the install location, so as to remove the dash in the
# directory "link-grammar".  Turns out python cannot tolerate dashes.
# Thus, typically, it will install into
# /usr/local/lib/python2.7/dist-packages/linkgrammar (without the dash)
pkgpythondir=$(pythondir)/linkgrammar
pkgpyexecdir=$(pythondir)/linkgrammar

pkgpython_PYTHON =                                 \
   linkgrammar.py                                  \
   $(top_builddir)/bindings/python/__init__.py     \
   $(top_builddir)/bindings/python/clinkgrammar.py


# These are packaged in the tarball; make clean should not remove them.
maintainer-clean-local:
	-rm -f $(BUILT_SOURCES)
	-rm -f $(top_builddir)/bindings/python/clinkgrammar.py

# Don't remove __init__.py; that one is build by configure!
DISTCLEANFILES =                                   \
   $(top_builddir)/bindings/python/__init__.py

if HAVE_SWIG
$(BUILT_SOURCES): $(SWIG_SOURCES)
	$(SWIG) -python -module clinkgrammar -I$(top_srcdir)/link-grammar -o $@ $<
endif

# The la MUST have the same name as the pm,
# which MUST be the swig -module name!
# We're calling this clinkgrammar, since its a direct map of the C
# interfaces.
pkgpyexec_LTLIBRARIES = _clinkgrammar.la

_clinkgrammar_la_SOURCES = $(BUILT_SOURCES) $(SWIG_SOURCES)

# $(top_builddir) to pick up autogened link-grammar/link-features.h
_clinkgrammar_la_CPPFLAGS =       \
   $(SWIG_PYTHON_CPPFLAGS)        \
   $(PYTHON_CPPFLAGS)             \
   -I$(top_srcdir)                \
   -I$(top_builddir)

_clinkgrammar_la_LDFLAGS = -version-info @VERSION_INFO@ $(PYTHON_LDFLAGS) -module
_clinkgrammar_la_LIBADD = $(top_builddir)/link-grammar/liblink-grammar.la


EXTRA_DIST =         \
   AUTHORS           \
   LICENSE           \
   README.md         \
   __init__.py.in    \
   linkgrammar.py
