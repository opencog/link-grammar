#
# Master Makefile for the Link Grammar system
#
SUBDIRS =               \
   link-grammar         \
   viterbi              \
   bindings             \
   link-parser          \
   morphology           \
   data                 \
   man                  \
   link-grammar.xcode

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = link-grammar.pc

ACLOCAL_AMFLAGS = -I m4

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
	NEWS                 \
	README               \
	docker-build.sh      \
	docker-base/Dockerfile          \
	docker-base/download-latest.sh  \
	docker-parser/Dockerfile        \
	docker-parser/build-parser.sh   \
	docker-python/Dockerfile        \
	docker-python/build-python.sh   \
	docker-server/Dockerfile        \
	docker-server/build-server.sh   \
	msvc9/LinkGrammarExe.vcproj     \
	msvc9/LinkGrammarJava.vcproj    \
	msvc9/LinkGrammar.sln           \
	msvc9/LinkGrammar.vcproj        \
	msvc9/README                    \
	msvc12/LinkGrammarExe.vcproj           \
	msvc12/LinkGrammarExe.vcxproj          \
	msvc12/LinkGrammarExe.vcxproj.filters  \
	msvc12/LinkGrammarJava.vcproj          \
	msvc12/LinkGrammarJava.vcxproj         \
	msvc12/LinkGrammarJava.vcxproj.filters \
	msvc12/LinkGrammar.sln                 \
	msvc12/LinkGrammar.vcproj              \
	msvc12/LinkGrammar.vcxproj             \
	msvc12/LinkGrammar.vcxproj.filters     \
	msvc12/LinkGrammar.vcxproj.user        \
	msvc12/README                          \
	TODO
