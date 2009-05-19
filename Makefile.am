SUBDIRS=link-grammar data man link-grammar.xcode msvc6 msvc9

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
LN_S = cp
else
LN_S = ln -sf
endif

install-data-hook: 
	(cd ${DESTDIR}${javadir} && $(LN_S) $(java_DATA) linkgrammar.jar)
endif 


EXTRA_DIST =            \
	autogen.sh           \
	link-grammar.pc.in   \
	link-grammar.spec.in \
	link-grammar.spec    \
	AUTHORS              \
	ChangeLog            \
	LICENSE              \
	MAINTAINERS          \
	README               \
	TODO
