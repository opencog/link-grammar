
DEFS = @DEFS@ -DVERSION=\"@VERSION@\" -DDICTIONARY_DIR=\"$(pkgdatadir)\"

INCLUDES = -I$(top_srcdir) $(ANSI_CFLAGS) $(SQLITE3_CFLAGS)

if WITH_CORPUS
lib_LTLIBRARIES = liblink-corpus.la
endif

liblink_corpus_la_LDFLAGS = -no-undefined

liblink_corpus_la_SOURCES = \
	corpus.h                 \
	corpus.c


EXTRA_DIST=                               \
	README

bin_PROGRAMS=sniff
sniff_SOURCES = sniff.c
sniff_LDADD = ${SQLITE3_LIBS}
