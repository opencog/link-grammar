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

if HAVE_ANT
# distclean-generic will delete build.xml, so clean-local must run
# first.... but then, build.xml doesn't get removed :-(
distclean-generic: clean-local
distclean-local: clean-local
	-rm -f build.xml

# Let ant delete the files that ant creates.
clean-local:
	ant clean
	-rm -f *.jar
endif

javadir=$(datadir)/java
java_DATA = linkgrammar-@VERSION@.jar

if OS_WIN32
LN_SF = cp
else
LN_SF = ln -sf
endif

install-data-hook:
	${LN_SF} ${javadir}/${java_DATA} ${DESTDIR}${javadir}/linkgrammar.jar
if HAVE_MVN
	: # Install linkgrammar into the local maven repository
	mvn install:install-file \
	  -Dfile=${java_DATA} \
	  -DgroupId=org.opencog \
	  -DartifactId=linkgrammar \
	  -Dversion=@VERSION@ \
	  -Dpackaging=jar
endif

uninstall-hook:
	-rm ${DESTDIR}${javadir}/linkgrammar.jar
if HAVE_MVN
	-rm -rf ~/.m2/repository/org/opencog/linkgrammar/@VERSION@
endif

dist-hook:
if HAVE_ANT
	: # Validate that the bindings can actually be created
	zipinfo $(java_DATA) org/linkgrammar/LinkGrammar.class >/dev/null
endif

EXTRA_DIST =                         \
   README                            \
   link-grammar-server.sh            \
   ${JAVA_SRCS}
