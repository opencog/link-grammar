#
# Makefile.am
#
# Master makefile for the viterbi decoder.
#

# Skip this entire directory, if not configured
if WITH_VITERBI

INCLUDES = -I.. -I$(top_srcdir) -I$(top_srcdir)/link-grammar

lib_LTLIBRARIES = libvitacog.la

libvitacog_la_SOURCES = \
	atom.cc \
	compile.cc \
	compress.cc \
	connect.cc \
	connector-utils.cc \
	disjoin.cc \
	garbage.cc \
	parser.cc \
	rewrite.cc \
	word-monad.cc \
	atom.h \
	compile.h \
	compress.h \
	connect.h \
	connector-utils.h \
	disjoin.h \
	garbage.h \
	parser.h \
	rewrite.h \
	viterbi.h \
	word-monad.h

# Unit test, to make sure the parser is working correctly.
noinst_PROGRAMS = vitest

vitest_SOURCES = vitest.cc
vitest_LDADD = libvitacog.la
vitest_LDADD += $(top_builddir)/link-grammar/liblink-grammar.la
vitest_LDADD += $(LIBGC_LIBS)


endif
