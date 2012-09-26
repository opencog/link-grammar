# 
# Automake file for building both liblink-grammar.so and 
# the command line application link-parser.
#

DEFS = @DEFS@ -DVERSION=\"@VERSION@\" -DDICTIONARY_DIR=\"$(pkgdatadir)\"

INCLUDES = -I$(top_srcdir)/link-grammar $(ANSI_CFLAGS) $(LINK_CFLAGS) $(HUNSPELL_CFLAGS)

# -----------------------------------------------------------
# Directives to build the link-parser command-line application
bin_PROGRAMS=link-parser
link_parser_SOURCES = link-parser.c \
                      command-line.c \
                      prefix.c \
                      command-line.h

EXTRA_DIST=           prefix.h

link_parser_LDADD = $(LIBEDIT_LIBS)

if WITH_SAT_SOLVER
link_parser_LDADD  += $(top_builddir)/link-grammar/sat-solver/libsat-solver.la
link_parser_LDADD  += $(top_builddir)/link-grammar/minisat/libminisat.la
endif

link_parser_LDADD += $(top_builddir)/link-grammar/liblink-grammar.la

