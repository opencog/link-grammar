
# Skip this entire directory, if not configured
if WITH_VITERBI

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/link-grammar

lib_LTLIBRARIES = libvitacog.la

libvitacog_la_SOURCES = \
	atom.cc \
	connector-utils.cc \
	state.cc \
	atom.h \
	connector-utils.h \
	viterbi.h

endif
