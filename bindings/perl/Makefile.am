#
# Makefile for generating perl bindings
#

SWIG_SOURCES = ../swig/link_grammar.i
BUILT_SOURCES = $(top_builddir)/bindings/perl/link_grammar_wrap.cc


# pkgperlexec_LTLIBRARIES = libperl_link_grammar.la

$(BUILT_SOURCES): $(SWIG_SOURCES)
	$(SWIG) $(SWIG_PERL_OPT) -I$(top_srcdir)/link-grammar -o $@ $<




