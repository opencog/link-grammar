
VERSION=0.0.1
dnl Version info for libraries = CURRENT:REVISION:AGE
SATSOLVER_VERSION_INFO=1:0:1

libsat_solver_la_LDFLAGS = -version-info $(SATSOLVER_VERSION_INFO)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/link-grammar -I$(top_srcdir)/link-grammar/minisat

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
