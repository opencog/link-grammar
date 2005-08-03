if OS_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32

install-libtool-import-lib:
	$(INSTALL) .libs/liblink-grammar.dll.a $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/liblink-grammar.dll.a
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

DEFS = @DEFS@ -DVERSION=\"@VERSION@\" -DDICTIONARY_DIR=\"$(pkgdatadir):\"

INCLUDES = -I$(top_srcdir) $(OTS_CFLAGS) $(ANSI_CFLAGS) $(BINRELOC_CFLAGS)

lib_LTLIBRARIES = liblink-grammar.la

liblink_grammar_la_LDFLAGS = -version-info @VERSION_INFO@ $(no_undefined) 
if WITH_BINRELOC
	liblink_grammar_la_LIBADD  = -lpthread
else
	liblink_grammar_la_LIBADD  = 
endif
liblink_grammar_la_SOURCES =		\
	analyze-linkage.c		\
	and.c				\
	api.c				\
	build-disjuncts.c		\
	command-line.c			\
	constituents.c			\
	count.c				\
	error.c				\
	extract-links.c			\
	fast-match.c			\
	idiom.c				\
	linkset.c			\
	massage.c			\
	post-process.c			\
	pp_knowledge.c			\
	pp_lexer.c			\
	pp_linkset.c			\
	preparation.c			\
	print.c				\
	print-util.c			\
	prune.c				\
	read-dict.c			\
	resources.c			\
	string-set.c			\
	tokenize.c			\
	utilities.c			\
	word-file.c			\
	prefix.c

liblink_grammar_includedir = $(includedir)/link-grammar
liblink_grammar_include_HEADERS = 	\
	link-includes.h			\
	structures.h			\
	api-structures.h		\
	api.h				\
	externs.h			\
	analyze-linkage.h		\
	and.h				\
	build-disjuncts.h		\
	constituents.h			\
	count.h				\
	error.h				\
	extract-links.h			\
	fast-match.h			\
	idiom.h				\
	linkset.h			\
	massage.h			\
	post-process.h			\
	pp_knowledge.h			\
	pp_lexer.h			\
	pp_linkset.h			\
	preparation.h			\
	print.h				\
	print-util.h			\
	prune.h				\
	read-dict.h			\
	resources.h			\
	string-set.h			\
	tokenize.h			\
	utilities.h			\
	word-file.h

EXTRA_DIST=				\
	prefix.h			\
	command-line.h

bin_PROGRAMS=grammar-parse
grammar_parse_SOURCES=parse.c
ldadd = $(top_builddir)/link-grammar/liblink-grammar.la

grammar_parse_LDADD=$(ldadd)

install-data-local: install-libtool-import-lib

uninstall-local: uninstall-libtool-import-lib
