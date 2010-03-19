# 
# Automake file for building both liblink-grammar.so and 
# the command line application link-parser.
#

if WITH_SAT_SOLVER
SUBDIRS=minisat sat-solver
endif

if WITH_CORPUS
SUBDIRS=corpus
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

DEFS = @DEFS@ -DVERSION=\"@VERSION@\" -DDICTIONARY_DIR=\"$(pkgdatadir)\"

INCLUDES = -I$(top_srcdir) $(ANSI_CFLAGS) $(LINK_CFLAGS) $(HUNSPELL_CFLAGS)

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

liblink_grammar_la_LIBADD  =  ${REGEX_LIBS}
liblink_grammar_java_la_LIBADD  = liblink-grammar.la  ${REGEX_LIBS}

if HAVE_HUNSPELL
liblink_grammar_la_LIBADD  +=  ${HUNSPELL_LIBS}
liblink_grammar_java_la_LIBADD  += ${HUNSPELL_LIBS}
endif

if HAVE_ASPELL
liblink_grammar_la_LIBADD  +=  ${ASPELL_LIBS}
liblink_grammar_java_la_LIBADD  += ${ASPELL_LIBS}
endif

if WITH_SAT_SOLVER
liblink_grammar_la_LIBADD  += $(top_builddir)/link-grammar/sat-solver/libsat-solver.la
liblink_grammar_la_LIBADD  += $(top_builddir)/link-grammar/minisat/libminisat.la
liblink_grammar_java_la_LIBADD  += $(top_builddir)/link-grammar/sat-solver/libsat-solver.la
liblink_grammar_java_la_LIBADD  += $(top_builddir)/link-grammar/minisat/libminisat.la
endif

if WITH_CORPUS
liblink_grammar_la_LIBADD  += $(top_builddir)/link-grammar/corpus/liblink-corpus.la ${SQLITE3_LIBS}
liblink_grammar_java_la_LIBADD  += $(top_builddir)/link-grammar/corpus/liblink-corpus.la ${SQLITE3_LIBS}
endif

if WITH_BINRELOC
liblink_grammar_la_LIBADD  += -lpthread
liblink_grammar_java_la_LIBADD  += -lpthread
endif

if WITH_PTHREADS
liblink_grammar_la_LIBADD  += -lpthread
liblink_grammar_java_la_LIBADD  += -lpthread
endif

# Math libraries are needed for floorf, etc.
liblink_grammar_la_LIBADD  += -lm
liblink_grammar_java_la_LIBADD  += -lm

liblink_grammar_la_SOURCES =        \
	analyze-linkage.c                \
	and.c                            \
	api.c                            \
	build-disjuncts.c                \
	command-line.c                   \
	constituents.c                   \
	count.c                          \
	disjunct-utils.c                 \
	disjuncts.c                      \
	error.c                          \
	expand.c                         \
	extract-links.c                  \
	fast-match.c                     \
	idiom.c                          \
	massage.c                        \
	post-process.c                   \
	pp_knowledge.c                   \
	pp_lexer.c                       \
	pp_linkset.c                     \
	preparation.c                    \
	print.c                          \
	print-util.c                     \
	prune.c                          \
	read-dict.c                      \
	read-regex.c                     \
	regex-morph.c                    \
	resources.c                      \
	spellcheck-aspell.c              \
	spellcheck-hun.c                 \
	string-set.c                     \
	tokenize.c                       \
	utilities.c                      \
	word-file.c                      \
	word-utils.c                     \
	prefix.c                         \
	api.h                            \
	api-structures.h                 \
	api-types.h                      \
	analyze-linkage.h                \
	and.h                            \
	build-disjuncts.h                \
	command-line.h                   \
	constituents.h                   \
	count.h                          \
	disjunct-utils.h                 \
	disjuncts.h                      \
	error.h                          \
	expand.h                         \
	externs.h                        \
	extract-links.h                  \
	fast-match.h                     \
	idiom.h                          \
	link-includes.h                  \
	massage.h                        \
	post-process.h                   \
	pp_knowledge.h                   \
	pp_lexer.h                       \
	pp_linkset.h                     \
	preparation.h                    \
	print.h                          \
	print-util.h                     \
	prune.h                          \
	read-dict.h                      \
	read-regex.h                     \
	regex-morph.h                    \
	resources.h                      \
	spellcheck.h                     \
	string-set.h                     \
	structures.h                     \
	tokenize.h                       \
	utilities.h                      \
	word-file.h                      \
	word-utils.h

liblink_grammar_includedir = $(includedir)/link-grammar
liblink_grammar_include_HEADERS =   \
	link-includes.h                  \
	link-features.h

liblink_grammar_java_la_SOURCES =   \
	jni-client.h                     \
	jni-client.c

EXTRA_DIST=                               \
	prefix.h                               \
	autoit/_LGTest.au3                     \
	autoit/_LinkGrammar.au3                \
	autoit/README                          \
	java/README                            \
	java/org/linkgrammar/JSONReader.java   \
	java/org/linkgrammar/LGConfig.java     \
	java/org/linkgrammar/LGRemoteClient.java  \
	java/org/linkgrammar/LGService.java    \
	java/org/linkgrammar/Linkage.java      \
	java/org/linkgrammar/LinkGrammar.java  \
	java/org/linkgrammar/Link.java         \
	java/org/linkgrammar/ParseResult.java  \
	link-grammar.def                       \
	link-grammar-java.def


# -----------------------------------------------------------
# Directives to build the link-parser command-line application
bin_PROGRAMS=link-parser
link_parser_SOURCES = link-parser.c
link_parser_LDADD = $(LIBEDIT_LIBS)

if WITH_SAT_SOLVER
link_parser_LDADD  += $(top_builddir)/link-grammar/sat-solver/libsat-solver.la
link_parser_LDADD  += $(top_builddir)/link-grammar/minisat/libminisat.la
endif

link_parser_LDADD += liblink-grammar.la

# -----------------------------------------------------------
install-data-local: install-libtool-import-lib

uninstall-local: uninstall-libtool-import-lib
