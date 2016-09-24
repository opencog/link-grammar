AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/link-grammar -I$(top_builddir) \
				  ${MINISAT_INCLUDES} $(WARN_CXXFLAGS)

AM_CXXFLAGS = ${MINISAT_CXXFLAGS}

noinst_LTLIBRARIES = libsat-solver.la

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
