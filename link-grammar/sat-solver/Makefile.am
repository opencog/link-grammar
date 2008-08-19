
VERSION=0.0.1
dnl Version info for libraries = CURRENT:REVISION:AGE
SATSOLVER_VERSION_INFO=0:0:1

libsatsolver_la_LDFLAGS = -version-info $(SATSOLVER_VERSION_INFO)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/minisat

libsatsolver_includedir = $(includedir)/link-grammar $(includedir)/link-grammar/minisat

lib_LTLIBRARIES = libsatsolver.la

libsatsolver_la_SOURCES = \
	clock.hpp         \
	fast-sprintf.cpp  \
	fast-sprintf.hpp  \
	guiding.hpp       \
	sat-encoder.cpp   \
	sat-encoder.hpp   \
	trie.hpp          \
	util.cpp          \
	util.hpp          \
	variables.cpp     \
	variables.hpp     \
	word-tag.cpp      \
	word-tag.hpp
