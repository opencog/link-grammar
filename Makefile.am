SUBDIRS=link-grammar data man link-grammar.xcode msvc6 msvc9

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = link-grammar.pc

# Build and install the jar file in $(PREFIX)/share/java ...
# XXX FIXME configure should look to see if it can find ant
linkgrammar-@VERSION@.jar:
	ant

if HAVE_JAVA
javadir=$(datadir)/java
java_DATA = linkgrammar-@VERSION@.jar
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
