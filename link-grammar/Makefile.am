#
# Automake file for building liblink-grammar.so
#

SUBDIRS=

if LIBMINISAT_BUNDLED
MINISAT_DIR = minisat
endif

if WITH_SAT_SOLVER
SUBDIRS += $(MINISAT_DIR) sat-solver
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

if OS_WIN32
   fileformat = $(if $(filter 1,$(words [$(pkgdatadir)])),windows,dos) # dos if whitespace
   pkgdatadir_realpath := $(shell cygpath --${fileformat} --mixed "$(pkgdatadir)")
else
   pkgdatadir_realpath := $(pkgdatadir)
endif

DEFS = @DEFS@ -DVERSION=\"@VERSION@\" -DDICTIONARY_DIR=\"$(pkgdatadir_realpath)\" -DCC=\"$(CC)\"

# $(top_builddir) to pick up autogened link-grammar/link-features.h
AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir)
AM_CFLAGS = $(WARN_CFLAGS) $(HUNSPELL_CFLAGS) $(REGEX_CFLAGS) $(SQLITE3_CFLAGS) $(PTHREAD_CFLAGS)

post-process/pp_lexer.lo: AM_CPPFLAGS += -I$(top_srcdir)/link-grammar/post-process
DISTCLEANFILES = $(top_builddir)/link-grammar/post-process/pp_lexer.c

lib_LTLIBRARIES = liblink-grammar.la

liblink_grammar_la_LDFLAGS = -version-info @VERSION_INFO@ \
	-export-dynamic -no-undefined \
	$(PTHREAD_CFLAGS)

liblink_grammar_la_LIBADD  =  ${REGEX_LIBS}

if HAVE_HUNSPELL
liblink_grammar_la_LIBADD  +=  ${HUNSPELL_LIBS}
endif

if HAVE_ASPELL
liblink_grammar_la_LIBADD  +=  ${ASPELL_LIBS}
endif

if HAVE_SQLITE3
liblink_grammar_la_LIBADD  +=  ${SQLITE3_LIBS}
endif

if HAVE_ATOMSPACE
liblink_grammar_la_LIBADD  +=  ${ATOMSPACE_LIBS}
endif

if WITH_SAT_SOLVER
# Dummy C++ source to cause C++ linking.
nodist_EXTRA_liblink_grammar_la_SOURCES = dummy.cxx

liblink_grammar_la_LIBADD  += $(top_builddir)/link-grammar/sat-solver/libsat-solver.la
if LIBMINISAT_BUNDLED
liblink_grammar_la_LIBADD  += $(top_builddir)/link-grammar/minisat/libminisat.la
else !LIBMINISAT_BUNDLED
liblink_grammar_la_LIBADD  += ${MINISAT_LIBS}
endif !LIBMINISAT_BUNDLED
endif WITH_SAT_SOLVER

# Needed for tss_create and tss_set
liblink_grammar_la_LIBADD  += ${PTHREAD_LIBS}

# It seems no mathematical function is used now. Keep it to be on the safe side.
liblink_grammar_la_LIBADD  += -lm

if WITH_CXXREGEX
dict-common/regex-morph.lo: AM_CFLAGS = -x c++ $(CXXFLAGS) $(WARN_CXXFLAGS)
dict-common/regex-morph.lo: CFLAGS =
endif

liblink_grammar_la_SOURCES =        \
	api.c                            \
	connectors.c                     \
	dict-common/dialect.c            \
	dict-common/dict-common.c        \
	dict-common/dict-impl.c          \
	dict-common/dict-utils.c         \
	dict-common/file-utils.c         \
	dict-common/idiom.c              \
	dict-common/print-dict.c         \
	dict-common/regex-morph.c        \
	dict-file/read-dialect.c         \
	dict-file/dictionary.c           \
	dict-file/read-dict.c            \
	dict-file/read-regex.c           \
	dict-file/word-file.c            \
	dict-ram/dict-ram.c              \
	dict-sql/read-sql.c              \
	dict-atomese/read-atomese.c      \
	dict-atomese/lookup-atomese.cc   \
	dict-atomese/fetch-cats.cc       \
	dict-atomese/word-pairs.cc       \
	disjunct-utils.c                 \
	error.c                          \
	linkage/analyze-linkage.c        \
	linkage/freeli.c                 \
	linkage/linkage.c                \
	linkage/lisjuncts.c              \
	linkage/sane.c                   \
	linkage/score.c                  \
	memory-pool.c                    \
	parse/count.c                    \
	parse/extract-links.c            \
	parse/fast-match.c               \
	parse/histogram.c                \
	parse/parse.c                    \
	parse/preparation.c              \
	parse/prune.c                    \
	post-process/constituents.c      \
	post-process/post-process.c      \
	post-process/pp_knowledge.c      \
	post-process/pp_lexer.l          \
	post-process/pp_linkset.c        \
	prepare/build-disjuncts.c        \
	prepare/exprune.c                \
	print/print.c                    \
	print/print-util.c               \
	print/wcwidth.c                  \
	resources.c                      \
	string-set.c                     \
	string-id.c                      \
	tokenize/anysplit.c              \
	tokenize/spellcheck-aspell.c     \
	tokenize/spellcheck-hun.c        \
	tokenize/tokenize.c              \
	tokenize/wg-display.c            \
	tokenize/wordgraph.c             \
	tracon-set.c                     \
	utilities.c                      \
	                                 \
	api-structures.h                 \
	api-types.h                      \
	connectors.h                     \
	const-prime.h                    \
	dict-common/dialect.h            \
	dict-common/dict-affix.h         \
	dict-common/dict-api.h           \
	dict-common/dict-common.h        \
	dict-common/dict-defines.h       \
	dict-common/dict-impl.h          \
	dict-common/dict-structures.h    \
	dict-common/dict-utils.h         \
	dict-common/file-utils.h         \
	dict-common/idiom.h              \
	dict-common/regex-morph.h        \
	dict-file/read-dialect.h         \
	dict-file/read-dict.h            \
	dict-file/read-regex.h           \
	dict-file/word-file.h            \
	dict-ram/dict-ram.h              \
	dict-sql/read-sql.h              \
	dict-atomese/dict-atomese.h      \
	dict-atomese/local-as.h          \
	dict-atomese/lookup-atomese.h    \
	dict-atomese/read-atomese.h      \
	disjunct-utils.h                 \
	error.h                          \
	externs.h                        \
	link-includes.h                  \
	linkage/analyze-linkage.h        \
	linkage/linkage.h                \
	linkage/lisjuncts.h              \
	linkage/sane.h                   \
	linkage/score.h                  \
	memory-pool.h                    \
	parse/count.h                    \
	parse/extract-links.h            \
	parse/fast-match.h               \
	parse/histogram.h                \
	parse/parse.h                    \
	parse/preparation.h              \
	parse/prune.h                    \
	post-process/post-process.h      \
	post-process/pp_knowledge.h      \
	post-process/pp_lexer.h          \
	post-process/pp_linkset.h        \
	post-process/pp-structures.h     \
	prepare/build-disjuncts.h        \
	prepare/exprune.h                \
	print/print.h                    \
	print/print-util.h               \
	print/wcwidth.h                  \
	resources.h                      \
	string-set.h                     \
	string-id.h                      \
	tokenize/anysplit.h              \
	tokenize/spellcheck.h            \
	tokenize/tok-structures.h        \
	tokenize/tokenize.h              \
	tokenize/word-structures.h       \
	tokenize/wordgraph.h             \
	tracon-set.h                     \
	utilities.h

liblink_grammar_includedir = $(includedir)/link-grammar
liblink_grammar_include_HEADERS =   \
	link-features.h                  \
	link-includes.h                  \
	dict-atomese/dict-atomese.h      \
	dict-common/dict-api.h           \
	dict-common/dict-defines.h       \
	dict-common/dict-structures.h

uninstall-hook:
	-rmdir $(liblink_grammar_includedir)

EXTRA_DIST=                         \
	README.md                        \
	dict-ram/README.md               \
	dict-sql/dict.sql                \
	dict-sql/demo.sql                \
	dict-sql/README.md               \
	dict-atomese/README.md           \
	tokenize/README.md

# -----------------------------------------------------------
install-data-local: install-libtool-import-lib

uninstall-local: uninstall-libtool-import-lib
