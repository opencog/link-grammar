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
   link-grammar.xcode   \
   msvc6                \
   msvc9                \
   msvc12

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
	TODO                 \
	docker-build.sh      \
	docker-base/Dockerfile          \
	docker-base/download-latest.sh
