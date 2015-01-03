
VERSION=0.0.4
SATSOLVER_VERSION_INFO=4:0:4

libsat_solver_la_LDFLAGS = -version-info $(SATSOLVER_VERSION_INFO) \
	$(LINK_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/link-grammar \
	-I$(top_srcdir)/link-grammar/minisat -I$(top_builddir) \
	$(WARN_CXXXFLAGS)

libsat_solver_includedir = $(includedir)/link-grammar $(includedir)/link-grammar/minisat

lib_LTLIBRARIES = libsat-solver.la

libsat_solver_la_SOURCES = \
	clock.hpp         \
	fast-sprintf.cpp  \
	fast-sprintf.hpp  \
	guiding.hpp       \
	matrix-ut.hpp     \
	sat-encoder.cpp   \
	sat-encoder.hpp   \
	sat-encoder.h     \
	trie.hpp          \
	util.cpp          \
	util.hpp          \
	variables.cpp     \
	variables.hpp     \
	word-tag.cpp      \
	word-tag.hpp
