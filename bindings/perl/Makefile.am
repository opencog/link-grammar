#
# Makefile for generating perl bindings
#

SWIG_SOURCES = ../swig/link_grammar.i
BUILT_SOURCES = $(top_builddir)/bindings/perl/lg_perl_wrap.cc

# PERL_EXT_LIB become /usr/local/share/perl/5.14.2 or something like that...
pkgperldir = @PERL_EXT_LIB@

pkgperl_SCRIPTS = \
   $(top_builddir)/bindings/perl/link_grammar_perl.pm

# The la MUST have the same name as the pm, 
# which MUST be the swig -module name!
pkgperl_LTLIBRARIES = link_grammar_perl.la

link_grammar_perl_la_SOURCES = $(BUILT_SOURCES) $(SWIG_SOURCES)

link_grammar_perl_la_CPPFLAGS = \
   $(SWIG_PERL_CPPFLAGS)           \
   $(PERL_CPPFLAGS)                \
   -I$(top_srcdir)/link-grammar    \
   $(ANSI_CFLAGS)

link_grammar_perl_la_LDFLAGS = -version-info @VERSION_INFO@ $(PERL_LDFLAGS) -module
link_grammar_perl_la_LIBADD = $(top_builddir)/link-grammar/liblink-grammar.la

if HAVE_HUNSPELL
link_grammar_perl_la_LIBADD  += ${HUNSPELL_LIBS}
endif

if HAVE_ASPELL
link_grammar_perl_la_LIBADD  += ${ASPELL_LIBS}
endif

$(BUILT_SOURCES): $(SWIG_SOURCES)
	$(SWIG) -perl5 -module link_grammar_perl -I$(top_srcdir)/link-grammar -o $@ $<


EXTRA_DIST =     \
   README        \
   example.pl    \
   test.pl


