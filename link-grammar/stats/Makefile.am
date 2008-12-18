
DEFS = @DEFS@ -DVERSION=\"@VERSION@\" -DDICTIONARY_DIR=\"$(pkgdatadir)\"

INCLUDES = -I$(top_srcdir) $(ANSI_CFLAGS) $(SQLITE3_CFLAGS)


EXTRA_DIST=                               \
	README

bin_PROGRAMS=sniff
sniff_SOURCES = sniff.c
sniff_LDADD = ${SQLITE3_LIBS}
