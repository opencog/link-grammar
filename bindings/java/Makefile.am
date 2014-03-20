#
# Makefile for the Java wrapper jar file
#

JAVA_SRCS =                             \
   org/linkgrammar/JSONReader.java      \
   org/linkgrammar/JSONUtils.java       \
   org/linkgrammar/LGConfig.java        \
   org/linkgrammar/LGRemoteClient.java  \
   org/linkgrammar/LGService.java       \
   org/linkgrammar/Linkage.java         \
   org/linkgrammar/LinkGrammar.java     \
   org/linkgrammar/Link.java            \
   org/linkgrammar/ParseResult.java

# Build and install the jar file in $(PREFIX)/share/java ...
linkgrammar-@VERSION@.jar: $(JAVA_SRCS)
	ant

# Let ant delete the files that ant creates.
clean-local:
	ant clean

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
   link-grammar-server.sh            \
   ${JAVA_SRCS}
