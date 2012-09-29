SUBDIRS=link-grammar viterbi link-parser data man link-grammar.xcode msvc6 msvc9

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = link-grammar.pc

ACLOCAL_AMFLAGS = -I m4

# Build and install the jar file in $(PREFIX)/share/java ...
# XXX FIXME configure should look to see if it can find ant
if HAVE_JAVA
linkgrammar-@VERSION@.jar:
	ant

javadir=$(datadir)/java
java_DATA = linkgrammar-@VERSION@.jar

if OS_WIN32
LN_SF = cp
else
LN_SF = ln -sf
endif

install-data-hook: 
	${LN_SF} ${javadir}/${java_DATA} ${DESTDIR}/${javadir}/linkgrammar.jar
endif 


EXTRA_DIST =            \
	autogen.sh           \
	link-grammar.pc.in   \
	link-grammar.spec.in \
	link-grammar.spec    \
	AUTHORS              \
	ChangeLog            \
	FindLinkGrammar.cmake\
	LICENSE              \
	MAINTAINERS          \
	README               \
	TODO
