# 
# Automake file for building both liblink-grammar.so and 
# the command line application link-parser.
#

DEFS = @DEFS@ -DVERSION=\"@VERSION@\" -DDICTIONARY_DIR=\"$(pkgdatadir)\"

# $(top_builddir) to pick up autogened link-grammar/link-features.h 
INCLUDES = -I$(top_srcdir) -I$(top_builddir) $(ANSI_CFLAGS) $(LINK_CFLAGS) $(HUNSPELL_CFLAGS)

# -----------------------------------------------------------
# Directives to build the link-parser command-line application
bin_PROGRAMS=link-parser
link_parser_SOURCES = link-parser.c \
                      command-line.c \
                      lg_readline.c \
                      command-line.h \
                      lg_readline.h

link_parser_LDADD = $(top_builddir)/link-grammar/liblink-grammar.la
link_parser_LDADD += $(LIBEDIT_LIBS)
if HAVE_SQLITE
link_parser_LDADD += $(SQLITE3_LIBS)
endif

if WITH_SAT_SOLVER
link_parser_LDADD  += $(top_builddir)/link-grammar/sat-solver/libsat-solver.la
link_parser_LDADD  += $(top_builddir)/link-grammar/minisat/libminisat.la
endif

if WITH_VITERBI
link_parser_LDADD += $(top_builddir)/viterbi/libvitacog.la
link_parser_LDADD += $(LIBGC_LIBS)
endif


