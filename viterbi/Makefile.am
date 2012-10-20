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
	connect.cc \
	connector-utils.cc \
	parser.cc \
	state.cc \
	atom.h \
	compile.h \
	connect.h \
	connector-utils.h \
	parser.h \
	state.h \
	viterbi.h

# Unit test, to make sure the parser is working correctly.
noinst_PROGRAMS = vitest

vitest_SOURCES = vitest.cc
vitest_LDADD = libvitacog.la
vitest_LDADD += $(top_builddir)/link-grammar/liblink-grammar.la



endif
