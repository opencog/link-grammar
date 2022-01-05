#
# Master Makefile for the Link Grammar system
#
SUBDIRS =               \
   link-grammar         \
   bindings             \
   link-parser          \
   tests                \
   morphology           \
   data                 \
   man

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = link-grammar.pc

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST =                            \
	autogen.sh                           \
	link-grammar.pc.in                   \
	link-grammar.spec.in                 \
	link-grammar.spec                    \
	AUTHORS                              \
	ChangeLog                            \
	FindLinkGrammar.cmake                \
	LICENSE                              \
	MAINTAINERS                          \
	NEWS                                 \
	README.md                            \
	debug/README.md                      \
	docker/docker-build.sh               \
	docker/docker-base/Dockerfile        \
	docker/docker-parser/Dockerfile      \
	docker/docker-python/Dockerfile      \
	docker/docker-server/Dockerfile      \
	m4/lg_visibility.m4                  \
	man/link-generator.1                 \
	man/link-parser.1                    \
	mingw/README-Cygwin.md               \
	mingw/README-MinGW64.md              \
	mingw/link-parser-mingw.bat          \
	mingw/link-parser-cygwin.bat         \
	msvc/LGlib-features.props            \
	msvc/LinkGrammarExe.vcxproj          \
	msvc/LinkGrammarExe.vcxproj.filters  \
	msvc/LinkGrammarJava.vcxproj         \
	msvc/LinkGrammarJava.vcxproj.filters \
	msvc/LinkGrammar.sln                 \
	msvc/LinkGrammar.vcxproj             \
	msvc/LinkGrammar.vcxproj.filters     \
	msvc/Local.props                     \
	msvc/confvar.bat                     \
	msvc/MSVC-common.props               \
	msvc/post-build.bat                  \
	msvc/Python3.vcxproj                 \
	msvc/Python3.vcxproj.filters         \
	msvc/README.md                       \
	msvc/make-check.py                   \
	TODO
