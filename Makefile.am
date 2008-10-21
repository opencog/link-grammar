SUBDIRS=link-grammar data man link-grammar.xcode

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = link-grammar.pc

# Build and install the jar file in $(PREFIX)/share/java ...
# XXX FIXME configure should look to see if it can find ant
link-grammar-@VERSION@.jar:
	ant

if HAVE_JAVA
javadir=$(datadir)/java
java_DATA = link-grammar-@VERSION@.jar
endif 


EXTRA_DIST =            \
	autogen.sh           \
	link-grammar.pc.in   \
	link-grammar.spec.in \
	link-grammar.spec    \
	msvc6                \
	AUTHORS              \
	ChangeLog            \
	LICENSE              \
	MAINTAINERS          \
	README               \
	TODO

# Specifying `msvc6' in EXTRA_DIST causes the .svn to also get
# sucked in. But we don't want to actually distribute these.
dist-hook:
	rm -rf $(distdir)/msvc6/.svn
	rm -rf $(distdir)/msvc6/*/.svn
