# 
# Automake file for building unit tests.
#

DEFS = @DEFS@ -DVERSION=\"@VERSION@\" -DDICTIONARY_DIR=\"$(pkgdatadir)\"

# $(top_builddir) to pick up autogened link-grammar/link-features.h 
AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir) $(ANSI_CFLAGS) \
    $(LINK_CFLAGS) $(HUNSPELL_CFLAGS) -std=c++11

# -----------------------------------------------------------
# Directives to build the threading unit test.
bin_PROGRAMS=multi-thread
multi_thread_SOURCES = multi-thread.cc 

multi_thread_LDADD = $(top_builddir)/link-grammar/liblink-grammar.la
if HAVE_SQLITE
multi_thread_LDADD += $(SQLITE3_LIBS)
endif

if WITH_SAT_SOLVER
multi_thread_LDADD  += $(top_builddir)/link-grammar/sat-solver/libsat-solver.la
multi_thread_LDADD  += $(top_builddir)/link-grammar/minisat/libminisat.la
endif

