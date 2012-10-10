# 
# Automake file for building both liblink-grammar.so and 
# the command line application link-parser.
#

DEFS = @DEFS@ -DVERSION=\"@VERSION@\" -DDICTIONARY_DIR=\"$(pkgdatadir)\"

# In link-grammar version 5, fat links will be gone!
DEFS += -DUSE_FAT_LINKAGES

INCLUDES = -I.. -I$(top_srcdir)/link-grammar $(ANSI_CFLAGS) $(LINK_CFLAGS) $(HUNSPELL_CFLAGS)

# -----------------------------------------------------------
# Directives to build the link-parser command-line application
bin_PROGRAMS=link-parser
link_parser_SOURCES = link-parser.c \
                      command-line.c \
                      command-line.h

link_parser_LDADD = $(LIBEDIT_LIBS)

if WITH_SAT_SOLVER
link_parser_LDADD  += $(top_builddir)/link-grammar/sat-solver/libsat-solver.la
link_parser_LDADD  += $(top_builddir)/link-grammar/minisat/libminisat.la
endif

if WITH_VITERBI
link_parser_LDADD += $(top_builddir)/viterbi/libvitacog.la
endif

link_parser_LDADD += $(top_builddir)/link-grammar/liblink-grammar.la


