#
# Makefile for the Java wrapper jar file
#

# Build and install the jar file in $(PREFIX)/share/java ...
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


EXTRA_DIST =                         \
   README                            \
   org/linkgrammar/JSONReader.java   \
   org/linkgrammar/JSONUtils.java    \
   org/linkgrammar/LGConfig.java     \
   org/linkgrammar/LGRemoteClient.java  \
   org/linkgrammar/LGService.java    \
   org/linkgrammar/Linkage.java      \
   org/linkgrammar/LinkGrammar.java  \
   org/linkgrammar/Link.java         \
   org/linkgrammar/ParseResult.java

