#
# Automake file for building the command line application `link-parser`.
#

# Directives to build the link-parser command-line application
bin_PROGRAMS = link-parser
if HAVE_ARGP
bin_PROGRAMS += link-generator
endif

link_parser_SOURCES = link-parser.c \
                      command-line.c \
                      lg_readline.c \
                      parser-utilities.h \
                      parser-utilities.c \
                      command-line.h \
                      lg_readline.h

# -I$(top_builddir) to pick up autogened link-grammar/link-features.h
link_parser_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir)
link_parser_CFLAGS = $(WARN_CFLAGS) $(HUNSPELL_CFLAGS) $(LIBEDIT_CFLAGS)
link_parser_LDFLAGS = $(LINK_CFLAGS)
link_parser_LDADD = $(LIBEDIT_LIBS) $(top_builddir)/link-grammar/liblink-grammar.la

link_generator_SOURCES = link-generator.c \
                         generator-utilities.h \
                         generator-utilities.c \
                         parser-utilities.h \
                         parser-utilities.c

link_generator_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/link-grammar -I$(top_srcdir)/link-grammar/dict-common -UHAVE_EDITLINE
link_generator_CFLAGS = $(WARN_CFLAGS)
link_generator_LDFLAGS = $(LINK_CFLAGS)
link_generator_LDADD = $(top_builddir)/link-grammar/liblink-grammar.la

# Installation checks, to be manually done after "make install".
# link-parser checks:
# 1. Show the location of its binary.
# 2. Check that it can find the system link-grammar dictionary directory.
# Note the "cd .libs" - it ensures that it will not find "../data".
# To run these checks, issue "make installcheck" from a regular user.
installcheck-local:
	$(AM_V_at)echo; echo -n 'Check location of '; cd .libs; \
		PATH=$(bindir) /usr/bin/which link-parser; echo
	$(AM_V_at)cd .libs; [ "`PATH=$(bindir) /usr/bin/which link-parser`" == $(bindir)/link-parser ] || \
		{ echo -n "Incorrect location for link-parser: "; \
		PATH=$(bindir) /usr/bin/which link-parser; echo; \
		echo "Expected $(bindir)/link-parser"; \
		echo "PATH is ${PATH}"; exit 1; }
	$(AM_V_at)cd .libs; for d in . .. ../data ./data; do \
		[ ! -d $$d/en ] ||  { echo "Unexpected dictionary $$d/en"; exit 1; }; \
	done
	cd .libs; echo "This is a test" | $(bindir)/link-parser
