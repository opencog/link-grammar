AC_INIT([link-grammar],[5.2.5],[link-grammar@googlegroups.com])

dnl Set release number
dnl This is derived from "Versioning" chapter of info libtool documentation.
PACKAGE=link-grammar
dnl     4a) Increment when removing or changing interfaces.
LINK_MAJOR_VERSION=5
dnl     4a) 5) Increment when adding interfaces.
dnl     6) Set to zero when removing or changing interfaces.
LINK_MINOR_VERSION=2
dnl     3) Increment when interfaces not changed at all,
dnl        only bug fixes or internal changes made.
dnl     4b) Set to zero when adding, removing or changing interfaces.
LINK_MICRO_VERSION=5
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
AM_MAINTAINER_MODE([enable])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_SRCDIR($PACKAGE)

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])], [AC_SUBST([AM_DEFAULT_VERBOSITY],1)])

# OS checks, including Win32
AC_CANONICAL_HOST

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_HEADER_STDC
LT_INIT([win32-dll])
AM_BINRELOC
PKG_PROG_PKG_CONFIG

# The below says "use the C compiler for all remaining tests".
# That way, if g++ is not installed, configure won't crap out
# (with a mystery error about not finding regex.h)
AC_LANG([C])

AC_C_CONST

AC_CHECK_FUNCS(towupper)

AC_FUNC_ALLOCA

# For the Wordgraph display code
AC_FUNC_FORK
AC_CHECK_FUNCS(prctl)

# ====================================================================

AC_MSG_CHECKING([for native Win32])
case "$host" in
  *-*-mingw*)
    native_win32=yes
    ;;
  *)
    native_win32=no
    ;;
esac
AC_MSG_RESULT([$native_win32])
AM_CONDITIONAL(OS_WIN32, test "x$native_win32" = "xyes")

# ====================================================================
CFLAGS="${CFLAGS} -O3" 
# CXXFLAGS="${CXXFLAGS} -O3 -Wall -std=c++11"
CXXFLAGS="${CXXFLAGS} -O3 -Wall -std=c++03"

# The std=c99 flag provides the proper float-pt math decls working, 
# e.g. fmaxf  However, it also undefined _BSD_SOURCE, etc which is
# needed to get fileno, strdup, etc. and so it needs to be manually 
# enabled again.

# Setting -D_POSIX_SOURCE messes up compilation on FreeBSD by 
# hiding strdup, etc. again.
# CFLAGS="${CFLAGS} -std=c99 -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE -D_GNU_SOURCE"

# Final solution: enable std=c99, explitictly turn on BSD and SVID and
# GNU, but do NOT turn on POSIX.
#
if test x${native_win32} = xyes; then
	# Vikas says that -std=gnu99 is needed on cygwin/mingw
	CFLAGS="${CFLAGS} -std=gnu99 -D_BSD_SOURCE -D_SVID_SOURCE -D_GNU_SOURCE"
	# Also -- Vikas sets MINGW explicitly, as well as assorted paths.
	# I'm unclear as to why these are not being set automatically,
	# by mingw itself .. but what the hey ... this can't hurt, right?
	# Oh yes it can hurt, do not set these at all.
	# Setting these screws up the remainder of the configure script.
	# CFLAGS="${CFLAGS} -I/usr/include/mingw -D__MINGW32__"
	# LDFLAGS="${LDFLAGS} -L/usr/lib/mingw -lmingwex -lcrtdll -lmsvcrt -lc -Wl,--allow-multiple-definition"
else
	# Else standard Linux/*BSD environment.
	CFLAGS="${CFLAGS} -std=c99 -D_BSD_SOURCE -D_SVID_SOURCE -D_GNU_SOURCE"
fi

# _BSD_SOURCE and _SVID_SOURCE are deprecated in glibc>=2.20. _DEFAULT_SOURCE
# is used there instead, but _BSD_SOURCE and _SVID_SOURCE can still be
# specified (no effect). So just add it.
CFLAGS="${CFLAGS} -D_DEFAULT_SOURCE"

# ====================================================================
# Even modern 64-bit Apple machines build 32-bit binaries by default.
# So force a 64-bit build if a 64-bit CPU is found.

AC_MSG_CHECKING([for 64-bit Apple OSX])
case "$host" in
  x86_64-*-darwin*)
    apple_osx=yes
    CFLAGS="${CFLAGS} -arch x86_64"
    ;;
  *)
    apple_osx=no
    ;;
esac
AC_MSG_RESULT([$apple_osx])

# ====================================================================
# Autopackage binreloc.

AM_CONDITIONAL(WITH_BINRELOC, test "x$br_cv_binreloc" = "xyes")

# ====================================================================
# Debugging

AC_ARG_ENABLE( debug,
  [  --enable-debug          compile with debugging flags set],
  [],
  [enable_debug=no])

if test "x$enable_debug" = "xyes"
then
	CFLAGS="${CFLAGS//-O[[2-9]]} -g"
	CXXFLAGS="${CXXFLAGS//-O[[2-9]]} -g"
	LDFLAGS="${LDFLAGS} -g"
	AC_DEFINE(DEBUG_MEMORY,1)
	AC_DEFINE(DEBUG,1)
else
	AC_DEFINE(DEBUG_MEMORY,0)
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
# pthreads disabled by default; pthreads not really needed.

AC_ARG_ENABLE( pthreads,
  [  --enable-pthreads       compile with pthread support],
  [],
  [enable_pthreads=no]
)
if test "x$enable_pthreads" = "xyes"
then
	CPPFLAGS="${CPPFLAGS} -DUSE_PTHREADS=1"
fi


AM_CONDITIONAL(WITH_PTHREADS, test x${enable_pthreads} = xyes)

# ====================================================================
# Java now enabled by default

AC_ARG_ENABLE( java-bindings,
  [  --disable-java-bindings disable build of java bindings (default is enabled)],
  [],
  [enable_java_bindings=yes])

AC_ARG_ENABLE( python-bindings,
  [  --enable-python-bindings  create python bindings to the link-grammar library],
  [],
  [enable_python_bindings=no]
)

# ====================================================================
# SAT solver now enabled by default

AC_ARG_ENABLE( sat-solver,
  [  --disable-sat-solver     disable use of the Boolean SAT parser (default is enabled)],
  [],
  [enable_sat_solver=yes]
)

if test "x$enable_sat_solver" = "xyes"; then
	CPPFLAGS="${CPPFLAGS} -DUSE_SAT_SOLVER=1"

	# We want to check for C++; the easiest way to do this is to
	# use c++ to compile stdio.h and bomb if it fails.
	AC_LANG([C++])
	AC_CHECK_HEADER([stdio.h],,
		[AC_MSG_ERROR([C++ compiler not found; it is needed for the SAT parser])])
	AC_LANG([C])
fi

AM_CONDITIONAL(WITH_SAT_SOLVER, test x${enable_sat_solver} = xyes)

# ====================================================================
# Viterbi disabled by default

AC_ARG_ENABLE( viterbi,
  [  --enable-viterbi        enable use of the Viterbi parser],
  [],
  [enable_viterbi=no]
)

if test "x$enable_viterbi" = "xyes"; then
	CPPFLAGS="${CPPFLAGS} -DUSE_VITERBI"

	# We want to check for C++; the easiest way to do this is to
	# use c++ to compile stdio.h and bomb if it fails.
	AC_LANG([C++])
	AC_CHECK_HEADER([stdio.h],,
		[AC_MSG_ERROR([C++ compiler not found; it is needed for the Viterbi parser])])
	AC_LANG([C])

	# The vieterbi decoder is designed to leak memory like crazy.
	# Must use garbage collection in order for it to work.
	PKG_CHECK_MODULES([LIBGC], [bdw-gc], [gc=yes], [gc=no])
	AC_SUBST(LIBGC_LIBS)
	AC_SUBST(LIBGC_CFLAGS)
	if test  "x${gc}" = "xno"; then
		AC_MSG_ERROR([Garbage collector not found; it is needed for the Viterbi parser])
	fi
fi

AM_CONDITIONAL(WITH_VITERBI, test x${enable_viterbi} = xyes)

# ====================================================================
# Corpus stats disabled by default

AC_ARG_ENABLE( corpus-stats,
  [  --enable-corpus-stats   use corpus statistics],
  [],
  [enable_corpus_stats=no]
)
if test "x$enable_corpus_stats" = "xyes"
then
	CPPFLAGS="${CPPFLAGS} -DUSE_CORPUS=1"
fi

AM_CONDITIONAL(WITH_CORPUS, test x${enable_corpus_stats} = xyes)

# ====================================================================

AC_ARG_ENABLE( anysplit,
  [  --enable-anysplit       enable use of the "any" language word splitter],
  [],
  [enable_anysplit=no]
)
if test "x$enable_anysplit" = "xyes"
then
	CPPFLAGS="${CPPFLAGS} -DUSE_ANYSPLIT=1"
fi

AM_CONDITIONAL(WITH_ANYSPLIT, test x${enable_anysplit} = xyes)

# ====================================================================

AC_CHECK_HEADER([sqlite3.h], [SQLiteFound=yes], SQLiteFound=no)

# If sqlite3 and pkg-config are not found on MacOS, then 
# recommend that user install MacPorts http://www.macports.org/
if test "x${SQLiteFound}" = "xyes"; then
 	PKG_CHECK_MODULES(SQLITE3, sqlite3 >= 3.0.0, [SQLiteFound=yes], [SQLiteFound=no])
fi

# Don't set the CPPFLAGS unless we can find the library too.
# Which is insane, of course, except that some Apple OSX systems
# appear to be borken... have header files but no libs ... 
if test "x${SQLiteFound}" = "xyes"; then
	CPPFLAGS="${CPPFLAGS} -DHAVE_SQLITE=1"
fi
AM_CONDITIONAL(HAVE_SQLITE, test x${SQLiteFound} = xyes)
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

AC_ARG_WITH([hunspell-dictdir], [AS_HELP_STRING([--with-hunspell-dictdir=DIR],
	[Use DIR to find HunSpell files (default=/usr/share/myspell/dicts])],
	[], with_hunspell_dictdir=)

# ====================================================================

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

		# If aspell enabled and found, then do NOT do hunspell
		do_hunspell=no
	fi
fi

AM_CONDITIONAL(HAVE_ASPELL, test x${ASpellFound} = xyes)

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
		HunSpellDictDir=/usr/share/myspell/dicts
		if test -n "$with_hunspell_dictdir"; then
			HunSpellDictDir=$with_hunspell_dictdir
		fi

		if ! test -d "$HunSpellDictDir" ; then
			echo "WARN HunSpell Dictionaries do not exist at \"$HunSpellDictDir\""
		fi
		AC_DEFINE_UNQUOTED(HUNSPELL_DICT_DIR, "$HunSpellDictDir", [Defining the  dictionary path])
	fi

fi

AM_CONDITIONAL(HAVE_HUNSPELL, test x${HunSpellFound} = xyes)

# ====================================================================

# It's not enough that locale.h is present; link-grammar really
# wants to have LC_MESSAGES as well, so test for it explicitly.

AC_DEFUN([GLIB_LC_MESSAGES],
  [AC_CHECK_HEADERS([locale.h])
    if test $ac_cv_header_locale_h = yes; then
      AC_CACHE_CHECK([for LC_MESSAGES], am_cv_val_LC_MESSAGES,
        [AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <locale.h>]], [[return LC_MESSAGES]])],[am_cv_val_LC_MESSAGES=yes],[am_cv_val_LC_MESSAGES=no])])
      if test $am_cv_val_LC_MESSAGES = yes; then
        AC_DEFINE(HAVE_LC_MESSAGES, 1,
          [Define if your <locale.h> file defines LC_MESSAGES.])
      fi
    fi])

GLIB_LC_MESSAGES

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

# Tokenizing by RegEx
AC_ARG_ENABLE( regex_tokenizer,
  [  --enable-regex-tokenizer enable use of regex word splitter],
  [],
  [enable_regex_tokenizer=no]
)
if test "x$enable_regex_tokenizer" = "xyes"
then
	CPPFLAGS="${CPPFLAGS} -DUSE_REGEX_TOKENIZER=1"
	AX_PATH_LIB_PCRE
fi

# ====================================================================

AC_CHECK_HEADER([regex.h],,[AC_MSG_ERROR([No regex.h header found])])

AC_CHECK_FUNCS(regexec,,[AC_CHECK_LIB(regex,regexec,
               [REGEX_LIBS=-lregex],
               [AC_MSG_ERROR([No regex library found])])])
AC_SUBST(REGEX_LIBS)

# =====================================================================
# Try to guess location of the jni.h file.
#
# The core issue here is that most Sun/Oracle java installations are
# missing the pkgconfig file, which means that a simple and easy check
# for jni is impossible on such systems.  What a bunch of losers.
# Java sucks.
#
# There's libgcj-4.2.pc  and libgcj8.pc but nothing for Sun java ...
#
# PKG_CHECK_MODULES(LIBJDK, libjdk-1.5 >= $LIBJDK_REQUIRED)
# AC_SUBST(LIBJDK_CFLAGS)
# AC_SUBST(LIBJDK_LIBS)

JNIfound=no

if test "x$enable_java_bindings" = "xyes"; then
   AC_MSG_CHECKING(for jni.h)

   # Try looking for jni in the standard locations first.
   AC_TRY_CPP(
      [#include <jni.h>], 
      [AC_MSG_RESULT([yes, will build java libs])
         JNIfound=yes], 
      [AC_MSG_WARN([cannot find jni.h in standard location.]) 
         JNIfound=no])

   # Didn't find it yet. Try looking in user-specified locations.
   if test $JNIfound = "no"; then
      JNI_GUESS=" \
         -I $JAVA_HOME/include \
         -I $JAVA_HOME/include/freebsd \
         -I $JAVA_HOME/include/linux \
         -I $JAVA_HOME/include/win32 \
         -I $JAVA_HOME/Headers \
         -I $JDK_HOME/include \
         -I $JDK_HOME/include/freebsd \
         -I $JDK_HOME/include/linux \
         -I $JDK_HOME/include/win32 \
         -I $JDK_HOME/Headers"
  
      AC_MSG_CHECKING(for jni.h in user-specified location)
      SAVE_CPPFLAGS=${CPPFLAGS}
      CPPFLAGS="${CPPFLAGS} ${JNI_GUESS}"
      AC_TRY_CPP(
         [#include <jni.h>], 
         [AC_MSG_RESULT([yes, will build java libs])
            JNIfound=yes], 
         [AC_MSG_WARN([cannot find jni.h in user-specified location.]) 
            CPPFLAGS=${SAVE_CPPFLAGS}
            JNIfound=no])
   fi

   # Still didn't find it. Try looking in some non-standard locations.
   if test $JNIfound = "no"; then
      JNI_GUESS=" \
         -I/usr/include/classpath/ \
         -I/usr/lib/jvm/default-java/include \
         -I/usr/lib/jvm/default-java/include/freebsd \
         -I/usr/lib/jvm/default-java/include/linux \
         -I/usr/lib/jvm/java-gcj/include \
         -I/usr/lib/jvm/java-gcj/include/freebsd \
         -I/usr/lib/jvm/java-gcj/include/linux \
         -I/usr/local/jdk1.6.0/include/\
         -I/usr/local/jdk1.6.0/include/freebsd \
         -I/usr/local/jdk1.6.0/include/linux \
         -I/usr/lib/jvm/java-6-sun/include/ \
         -I/usr/lib/jvm/java-6-sun/include/freebsd \
         -I/usr/lib/jvm/java-6-sun/include/linux \
         -I/usr/lib/j2sdk1.6-ibm/include \
         -I/usr/lib/j2sdk1.6-ibm/include/freebsd \
         -I/usr/lib/j2sdk1.6-ibm/include/linux \
         -I/usr/lib/jvm/java-7-openjdk-amd64/include \
         -I/usr/lib/jvm/java-7-openjdk-amd64/include/freebsd \
         -I/usr/lib/jvm/java-7-openjdk-amd64/include/linux \
         -I/c/java/jdk1.6.0/include/ \
         -I/c/java/jdk1.6.0/include/win32/ \
         -I/Developer/SDKs/MacOSX10.5.sdk/System/Library/Frameworks/JavaVM.framework/Headers/ \
         -I/Developer/SDKs/MacOSX10.4u.sdk/System/Library/Frameworks/JavaVM.framework/Headers/ \
         -I /System/Library/Frameworks/JavaVM.framework/Versions/Current/Headers"
  
      AC_MSG_CHECKING(for jni.h in non-standard location)
      SAVE_CPPFLAGS=${CPPFLAGS}
      CPPFLAGS="${CPPFLAGS} ${JNI_GUESS}"
      AC_TRY_CPP(
         [#include <jni.h>], 
         [AC_MSG_RESULT([yes, will build java libs])
            JAVA_CPPFLAGS=${JNI_GUESS}
            JNIfound=yes], 
         [AC_MSG_WARN([cannot find jni.h in anywhere.]) 
            JNIfound=no])
      CPPFLAGS=${SAVE_CPPFLAGS}
   fi

	# This allows jni-client.c to be built
	AM_CONDITIONAL(HAVE_JAVA, test x${JNIfound} = xyes)

	AC_SUBST(JAVA_CPPFLAGS)

	# For now, we assume that if the ant tool is there, then the javac and
	# jar compiler/packager are installed as well. These are needed to build
	# the jar file and install it.
	AC_CHECK_PROG(ANTfound, ant, yes, no)
	AM_CONDITIONAL(HAVE_ANT, test x${ANTfound} = xyes)

else
   AM_CONDITIONAL(HAVE_JAVA, false)
   AM_CONDITIONAL(HAVE_ANT, false)
fi

# ===================================================================
# swig is needed for compiling the perl and python bindings
# ... well, actualy, no, its not. 'make dist' is currently set up to
# package all of the files generated by swig, so the user does not need
# to actually install it.  However, it is needed to build the package,
# though... and also needed to build from a github pull.  Hmmm. What to
# do ... 

AX_PKG_SWIG(2.0.0, [SwigF=yes], [SwigF=no])
AC_CHECK_PROG(SWIGfound, swig, yes, no)
if test "x$SwigFound" = "xyes"; then
	AX_SWIG_ENABLE_CXX
fi
AM_CONDITIONAL(HAVE_SWIG, test x${SWIGfound} = xyes)

# ===================================================================
# Look for the perl bits and pieces...
# AX_PERL_EXT isn't working for me ... 
# AX_PATH_EXT
if test -z "$PERL_EXT_INC" ; then
	PERL_EXT_INC=`perl -MConfig -e 'print $Config{archlib}'`
fi
AC_MSG_RESULT([$PERL_EXT_INC])
PERL_CFLAGS="-I${PERL_EXT_INC} -I${PERL_EXT_INC}/CORE"
AC_SUBST(PERL_EXT_INC)
AC_SUBST(PERL_CFLAGS)

# PERL_EXT_LIB is where to install, typically /usr/local/lib/perl/5.14.2
# or similar.
if test -z "$PERL_EXT_LIB" ; then
	PERL_EXT_LIB=`perl -MConfig -e 'print $Config{sitearchexp}'`
fi
AC_MSG_RESULT([$PERL_EXT_LIB])
AC_SUBST(PERL_EXT_LIB)

save_cpp_flags=${CPPFLAGS}
CPPFLAGS="${CPPFLAGS} ${PERL_CFLAGS}"
AC_CHECK_HEADER([EXTERN.h], [PerlFound=yes], [PerlFound=no])
AM_CONDITIONAL(HAVE_PERL, test x${PerlFound} = xyes)
CPPFLAGS=$save_cpp_flags

# ===================================================================
# Find python things.  
#
# Since AX_PYTHON_DEVEL will hard-fail if python-devel is not installed,
# it is up to the user to explicitly ask for python, if they want it.

PythonFound=no
if test "x$enable_python_bindings" = "xyes"; then
	AM_PATH_PYTHON(2.6, [PythonFound=yes], [PythonFound=no])
	AX_PYTHON_DEVEL
	AM_CONDITIONAL(HAVE_PYTHON, test x${PythonFound} = xyes)
else
	AM_CONDITIONAL(HAVE_PYTHON, false)
fi

# ====================================================================
# check compiler flags

AC_DEFUN([LINK_CC_TRY_FLAG], [
  AC_MSG_CHECKING([whether $CC supports $1])

  link_save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $1"

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
-Wsign-compare -Werror-implicit-function-declaration \
-Wpointer-arith -Wwrite-strings -Wmissing-declarations \
-Wpacked -Wswitch-enum -Wmissing-format-attribute \
-Wstrict-aliasing=2 -Winit-self \
-Wno-missing-field-initializers -Wno-unused-parameter \
-Wno-attributes -Wno-long-long -Winline"

MAYBE_WARN_C="-Wstrict-prototypes -Wmissing-prototypes \
-Wnested-externs -Wdeclaration-after-statement -Wold-style-definition"

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

LINK_CC_TRY_FLAG([-fno-strict-aliasing],
		[LINK_CFLAGS="$LINK_CFLAGS -fno-strict-aliasing"])

AC_SUBST(LINK_CFLAGS)

# ===================================================================

AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(CXXFLAGS)

AC_CONFIG_FILES([
Makefile
link-grammar.pc
link-grammar.spec
link-grammar/Makefile
link-grammar/corpus/Makefile
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
data/amy/Makefile
data/any/Makefile
data/ar/Makefile
data/ar/words/Makefile
data/de/Makefile
data/en/Makefile
data/en/words/Makefile
data/fa/Makefile
data/fa/words/Makefile
data/he/Makefile
data/id/Makefile
data/lt/Makefile
data/ru/Makefile
data/ru/words/Makefile
data/tr/Makefile
data/demo-sql/Makefile
man/Makefile
autopackage/default.apspec
link-grammar.xcode/Makefile
viterbi/Makefile
])
AC_OUTPUT

# ====================================================================
echo "
$PACKAGE-$VERSION  build configuration settings

	prefix:                         ${prefix}
	C compiler:                     ${CC} ${CPPFLAGS} ${CFLAGS}
	C++ compiler:                   ${CXX} ${CPPFLAGS} ${CXXFLAGS}
	autopackage relocatable binary: ${br_cv_binreloc}
	Posix threads:                  ${enable_pthreads}
	Editline command-line history:  ${edlin}
	UTF8 editline support:          ${wedlin}
	Java libraries:                 ${JNIfound}
	Java interfaces:                ${ANTfound}
	Swig interfaces generator:      ${SWIGfound}
	Perl interfaces:                ${PerlFound}
	Perl install location:          ${PERL_EXT_LIB}
	Python interfaces:              ${PythonFound}
	ASpell spell checker:           ${ASpellFound}
	HunSpell spell checker:         ${HunSpellFound}
	HunSpell dictionary location:   ${HunSpellDictDir}
	Boolean SAT parser:             ${enable_sat_solver}
	SQLite-backed dictionary:       ${SQLiteFound}
	Viterbi algorithm parser:       ${enable_viterbi}
	Corpus statistics database:     ${enable_corpus_stats}
	Anysplit word splitter:         ${enable_anysplit}
	RegEx tokenizer:                ${enable_regex_tokenizer}
"
