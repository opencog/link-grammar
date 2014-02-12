#
# Makefile for generating perl bindings
#

SWIG_SOURCES = ../swig/link_grammar.i
BUILT_SOURCES = $(top_builddir)/bindings/perl/link_grammar_wrap.cc

# PERL_EXT_LIB become /usr/local/share/perl/5.14.2 or something like that...
pkgperldir = @PERL_EXT_LIB@

pkgperl_SCRIPTS = \
   $(top_builddir)/bindings/perl/clinkgrammar.pm

# Hmmm .. what's the right place ???
# pkgperl_LTLIBRARIES = liblink-grammar-perl.la
lib_LTLIBRARIES = liblink-grammar-perl.la

liblink_grammar_perl_la_SOURCES = $(BUILT_SOURCES) $(SWIG_SOURCES)

liblink_grammar_perl_la_CPPFLAGS = \
   $(SWIG_PERL_CPPFLAGS)           \
   $(PERL_CPPFLAGS)                \
   -I$(top_srcdir)/link-grammar    \
   $(ANSI_CFLAGS)

liblink_grammar_perl_la_LDFLAGS = -version-info @VERSION_INFO@ $(PERL_LDFLAGS) -module
liblink_grammar_perl_la_LIBADD = $(top_builddir)/link-grammar/liblink-grammar.la

if HAVE_HUNSPELL
liblink_grammar_perl_la_LIBADD  += ${HUNSPELL_LIBS}
endif

if HAVE_ASPELL
liblink_grammar_perl_la_LIBADD  += ${ASPELL_LIBS}
endif

$(BUILT_SOURCES): $(SWIG_SOURCES)
	$(SWIG) -perl5 -I$(top_srcdir)/link-grammar -o $@ $<
#	$(SWIG) $(SWIG_PERL_OPT) -I$(top_srcdir)/link-grammar -o $@ $<


EXTRA_DIST =     \
   README        \
   test.pl


