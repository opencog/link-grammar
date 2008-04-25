if OS_WIN32

install-libtool-import-lib:
	$(INSTALL) .libs/liblink-grammar.dll.a $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/liblink-grammar.dll.a
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

DEFS = @DEFS@ -DVERSION=\"@VERSION@\" -DDICTIONARY_DIR=\"$(pkgdatadir)\"

INCLUDES = -I$(top_srcdir) $(ANSI_CFLAGS) $(LINK_CFLAGS)

lib_LTLIBRARIES = liblink-grammar.la

if HAVE_JAVA
lib_LTLIBRARIES += liblink-grammar-java.la
endif

liblink_grammar_la_LDFLAGS = -version-info @VERSION_INFO@ -export-dynamic -no-undefined -export-symbols $(srcdir)/link-grammar.def

liblink_grammar_java_la_LDFLAGS = -version-info @VERSION_INFO@ -export-dynamic -no-undefined -export-symbols $(srcdir)/link-grammar-java.def

if OS_WIN32
liblink_grammar_la_LDFLAGS += -D_JNI_IMPLEMENTATION_ -Wl,--kill-at
liblink_grammar_java_la_LDFLAGS += -D_JNI_IMPLEMENTATION_ -Wl,--kill-at
endif

if WITH_BINRELOC
liblink_grammar_la_LIBADD  = -lpthread
liblink_grammar_java_la_LIBADD  = -lpthread liblink-grammar.la
else
liblink_grammar_la_LIBADD  = 
liblink_grammar_java_la_LIBADD  = liblink-grammar.la
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
	word-utils.c			\
	prefix.c			\
	api.h				\
	link-includes.h			\
	structures.h			\
	api-structures.h		\
	api.h				\
	externs.h			\
	analyze-linkage.h		\
	and.h				\
	build-disjuncts.h		\
	command-line.h			\
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
	word-file.h \
	word-utils.h

liblink_grammar_includedir = $(includedir)/link-grammar
liblink_grammar_include_HEADERS = 	\
	link-includes.h			\
	link-features.h

liblink_grammar_java_la_SOURCES =		\
	jni-client.h			\
	jni-client.c

EXTRA_DIST=				\
	prefix.h			\
	java/org/link-grammar/LinkGrammar.java		\
	link-grammar.def		\
	link-grammar-java.def

bin_PROGRAMS=link-parser
link_parser_SOURCES = link-parser.c
link_parser_LDADD = $(top_builddir)/link-grammar/liblink-grammar.la

if HAVE_EDITLINE
link_parser_LDADD += -ledit
endif

install-data-local: install-libtool-import-lib

uninstall-local: uninstall-libtool-import-lib
