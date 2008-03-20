SUBDIRS=link-grammar data man link-grammar.xcode

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = link-grammar.pc

# install the jar file in $(PREFIX)/share/java ...
# but only if it got built...!?
link-grammar-@VERSION@.jar:
	ant

if HAVE_JAVA
javadir=$(datadir)/java
java_DATA = link-grammar-@VERSION@.jar
endif 


EXTRA_DIST =			\
	autogen.sh		\
	link-grammar.pc.in	\
	link-grammar.spec.in	\
	link-grammar.spec	\
	AUTHORS			\
	ChangeLog		\
	LICENSE			\
	MAINTAINERS		\
	README 			\
	TODO
