Link Grammar Parser
===================
***Version 5.12.0***

![Main](https://github.com/opencog/link-grammar/actions/workflows/main.yml/badge.svg)
![node.js](https://github.com/opencog/link-grammar/actions/workflows/bindings-js.yml/badge.svg)

The Link Grammar Parser exhibits the linguistic (natural language)
structure of English, Russian, Arabic, Persian and limited subsets
of a half-dozen other languages. This structure is a graph of typed
links (edges) between the words in a sentence. One may obtain the
more conventional HPSG (constituent) and dependency style parses
from Link Grammar by applying a collection of rules to convert to
these different formats. This is possible because Link Grammar goes
a bit "deeper" into the "syntactico-semantic" structure of a sentence:
it provides considerably more fine-grained and detailed information
than what is commonly available in conventional parsers.

The theory of Link Grammar parsing was originally developed in 1991
by Davy Temperley, John Lafferty and Daniel Sleator, at the time
professors of linguistics and computer science at the Carnegie Mellon
University.  The three initial publications on this theory provide the
best introduction and overview; since then, there have been hundreds
of publications further exploring, examining and extending the ideas.

Although based on the
[original Carnegie-Mellon code base](https://www.link.cs.cmu.edu/link/)
the current Link Grammar package has dramatically evolved and is
profoundly different from earlier versions.  There have been innumerable
bug fixes; performance has improved by more than an order of magnitude.
The package is fully multi-threaded, fully UTF-8 enabled, and has been
scrubbed for security, enabling cloud deployment.  Parse coverage of
English has been dramatically improved; other languages have been added
(most notably, Russian). There is a raft of new features, including support
for morphology, log-likelihood semantic selection, and a sophisticated
tokenizer that moves far beyond white-space-delimited sentence-splitting.
Detailed lists can be found in the [ChangeLog](ChangeLog).

This code is released under the LGPL license, making it freely
available for both private and commercial use, with few restrictions.
The terms of the license are given in the LICENSE file included with
this software.

Please see the
[main web page](http://www.abisource.com/projects/link-grammar/)
for more information.  This version is a continuation of the
[original CMU parser](http://www.link.cs.cmu.edu/link).

### New!
As of version 5.9.0, the system include an experimental system for
generating sentences. These are specified using a "fill in the blanks"
API, where words are substituted into wild-card locations, whenever the
result is a grammatically valid sentence. Additional details are in
the man page: `man link-generator` (in the `man` subdirectory).

This generator is used in the
[OpenCog Language Learning](https://github.com/opencog/learn)
project, which aims to automatically learn Link Grammars from corpora,
using brand-new and innovative information theoretic techniques,
somewhat similar to those found in artificial neural nets (deep
learning), but using explicitly symbolic representations.

Quick Overview
---------------
The parser includes API's in various different programming languages,
as well as a handy command-line tool for playing with it.  Here's some
typical output:
```
linkparser> This is a test!
	Linkage 1, cost vector = (UNUSED=0 DIS= 0.00 LEN=6)

    +-------------Xp------------+
    +----->WV----->+---Ost--+   |
    +---Wd---+-Ss*b+  +Ds**c+   |
    |        |     |  |     |   |
LEFT-WALL this.p is.v a  test.n !

(S (NP this.p) (VP is.v (NP a test.n)) !)

            LEFT-WALL    0.000  Wd+ hWV+ Xp+
               this.p    0.000  Wd- Ss*b+
                 is.v    0.000  Ss- dWV- O*t+
                    a    0.000  Ds**c+
               test.n    0.000  Ds**c- Os-
                    !    0.000  Xp- RW+
           RIGHT-WALL    0.000  RW-

```
This rather busy display illustrates many interesting things. For
example, the `Ss*b` link connects the verb and the subject, and
indicates that the subject is singular.  Likewise, the `Ost` link
connects the verb and the object, and also indicates that the object
is singular. The `WV` (verb-wall) link points at the head-verb of
the sentence, while the `Wd` link points at the head-noun. The `Xp`
link connects to the trailing punctuation. The `Ds**c` link connects
the noun to the determiner: it again confirms that the noun is singular,
and also that the noun starts with a consonant. (The `PH` link, not
required here, is used to force phonetic agreement, distinguishing
'a' from 'an').  These link types are documented in the
[English Link Documentation](https://www.abisource.com/projects/link-grammar/dict/index.html).

The bottom of the display is a listing of the "disjuncts" used for
each word. The disjuncts are simply a list of the connectors that
were employed to form the links. They are particularly interesting
because they serve as an extremely fine-grained form of a "part of
speech".  Thus, for example: the disjunct `S- O+` indicates a
transitive verb: its a verb that takes both a subject and an object.
The additional markup above indicates that 'is' is not only being used
as a transitive verb, but it also indicates finer details: a transitive
verb that took a singular subject, and was used (is usable as) the head
verb of a sentence.  The floating-point value is the "cost" of the
disjunct; it very roughly captures the idea of the log-probability
of this particular grammatical usage.  Much as parts-of-speech correlate
with word-meanings, so also fine-grains parts-of-speech correlate with
much finer distinctions and gradations of meaning.

The link-grammar parser also supports morphological analysis. Here is
an example in Russian:
```
linkparser> это теста
	Linkage 1, cost vector = (UNUSED=0 DIS= 0.00 LEN=4)

             +-----MVAip-----+
    +---Wd---+       +-LLCAG-+
    |        |       |       |
LEFT-WALL это.msi тест.= =а.ndnpi
```

The `LL` link connects the stem 'тест' to the suffix 'а'. The `MVA`
link connects only to the suffix, because, in Russian, it is the
suffixes that carry all of the syntactic structure, and not the stems.
The Russian lexis is
[documented here](https://www.abisource.com/projects/link-grammar/russian/doc/).

The Thai dictionary is now fully developed, effectively covering the
entire language.  An example in Thai:
```
linkparser> นายกรัฐมนตรี ขึ้น กล่าว สุนทรพจน์
	Linkage 1, cost vector = (UNUSED=0 DIS= 2.00 LEN=2)

    +---------LWs--------+
    |           +<---S<--+--VS-+-->O-->+
    |           |        |     |       |
LEFT-WALL นายกรัฐมนตรี.n ขึ้น.v กล่าว.v สุนทรพจน์.n
```

The `VS` link connects two verbs 'ขึ้น' and 'กล่าว' in a serial verb
construction. A summary of link types is
[documented here](data/th/README.md). Full documentation of Thai Link
Grammar can be [found here](data/th/LINKDOC.md).

Thai Link Grammar also accepts POS-tagged and named-entity-tagged
inputs. Each word can be annotated with the Link POS tag. For example:

```
linkparser> เมื่อวานนี้.n มี.ve คน.n มา.x ติดต่อ.v คุณ.pr ครับ.pt
Found 1 linkage (1 had no P.P. violations)
	Unique linkage, cost vector = (UNUSED=0 DIS= 0.00 LEN=12)

                          +---------------------PT--------------------+
    +---------LWs---------+---------->VE---------->+                  |
    |           +<---S<---+-->O-->+       +<--AXw<-+--->O--->+        |
    |           |         |       |       |        |         |        |
LEFT-WALL เมื่อวานนี้.n[!] มี.ve[!] คน.n[!] มา.x[!] ติดต่อ.v[!] คุณ.pr[!] ครับ.pt[!]
```

Full documentation for the Thai dictionary can be
[found here](th/INPUT_FORMATS.md).

The Thai dictionary accepts LST20 tagsets for POS and named entities,
to bridge the gap between fundamental NLP tools and the Link Parser.
For example:

```
linkparser> linkparser> วันที่_25_ธันวาคม@DTM ของ@PS ทุก@AJ ปี@NN เป็น@VV วัน@NN คริสต์มาส@NN
Found 348 linkages (348 had no P.P. violations)
	Linkage 1, cost vector = (UNUSED=0 DIS= 1.00 LEN=10)

    +--------------------------------LWs--------------------------------+
    |               +<------------------------S<------------------------+
    |               |                +---------->PO--------->+          |
    |               +----->AJpr----->+            +<---AJj<--+          +---->O---->+------NZ-----+
    |               |                |            |          |          |           |             |
LEFT-WALL วันที่_25_ธันวาคม@DTM[!] ของ@PS[!].pnn ทุก@AJ[!].jl ปี@NN[!].n เป็น@VV[!].v วัน@NN[!].na คริสต์มาส@NN[!].n
```

Note that each word above is annotated with LST20 POS tags and NE tags.
Full documentation for both the Link POS tags and the LST20 tagsets can
be [found here](data/th/TAGSETS.md). More information about LST20, e.g.
annotation guideline and data statistics, can be
[found here](https://arxiv.org/abs/2008.05055).

Theory and Documentation
------------------------
An extended overview and summary can be found in the
[Link Grammar Wikipedia page](https://en.wikipedia.org/wiki/Link_grammar),
which touches on most of the import, primary aspects of the theory.
However, it is no substitute for the original papers published on the
topic:
* Daniel D. K. Sleator, Davy Temperley,
  ["Parsing English with a Link Grammar"](http://www.cs.cmu.edu/afs/cs.cmu.edu/project/link/pub/www/papers/ps/tr91-196.pdf)
  October 1991 *CMU-CS-91-196*.
* Daniel D. Sleator, Davy Temperley,
  ["Parsing English with a Link Grammar"](http://www.cs.cmu.edu/afs/cs.cmu.edu/project/link/pub/www/papers/ps/LG-IWPT93.pdf),
  *Third International Workshop on Parsing Technologies* (1993).
* Dennis Grinberg, John Lafferty, Daniel Sleator,
  ["A Robust Parsing Algorithm for Link Grammars"](http://www.cs.cmu.edu/afs/cs.cmu.edu/project/link/pub/www/papers/ps/tr95-125.pdf),
  August 1995 *CMU-CS-95-125*.
* John Lafferty, Daniel Sleator, Davy Temperley,
  ["Grammatical Trigrams: A Probabilistic Model of Link Grammar"](http://www.cs.cmu.edu/afs/cs.cmu.edu/project/link/pub/www/papers/ps/gram3gram.pdf),
  1992 *AAAI Symposium on Probabilistic Approaches to Natural Language*.

There are many more papers and references listed on the
[primary Link Grammar website](https://www.abisource.com/projects/link-grammar/).

See also the
[C/C++ API documentation](https://www.abisource.com/projects/link-grammar/api/index.html).
Bindings for other programming languages, including python3, java
and node.js, can be found in the [bindings directory](bindings).
(There are two sets of javascript bindings: one set for the library API,
and another set for the command-line parser.)

Contents
--------

| Content       | Description |
| ------------- |-------------|
| LICENSE     | The license describing terms of use |
| link-grammar/*.c | The program.  (Written in ANSI-C) |
| ---- | ---- |
| bindings/autoit/  | Optional AutoIt language bindings. |
| bindings/java/ | Optional Java language bindings. |
| bindings/js/ | Optional JavaScript language bindings. |
| bindings/lisp/ | Optional Common Lisp language bindings. |
| bindings/node.js/ | Optional node.js language bindings. |
| bindings/ocaml/ | Optional OCaML language bindings. |
| bindings/python/  | Optional Python3 language bindings. |
| bindings/python-examples/ | Link-grammar test suite and Python language binding usage example. |
| bindings/swig/ | SWIG interface file, for other FFI interfaces. |
| bindings/vala/  | Optional Vala language bindings. |
| ---- | ---- |
| data/en/ | English language dictionaries. |
| data/en/4.0.dict | The file containing the dictionary definitions. |
| data/en/4.0.knowledge | The post-processing knowledge file. |
| data/en/4.0.constituents | The constituent knowledge file. |
| data/en/4.0.affix | The affix (prefix/suffix) file. |
| data/en/4.0.regex | Regular expression-based morphology guesser. |
| data/en/tiny.dict | A small example dictionary. |
| data/en/words/ | A directory full of word lists. |
| data/en/corpus*.batch | Example corpora used for testing. |
| ---- | ---- |
| data/ru/ | A full-fledged Russian dictionary |
| data/th/ | A full-fledged Thai dictionary (100,000+ words) |
| data/ar/ | A fairly complete Arabic dictionary |
| data/fa/ | A Persian (Farsi) dictionary |
| data/de/ | A small prototype German dictionary |
| data/lt/ | A small prototype Lithuanian dictionary |
| data/id/ | A small prototype Indonesian dictionary |
| data/vn/ | A small prototype Vietnamese dictionary |
| data/he/ | An experimental Hebrew dictionary |
| data/kz/ | An experimental Kazakh dictionary |
| data/tr/ | An experimental Turkish dictionary |
| ---- | ---- |
| morphology/ar/ | An Arabic morphology analyzer |
| morphology/fa/ | An Persian morphology analyzer |
| ---- | ---- |
| LICENSE | The license for this code and data |
| ChangeLog | A compendium of recent changes. |
| configure | The GNU configuration script |
| autogen.sh | Developer's configure maintenance tool |
| debug/ | Information about debugging the library |
| msvc/ | Microsoft Visual-C project files |
| mingw/ | Information on using MinGW under MSYS or Cygwin |

UNPACKING and signature verification
------------------------------------
The system is distributed using the conventional `tar.gz` format;
it can be extracted using the `tar -zxf link-grammar.tar.gz` command
at the command line.

A tarball of the latest version can be downloaded from:<br>
http://www.abisource.com/downloads/link-grammar

The files have been digitally signed to make sure that there was no
corruption of the dataset during download, and to help ensure that
no malicious changes were made to the code internals by third
parties. The signatures can be checked with the gpg command:

`gpg --verify link-grammar-5.12.0.tar.gz.asc`

which should generate output identical to (except for the date):
```
gpg: Signature made Thu 26 Apr 2012 12:45:31 PM CDT using RSA key ID E0C0651C
gpg: Good signature from "Linas Vepstas (Hexagon Architecture Patches) <linas@codeaurora.org>"
gpg:                 aka "Linas Vepstas (LKML) <linasvepstas@gmail.com>"
```
Alternately, the md5 check-sums can be verified. These do not provide
cryptographic security, but they can detect simple corruption. To
verify the check-sums, issue `md5sum -c MD5SUM` at the command line.

Tags in `git` can be verified by performing the following:
```
gpg --recv-keys --keyserver keyserver.ubuntu.com EB6AA534E0C0651C
git tag -v link-grammar-5.10.5
```


CREATING the system
-------------------
To compile the link-grammar shared library and demonstration program,
at the command line, type:
```
./configure
make
make check
```

To install, change user to "root" and say
```
make install
ldconfig
```

This will install the `liblink-grammar.so` library into `/usr/local/lib`,
the header files in `/usr/local/include/link-grammar`, and the
dictionaries into `/usr/local/share/link-grammar`.  Running `ldconfig`
will rebuild the shared library cache.  To verify that the install was
successful, run (as a non-root user)
```
make installcheck
```

### Optional system libraries
The link-grammar library has optional features that are enabled automatically
if `configure` detects certain libraries. These libraries are optional on most
of the systems and if the feature they add is desired, corresponding libraries
need to be installed before running `configure`.

The library package names may vary on various systems (consult Google if
needed...).  For example, the names may include `-devel` instead of `-dev`, or
be without it altogether. The library names may be without the prefix `lib`.

* `libsqlite3-dev` (for SQLite-backed dictionary)<br>
* `libz1g-dev` or `libz-devel` (currently needed for the bundled `minisat2`)<br>
* `libedit-dev` (see [Editline](#Editline))<br>
* `libhunspell-dev` or `libaspell-dev` (and the corresponding English dictionary).<br>
* `libtre-dev` or `libpcre2-dev` (much faster than the libc REGEX
implementation, and needed for correctness on FreeBSD and Cygwin).<br>
Using `libpcre2-dev` is strongly recommended. It must be used on certain
systems (as specified in their BUILDING sections).

Note: BSD-derived operating systems (including macOS) need the
`argp-standalone` library in order to build the `link-generator` program.

### Editline
If `libedit-dev` is installed, then the arrow keys can be used to edit
the input to the link-parser tool; the up and down arrow keys will
recall previous entries.  You want this; it makes testing and
editing much easier.


### Node.js Bindings
Two versions of node.js bindings are included. One version wraps the
library; the other uses emscripten to wrap the command-line tool. The
library bindings are in `bindings/node.js` while the emscripten wrapper
is in `bindings/js`.

These are built using `npm`. First, you must build the core C library.
Then do the following:
```
   cd bindings/node.js
   npm install
   npm run make
```
This will create the library bindings and also run a small unit test
(which should pass). An example can be found in
`bindings/node.js/examples/simple.js`.

For the command-line wrapper, do the following:
```
   cd bindings/js
   ./install_emsdk.sh
   ./build_packages.sh
```

### Python3 Bindings
The Python3 bindings are built by default, providing that
the corresponding Python development packages are installed.
(Python2 bindings are no longer supported.)

These packages are:
- Linux:
   * Systems using 'rpm' packages: `python3-devel`
   * Systems using 'deb' packages: `python3-dev`
- Windows:
   * Install Python3 from https://www.python.org/downloads/windows/ .
     You also have to install SWIG from http://www.swig.org/download.html .
- macOS:
   * Install python3 using [HomeBrew](http://brew.sh/).

NOTE: Before issuing `configure` (see below) you have to validate that
the required python versions can be invoked using your `PATH`.

The use of the Python bindings is *OPTIONAL*; you do not need these if you
do not plan to use link-grammar with Python.  If you like to disable
Python bindings, use:

```
./configure --disable-python-bindings
```

The `linkgrammar.py` module provides a high-level interface in Python.
The `example.py` and `sentence-check.py` scripts provide a demo,
and `tests.py` runs unit tests.

- macOS:
   * Due to file permissions settings, macOS users may need to install
     python bindindings into custom directory locations. This can be
     done by saying
     `make install pythondir=/where/to/install`

### Java Bindings
By default, the `Makefile`s attempt to build the Java bindings.
The use of the Java bindings is *OPTIONAL*; you do not need these if
you do not plan to use link-grammar with Java.  You can skip building
the Java bindings by disabling as follows:
```
./configure --disable-java-bindings
```

If `jni.h` isn't found, or if `ant` isn't found,
then the java bindings will not be built.

Notes about finding `jni.h`:<br>
Some common java JVM distributions (most notably, the ones from Sun)
place this file in unusual locations, where it cannot be
automatically found.  To remedy this, make sure that environment variable
`JAVA_HOME` is set correctly. The configure script looks for `jni.h` in
`$JAVA_HOME/Headers` and in `$JAVA_HOME/include`; it also examines
corresponding locations for `$JDK_HOME`.  If `jni.h `still cannot be
found, specify the location with the `CPPFLAGS` variable: so, for example,
```
export CPPFLAGS="-I/opt/jdk1.5/include/:/opt/jdk1.5/include/linux"
```
or
```
export CPPFLAGS="-I/c/java/jdk1.6.0/include/ -I/c/java/jdk1.6.0/include/win32/"
```
Please note that the use of `/opt` is non-standard, and most system
tools will fail to find packages installed there.

Install location
----------------
The `/usr/local` install target can be over-ridden using the
standard GNU `configure --prefix` option; so, for example:
```
./configure --prefix=/opt/link-grammar
```

By using `pkg-config` (see below), non-standard install locations
can be automatically detected.

Custom builds
-------------
Additional config options are printed by
```
./configure --help
```

The system has been tested and works well on 32 and 64-bit Linux
systems, FreeBSD, macOS, as well as on Microsoft Windows systems.
Specific OS-dependent notes follow.

### BUILDING from the [GitHub repository](https://github.com/opencog/link-grammar)

End users should download the tarball (see
[UNPACKING and signature verification](#unpacking-and-signature-verification)).

The current GitHub version is intended for developers (including anyone who
is willing to provide a fix, a new feature or an improvement). The tip of
the master branch is often unstable, and can sometimes have bad code in it
as it is under development. It also needs installing of development tools
that are not installed by default. Due to these reason the use of the GitHub
version is discouraged for regular end users.

#### Installing from GitHub
Clone it:
`git clone https://github.com/opencog/link-grammar.git`<br>
Or download it as a ZIP:<br>
`https://github.com/opencog/link-grammar/archive/master.zip`

#### Prerequisite tools
Tools that may need installation before you can build link-grammar:

`make` (the `gmake` variant may be needed)<br>
`m4`<br>
`gcc` or `clang`<br>
`autoconf`<br>
`libtool`<br>
`autoconf-archive`<br>
`pkg-config` (may be named `pkgconf` or `pkgconfig`)<br>
`pip3` (for the Python bindings)<br>

Optional:<br>
`swig` (for language bindings)<br>
`flex`<br>
Apache Ant (for Java bindings)<br>
`graphviz` (if you wish to use the word-graph display feature)

The GitHub version doesn't include a `configure` script.
To generate it, use:
```
autogen.sh
```

If you get errors, make sure you have installed the above-listed
development packages, and that your system installation is up to date.
Especially, missing `autoconf` or `autoconf-archive` may
cause strange and misleading errors.

For more info about how to proceed, continue at the section
[CREATING the system](#creating-the-system) and the relevant sections after it.

#### Additional notes for developers

To configure **debug** mode, use:
```
configure --enable-debug
```
It adds some verification debug code and functions that can
pretty-print several data structures.

A feature that may be useful for debugging is the word-graph
display.  It is enabled by default. For more details on this feature, see
[Word-graph display](link-grammar/tokenize/README.md#word-graph-display).


### BUILDING on FreeBSD

The current configuration has an apparent standard C++ library mixing problem
when `gcc` is used (a fix is welcome). However, the common practice on FreeBSD
is to compile with `clang`, and it doesn't have this problem. In addition,
the add-on packages are installed under `/usr/local`.

So here is how `configure` should be invoked:
```
env LDFLAGS=-L/usr/local/lib CPPFLAGS=-I/usr/local/include \
CC=clang CXX=clang++ configure
```

Note that `pcre2` is a required package as the existing `libc`
regex implementation doesn't have the needed level of regex support.

Some packages have different names than the ones mentioned in the previous
sections:

`minisat` (minisat2)
`pkgconf` (pkg-config)

### BUILDING on macOS
Plain-vanilla Link Grammar should compile and run on Apple macOS
just fine, as described above.  At this time, there are no reported
issues.

If you do NOT need the java bindings, you should almost surely
configure with:
```
./configure --disable-java-bindings
```

By default, java requires a 64-bit binary, and not all macOS systems
have a 64-bit devel environment installed.

If you do want Java bindings, be sure to set the JDK_HOME environment
variable to wherever `<Headers/jni.h>` is.   Set the JAVA_HOME variable
to the location of the java compiler.  Make sure you have ant
installed.

If you would like to build from GitHub
(see [BUILDING from the GitHub repository](#building-from-the-github-repository))
you can install the tools that are listed there using
[HomeBrew](http://brew.sh/).

### BUILDING on Windows
There are three different ways in which link-grammar can be compiled
on Windows.  One way is to use Cygwin, which provides a Linux
compatibility layer for Windows. Another way is use the
MSVC system. A third way is to use the MinGW system, which uses the
Gnu toolset to compile windows programs. The source code supports
Windows systems from Vista on.

The Cygwin way currently produces the best result, as it supports line editing
with command completion and history and also supports word-graph displaying on
X-windows. (MinGW currently doesn't  have `libedit`, and the MSVC port
currently doesn't support command completion and history, and also spelling.

### BUILDING on Windows (Cygwin)
The easiest way to have link-grammar working on MS Windows is to
use Cygwin, a Linux-like environment for Windows making it possible
to port software running on POSIX systems to Windows.  Download and
install [Cygwin](http://www.cygwin.com/).

Note that the installation of the `pcre2` package is required because the libc
REGEX implementation is not capable enough.

For more details See [mingw/README-Cygwin.md](mingw/README-Cygwin.md).

### BUILDING on Windows (MinGW)
Another way to build link-grammar is to use MinGW, which uses the GNU
toolset to compile POSIX-compliant programs for Windows. Using MinGW/MSYS2 is
probably the easiest way to obtain workable Java bindings for Windows.
Download and install MinGW/MSYS2 from [msys2.org](msys2.org).

Note that the installation of the `pcre2` package is required because the libc
REGEX implementation is not capable enough.

For more details see [mingw/README-MinGW64.md](mingw/README-MinGW64.md).

### BUILDING and RUNNING on Windows (MSVC)
Microsoft Visual C/C++ project files can be found in the `msvc` directory.
For directions see the [README.md](msvc/README.md) file there.

RUNNING the program
-------------------
To run the program issue the command (supposing it is in your PATH):
```
link-parser [arguments]
```

This starts the program.  The program has many user-settable variables
and options. These can be displayed by entering `!var` at the link-parser
prompt.  Entering `!help` will display some additional commands.

The dictionaries are arranged in directories whose name is the 2-letter
language code. The link-parser program searches for such a language
directory in that order, directly or under a directory names `data`:

1. Under your current directory.
2. Unless compiled with MSVC or run under the Windows console:
   At the installed location (typically in `/usr/local/share/link-grammar`).
3. If compiled on Windows: In the directory of the link-parser
   executable (may be in a different location than the link-parser
   command, which may be a script).

If link-parser cannot find the desired dictionary, use verbosity
level 4 to debug the problem; for example:
```
link-parser ru -verbosity=4
```

Other locations can be specified on the command line; for example:
```
link-parser ../path/to-my/modified/data/en
```

When accessing dictionaries in non-standard locations, the standard
file-names are still assumed (*i.e.* `4.0.dict`, `4.0.affix`, *etc.*).

The Russian dictionaries are in `data/ru`. Thus, the Russian parser
can be started as:
```
link-parser ru
```

If you don't supply an argument to link-parser, it searches for a
language according to your current locale setup. If it cannot find such
a language directory, it defaults to "en".

If you see errors similar to this:
```
Warning: The word "encyclop" found near line 252 of en/4.0.dict
matches the following words:
encyclop
This word will be ignored.
```

then your UTF-8 locales are either not installed or not configured.
The shell command `locale -a` should list `en_US.utf8` as a locale.
If not, then you need to `dpkg-reconfigure locales` and/or run
`update-locale` or possibly `apt-get install locales`, or
combinations or variants of these, depending on your operating
system.


TESTING the system
------------------
There are several ways to test the resulting build.  If the Python
bindings are built, then a test program can be found in the file
`./bindings/python-examples/tests.py` -- When run, it should pass.
For more details see [README.md](bindings/python-examples/README.md)
in the `bindings/python-examples` directory.

There are also multiple batches of test/example sentences in the
language data directories, generally having the names `corpus-*.batch`
The parser program can be run in batch mode, for testing the system
on a large number of sentences.  The following command runs the
parser on a file called `corpus-basic.batch`;
```
link-parser < corpus-basic.batch
```

The line `!batch` near the top of corpus-basic.batch turns on batch
mode.  In this mode, sentences labeled with an initial `*` should be
rejected and those not starting with a `*` should be accepted.  This
batch file does report some errors, as do the files `corpus-biolg.batch`
and `corpus-fixes.batch`.  Work is ongoing to fix these.

The `corpus-fixes.batch` file contains many thousands of sentences
that have been fixed since the original 4.1 release of link-grammar.
The `corpus-biolg.batch` contains biology/medical-text sentences from
the BioLG project. The `corpus-voa.batch` contains samples from Voice
of America; the `corpus-failures.batch` contains a large number of
failures.

The following numbers are subject to change, but, at this time, the
number of errors one can expect to observe in each of these files
are roughly as follows:
```
en/corpus-basic.batch:      88 errors
en/corpus-fixes.batch:     371 errors
lt/corpus-basic.batch:      15 errors
ru/corpus-basic.batch:      47 errors
```
The bindings/python directory contains a unit test for the Python
bindings. It also performs several basic checks that stress the
link-grammar libraries.


USING the system
-----------------
There is an API (application program interface) to the parser.  This
makes it easy to incorporate it into your own applications.  The API
is documented on the web site.


### USING CMake
The `FindLinkGrammar.cmake` file can be used to test for and set up
compilation in CMake-based build environments.


### USING pkg-config
To make compiling and linking easier, the current release uses
the pkg-config system. To determine the location of the link-grammar
header files, say `pkg-config --cflags link-grammar`  To obtain
the location of the libraries, say `pkg-config --libs link-grammar`
Thus, for example, a typical makefile might include the targets:
```
.c.o:
   cc -O2 -g -Wall -c $< `pkg-config --cflags link-grammar`

$(EXE): $(OBJS)
   cc -g -o $@ $^ `pkg-config --libs link-grammar`
```

### Using JAVA
This release provides java files that offer three ways of accessing
the parser.  The simplest way is to use the org.linkgrammar.LinkGrammar
class; this provides a very simple Java API to the parser.

The second possibility is to use the LGService class.  This implements
a TCP/IP network server, providing parse results as JSON messages.
Any JSON-capable client can connect to this server and obtain parsed
text.

The third possibility is to use the org.linkgrammar.LGRemoteClient
class, and in particular, the parse() method.  This class is a network
client that connects to the JSON server, and converts the response
back to results accessible via the ParseResult API.

The above-described code will be built if Apache `ant` is installed.


### Using the JSON Network Server
The network server can be started by saying:
```
java -classpath linkgrammar.jar org.linkgrammar.LGService 9000
```

The above starts the server on port 9000. It the port is omitted,
help text is printed.  This server can be contacted directly via
TCP/IP; for example:
```
telnet localhost 9000
```

(Alternately, use netcat instead of telnet). After connecting, type
in:
```
text:  this is an example sentence to parse
```

The returned bytes will be a JSON message providing the parses of
the sentence.  By default, the ASCII-art parse of the text is not
transmitted. This can be obtained by sending messages of the form:
```
storeDiagramString:true, text: this is a test.
```

### Spell Guessing
The parser will run a spell-checker at an early stage, if it
encounters a word that it does not know, and cannot guess, based on
morphology.  The configure script looks for the aspell or hunspell
spell-checkers; if the aspell devel environment is found, then
aspell is used, else hunspell is used.

Spell guessing may be disabled at runtime, in the link-parser client
with the `!spell=0` flag.  Enter `!help` for more details.


### Multi-threading
It is safe to use link-grammar for parsing in multiple threads.
Different threads may use different dictionaries, or the same dictionary.
Parse options can be set on a per-thread basis, with the exception of
verbosity, which is a global, shared by all threads.  It is the only
global.

Linguistic Commentary
=====================

Phonetics
---------
A/An phonetic determiners before consonants/vowels are handled by a
new PH link type, linking the determiner to the word immediately
following it.  Status: Introduced in version 5.1.0 (August 2014).
Mostly done, although many special-case nouns are unfinished.


Directional Links
-----------------
Directional links are needed for some languages, such as Lithuanian,
Turkish and other free word-order languages. The goal is to have
a link clearly indicate which word is the head word, and which is
the dependent. This is achieved by prefixing connectors with
a single *lower case* letter: h,d, indicating 'head' and 'dependent'.
The linkage rules are such that h matches either nothing or d, and
d matches h or nothing. This is a new feature in version 5.1.0
(August 2014). The website provides additional documentation.

Although the English-language link-grammar links are un-oriented,
it seems that a defacto direction can be given to them that is
completely consistent with standard conceptions of a dependency
grammar.

The dependency arrows have the following properties:

 * Anti-reflexive (a word cannot depend on itself; it cannot point
   at itself.)

 * Anti-symmetric (if Word1 depends on Word2, then Word2 cannot
   depend on Word1) (so, e.g. determiners depend on nouns, but
   never vice-versa)

 * The arrows are neither transitive, nor anti-transitive: a single
   word may be ruled by several heads.  For example:
```text
    +------>WV------->+
    +-->Wd-->+<--Ss<--+
    |        |        |
LEFT-WALL   she    thinks.v
```
That is, there is a path to the subject, "she", directly from the
left wall, via the Wd link, as well as indirectly, from the wall
to the root verb, and thence to the subject.  Similar loops form
with the B and R links.  Such loops are useful for constraining
the possible number of parses: the constraint occurs in
conjunction with the "no links cross" meta-rule.

 * The graphs are planar; that is, no two edges may cross. See,
   however, the "link-crossing" discussion below.


There are several related mathematical notions, but none quite
capture directional LG:

 * Directional LG graphs resemble DAGS, except that LG allows only
   one wall (one "top" element).

 * Directional LG graphs resemble strict partial orders, except that
   the LG arrows are usually not transitive.

 * Directional LG graphs resemble
   [catena](http://en.wikipedia.org/wiki/Catena_(linguistics))
   except that catena are strictly anti-transitive -- the path to
   any word is unique, in a catena.


Link Crossing
-------------
The foundational LG papers mandate the planarity of the parse graphs.
This is based on a very old observation that dependencies almost never
cross in natural languages: humans simply do not speak in sentences
where links cross.  Imposing planarity constraints then provides a
strong engineering and algorithmic constraint on the resulting parses:
the total number of parses to be considered is sharply reduced, and
thus the overall speed of parsing can be greatly increased.

However, there are occasional, relatively rare exceptions to this
planarity rule; such exceptions are observed in almost all languages.
A number of these exceptions are given for English, below.

Thus, it seems important to relax the planarity constraint, and find
something else that is almost as strict, but still allows infrequent
exceptions.  It would appear that the concept of "landmark transitivity"
as defined by Richard Hudson in his theory of "Word Grammar", and then
advocated by Ben Goertzel, just might be such a mechanism.

ftp://ftp.phon.ucl.ac.uk/pub/Word-Grammar/ell2-wg.pdf<br>
http://www.phon.ucl.ac.uk/home/dick/enc/syntax.htm<br>
http://goertzel.org/ProwlGrammar.pdf

Planarity: Theory vs. Practice
------------------------------
In practice, the planarity constraint allows very efficient algorithms
to be used in the implementation of the parser. Thus, from the point of
view of the implementation, we want to keep planarity. Fortunately,
there is a convenient and unambiguous way to have our cake and eat it,
too. A non-planar diagram can be drawn on a sheet of paper using
standard electrical-engineering notation: a funny symbol, wherever
wires cross. This notation is very easily adapted to LG connectors;
below is an actual working example, already implemented in the current
LG English dictionary. *All* link crossings can be implemented in this
way!  So we do not have to actually abandon the current parsing
algorithms to get non-planar diagrams. We don't even have to modify them!
Hurrahh!

Here is a working example: "I want to look at and listen to everything."
This wants two `J` links pointing to 'everything'.  The desired diagram
would need to look like this:

```text
    +---->WV---->+
    |            +--------IV---------->+
    |            |           +<-VJlpi--+
    |            |           |    +---xxx------------Js------->+
    +--Wd--+-Sp*i+--TO-+-I*t-+-MVp+    +--VJrpi>+--MVp-+---Js->+
    |      |     |     |     |    |    |        |      |       |
LEFT-WALL I.p want.v to.r look.v at and.j-v listen.v to.r everything
```
The above really wants to have a `Js` link from 'at' to 'everything',
but this `Js` link crosses (clashes with - marked by xxx) the link
to the conjunction.  Other examples suggest that one should
allow most links to cross over the down-links to conjunctions.

The planarity-maintaining worked-around is to split the `Js` link into
two: a `Jj` part and a `Jk` part; the two are used together to cross
over the conjunction. This is currently implemented in the English
dictionary, and it works.

This work-around is in fact completely generic, and can be extended
to any kind of link crossing. For this to work, a better notation would
be convenient; perhaps `uJs-` instead of `Jj-` and `vJs-` instead of
`Jk-`, or something like that ... (TODO: invent better notation.)
(NB: This is a kind of re-invention of "fat links", but in the
dictionary, not in the code.)

Landmark Transitivity: Theory
-----------------------------
Given that non-planar parses can be enabled without any changes to the
parser algorithm, all that is required is to understand what sort of
theory describes link-crossing in a coherent grounding. That theory is
Dick Hudson's Landmark Transitivity, explained here.

This mechanism works as follows:

 * First, every link must be directional, with a head and a dependent.
That is, we are concerned with directional-LG links, which are
of the form x--A-->y or y<--A--x for words x,y and LG link type A.

 * Given either the directional-LG relation x--A-->y or y<--A--x,
define the dependency relation x-->y.  That is, ignore the link-type
label.

 * Heads are landmarks for dependents. If the dependency relation
x-->y holds, then x is said to be a landmark for y, and the
predicate land(x,y) is true, while the predicate land(y,x) is false.
Here, x and y are words, while --> is the landmark relation.

 * Although the basic directional-LG links form landmark relations,
the total set of landmark relations is extended by transitive closure.
That is, if land(x,y) and land(y,z) then land(x,z).  That is, the
basic directional-LG links are "generators" of landmarks; they
generate by means of transitivity.  Note that the transitive closure
is unique.

 * In addition to the above landmark relation, there are two additional
relations: the before and after landmark relations. (In English,
these correspond to left and right; in Hebrew, the opposite).
That is, since words come in chronological order in a sentence,
the dependency relation can point either left or right.  The
previously-defined landmark relation only described the dependency
order; we now introduce the word-sequence order. Thus, there are
are land-before() and land-after() relations that capture both
the dependency relation, and the word-order relation.

 * Notation: the before-landmark relation land-B(x,y) corresponds to
x-->y (in English, reversed in right-left languages such as Hebrew),
whereas the after-landmark relation land-A(x,y) corresponds to y<--x.
That is, land(x,y) == land-B(x,y) or land-A(x,y) holds as a statement
about the predicate form of the relations.

 * As before, the full set of directional landmarks are obtained by
transitive closure applied to the directional-LG links.  Two
different rules are used to perform this closure:
```
-- land-B(x,y) and land(y,z) ==> land-B(x,y)
-- land-A(x,y) and land(y,z) ==> land-A(x,y)
```
Parsing is then performed by joining LG connectors in the usual manner,
to form a directional link. The transitive closure of the directional
landmarks are then computed. Finally, any parse that does not conclude
with the "left wall" being the upper-most landmark is discarded.

Here is an example where landmark transitivity provides a natural
solution to a (currently) broken parse. The "to.r" has a disjunct
"I+ & MVi-" which allows "What is there to do?" to parse correctly.
However, it also allows the incorrect parse "He is going to do".
The fix would be to force "do" to take an object; however, a link
from "do" to "what" is not allowed, because link-crossing would
prevent it.

Fixing this requires only a fix to the dictionary, and not to the
parser itself.

Link-crossing Examples
----------------------
Examples where the no-links-cross constraint seems to be violated,
in English:
```text
  "He is either in the 105th or the 106th battalion."
  "He is in either the 105th or the 106th battalion."
```
Both seem to be acceptable in English, but the ambiguity of the
"in-either" temporal ordering requires two different parse trees, if
the no-links-cross rule is to be enforced. This seems un-natural.
Similarly:
```text
  "He is either here or he is there."
  "He either is here or he is there."
```

A different example involves a crossing to the left wall.  That is, the
links *LEFT-WALL--remains* crosses over *here--found*:
```text
  "Here the remains can be found."
```

Other examples, per And Rosta:

The *allowed--by* link crosses *cake--that*:
```text
He had been allowed to eat a cake by Sophy that she had made him specially
```

*a--book*, *very--indeed*
```text
"a very much easier book indeed"
```

*an--book*, *easy--to*
```text
"an easy book to read"
```

*a--book*, *more--than*
```text
"a more difficult book than that one"
```

*that--have* crosses *remains--of*
```text
"It was announced that remains have been found of the ark of the covenant"
```

There is a natural crossing, driven by conjunctions:
```text
"I was in hell yesterday and heaven on Tuesday."
```

the "natural" linkage is to use MV links to connect "yesterday" and "on
Tuesday" to the verb. However, if this is done, then these must cross
the links from the conjunction "and" to "heaven" and "hell".  This can
be worked around partly as follows:
```text
              +-------->Ju--------->+
              |    +<------SJlp<----+
+<-SX<-+->Pp->+    +-->Mpn->+       +->SJru->+->Mp->+->Js->+
|      |      |    |        |       |        |      |      |
I     was    in  hell   yesterday  and    heaven    on  Tuesday
```
but the desired MV links from the verb to the time-prepositions
"yesterday" and "on Tuesday" are missing -- whereas they are present,
when the individual sentences "I was in hell yesterday" and
"I was in heaven on Tuesday" are parsed.  Using a conjunction should
not wreck the relations that get used; but this requires link-crossing.

```text
"Sophy wondered up to whose favorite number she should count"
```
Here, "up_to" must modify "number", and not "whose". There's no way to
do this without link-crossing.


Type Theory
-----------
Link Grammar can be understood in the context of type theory.
A simple introduction to type theory can be found in chapter 1
of the [HoTT book](https://homotopytypetheory.org/book/).
This book is freely available online and strongly recommended if
you are interested in types.

Link types can be mapped to types that appear in categorial grammars.
The nice thing about link-grammar is that the link types form a type
system that is much easier to use and comprehend than that of categorial
grammar, and yet can be directly converted to that system!  That is,
link-grammar is completely compatible with categorial grammar, and is
easier-to-use. See the paper
[<i>"Combinatory Categorial Grammar and Link Grammar are Equivalent"</i>](https://github.com/opencog/atomspace/raw/master/opencog/sheaf/docs/ccg.pdf)
for details.

The foundational LG papers make comments to this effect; however, see
also work by Bob Coecke on category theory and grammar.  Coecke's
diagramatic approach is essentially identical to the diagrams given in
the foundational LG papers; it becomes abundantly clear that the
category theoretic approach is equivalent to Link Grammar. See, for
example, this introductory sketch
http://www.cs.ox.ac.uk/people/bob.coecke/NewScientist.pdf
and observe how the diagrams are essentially identical to the LG
jigsaw-puzzle piece diagrams of the foundational LG publications.


ADDRESSES
---------
If you have any questions, please feel free to send a note to the
[mailing list](http://groups.google.com/group/link-grammar).

The source code of link-parser and the link-grammar library is located at
[GitHub](https://github.com/opencog/link-grammar).<br>
For bug reports, please open an **issue** there.

Although all messages should go to the mailing list, the current
maintainers can be contacted at:
```text
  Linas Vepstas - <linasvepstas@gmail.com>
  Amir Plivatsky - <amirpli@gmail.com>
  Dom Lachowicz - <domlachowicz@gmail.com>
```
A complete list of authors and copyright holders can be found in the
AUTHORS file.  The original authors of the Link Grammar parser are:
```text
  Daniel Sleator                    sleator@cs.cmu.edu
  Computer Science Department       412-268-7563
  Carnegie Mellon University        www.cs.cmu.edu/~sleator
  Pittsburgh, PA 15213

  Davy Temperley                    dtemp@theory.esm.rochester.edu
  Eastman School of Music           716-274-1557
  26 Gibbs St.                      www.link.cs.cmu.edu/temperley
  Rochester, NY 14604

  John Lafferty                     lafferty@cs.cmu.edu
  Computer Science Department       412-268-6791
  Carnegie Mellon University        www.cs.cmu.edu/~lafferty
  Pittsburgh, PA 15213
```


TODO -- Working Notes
---------------------
Some working notes.

Easy to fix: provide a more uniform API to the constituent tree.
i.e provide word index.   Also, provide a better word API,
showing word extent, subscript, etc.

### Capitalized first words:
There are subtle technical issues for handling capitalized first
words. This needs to be fixed. In addition, for now these words are
shown uncapitalized in the result linkages. This can be fixed.

Maybe capitalization could be handled in the same way that a/an
could be handled!  After all, it's essentially a nearest-neighbor
phenomenon!

See also [issue 690](https://github.com/opencog/link-grammar/issues/690)

#### Capitalization-mark tokens:
The proximal issue is to add a cost, so that Bill gets a lower
cost than bill.n when parsing "Bill went on a walk".  The best
solution would be to add a 'capitalization-mark token' during
tokenization; this token precedes capitalized words. The
dictionary then explicitly links to this token, with rules similar
to the a/an phonetic distinction.  The point here is that this
moves capitalization out of ad-hoc C code and into the dictionary,
where it can be handled like any other language feature.
The tokenizer includes experimental code for that.

### Corpus-statistics-based parse ranking:
The old for parse ranking via corpus statistics needs to be revived.
The issue can be illustrated with these example sentences:
```text
"Please the customer, bring in the money"
"Please, turn off the lights"
```
In the first sentence, the comma acts as a conjunction of two
directives (imperatives). In the second sentence, it is much too
easy to mistake "please" for a verb, the comma for a conjunction,
and come to the conclusion that one should please some unstated
object, and then turn off the lights. (Perhaps one is pleasing
by turning off the lights?)

### Bad grammar:
When a sentence fails to parse, look for:
 * confused words: its/it's, there/their/they're, to/too, your/you're ...
   These could be added at high cost to the dicts.
 * missing apostrophes in possessives: "the peoples desires"
 * determiner agreement errors: "a books"
 * aux verb agreement errors: "to be hooks up"

Poor agreement might be handled by giving a cost to mismatched
lower-case connector letters.

### Elision/ellipsis/zero/phantom words:
An common phenomenon in English is that some words that one might
expect to "properly" be present can disappear under various conditions.
Below is a sampling of these. Some possible solutions are given below.

Expressions such as "Looks good" have an implicit "it" (also called
a zero-it or phantom-it) in them; that is, the sentence should really
parse as "(it) looks good".  The dictionary could be simplified by
admitting such phantom words explicitly, rather than modifying the
grammar rules to allow such constructions.  Other examples, with the
phantom word in parenthesis, include:
 * I ate all (of) the cookies.
 * I've known him only (for) a week.
 * I taught him (how) to swim.
 * I told him (that) it was gone.
 * It stopped me (from) flying off the cliff.
 * (It) looks good.
 * (You) go home!
 * (You) do tell (me).
 * (That is) enough!
 * (I) heard that he's giving a test.
 * (Are) you all right?
 * He opened the door and (he) went in.
 * Emma was the younger (daughter) of two daughters.

This can extend to elided/unvoiced syllables:
 * (I'm a)'fraid so.

Elided punctuation:
 * God (,) give me strength.

Normally, the subjects of imperatives must always be offset by a comma:
"John, give me the hammer", but here, in muttering an oath, the comma
is swallowed (unvoiced).

Some complex phantom constructions:
 * They play billiards but (they do) not (play) snooker.
 * I know Ringo, but (I do) not (know) his brother.
 * She likes Indian food, but (she does) not (like) Chinese (food).
 * If this is true, then (you should) do it.
 * Perhaps he will (do it), if he sees enough of her.

See also [github issue #224](https://github.com/opencog/link-grammar/issues/224).

Actual ellipsis:
 * At first, it seemed like ...
 * It became clear that ...

Here, the ellipsis stands for a subordinate clause, which attaches
with not one, but two links: `C+ & CV+`, and thus requires two words,
not one. There is no way to have the ellipsis word to sink two
connectors starting from the same word, and so some more complex
mechanism is needed. The solution is to infer a second phantom ellipsis:

 * It became clear that ... (...)

where the first ellipsis is a stand in for the subject of a subordinate
clause, and the second stands in for an unknown verb.

#### Elision of syllables
Many (unstressed) syllables can be elided; in modern English, this occurs
most commonly in the initial unstressed syllable:
* (a)'ccount (a)'fraid (a)'gainst (a)'greed (a)'midst (a)'mongst
* (a)'noint (a)'nother (a)'rrest (at)'tend
* (be)'fore (be)'gin (be)'havior (be)'long (be)'twixt
* (con)'cern (e)'scape (e)'stablish
And so on.

#### Punctuation, zero-copula, zero-that:
Poorly punctuated sentences cause problems:  for example:
```text
"Mike was not first, nor was he last."
"Mike was not first nor was he last."
```
The one without the comma currently fails to parse.  How can we
deal with this in a simple, fast, elegant way?  Similar questions
for zero-copula and zero-that sentences.

#### Context-dependent zero phrases.
Consider an argument between a professor and a dean, and the dean
wants the professor to write a brilliant review. At the end of the
argument, the dean exclaims: "I want the review brilliant!"  This
is a predicative adjective; clearly it means "I want the review
[that you write to be] brilliant."  However, taken out of context,
such a construction is ungrammatical, as the predictiveness is not
at all apparent, and it reads just as incorrectly as would
"*Hey Joe, can you hand me that review brilliant?"

#### Imperatives as phantoms:
```text
"Push button"
"Push button firmly"
```
The subject is a phantom; the subject is "you".

#### Handling zero/phantom words by explicitly inserting them:
One possible solution is to perform a one-point compactification.
The dictionary contains the phantom words, and their connectors.
Ordinary disjuncts can link to these, but should do so using
a special initial lower-case letter (say, 'z', in addition to
'h' and 'd' as is currently implemented).  The parser, as it
works, examines the initial letter of each connector: if it is
'z', then the usual pruning rules no longer apply, and one or
more phantom words are selected out of the bucket of phantom words.
(This bucket is kept out-of-line, it is not yet placed into
sentence word sequence order, which is why the usual pruning rules
get modified.)  Otherwise, parsing continues as normal. At the end
of parsing, if there are any phantom words that are linked, then
all of the connectors on the disjunct must be satisfied (of course!)
else the linkage is invalid. After parsing, the phantom words can
be inserted into the sentence, with the location deduced from link
lengths.

#### Handling zero/phantom words as re-write rules.
A more principled approach to fixing the phantom-word issue is to
borrow the idea of re-writing from the theory of
[operator grammar](https://en.wikipedia.org/wiki/Operator_grammar).
That is, certain phrases and constructions can be (should be)
re-written into their "proper form", prior to parsing. The re-writing
step would insert the missing words, then the parsing proceeds. One
appeal of such an approach is that re-writing can also handle other
"annoying" phenomena, such as typos (missing apostrophes, e.g. "lets"
vs. "let's", "its" vs. "it's") as well as multi-word rewrites (e.g.
"let's" vs. "let us", or "it's" vs. "it is").

Exactly how to implement this is unclear.  However, it seems to open
the door to more abstract, semantic analysis. Thus, for example, in
Meaning-Text Theory (MTT), one must move between SSynt to DSynt
structures.  Such changes require a graph re-write from the surface
syntax parse (e.g. provided by link-grammar) to the deep-syntactic
structure.  By contrast, handling phantom words by graph re-writing
prior to parsing inverts the order of processing. This suggests that
a more holistic approach is needed to graph rewriting: it must somehow
be performed "during" parsing, so that parsing can both guide the
insertion of the phantom words, and, simultaneously guide the deep
syntactic rewrites.

Another interesting possibility arises with regards to tokenization.
The current tokenizer is clever, in that it splits not only on
whitespace, but can also strip off prefixes, suffixes, and perform
certain limited kinds of morphological splitting. That is, it currently
has the ability to re-write single-words into sequences of words. It
currently does so in a conservative manner; the letters that compose
a word are preserved, with a few exceptions, such as making spelling
correction suggestions. The above considerations suggest that the
boundary between tokenization and parsing needs to become both more
fluid, and more tightly coupled.

### Poor linkage choices:
Compare "she will be happier than before" to "she will be more happy
than before." Current parser makes "happy" the head word, and "more"
a modifier w/EA link.  I believe the correct solution would be to
make "more" the head (link it as a comparative), and make "happy"
the dependent.  This would harmonize rules for comparatives... and
would eliminate/simplify rules for less,more.

However, this idea needs to be double-checked against, e.g. Hudson's
word grammar.  I'm confused on this issue ...

### Stretchy links:
Currently, some links can act at "unlimited" length, while others
can only be finite-length.  e.g. determiners should be near the
noun that they apply to.  A better solution might be to employ
a 'stretchiness' cost to some connectors: the longer they are, the
higher the cost. (This eliminates the "unlimited_connector_set"
in the dictionary).

### Opposing (repulsing) parses:
Sometimes, the existence of one parse should suggest
that another parse must surely be wrong: if one parse is possible,
then the other parses must surely be unlikely. For example: the
conjunction and.j-g allows the "The Great Southern and Western
Railroad" to be parsed as the single name of an entity. However,
it also provides a pattern match for "John and Mike" as a single
entity, which is almost certainly wrong. But "John and Mike" has
an alternative parse, as a conventional-and -- a list of two people,
and so the existence of this alternative (and correct) parse suggests
that perhaps the entity-and is really very much the wrong parse.
That is, the mere possibility of certain parses should strongly
disfavor other possible parses. (Exception: Ben & Jerry's ice
cream; however, in this case, we could recognize Ben & Jerry as the
name of a proper brand; but this is outside of the "normal"
dictionary (?) (but maybe should be in the dictionary!))

More examples: "high water" can have the connector A joining high.a
and AN joining high.n; these two should either be collapsed into
one, or one should be eliminated.


### WordNet hinting:
Use WordNet to reduce the number for parses for sentences containing
compound verb phrases, such as "give up", "give off", etc.

### Sliding-window (Incremental) parsing:
To avoid a combinatorial explosion of parses, it would be nice to
have an incremental parsing, phrase by phrase, using a sliding window
algorithm to obtain the parse. Thus, for example, the parse of the
last half of a long, run-on sentence should not be sensitive to the
parse of the beginning of the sentence.

Doing so would help with combinatorial explosion. So, for example,
if the first half of a sentence has 4 plausible parses, and the
last half has 4 more, then currently, the parser reports 16 parses
total.  It would be much more useful if it could instead report the
factored results: i.e. the four plausible parses for the first half,
and the four plausible parses for the last half.  This would ease
the burden on downstream users of link-grammar.

This approach has at psychological support. Humans take long sentences
and split them into smaller chunks that "hang together" as phrase-
structures, viz compounded sentences. The most likely parse is the
one where each of the quasi sub-sentences is parsed correctly.

This could be implemented by saving dangling right-going connectors
into a parse context, and then, when another sentence fragment
arrives, use that context in place of the left-wall.

This somewhat resembles the application of construction grammar
ideas to the link-grammar dictionary. It also somewhat resembles
Viterbi parsing to some fixed depth. Viz. do a full backward-forward
parse for a phrase, and then, once this is done, take a Viterbi-step.
That is, once the phrase is done, keep only the dangling connectors
to the phrase, place a wall, and then step to the next part of the
sentence.

Caution: watch out for garden-path sentences:
``` text
  The horse raced past the barn fell.
  The old man the boat.
  The cotton clothing is made of grows in Mississippi.
```
The current parser parses these perfectly; a viterbi parser could trip on these.

Other benefits of a Viterbi decoder:
* Less sensitive to sentence boundaries: this would allow longer,
  run-on sentences to be parsed far more quickly.
* Could do better with slang, hip-speak.
* Support for real-time dialog (parsing of half-uttered sentences).
* Parsing of multiple streams, e.g. from play/movie scripts.
* Would enable (or simplify) co-reference resolution across sentences
  (resolve referents of pronouns, etc.)
* Would allow richer state to be passed up to higher layers:
  specifically, alternate parses for fractions of a sentence,
  alternate reference resolutions.
* Would allow plug-in architecture, so that plugins, employing
  some alternate, higher-level logic, could disambiguate (e.g.
  by making use of semantic content).
* Eliminate many of the hard-coded array sizes in the code.

One may argue that Viterbi is a more natural, biological way of
working with sequences.  Some experimental, psychological support
for this can be found at
http://www.sciencedaily.com/releases/2012/09/120925143555.htm
per Morten Christiansen, Cornell professor of psychology.


### Registers, sociolects, dialects (cost vectors):
Consider the sentence "Thieves rob bank" -- a typical newspaper
headline. LG currently fails to parse this, because the determiner
is missing ("bank" is a count noun, not a mass noun, and thus
requires a determiner. By contrast, "thieves rob water" parses
just fine.) A fix for this would be to replace mandatory
determiner links by (D- or {[[()]] & headline-flag}) which allows
the D link to be omitted if the headline-flag bit is set.
Here, "headline-flag" could be a new link-type, but one that is
not subject to planarity constraints.

Note that this is easier said than done: if one simply adds a
high-cost null link, and no headline-flag, then all sorts of
ungrammatical sentences parse, with strange parses; while some
grammatical sentences, which should parse, but currently don't,
become parsable, but with crazy results.

More examples, from And Rosta:
```text
   "when boy meets girl"
   "when bat strikes ball"
   "both mother and baby are well"
```

A natural approach would be to replace fixed costs by formulas.
This would allow the dialect/sociolect to be dynamically
changeable.  That is, rather than having a binary headline-flag,
there would be a formula for the cost, which could be changed
outside of the parsing loop.  Such formulas could be used to
enable/disable parsing specific to different dialects/sociolects,
simply by altering the network of link costs.

A simpler alternative would be to have labeled costs (a cost vector),
so that different dialects assign different costs to various links.
A dialect would be specified during the parse, thus causing the costs
for that dialect to be employed during parse ranking.

This has been implemented; what's missing is a practical tutorial on
how this might be used.

### Hand-refining verb patterns:
A good reference for refining verb usage patterns is:
"COBUILD GRAMMAR PATTERNS 1: VERBS from THE COBUILD SERIES",
from THE BANK OF ENGLISH, HARPER COLLINS.  Online at
https://arts-ccr-002.bham.ac.uk/ccr/patgram/ and
http://www.corpus.bham.ac.uk/publications/index.shtml


### Quotations:
   Currently tokenize.c tokenizes double-quotes and some UTF8 quotes
   (see the RPUNC/LPUNC class in en/4.0.affix - the QUOTES class is
   not used for that, but for capitalization support), with some very
   basic support in the English dictionary (see "% Quotation marks."
   there).  However, it does not do this for the various "curly" UTF8
   quotes, such as ‘these’ and “these”.  This results is some ugly
   parsing for sentences containing such quotes. (Note that these are
   in 4.0.affix).

   A mechanism is needed to disentangle the quoting from the quoted
   text, so that each can be parsed appropriately.  It's somewhat
   unclear how to handle this within link-grammar. This is somewhat
   related to the problem of morphology (parsing words as if they
   were "mini-sentences",) idioms (phrases that are treated as if
   they were single words), set-phrase structures (if ... then ... not
   only... but also ...) which have a long-range structure similar to
   quoted text (he said ...).

   See also [github issue #42](https://github.com/opencog/link-grammar/issues/42).

### Semantification of the dictionary:
  "to be fishing": Link grammar offers four parses of "I was fishing for
  evidence", two of which are given low scores, and two are given
  high scores. Of the two with high scores, one parse is clearly bad.
  Its links "to be fishing.noun" as opposed to the correct
  "to be fishing.gerund". That is, I can be happy, healthy and wise,
  but I certainly cannot be fishing.noun.  This is perhaps not
  just a bug in the structure of the dictionary, but is perhaps
  deeper: link-grammar has little or no concept of lexical units
  (i.e. collocations, idioms, institutional phrases), which thus
  allows parses with bad word-senses to sneak in.

  The goal is to introduce more knowledge of lexical units into LG.

  Different word senses can have different grammar rules (and thus,
  the links employed reveal the sense of the word): for example:
  "I tend to agree" vs. "I tend to the sheep" -- these employ two
  different meanings for the verb "tend", and the grammatical
  constructions allowed for one meaning are not the same as those
  allowed for the other. Yet, the link rules for "tend.v" have
  to accommodate both senses, thus making the rules rather complex.
  Worse, it potentially allows for non-sense constructions.
  If, instead, we allowed the dictionary to contain different
  rules for "tend.meaning1" and "tend.meaning2", the rules would
  simplify (at the cost of inflating the size of the dictionary).

  Another example: "I fear so" -- the word "so" is only allowed
  with some, but not all, lexical senses of "fear". So e.g.
  "I fear so" is in the same semantic class as "I think so" or
  "I hope so", although other meanings of these verbs are
  otherwise quite different.

  [Sin2004] "New evidence, new priorities, new attitudes" in J.
  Sinclair, (ed) (2004) How to use corpora in language teaching,
  Amsterdam: John Benjamins

  See also: Pattern Grammar: A Corpus-Driven Approach to the Lexical
  Grammar of English<br>
  Susan Hunston and Gill Francis (University of Birmingham)<br>
  Amsterdam: John Benjamins (Studies in corpus linguistics,
  edited by Elena Tognini-Bonelli, volume 4), 2000<br>
  [Book review](http://www.aclweb.org/anthology/J01-2013).

  “The Molecular Level of Lexical Semantics”, EA Nida, (1997)
  International Journal of Lexicography, 10(4): 265–274.
  [Online](https://www.academia.edu/36534355/The_Molecular_Level_of_Lexical_Semantics_by_EA_Nida)

### "holes" in collocations (aka "set phrases" of "phrasemes"):
  The link-grammar provides several mechanisms to support
  circumpositions or even more complicated multi-word structures.
  One mechanism is by ordinary links; see the V, XJ and RJ links.
  The other mechanism is by means of post-processing rules.
  (For example, the "filler-it" SF rules use post-processing.)
  However, rules for many common forms have not yet been written.
  The general problem is of supporting structures that have "holes"
  in the middle, that require "lacing" to tie them together.

  For a general theory, see
  [catena](http://en.wikipedia.org/wiki/Catena_(linguistics)).

  For example, the adposition:
```text
... from [xxx] on.
    "He never said another word from then on."
    "I promise to be quiet from now on."
    "Keep going straight from that point on."
    "We went straight from here on."

... from there on.
    "We went straight, from the house on to the woods."
    "We drove straight, from the hill onwards."
```
Note that multiple words can fit in the slot [xxx].
Note the tangling of another prepositional phrase:
`"... from [xxx] on to [yyy]"`

More complicated collocations with holes include
```text
 "First.. next..."
 "If ... then ..."
```
'Then' is optional ('then' is a 'null word'), for example:
```text
"If it is raining, stay inside!"
"If it is raining, [then] stay inside!"


"if ... only ..." "If there were only more like you!"
"... not only, ... but also ..."


"As ..., so ..."  "As it was commanded, so it shall be done"


"Either ... or ..."
"Both ... and  ..."  "Both June and Tom are coming"
"ought ... if ..." "That ought to be the case, if John is not lying"


"Someone ... who ..."
"Someone is outside who wants to see you"


"... for ... to ..."
"I need for you to come to my party"
```
The above are not currently supported. An example that is supported
is the "non-referential it", e.g.
```
"It ... that ..."
"It seemed likely that John would go"
```
The above is supported by means of special disjuncts for 'it' and
'that', which must occur in the same post-processing domain.

See also:<br>
http://www.phon.ucl.ac.uk/home/dick/enc2010/articles/extraposition.htm<br>
http://www.phon.ucl.ac.uk/home/dick/enc2010/articles/relative-clause.htm

 "...from X and from Y"
 "By X, and by Y, ..."
 Here, X and Y might be rather long phrases, containing other
 prepositions. In this case, the usual link-grammar linkage rules
 will typically conjoin "and from Y" to some preposition in X,
 instead of the correct link to "from X". Although adding a cost to
 keep the lengths of X and Y approximately equal can help, it would
 be even better to recognize the "...from ... and from..." pattern.

 The correct solution for the "Either ... or ..." appears to be this:
```text
---------------------------+---SJrs--+
       +------???----------+         |
       |     +Ds**c+--SJls-+    +Ds**+
       |     |     |       |    |    |
   either.r the lorry.n or.j-n the van.n
```
 The wrong solution is
```text
--------------------------+
     +-----Dn-----+       +---SJrs---+
     |      +Ds**c+--SJn--+     +Ds**+
     |      |     |       |     |    |
 neither.j the lorry.n nor.j-n the van.n
```
 The problem with this is that "neither" must coordinate with "nor".
 That is, one cannot say "either.. nor..." "neither ... or ... "
 "neither ...and..." "but ... nor ..."  The way I originally solved
 the coordination problem was to invent a new link called Dn, and a
 link SJn and to make sure that Dn could only connect to SJn, and
 nothing else. Thus, the lower-case "n" was used to propagate the
 coordination across two links. This demonstrates how powerful the
 link-grammar theory is: with proper subscripts, constraints can be
 propagated along links over large distances. However, this also
 makes the dictionary more complex, and the rules harder to write:
 coordination requires a lot of different links to be hooked together.
 And so I think that creating a single, new link, called ???, will
 make the coordination easy and direct. That is why I like that idea.

 The ??? link should be the XJ link, which-see.


 More idiomatic than the above examples:
 "...the chip on X's shoulder"
 "to do X a favour"
 "to give X a look"

 The above are all examples of "set phrases" or "phrasemes", and are
 most commonly discussed in the context of MTT or Meaning-Text Theory
 of Igor Mel'cuk et al (search for "MTT Lexical Function" for more
 info). Mel'cuk treats set phrases as lexemes, and, for parsing, this
 is not directly relevant. However, insofar as phrasemes have a high
 mutual information content, they can dominate the syntactic
 structure of a sentence.

### Preposition linking:
 The current parse of "he wanted to look at and listen to everything."
 is inadequate: the link to "everything" needs to connect to "and", so
 that "listen to" and "look at" are treated as atomic verb phrases.

### Lexical functions:
 MTT suggests that perhaps the correct way to understand the contents
 of the post-processing rules is as an implementation of 'lexical
 functions' projected onto syntax.  That is, the post-processing
 rules allow only certain syntactical constructions, and these are
 the kinds of constructions one typically sees in certain kinds
 of lexical functions.

 Alternately, link-grammar suffers from a combinatoric explosion
 of possible parses of a given sentence. It would seem that lexical
 functions could be used to rule out many of these parses.  On the
 other hand, the results are likely to be similar to that of
 statistical parse ranking (which presumably captures such
 quasi-idiomatic collocations at least weakly).

 Ref. I. Mel'cuk: "Collocations and Lexical Functions", in ''Phraseology:
 theory, analysis, and applications'' Ed. Anthony Paul Cowie (1998)
 Oxford University Press pp. 23-54.

 More generally, all of link-grammar could benefit from a MTT-izing
 of infrastructure.

### Morphology:
Compare the above commentary on lexical functions to Hebrew morphological
analysis. To quote Wikipedia:

   > This distinction between the word as a unit of speech and the
   > root as a unit of meaning is even more important in the case of
   > languages where roots have many different forms when used in
   > actual words, as is the case in Semitic languages. In these,
   > roots are formed by consonants alone, and different words
   > (belonging to different parts of speech) are derived from the
   > same root by inserting vowels. For example, in Hebrew, the root
   > gdl represents the idea of largeness, and from it we have gadol
   > and gdola (masculine and feminine forms of the adjective "big"),
   > gadal "he grew", higdil "he magnified" and magdelet "magnifier",
   > along with many other words such as godel "size" and migdal
   > "tower".

### Morphology printing:
   Instead of hard-coding LL, declare which links are morpho links
   in the dict.

### Assorted minor cleanup:
   * Should provide a query that returns compile-time consts,
      e.g. the max number of characters in a word, or max words
      in a sentence.
   * Should remove compile-time constants, e.g. max words, max
      length etc.

### Version 6.0 TODO list:
Version 6.0 will change `Sentence` to `Sentence*,` `Linkage` to
`Linkage*` in the API.  But perhaps this is a bad idea...
