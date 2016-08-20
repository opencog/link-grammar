#
# Master Makefile for the Link Grammar system
#
SUBDIRS =               \
   link-grammar         \
   viterbi              \
   bindings             \
   link-parser          \
   tests                \
   morphology           \
   data                 \
   man

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
	docker/docker-build.sh                 \
	docker/docker-base/Dockerfile          \
	docker/docker-parser/Dockerfile        \
	docker/docker-python/Dockerfile        \
	docker/docker-server/Dockerfile        \
	msvc14/LGlib-features.props            \
	msvc14/LinkGrammarExe.vcxproj          \
	msvc14/LinkGrammarExe.vcxproj.filters  \
	msvc14/LinkGrammarJava.vcxproj         \
	msvc14/LinkGrammarJava.vcxproj.filters \
	msvc14/LinkGrammar.sln                 \
	msvc14/LinkGrammar.vcxproj             \
	msvc14/LinkGrammar.vcxproj.filters     \
	msvc14/Local.props                     \
	msvc14/mk-link-features-h.bat          \
	msvc14/MSVC-common.props               \
	msvc14/post-build.bat                  \
	msvc14/README                          \
	mingw/README.Cygwin
	mingw/README.MSYS
	TODO
