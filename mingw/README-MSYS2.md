BUILDING on Windows (MinGW/MSYS2)
=================================

General
-------
In this document we will suppose you use a 64-bit machine.

MinGW/MSYS2 uses the Gnu toolset to compile Windows programs for
Windows.  This is probably the easiest way to obtain workable Java
bindings for Windows.

In order to prepare this document, MSYS2 version 20161025 was installed.

A note for new MSYS2 users
--------------------------
Download and install MinGW/MSYS2 from http://msys2.org.

MSYS2 uses the `pacman` package management. If you are not familiar
with it, consult the
[Pacman Rosetta](https://wiki.archlinux.org/index.php/Pacman/Rosetta).

Also note that MSYS2 have two working modes (shells): MSYS and MINGW64.
The difference between them is the value of environment variables, e.g.
PATH, PKG_CONFIG_PATH and MANPATH.  For native Windows software
development, the MINGW64 shell must be used (the MSYS shell is for
MinGW/MSYS2 development- applications compiled from it recognize the MSYS
virtual filesystem).

First install `mingw-w64-x86_64-toolchain`. Also install the rest of the
prerequisite tools from the list in the main
[README](/README.md#building-from-the-github-repository).

NOTE: You must also install **mingw-w64-x86_64-pkg-config** .

You may find that the system is extremely slow. In that case, consult the
Web for how to make tweaks that considerably speed it up. In addition, to
avoid I/O trashing, don't use a too high `make` parallelism (maybe even
only `-j 2`).

Packages that are used by the link-grammar library
--------------------------------------------------

mingw-w64-x86_64-sqlite3<br>
mingw-w64-x86_64-libtre-git<br>
mingw-w64-x86_64-gettext<br>
mingw-w64-x86_64-hunspell, mingw-w64-x86_64-hunspell-en (optional)<br>
zlib-devel (optional - for the SAT parser)<br>

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

In case you would like to build with hunspell, invoke `configure` as follows:<br>
```
configure --with-hunspell-dictdir=`cygpath -m /mingw64/share/myspell/dicts`
```


Python bindings
---------------
The bindings for Python2 (package `python2 2.7.13-1` for MSYS) work fine.<br>
All the tests pass (when configured with `hunspell` and the SAT parser).

Here is a way to work with it from Windows:
```
C:\>cd msys64\mingw64\bin
C:\msys64\mingw64\bin>C:\msys64\usr\bin\python2.exe
Python 2.7.13 (default, Feb 14 2017, 14:46:01)
[GCC 6.3.0] on msys
Type "help", "copyright", "credits" or "license" for more information.
>>> import sys
>>> sys.path.insert(0, 'C:\msys64\mingw64\lib\python2.7\site-packages')
>>> import linkgrammar
>>>
```
(Alternatively, you can add `C:\>cd msys64\mingw64;C:\msys64\usr\bin` to the PATH
and set `PYTHONPATH=C:\msys64\mingw64\lib\python2.7\site-packages`).

However the bindings for the MINGW64 Python2 and Python3 don't work. For the MINGW64 version
`mingw-w64-x86_64-python3-3.6.4-2` (and similarly for mingw-w64-x86_64-python2-2.7.14-5`)
it even doesn't compile due to problems in its `pyconfig.h`:
```
/mingw64/include/python3.6m/pyconfig.h:1546:15: error: two or more data types in declaration specifiers
 #define uid_t int
/mingw64/include/python3.6m/pyport.h:705:2: error: #error "LONG_BIT definition appears wrong for platform (bad gcc/glibc config?)."
 #error "LONG_BIT definition appears wrong for platform (bad gcc/glibc config?)."
```

The binding for the MSYS Python3 (named just `python`) compile, but
importing `link-grammar` causes the infamous problem of
`Fatal Python error: PyThreadState_Get: no current thread`.
(This version also insists that the _clinkparser module name will end with
`.pyd` and not `dll`.)

Test results
------------
All the `tests.py` tests pass, along with all the tests in the `tests`
directory (including the `multi-java` test) and `make installcheck`.

Here is how to run the `java-multi` test directly:<br>
`cd tests; make TEST_LOGS=multi-java check-TESTS`

Running
-------
On MINGW64, just invoke `link-parser`.<br>
In Windows, put `C:\msys64\mingw64\bin` in your PATH (or cd to it), then invoke `link-parser`.
For more details see [RUNNING the program](/README.md#running-the-program).
