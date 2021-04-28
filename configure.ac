# This file is processed by the Windows build system using confvar.bat. It
# assumes AC_INIT is the first command, with no blank lines before it and with
# a blank line after it. If this things are changed, change confvar.bat
# accordingly.
# Don't end the URLs with a slash even if it appears in the official address,
# because we may append a component. Also, they cannot include "!".
AC_INIT([link-grammar],[5.9.1],[https://github.com/opencog/link-grammar],[],
		  [https://www.abisource.com/projects/link-grammar])

dnl Check whether we want to set defaults for CFLAGS and CXXFLAGS
AC_MSG_CHECKING([whether configure should try to set CFLAGS/CXXFLAGS])
AS_IF([test "x${CFLAGS+set}" = "xset" || test "x${CXXFLAGS+set}" = "xset"], [
    enable_flags_setting=no
    : ${CFLAGS="-g -O3"}
    : ${CXXFLAGS="-g -O3"}
  ], [
    enable_flags_setting=yes
    dnl Set to -g -O3 here so AC_PROG_CC and AC_PROG_CXX do not add -g -O2
    CFLAGS="-g -O3"
    CXXFLAGS="-g -O3"
  ])
AC_MSG_RESULT([${enable_flags_setting}])

DISCUSSION_GROUP=https://groups.google.com/d/forum/link-grammar
AC_SUBST(DISCUSSION_GROUP)
OVERVIEW=https://en.wikipedia.org/wiki/Link_grammar
AC_SUBST(OVERVIEW)

dnl Set release number
dnl This is derived from "Versioning" chapter of info libtool documentation.
PACKAGE=link-grammar
dnl     4a) Increment when removing or changing interfaces.
LINK_MAJOR_VERSION=5
dnl     4a) 5) Increment when adding interfaces.
dnl     6) Set to zero when removing or changing interfaces.
LINK_MINOR_VERSION=9
dnl     3) Increment when interfaces not changed at all,
dnl        only bug fixes or internal changes made.
dnl     4b) Set to zero when adding, removing or changing interfaces.
LINK_MICRO_VERSION=1
dnl
dnl     Set this too
MAJOR_VERSION_PLUS_MINOR_VERSION=`expr $LINK_MAJOR_VERSION + $LINK_MINOR_VERSION`
dnl
VERSION=$LINK_MAJOR_VERSION.$LINK_MINOR_VERSION.$LINK_MICRO_VERSION
# Version info for libraries = CURRENT:REVISION:AGE
VERSION_INFO=$MAJOR_VERSION_PLUS_MINOR_VERSION:$LINK_MICRO_VERSION:$LINK_MINOR_VERSION

AC_SUBST(VERSION_INFO)
AC_SUBST(LINK_MAJOR_VERSION)
AC_SUBST(LINK_MINOR_VERSION)
AC_SUBST(LINK_MICRO_VERSION)

AM_INIT_AUTOMAKE([foreign subdir-objects])

# When maintainer-mode is enabled, downstream users sometimes trip
# trip over automake issues. See bug #649.
# AM_MAINTAINER_MODE([disable])
AM_MAINTAINER_MODE([enable])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_SRCDIR($PACKAGE)

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])], [AC_SUBST([AM_DEFAULT_VERBOSITY],1)])

dnl "Undefined AX_ macro" is checked in autogen.sh, so if this message
dnl is changed here it should be changed there too.
m4_pattern_forbid([^AX_(COMPILER_VENDOR|TLS|AX_CHECK_ZLIB|PKG_SWIG)\b],
	[Undefined AX_ macro. Please install the "autoconf-archive" package.])

# OS checks, including Win32
AC_CANONICAL_HOST
dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AX_COMPILER_VENDOR
AC_PROG_CPP
AM_PROG_LEX(noyywrap)
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
LT_INIT([win32-dll])
PKG_PROG_PKG_CONFIG

# The below says "use the C compiler for all remaining tests".
# That way, if g++ is not installed, configure won't crap out
# (with a mystery error about not finding regex.h)
AC_LANG([C])

AC_C_CONST

AC_CHECK_FUNCS(strndup strtok_r sigaction)
AC_CHECK_FUNCS(aligned_alloc posix_memalign _aligned_malloc)

AC_FUNC_ALLOCA

# For the Wordgraph display code
AC_FUNC_FORK
AC_CHECK_FUNCS(prctl)

dnl CentOS-7 (at least) needs these to expose the C99 format and
dnl limit macros in C++ source code.  They are needed for minisat.
AC_DEFINE(__STDC_FORMAT_MACROS)
AC_DEFINE(__STDC_LIMIT_MACROS)

dnl Check for a keyword for Thread Local Storage declaration.
dnl If found - define TLS to it.
lg_tls=no
AX_TLS(,:)
test "$ac_cv_tls" != "none" && lg_tls=yes

AX_PTHREAD
if [ test -n "$ax_pthread_ok" ]; then
	CC="${PTHREAD_CC:-CC}"
	#CCX="${PTHREAD_CC:-CCX}"

	dnl Currently, libtool uses --nostdlib when linking shared libraries,
	dnl which causes the -pthread flag to be ignored. Try to work around that
	dnl by explicitly specifying the pthread library unless it is already set.
	if [ test -z "$PTHREAD_LIBS" ]; then
		PTHREAD_LIBS=-lpthread
	fi
fi

dnl Check if we can use C11 threads functions
AC_CHECK_HEADERS_ONCE([threads.h])

dnl If the visibility __attribute__  is supported, define HAVE_VISIBILITY
dnl and a variable a CFLAG_VISIBILITY, to be added to CFLAGS/CXXFLAGS.
LG_C_ATTRIBUTE_VISIBILITY

dnl Check for specific OSs
# ====================================================================

AC_MSG_CHECKING([for native Win32])
case "$host_os" in
  *cygwin* | *msys* | *mingw* | *pw32 | *cegcc*)
    native_win32=yes
    ;;
  *)
    native_win32=no
    ;;
esac
AC_MSG_RESULT([$native_win32])
AM_CONDITIONAL(OS_WIN32, test "x$native_win32" = "xyes")

AC_MSG_CHECKING([for Cygwin])
case "$host_os" in
  *cygwin*)
    cygwin=yes
    ;;
  *)
    cygwin=no
    ;;
esac
AC_MSG_RESULT([$cygwin])
AM_CONDITIONAL(OS_CYGWIN, test "x$cygwin" = "xyes")

AC_MSG_CHECKING([for 64-bit Apple OSX])
case "$host_os" in
  darwin*)
    apple_osx=yes
    ;;
  *)
    apple_osx=no
    ;;
esac
AC_MSG_RESULT([$apple_osx])
AM_CONDITIONAL(OS_X, test "x$apple_osx" = "xyes")

HOST_OS="$host_os"
AC_SUBST(HOST_OS)
# ====================================================================

CXXFLAGS="${CXXFLAGS} -Wall"

# The std=c11 flag provides the proper float-pt math decls working,
# e.g. fmax  However, it also undefined _BSD_SOURCE, etc. which is
# needed to get fileno, strdup, etc. and so it needs to be manually
# enabled again.

# Setting -D_POSIX_SOURCE messes up compilation on FreeBSD by
# hiding strdup, etc. again.

# Final solution: enable std=c11, explicitly turn on BSD and SVID and
# GNU, but do NOT turn on POSIX.

CFLAGS="-std=c11 -D_BSD_SOURCE -D_SVID_SOURCE -D_GNU_SOURCE -D_ISOC11_SOURCE ${CFLAGS} ${CFLAG_VISIBILITY}"
CXXFLAGS="-std=c++11 ${CXXFLAGS} ${CFLAG_VISIBILITY}"

if test x${native_win32} = xyes; then
	# We need the shlwapi for PathRemoveFileSpecA().
	LDFLAGS="${LDFLAGS} -lshlwapi"

	# By default, MinGW doesn't know about %zu, which we use a lot, even
	# on systems which have MSVC14, which has c99 stdio (printf prints
	# "zu"). Add __USE_MINGW_ANSI_STDIO in order to solve that.
	AC_DEFINE(__USE_MINGW_ANSI_STDIO, 1)
	AC_DEFINE(__printf__, gnu_printf)

	# Only running on Vista and on is supported.
	AC_DEFINE(NTDDI_VERSION, NTDDI_VISTA)
	AC_DEFINE(_WIN32_WINNT, _WIN32_WINNT_VISTA)
fi

# _BSD_SOURCE and _SVID_SOURCE are deprecated in glibc>=2.20. _DEFAULT_SOURCE
# is used there instead, but _BSD_SOURCE and _SVID_SOURCE can still be
# specified (no effect). So just add it.
CFLAGS="-D_DEFAULT_SOURCE ${CFLAGS}"
CXXFLAGS="-D_DEFAULT_SOURCE ${CXXFLAGS}"

# ====================================================================

dnl For locale_t
AC_MSG_CHECKING(for locale_t in locale.h)
AC_LINK_IFELSE(
[AC_LANG_PROGRAM([#include <locale.h>], [locale_t lt = NULL])],
AC_MSG_RESULT(yes)
AC_DEFINE([HAVE_LOCALE_T_IN_LOCALE_H], 1, [Define to 1 if locale_t is defined in locale.h]),
AC_MSG_RESULT(no)
	AC_MSG_CHECKING(for locale_t in xlocale.h)
	AC_LINK_IFELSE(
	[AC_LANG_PROGRAM([#include <xlocale.h>], [locale_t lt = NULL])],
	AC_MSG_RESULT(yes)
	AC_DEFINE([HAVE_LOCALE_T_IN_XLOCALE_H], 1, [Define to 1 if locale_t is defined in xlocale.h]),
	AC_MSG_RESULT(no)
	)
)

# We will try to include it for a possible prototype of strtod_l()
# (like in BSD).
AC_CHECK_HEADERS([xlocale.h])

# ====================================================================

dnl For stdatomic.h -- missing in gcc-4.8 (Ubuntu Trusty 14.04)
dnl Its used in the Java bindings, to avoid an ugly server-init race.
AC_MSG_CHECKING(for stdatomic.h)
AC_CHECK_HEADER([stdatomic.h],
	[AC_DEFINE([HAVE_STDATOMIC_H], 1, [Define to 1 if stdatomic.h exists])],
	[],
	[#include <stdatomic.h>
	])

# ====================================================================
# Debugging

AC_ARG_ENABLE( debug,
	[  --enable-debug          compile with debugging flags set],
	[],
	[enable_debug=no])

if test "x$enable_debug" = "xyes"
then
	# Only bash supports double-backslash... Well, dash does too...
	# But not the NetBSD sh, so use sed instead.
	# CFLAGS="${CFLAGS//-O[[2-9]]} -g"
	# CXXFLAGS="${CXXFLAGS//-O[[2-9]]} -g"
	CFLAGS=`echo -g ${CFLAGS} |sed "s/-O[[2-9]]//g"`
	CXXFLAGS=`echo -g ${CXXFLAGS} |sed "s/-O[[2-9]]//g"`
	LDFLAGS="-g ${LDFLAGS}"
	AC_DEFINE(DEBUG,1)
fi

# ====================================================================

AC_ARG_ENABLE( mudflap,
	[  --enable-mudflap        compile with mudflap checking],
	[],
	[enable_mudflap=no])

if test "x$enable_mudflap" = "xyes"
then
	CFLAGS="${CFLAGS} -fmudflap"
	LDFLAGS="${LDFLAGS} -fmudflap -lmudflap"
fi

# ====================================================================

AC_ARG_ENABLE( profile,
	[  --enable-profile        compile with profiling set],
	[],
	[enable_profile=no]
)
if test "x$enable_profile" = "xyes"
then
	CFLAGS="${CFLAGS} -pg"
	LDFLAGS="${LDFLAGS} -pg"
fi

# ====================================================================
# Java now enabled by default
AC_ARG_ENABLE( java-bindings,
	[  --disable-java-bindings disable build of java bindings (default is enabled)],
	[],
	[enable_java_bindings=yes])

# Python is now enabled by default
define([PYTHON3_REQUIRED], [3.4])
AC_ARG_ENABLE( python-bindings,
	[  --disable-python-bindings
	                  disable build of python bindings (default is enabled)],
                          [],
                          [enable_python_bindings=yes]
)

# Perl now disabled by default
AC_ARG_ENABLE( perl-bindings,
	[AS_HELP_STRING([--enable-perl-bindings],
	[create perl bindings to the link-grammar library] (disabled by default))],
	[],
	[enable_perl_bindings=no]
)

# ====================================================================
# SAT solver is disabled by default. Its always slower than the
# regular parser.
#
# First try to use the minisat system library.
# If the library or the headers are not found, use the bundled library

AC_ARG_ENABLE( sat-solver,
	[  --disable-sat-solver    disable use of the Boolean SAT parser
                          (default is disabled)
  --enable-sat-solver[[=ARG]]
                          enable use of the Boolean SAT parser
                          ARG=yes: Use the system minisat library,
                          if possible
                          ARG=bundled: Use the bundled minisat library],
	[if test "x$enableval" = xbundled; then
		use_minisat_bundled_library=yes
	   enable_sat_solver=yes
	fi], [enable_sat_solver=no])

if test "x$enable_sat_solver" = xyes; then
	# The sat-solver code is in C++; so the link-grammar library should now
	# directly require libstdc++ instead of indirectly depend on its
	# availability via the minisat2 library.
	#test x${apple_osx} = xyes || AC_CHECK_LIB(stdc++, main)

	# We want to check for C++; the easiest way to do this is to
	# use c++ to compile stdio.h and bomb if it fails.
	AC_LANG([C++])
	AC_CHECK_HEADER([stdio.h],,
		[AC_MSG_ERROR([C++ compiler not found; it is needed for the SAT parser])])

	dnl 1. Abort and notify if no zlib.h. 2. Adapt for non-standard location.
	AC_MSG_NOTICE([The minisat2 headers include zlib.h])
	dnl The bundled library doesn't actually need -lz
	AX_CHECK_ZLIB([ZLIB_CPPFLAGS="-isystem $ZLIB_HOME/include"],
		[AC_MSG_NOTICE([No zlib library found - not building sat solver])
		enable_sat_solver=no])
fi

# If zlib not found, then don't continue with minisat.
if test "x$enable_sat_solver" = xyes; then
	test "x$ZLIB_CPPFLAGS" = "x-isystem /usr/include" && ZLIB_CPPFLAGS=

	CPPFLAGS_SAVE="$CPPFLAGS"

	if test -z "$use_minisat_bundled_library"; then
		# Check if we can link with the system's minisat2 library.
		# If this is not possible, arrange for using the bundled minisat2
		# library code by setting use_minisat_bundled_library=yes.

		dnl FIXME: Allow using --with-minisat2=DIR
		dnl and/or specifying specific include/library locations.
		minisat_headers="-isystem /usr/include/minisat"
		CPPFLAGS="$CPPFLAGS $minisat_headers $ZLIB_CPPFLAGS"

		AC_CHECK_LIB(minisat, main, [MINISAT_LIBS=-lminisat],
			[ use_minisat_bundled_library=yes ]
			[AC_MSG_NOTICE([No system minisat2 library found - using the bundled library])])

		if test -z "$use_minisat_bundled_library"; then
			AC_CHECK_HEADER([minisat/core/Solver.h], [MINISAT_INCLUDES="$minisat_headers"],
				[ use_minisat_bundled_library=yes ]
				[AC_MSG_NOTICE([System minisat2 headers not found in $CPPFLAGS - using the bundled library])])
		fi
	fi

	dnl Note that the above checks may find the need to use the bundled library
	dnl code even if it wasn't specifically requested.

	if test -n "$use_minisat_bundled_library"; then
		use_minisat_bundled_library=' (using the bundled minisat library)'
		AC_DEFINE(HAVE_MKLIT, 1)
	else
		dnl We are going to use the system's minisat2 library.
		dnl Adapt to slight variations in minisat2.2 code.

		# The function for making a literal can be Lit() or mkLit()
		AC_CHECK_DECL([mkLit(Var, bool)], [AC_DEFINE(HAVE_MKLIT, 1, [Define if exists])], [],
			[#include <minisat/core/Solver.h>]
			[using namespace Minisat;]
			[Solver x;])
		# setPolarity() second argument can be bool or lbool
		AC_CHECK_DECL([x.Solver::setPolarity(Var, bool)],
			[AC_MSG_NOTICE([Found setPolarity bool])]
			[AC_DEFINE(HAVE_SETPOLARITY_BOOL, 1, [Define if setPolarity has bool argument])],
			[AC_MSG_NOTICE([Supposing setPolarity lbool])],
			[#include <minisat/core/Solver.h>]
			[using namespace Minisat;]
			[Solver x;])

		AC_SUBST(MINISAT_LIBS)
		AC_SUBST(MINISAT_INCLUDES)
	fi

	AC_LANG([C])
	CPPFLAGS="$CPPFLAGS_SAVE"
	AC_DEFINE(USE_SAT_SOLVER, 1, [Define for compilation])
	AC_SUBST(ZLIB_CPPFLAGS)
fi

AM_CONDITIONAL(LIBMINISAT_BUNDLED, test -n "$use_minisat_bundled_library")
AM_CONDITIONAL(WITH_SAT_SOLVER, test "x$enable_sat_solver" = xyes)

# ====================================================================

AC_ARG_ENABLE( wordgraph_display,
	[AS_HELP_STRING([--disable-wordgraph-display],
	[disable graphical display of the Wordgraph (default is enabled)])],
	[],
	[enable_wordgraph_display=yes]
)
if test "x$enable_wordgraph_display" = "xyes"
then
	AC_DEFINE(USE_WORDGRAPH_DISPLAY, 1, [Define for compilation])
fi

AM_CONDITIONAL(WITH_ANYSPLIT, test x${enable_wordgraph_display} = xyes)

dnl ====================================================================

# Couldn't use AX_LIB_SQLITE3 since it is currently (04/2021) buggy.

AC_CHECK_HEADER([sqlite3.h], [SQLiteFound=yes], SQLiteFound=no)

if test "${SQLiteFound}" = yes; then
	PKG_CHECK_MODULES(SQLITE3, sqlite3 >= 3.0.0, [SQLiteFound=yes],
							[SQLiteFound=nopc])
fi

dnl On macOS, the sqlite3 library exists but the .pc file may not exist.
if test "${SQLiteFound}" = nopc; then
	AC_CHECK_LIB(sqlite3, sqlite3_open, [SQLITE3_LIBS=-lsqlite3])
fi

if test -n "${SQLITE3_LIBS}"; then
	AC_DEFINE(HAVE_SQLITE3, 1, [Support sqlite3 dictionary])
	SQLiteFound=yes
else
	SQLiteFound=no
fi

AM_CONDITIONAL(HAVE_SQLITE3, test "${SQLiteFound}" = yes)
AC_SUBST(SQLITE3_LIBS)
AC_SUBST(SQLITE3_CFLAGS)

# ====================================================================
# Set Default Spell Checker settings

do_aspell=yes
# ASpell Support is handled here
AC_ARG_ENABLE([aspell], [AS_HELP_STRING([--disable-aspell],
	[Build without ASpell support (default is enabled)])],
	[do_aspell="${enableval}"])

AM_CONDITIONAL(WITH_ASPELL, test x${do_aspell} = xyes)

# Hunspell Support is handled here
do_hunspell=yes
AC_ARG_ENABLE([hunspell], [AS_HELP_STRING([--disable-hunspell],
	[Build without HunSpell support (default is enabled)])],
	[do_hunspell="${enableval}"])

AM_CONDITIONAL(WITH_HUNSPELL, test x${do_hunspell} = xyes)

case "$host_os$host_cpu" in
	*mingw*64)
		default_hunspell_dictdir=/mingw64/share/myspell/dicts
		;;
	*mingw*32)
		# Not checked
		default_hunspell_dictdir=/mingw32/share/myspell/dicts
		;;
	darwin*)
		default_hunspell_dictdir=/Library/Spelling
		;;
	*)
		default_hunspell_dictdir=/usr/share/myspell/dicts
		;;
esac
AC_ARG_WITH([hunspell-dictdir], [AS_HELP_STRING([--with-hunspell-dictdir=DIR],
	[Use DIR to find HunSpell files (default=$default_hunspell_dictdir])],
	[], with_hunspell_dictdir=)

# ====================================================================

# 2015-08: The current default for speller is hunspell, since aspell is not
# yet thread safe. Jan 2018 - aspell still not thread-safe...

HunSpellDictDir=
HunSpellFound=no
if test x"$do_hunspell" = xyes; then
	HunSpellFound=no

	# First, look for the libraries.
	PKG_CHECK_MODULES([HUNSPELL], [hunspell], [HunSpellFound=yes], [HunSpellFound=no])
	save_cpp_flags=${CPPFLAGS}
	CPPFLAGS="${CPPFLAGS} ${HUNSPELL_CFLAGS}"
	AC_CHECK_HEADER([hunspell.h], [HunSpellFound=yes], HunSpellFound=no)
	CPPFLAGS=$save_cpp_flags
	if test "x${HunSpellFound}" = "xyes"; then
		AC_DEFINE(HAVE_HUNSPELL, 1, [Define for compilation])
		AC_SUBST(HUNSPELL_LIBS)
		AC_SUBST(HUNSPELL_CFLAGS)

		# Now, look for the dictionaries.
		HunSpellDictDir=$default_hunspell_dictdir
		if test -n "$with_hunspell_dictdir"; then
			HunSpellDictDir=$with_hunspell_dictdir
		fi

		if test "$native_win32" = yes; then
			# Convert to native format with forward slashes
			# (allow spaces in DIR when using --with-hunspell-dictdir="DIR").
			case $HunSpellDictDir in
				*\ *)
					file_format=dos
					;;
				*)
					file_format=windows
					;;
			esac
			HunSpellDictDir=`cygpath --$file_format --mixed "$HunSpellDictDir"`
		fi

		if ! test -d "$HunSpellDictDir" ; then
			AC_MSG_WARN([HunSpell dictionary directory "$HunSpellDictDir" not found.])
			HunSpellDictDir_status=' (not found)'
		fi
		AC_DEFINE_UNQUOTED(HUNSPELL_DICT_DIR, "$HunSpellDictDir", [Defining the  dictionary path])

		# If hunspell enabled and found, then do NOT do aspell
		do_aspell=no
	fi

fi

AM_CONDITIONAL(HAVE_HUNSPELL, test x${HunSpellFound} = xyes)

ASpellFound=no
if test "$do_aspell" = yes ; then
	PKG_CHECK_MODULES([ASPELL], [aspell], [ASpellFound=yes], [ASpellFound=no])
	save_cpp_flags=${CPPFLAGS}
	CPPFLAGS="${CPPFLAGS} ${ASPELL_CFLAGS}"
	AC_CHECK_HEADER([aspell.h], [ASpellFound=yes], ASpellFound=no)
	AC_CHECK_LIB(aspell, new_aspell_config, [], [ASpellFound=no])
	CPPFLAGS=$save_cpp_flags
	if test  "x${ASpellFound}" = "xyes"; then
		AC_DEFINE(HAVE_ASPELL, 1, [Define for compilation])
		AC_SUBST(ASPELL_LIBS)
		AC_SUBST(ASPELL_CFLAGS)
	fi
fi

AM_CONDITIONAL(HAVE_ASPELL, test x${ASpellFound} = xyes)

# ====================================================================

# Enable editline by default
AC_ARG_ENABLE( editline,
  [  --disable-editline      disable use of editline],
       [],
       [enable_editline=yes])

edlin=no
wedlin=no
if test "x$enable_editline" = "xyes"; then
	PKG_CHECK_MODULES([LIBEDIT], [libedit], [edlin=yes], [edlin=no])
	if test  "x${edlin}" = "xyes"; then
		AM_CONDITIONAL(HAVE_EDITLINE, true)
		AC_DEFINE(HAVE_EDITLINE, 1, [Define for compilation])

		# Now test for widechar support
		AC_CHECK_LIB(edit, history_winit, [wedlin=yes], [wedlin=no])

		if test "x${wedlin}" = "xyes"; then
			AM_CONDITIONAL(HAVE_WIDECHAR_EDITLINE, true)
			AC_DEFINE(HAVE_WIDECHAR_EDITLINE, 1, [Define for compilation])
		else
			AM_CONDITIONAL(HAVE_WIDECHAR_EDITLINE, false)
		fi

	else
		AM_CONDITIONAL(HAVE_EDITLINE, false)
		AM_CONDITIONAL(HAVE_WIDECHAR_EDITLINE, false)
	fi
else
	AM_CONDITIONAL(HAVE_EDITLINE, false)
	AM_CONDITIONAL(HAVE_WIDECHAR_EDITLINE, false)
fi

# ====================================================================
# Configure a regex library that supports UTF-8 and a POSIX interface.
dnl AC_CHECK_HEADERS defines HAVE_FILENAME_H if found.

AC_ARG_WITH([regexlib],
	[AS_HELP_STRING([--with-regexlib=pcre2|tre|regex|c],
	[Use the pecified regex library (default=first found)])],
	[], with_regexlib=)

lgregexlib=--with-regexlib="$with_regexlib"

case $with_regexlib in
	'')
		break
		;;
	pcre2)
		AC_CHECK_HEADERS([pcre2.h], ,
		                 [AC_MSG_ERROR([$lgregexlib: pcre2.h not found])],
							  [#define PCRE2_CODE_UNIT_WIDTH 8])
		PKG_CHECK_MODULES([REGEX], [libpcre2-8])
		;;
	tre)
		AC_CHECK_HEADERS([tre/regex.h], ,
		                 [AC_MSG_ERROR([$lgregexlib: tre/regex.h not found])])
		PKG_CHECK_MODULES([REGEX], [tre])
		;;
	regex)
		AC_CHECK_HEADERS([regex.h], ,[AC_MSG_ERROR([No regex.h header found])])
		AC_CHECK_LIB([regex], [regexec], [REGEX_LIBS=-lregex],
		             [AC_MSG_ERROR([$lgregexlib: regexec() not found])])
		;;
	c)
		AC_CHECK_HEADERS([regex.h], ,[AC_MSG_ERROR([No regex.h header found])])
		AC_CHECK_FUNCS(regexec, [REGEX_LIBS=-lc],
		               [AC_MSG_ERROR([$lgregexlib: regexec() not found])])
		;;
	*)
		AC_MSG_ERROR([$lgregexlib: Library not configurable (see configure --help)])
		;;
esac

if test -z "$REGEX_LIBS"; then
	AC_MSG_NOTICE([Checking for REGEX implementation:])
fi
dnl Find the first regex library according to this order:
dnl libpcre2-8, libtre, libregex, libc.

if test -z "$REGEX_LIBS"; then
	AC_CHECK_HEADER([pcre2.h],
		[PKG_CHECK_MODULES([REGEX], [libpcre2-8],
		                   [AC_DEFINE([HAVE_PCRE2_H], 1)],
		                   [AC_MSG_RESULT([... missing libpcre2-8, continuing])])]
		                   , , [#define PCRE2_CODE_UNIT_WIDTH 8])

fi
if test -z "$REGEX_LIBS"; then
	AC_CHECK_HEADER([tre/regex.h],
		[PKG_CHECK_MODULES([REGEX], [tre],
		                   [AC_DEFINE([HAVE_TRE_REGEX_H], 1)],
		                   [AC_MSG_RESULT([... missing libtre, continuing])])])
fi
if test -z "$REGEX_LIBS"; then
	AC_CHECK_HEADERS([regex.h],
		[AC_CHECK_LIB([regex], [regex], [REGEX_LIBS=-lregex],
		              [AC_MSG_RESULT([... libregex not found, continuing])]
		              [AC_CHECK_FUNCS([regexec], [REGEX_LIBS=-lc])])],
		[AC_MSG_ERROR([No regex.h header found])])
fi

if test -n "$REGEX_LIBS"; then
	AC_MSG_RESULT([... found REGEX implementation: $REGEX_LIBS])
	test "x$REGEX_LIBS" = x-lc && REGEX_LIBS=
	test -n "$REGEX_CFLAGS" && CFLAGS="$CFLAGS $REGEX_CFLAGS"
else
	AC_MSG_ERROR([No REGEX implementation found])
fi

AC_SUBST(REGEX_LIBS)
AC_SUBST(REGEX_CFLAGS)

# =====================================================================
# Find the location of the jni.h file.

JNIfound=no

if test "x$enable_java_bindings" = "xyes"; then

	if test x${apple_osx} = xyes; then
		# Java on Apple OSX requies a 64-bit build. However, even
		# modern 64-bit Apple machines build 32-bit binaries by default.
		# So force a 64-bit build if a 64-bit CPU is found.
		CFLAGS="${CFLAGS} -arch x86_64"
	fi

	absolute_srcdir=`(cd "$srcdir"; pwd)`
	JAVA_SRCDIR=$absolute_srcdir/bindings/java
	if test "$native_win32" = yes; then
		# javac needs a Windows path
		JAVA_SRCDIR=`cygpath -w "$JAVA_SRCDIR"`

		# The standard location of Windows Java is under "\Program Files".
		# However, automake is unable to handle spaces in a filename even
		# when quoted (e.g. "-I '$JAVA_HOME/include'"). As a workaround,
		# convert $JAVA_HOME to a short DOS path.
		test -n "${JAVA_HOME}" && JAVA_HOME=`cygpath --dos --mixed "$JAVA_HOME"`

		# win32/jni_md.h:34:9: error: ‘__int64’ does not name a type
		JAVA_CPPFLAGS=-D__int64=int64_t
	fi
	AC_SUBST(JAVA_SRCDIR)

	pushdef([AC_MSG_ERROR])
	define([AC_MSG_ERROR],
		[AC_MSG_NOTICE([$1])])

	AX_JNI_INCLUDE_DIR

	popdef([AC_MSG_ERROR])

	if test -n "$JNI_INCLUDE_DIRS"; then
		JNIfound=yes
	fi

	for JNI_INCLUDE_DIR in $JNI_INCLUDE_DIRS
	do
		JAVA_CPPFLAGS="$JAVA_CPPFLAGS -I$JNI_INCLUDE_DIR"
	done
	AC_SUBST(JAVA_CPPFLAGS)

	# This allows jni-client.c to be built
	AM_CONDITIONAL(HAVE_JAVA, test x${JNIfound} = xyes)

	# For now, we assume that if the ant tool is there, then the javac and
	# jar compiler/packager are installed as well. These are needed to build
	# the jar file and install it.
	AC_CHECK_PROG(ANTfound, ant, yes, no)
	AM_CONDITIONAL(HAVE_ANT, test x${ANTfound} = xyes)

	# If Maven is found then use it to install the jar library
	# into the local maven repository
	AC_CHECK_PROG(MVNfound, mvn, yes, no)
	AM_CONDITIONAL(HAVE_MVN, test x${MVNfound} = xyes)
else
   AM_CONDITIONAL(HAVE_JAVA, false)
   AM_CONDITIONAL(HAVE_ANT, false)
   AM_CONDITIONAL(HAVE_MVN, false)
fi

# ===================================================================

PerlFound=no
if test "x$enable_perl_bindings" = "xyes"; then
	# Look for the perl bits and pieces...
	# AX_PERL_EXT isn't working for me ...
	# AX_PATH_EXT
	AC_MSG_CHECKING([for Perl XS include directory])
	if test -z "$PERL_EXT_INC" ; then
		PERL_EXT_INC=`perl -MConfig -e 'print $Config{archlib}'`
	fi
	test -z "$PERL_EXT_INC" && PERL_EXT_INC=no
	AC_MSG_RESULT([$PERL_EXT_INC])
	PERL_CFLAGS="-I${PERL_EXT_INC} -I${PERL_EXT_INC}/CORE"
	AC_SUBST(PERL_EXT_INC)
	AC_SUBST(PERL_CFLAGS)

	# PERL_EXT_LIB is where to install, typically /usr/local/lib/perl/5.14.2
	# or similar.
	AC_MSG_CHECKING([for Perl extensions destination directory])
	if test -z "$PERL_EXT_LIB" ; then
		PERL_EXT_LIB=`perl -MConfig -e 'print $Config{sitearchexp}'`
	fi
	test -z "$PERL_EXT_LIB" && PERL_EXT_LIB=no
	AC_MSG_RESULT([$PERL_EXT_LIB])
	AC_SUBST(PERL_EXT_LIB)

	save_cpp_flags=${CPPFLAGS}
	CPPFLAGS="${CPPFLAGS} ${PERL_CFLAGS}"
	AC_CHECK_HEADER([EXTERN.h], [PerlFound=yes], [PerlFound=no])
	AM_CONDITIONAL(HAVE_PERL, test x${PerlFound} = xyes)
	CPPFLAGS=$save_cpp_flags
else
	AM_CONDITIONAL(HAVE_PERL, false)
fi

# ===================================================================
# Find python things.

dnl AX_PYTHON_DEVEL uses AC_MSG_ERROR in case it doesn't find things.
dnl Temporarily make AC_MSG_ERROR a soft error (to be restored by popdef()).
pushdef([AC_MSG_ERROR])
define([AC_MSG_ERROR],
	lg_python_unusable=yes
	[AC_MSG_NOTICE([$1])])

PythonFound=no
if test "x$enable_python_bindings" = xyes; then
	AM_PATH_PYTHON(PYTHON3_REQUIRED, [PythonFound=yes])
	AX_PYTHON_DEVEL(>= 'PYTHON3_REQUIRED')
	test -n "$lg_python_unusable" && PythonFound=no
	if test x$PythonFound = xyes; then
		PythonFound=$PYTHON_VERSION
		lg_display_PYTHON=" ($PYTHON)"
		PYTHON_CPPFLAGS=$PYTHON_CPPFLAGS
		PYTHON_LIBS=$PYTHON_LIBS
		PYTHON_LDFLAGS=$PYTHON_LDFLAGS
		pythondir=$pythondir
		AC_SUBST(PYTHON_CPPFLAGS)
		AC_SUBST(PYTHON_LIBS)
		AC_SUBST(PYTHON_LDFLAGS)
		AC_SUBST(pythondir)
	fi
fi
AM_CONDITIONAL(HAVE_PYTHON, test x$PythonFound != xno)

AC_SUBST(PYTHON)
AC_SUBST(pythondir)

# Formerly both python2 and python3 bindings got generated. This caused a
# redefinition of the preprocessor symbol HAVE_PYTHON and generates a lot
# of compiler warnings, so it got discarded here. Now we generate only
# python3 bindings, but since we don't use this definition, it is still
# discarded here.
# Note: The v command allows to differentiate between the two flavors of sed.
edit_in_place=-i
sed v </dev/null 2>/dev/null || edit_in_place="-i ''"
sed $edit_in_place '/HAVE_PYTHON/d' confdefs.h

popdef([AC_MSG_ERROR])

if test x$enable_python_bindings = xyes; then
	if test x$PythonFound = xno; then
		AC_MSG_NOTICE(
			[Python binding is enabled, but an approprite Python version
                  could not be configured (minimum version: PYTHON3_REQUIRED).])
	fi
fi

# ===================================================================
# argp is needed for link-generate.

AC_CHECK_HEADER([argp.h], [AC_SEARCH_LIBS([argp_parse], [argp],, [no_argp=1])], [no_argp=1])

if test -n "$no_argp"; then
	AC_MSG_WARN([No argp library found - "link-generator" will not get built.])
fi
AM_CONDITIONAL(HAVE_ARGP, test -z "$no_argp")

# ===================================================================
# swig is needed for compiling the Perl and Python bindings ...
# ... well, actually, no, its not. 'make dist' is currently set up to
# package all of the files generated by swig, so the user does not need
# to actually install it.  However, swig is needed to create the package,
# and also needed to build from a GitHub pull.

if ! test -s "$srcdir/bindings/python/lg_python_wrap.cc"; then
	AM_COND_IF(HAVE_PYTHON, [swig_required="$swig_required Python"])
fi
if ! test -s "$srcdir/bindings/perl/lg_perl_wrap.cc"; then
	AM_COND_IF(HAVE_PERL, [swig_required="$swig_required Perl"])
fi

if test -n "$swig_required"; then
	AX_PKG_SWIG(2.0.0,, [AC_MSG_ERROR(['swig' is required to create bindings for: $swig_required])])
	AX_SWIG_ENABLE_CXX
	SWIGfound="$SWIG"
else
	SWIGfound=no
fi
AM_CONDITIONAL(HAVE_SWIG, test -n "$SWIG")

# ====================================================================
# check compiler flags

AC_DEFUN([LINK_CC_TRY_FLAG], [
  AC_MSG_CHECKING([whether $CC supports $1])

  link_save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -Werror $1"

  AC_COMPILE_IFELSE([AC_LANG_SOURCE([[ ]])], [link_cc_flag=yes], [link_cc_flag=no])
  CFLAGS="$link_save_CFLAGS"

  if test "x$link_cc_flag" = "xyes"; then
    ifelse([$2], , :, [$2])
  else
    ifelse([$3], , :, [$3])
  fi
  AC_MSG_RESULT([$link_cc_flag])
])

dnl Use lots of warning flags with with gcc and compatible compilers

dnl Note: if you change the following variable, the cache is automatically
dnl skipped and all flags rechecked.  So there's no need to do anything
dnl else.  If for any reason you need to force a recheck, just change
dnl MAYBE_WARN in an ignorable way (like adding whitespace)

dnl MAYBE_WARN works for C++ and C, while MAYBE_WARN_C is for C only.
MAYBE_WARN_CXX="-Wall -Wextra \
-Wsign-compare \
-Wpointer-arith -Wwrite-strings -Wmissing-declarations \
-Wpacked -Wswitch-enum -Wmissing-format-attribute \
-Wstrict-aliasing -Winit-self -Wshadow \
-Wno-missing-field-initializers -Wno-unused-parameter \
-Wno-attributes -Wno-long-long -Winline"

MAYBE_WARN_C="-Wstrict-prototypes -Wmissing-prototypes -Wformat-signedness \
-Wnested-externs -Wold-style-definition -Werror-implicit-function-declaration"

MAYBE_WARN="$MAYBE_WARN_CXX $MAYBE_WARN_C"

# Invalidate cached value if MAYBE_WARN has changed
if test "x$link_cv_warn_maybe" != "x$MAYBE_WARN"; then
	unset link_cv_warn_cflags
fi
AC_CACHE_CHECK([for supported warning flags], link_cv_warn_cflags, [
	echo
	WARN_CFLAGS=""

	# Some warning options are not supported by all versions of
	# gcc (or clang), so test all desired options against the
	# current compiler. With only one issue: clang fails to return
	# an error when it doesn't know the option. So this test fails.
	#
	# Note that there are some order dependencies
	# here. Specifically, an option that disables a warning will
	# have no net effect if a later option then enables that
	# warnings, (perhaps implicitly). So we put some grouped
	# options (-Wall and -Wextra) up front and the -Wno options
	# last.

	for W in $MAYBE_WARN_C; do
		LINK_CC_TRY_FLAG([$W], [WARN_CFLAGS="$WARN_CFLAGS $W"])
	done

	for W in $MAYBE_WARN_CXX; do
		LINK_CC_TRY_FLAG([$W], [WARN_CXXFLAGS="$WARN_CXXFLAGS $W"])
	done

	link_cv_warn_cflags=$WARN_CFLAGS
	link_cv_warn_cxxflags=$WARN_CXXFLAGS

	AC_MSG_CHECKING([which warning flags were supported])])

WARN_CFLAGS="$link_cv_warn_cflags $link_cv_warn_cxxflags"
WARN_CXXFLAGS="$link_cv_warn_cxxflags"

AC_SUBST(WARN_CFLAGS)
AC_SUBST(WARN_CXXFLAGS)

dnl No LINK_CFLAGS for now.
AC_SUBST(LINK_CFLAGS)

LINK_CC_TRY_FLAG([-Wmaybe-uninitialized -Werror], [AC_DEFINE(HAVE_MAYBE_UNINITIALIZED)])

# ===================================================================

AC_FUNC_STRERROR_R

dnl Save the compilation definitions for an extended version printout
AC_OUTPUT_MAKE_DEFS()
LG_DEFS=`$ECHO "$DEFS" | $SED 's/\\\\//g'`
AC_SUBST(LG_DEFS)

AC_CONFIG_FILES([
Makefile
link-grammar.pc
link-grammar.spec
link-grammar/Makefile
link-grammar/minisat/Makefile
link-grammar/sat-solver/Makefile
link-grammar/link-features.h
bindings/Makefile
bindings/java-jni/Makefile
bindings/java/Makefile
bindings/java/build.xml
bindings/ocaml/Makefile
bindings/perl/Makefile
bindings/python/Makefile
bindings/python/__init__.py
bindings/python-examples/Makefile
morphology/Makefile
link-parser/Makefile
tests/Makefile
data/Makefile
data/ady/Makefile
data/amy/Makefile
data/any/Makefile
data/gen/Makefile
data/ar/Makefile
data/ar/words/Makefile
data/de/Makefile
data/en/Makefile
data/en/words/Makefile
data/fa/Makefile
data/fa/words/Makefile
data/he/Makefile
data/id/Makefile
data/kz/Makefile
data/lt/Makefile
data/ru/Makefile
data/ru/words/Makefile
data/tr/Makefile
data/vn/Makefile
data/demo-sql/Makefile
man/Makefile
])
AC_OUTPUT

# ====================================================================
lglibs=`${ECHO} "$LIBEDIT_LIBS $HUNSPELL_LIBS $ASPELL_LIBS $REGEX_LIBS $MINISAT_LIBS $SQLITE3_LIBS $LIBS"|${SED} 's/  */ /g'`

echo "
$PACKAGE-$VERSION  build configuration settings

	prefix:                         ${prefix}
	datadir:                        $(eval "echo ${datadir}")
	C compiler:                     ${CC} ${CPPFLAGS} ${CFLAGS} ${PTHREAD_CFLAGS}
	C++ compiler:                   ${CXX} ${CPPFLAGS} ${CXXFLAGS}
	Regex library:                  ${REGEX_LIBS}
	TLS:                            ${lg_tls}
	Editline command-line history:  ${edlin}
	UTF8 editline support:          ${wedlin}
	Java libraries:                 ${JNIfound}
	Java interfaces:                ${ANTfound}
	Swig interfaces generator:      ${SWIGfound}
	Perl interfaces:                ${PerlFound}
	Perl install location:          ${PERL_EXT_LIB}
	Python interfaces:              ${PythonFound}${lg_display_PYTHON}
	ASpell spell checker:           ${ASpellFound}
	HunSpell spell checker:         ${HunSpellFound}
	HunSpell dictionary location:   ${HunSpellDictDir}${HunSpellDictDir_status}
	Boolean SAT parser:             ${enable_sat_solver}${use_minisat_bundled_library}
	SQLite-backed dictionary:       ${SQLiteFound}
	Definitions:                    ${DEFS}
	Libraries:                      ${lglibs}
"
