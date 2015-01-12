#
# Automake file for building unit tests.
#

# The tests really require c++11 to build...
CXXFLAGS += -std=c++11

# $(top_builddir) to pick up autogened link-grammar/link-features.h
AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir) $(CXX_FLAGS) \
    $(WARN_CXXFLAGS)

# -----------------------------------------------------------
# TESTS declares the tests to actually run;
# check_PROGRAMS are the binaries to build.
TESTS = dict-reopen multi-thread
check_PROGRAMS = dict-reopen multi-thread

dict_reopen_SOURCES = dict-reopen.cc
multi_thread_SOURCES = multi-thread.cc

LDADD = $(top_builddir)/link-grammar/liblink-grammar.la
if HAVE_SQLITE
LDADD += $(SQLITE3_LIBS)
endif

if WITH_SAT_SOLVER
LDADD  += $(top_builddir)/link-grammar/sat-solver/libsat-solver.la
LDADD  += $(top_builddir)/link-grammar/minisat/libminisat.la
endif

