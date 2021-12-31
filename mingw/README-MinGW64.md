BUILDING on Windows (MinGW/MSYS2)
=================================

General
-------
In this document we will suppose you use a 64-bit machine.

MinGW/MSYS2 uses the Gnu toolset to compile Windows programs for
Windows.  This is probably the easiest way to obtain workable Java
bindings for Windows.

In order to prepare this document, MSYS2 and Mingw-w64 (current to the start of 04/2021,
based on Cygwin 3.2.0, with GCC 102.0) were installed.

A note for new MSYS2 users
--------------------------
Download and install MinGW/MSYS2 from http://msys2.org.

MSYS2 uses the `pacman` package management. If you are not familiar
with it, consult the
[Pacman Rosetta](https://wiki.archlinux.org/index.php/Pacman/Rosetta).<br>
For specific details of the MSYS2 package management, consult with
[Package Management](https://www.msys2.org/docs/package-management/).

Also note that MSYS2 have two working modes (shells): MSYS and MINGW64.
The difference between them is the value of environment variables, e.g.
PATH, PKG_CONFIG_PATH and MANPATH.  For native Windows software
development, the MINGW64 shell must be used (the MSYS shell is for
MinGW/MSYS2 development- applications compiled from it recognize the MSYS
virtual filesystem).

First install `mingw-w64-x86_64-toolchain`. Also install the rest of the
prerequisite tools from the list in the main
[README](/README.md#building-from-the-github-repository).

NOTE (may not needed with recent installations):
You must also install **mingw-w64-x86_64-pkg-config** .

You may find that the system is extremely slow. In that case, consult the
Web for how to make tweaks that considerably speed it up. In addition, to
avoid I/O thrashing, don't use a too high `make` parallelism (maybe even
only `-j 2` or nothing at all).

Packages that are used by the link-grammar library
--------------------------------------------------

mingw-w64-x86_64-sqlite3<br>
mingw-w64-x86_64-libtre-git<br>
mingw-w64-x86_64-gettext<br>
mingw-w64-x86_64-hunspell, mingw-w64-x86_64-hunspell-en (optional)<br>
zlib-devel (optional - for the SAT parser)<br>
mingw-w64-x86_64-python3<br>

Java bindings
-------------
Install [Apache Ant](ant.apache.org/manual/install.html) and
Java JDK & JRE (both under Windows). Make sure you have
the environment variable JAVA_HOME set as needed (under Windows,
the MINGW64 shell will inherit it).

Then build and install link-grammar with

     mkdir build
     cd build
     ../configure
     make
     make install

In MINGW64, the default install prefix is `/mingw64` which is mapped to
`C:\msys64\mingw64`, so after 'make install', the libraries and executable
will be found at `C:\msys64\mingw64\bin` and the dictionary files at
`C:\msys64\mingw64\share\link-grammar`.


Python bindings
---------------
The bindings for Python3 (3.7.0) works fine.<br>
All the tests pass (when configured with `hunspell` and the SAT parser).

Here is a way to work with python3 from Windows:
```
C:\>cd \msys64\mingw64\bin
C:\msys64\mingw64\bin>.\python3
Python 3.7.0 (default, Jul 14 2018, 09:27:14)  [GCC 7.3.0 64 bit (AMD64)] on win32
Type "help", "copyright", "credits" or "license" for more information.
>>> from linkgrammar import *
>>> print(Sentence("This is a test",Dictionary(),ParseOptions()).parse().next().diagram())

    +----->WV----->+---Ost--+
    +-->Wd---+-Ss*b+  +Ds**c+
    |        |     |  |     |
LEFT-WALL this.p is.v a  test.n

>>>
```

Build break due to recent FORTIFY_SOURCES misconfiguration in Mingw-w64
-----------------------------------------------------------------------
Some packages are compiled with FORTIFY_SOURCES=2. Their configurations updates
CFLAGS but not LIBS. If you get linking errors on undefined references to `__strcpy_chk`
and `__stack_chk_fail`, add LIBS=-lssp to `configure` as follows:<br>
`configure LIBS=-lssp ...`

Test results
------------
All the `tests.py` tests pass, and also `make installcheck` is fine.
From the tests in the `tests/`directory, the `multi-thread` test fails due to segfault
at the hunspell library (a fix for that is being investigated). Regretfully, using
Aspell for now is not an option, as it is not available yet for MinGW.

BTW, here is how to run the `java-multi` test directly:<br>
`cd tests; make TEST_LOGS=multi-java check-TESTS`

Running
-------
On MINGW64, just invoke `link-parser`.<br>
Note: Currently it gets stuck under MINGW64 just before printing the `linkparser>` prompt (will be checked).

In Windows, put `C:\msys64\mingw64\bin` in your PATH (or `cd` to it), then invoke `link-parser`.
On both MINGW64 and Windows console, you can invoke `mingw/link-parser.bat`, which also
sets PATH for the `dot` and `PhotoViewer` commands (needed for the wordgraph display feature).
For more details see [RUNNING the program](/README.md#running-the-program).
