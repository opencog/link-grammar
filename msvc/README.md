Building and Running on Windows
===============================
Note: See also [BUILDING on Windows](/README.md#building-on-windows)
in the main README.

This directory contains project files for building Link Grammar with the
Microsoft Visual Studio 2019 IDE (MSVC 16). They were created and tested with
the Community Edition of that product.

SQLite dictionaries are not currently supported. Patches are welcome!

**!!!WARNING!!!**<br>
In the examples below, "console-prompt>" at start of line means the console
prompt.  Don't type it because the result could be destructive!

Supported target versions
-------------------------
The system compatibility definitions:
In each project file - Target Platform version: 8.1
In the `MSVC-common` property sheet - **Common properties->User Macros/CFLAGS**:

`NTDDI_VERSION=NTDDI_VISTA;_WIN32_WINNT=_WIN32_WINNT_VISTA;`

However, the package is tested only on Windows 10.

Regular-expression (regex) support
----------------------------------
The default configuration uses the C++ regex library. An external regex package can be
used instead, but this option is only for special needs and you can safely ignore it.
More details are in [Using an external regex package](/msvc/LIBREGEX-README.md).

Setup
-----
The build files use directory locations as defined in the property sheet `Local`:

- The User Macro JAVA_HOME, if used, must be pointing to a locally installed SDK/JDK,
   which has the subdirectories "include" and `include/win32` (defined in
   the LinkGrammarJava project under **Common properties->C/C++->General->
   Additional Include Directories**.
   If your JAVA SDK/JDK installation has defined the JAVA_HOME environment
   variable (check it) then there is no need to define this User Macro.

- The User Macro WINFLEXBISON should be the directory of the "Win flex-bison" project,
  as downloaded from its [Web site](https://winflexbison.sourceforge.io/).
  Tested with version 2.5.9.
  Leave it blank if would like to use a ready **pp_lexer.c** file.
  The default is **C:\win_flex_bison**.

-  The User Macros GNUREGEX_DIR and LG_DLLPATH are related to the optional external
   regex library mentioned above. You can safely ignore them.

Python bindings
---------------
Install the desired Python3 distributions from
[Python Releases for Windows](https://www.python.org/downloads/windows/).
You also have to install [SWIG](http://www.swig.org/download.html).

The bindings were testes using swigwin-4.0.2 with Python 3.4.4.

### Related macros ion property sheet `Local`.

 Macro | Default value |
---|---|
PYTHON3         | C:\Python34 |
PYTHON3_INCLUDE | $(PYTHON3)\include |
PYTHON3_LIB     | $(PYTHON3)\lib |
PYTHON3_EXE     | $(PYTHON3)\python.exe |

Make sure the `python3` project is marked for build in the configuration
manager, or select "build" for it in Solution Explorer.

Compiling
---------
- Compiling and running got checked on Windows 10, but is intended to be
  compatible to Vista and on (XP is not supported).

- To compile LinkGrammar, open the solution file LinkGrammar.sln, change
  the solution platform to x64 if desired, and build.

- The solution configuration is configured to create a debug version by
  default.  It's probably a good idea to switch a "Release" configuration.
  You can do this at **Build Menu->Configuration Manager**.

- The wordgraph-display feature is enabled when compiled with
  USE_WORDGRAPH_DISPLAY (already defined in the `LGlib-features` property sheet
  in **Common properties->C/User Macros/DEFS**).

- By default, the library is configured to create a DLL. If you want
  to instead build a static library, the macro LINK_GRAMMAR_STATIC must
  be defined before the inclusion of any header files for both the compiling
  of the link-grammar library and for the application that uses it. Other
  compiler settings will also have to be changed to create a static library
  of course.

Running
-------
The last step of the "LinkGrammarExe" project creates `link-parser.bat`.  Copy
it to somewhere in your PATH and then customize it if needed (don't customize
it in-place, since rebuilding would overwrite it). Note that it prepends PATH
with the User Macro LG_DLLPATH (see Setup above).  If you would like to display
the wordgraph (see below) set also the PATH for `dot.exe` if it is not already
in your PATH.

If USE_WORDGRAPH_DISPLAY has been used when compiling (the default), then
typing `!test=w`g at the `linkparser>` prompt can be used in order to display
the wordgraph of the next sentences to be parsed. See
[Word-graph display](/linkgrammar/tokenize/README.md#word-graph-display)
in `/linkgrammar/tokenize/README.md`.
for how to use optional display flags.  By default, `PhotoViewer.dll` is
invoked to display the graph.  If X11 is available and your `dot.exe` command
has the "xlib" driver, it can be used to display the wordgraph when the x flag
is set (i.e. `!test=wg:x`), for example when running under Cygwin/X. In any
case it uses the `dot` command of Graphviz. Graphviz can be installed as part
of Cygwin (in which case it included the "xlib" driver"), or separately from
[Graphviz](http://www.graphviz.org/Download_windows.php).
Both `PhotoViewer.dll` (if used) and `dot` must be in your PATH (if needed,
you can customized that in a copy of `link-parser.bat`, as described above).

BTW, when running an MSVC-compiled binary under Cygwin, don't exit
link-parser by using `^Z` - the shell may get stuck because the program
somehow may continue to run in the background.  Instead, exit using `!exit` .

NOTE: The created DLLs need the MSVC 16 runtime environment to run. This is
normally already installed in your machine with the installation of the IDE.
But to be able to run Link Grammar on other computer you need to install
[Visual C++ Redistributable for Visual Studio 2019](https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads).
This redistributable does not contain debug version of the MSVC runtime, so
only "Release" Link Grammar will work with it.

Running Python programs
-----------------------
Since the Link Grammar library has no installation script yet,
running Python programs that use the bindings needs a careful setup
of PYTHONPATH and PATH. A program named `make-check.py` (named after
"make check" that runs `tests.py` in POSIX systems) is provided to
set them automatically.

Also see "Permanent installation".

### Using make-check.py
The `make-check.py` program is designed to reside in the MSVC
configuration directory.  However, it can run from any directory
using a full or a relative path to invoke it.

Usage:
```
console-prompt>make-check [PYTHON_FLAG] PYTHON_OUTDIR [script.py] [ARGUMENTS]
```

- PYTHON_FLAG: Optional flag for the Python program, e.g. `-vv` to debug
imports.
- PYTHON_OUTDIR: The directory to which the Python bindings got written.<br>
For example, `x64\Release\Python3`.
- script.py: Path leading to the script. If only a filename is specified
(i.e. no `\` in the path) the specified script file is taken from the
`bindings\python-examples\` directory.  In order to run tests.py in
its default location you can leave it empty.
- ARGUMENTS: Optional script arguments, for example `-v` for `tests.py`.

So in order to run `tests.py` with Python3 for a Debug compilation on x64
platform, enter:
```
console-prompt>make-check x64\Debug\Python3
```
To debug a Python3 script "mylgtest.py" that resides in
`bindings\python-examples`:
```
console-prompt>make-check -mpdb x64\Debug\Python3 mylgtest.py
```
To run a script in your home directory:
```
console-prompt>make-check x64\Debug\Python3 \Users\username\mylgtest.py
```
The following starts an interactive Python with the correct PYTHONPATH
and PATH:
```
console-prompt>make-check.py x64\Debug\Python3 ""
```
Locale and code page settings
-----------------------------
In this version, the language dictionaries under the data directory define
the locale that they need in order to process input sentences and the
library automatically uses this locale, in a manner that is intended to be
compatible with creating several different dictionaries per thread.

If you use a dictionary which doesn't have this definition, or would like
to set the default language when link-parser is invoked without a language
argument, you can set the locale using the environment variable LANG,
using a 2-letter language code and a 2-letter country code:
```
console-prompt>set LANG=ll-CC
```
For example:
```
console-prompt>set LANG=en-US
```

If you open a dictionary which doesn't have a locale definition, from a
program (C program or a language with LG bindings) you have to set the
correct locale (for the whole program or the particular thread)for the
dictionary before creating the dictionary.  Alternatively, the LANG
environment variable can be set before creating the dictionary.

The code page of the console should not be changed, unless it is desired
to pipe the output to another program. In that case, this other program
may read garbage due to a limitation in the way cmd.exe implements pipes.
This can be solved as following ("more" is the example program)
```
link-parser | (chcp 65001 & more)
```
In that case `more` will be able to read UTF-8 input. However, the
display may still not be perfect due to additional cmd.exe limitations.
The code page can be changed manually to 65001, see below in the "Note for
Python bindings".

### A note for the Python bindings
In order to produce UTF-8 output to the console, it needs use the CP_UTF8
code page (65001):

```
console-prompt>chcp
Active code page NNNN
console-prompt>chcp 65001
Active code page: 65001
```

Other programs may malfunction with this code page, so it can be restored
when needed (or this console window can be used only for working with
link-grammar). The link-parser command also changes the code page to
CP_UTF8, but it restores the original one on exit.

Console fonts
-------------
Courier New may be appropriate for all the languages in the data
directory. If you don't have it in the console menu, it can be added
through the registry (Google it).

Permanent installation
----------------------
For using the library independently of the build directory:

1) If Python bindings were generated, copy the following modules to a
   directory `linkgrammar` in a fixed location: `linkgrammar.py`,
   `clinkgrammar.py`, `__init__.py`, `_clinkgrammar.pyd`.

   Set the PYTHONPATH environment variable to point to the said
   "linkgrammar" directory.

2) Copy the link-grammar DLL to a fixed location.<br>
   Add the DLL location permanently to PATH.

3) Copy the `data` directory to the location of the DLL so it will get found.


Implementation notes
--------------------

- The file `link-grammar/link-features.h.in` has a Custom Build Tool definition
  which invokes `mk-link-features-h` to generate
  `link-grammar/link-features.h`, with LINK_*_VERSION variable replacement as
  defined in `configure.ac`.

- The project file `LinkGrammarExe` has a Post-Build-Event definition for
  generating `link-parser.bat`.

  The file `regex-morph.c` has a compiler setting of C++ in its property sheet:
  **View->Solution Explorer->Source Files->regex-morph.c->Right Click->
      Properties->C/C++->CompileAS->CompileAsCpp**

Using a remote network share
----------------------------
In order to use a link-grammar source repository from a network share (e.g. via
Samba), and still be able to use the custom build steps in the Project files,
there is a need to "convince" Windows it is a local filesystem.  Else you will
get "UNC path are not supported." on the batch runs, with bad results. This
method will also allow the `link-parser.bat` file to run.  (For other solutions
see https://stackoverflow.com/questions/9013941). You will need to find out by
yourself if this makes a security or another problem in your case.

Here is what worked for me:<br>
Suppose you use `host:/usr/local/src` remotely as share `src`:
```
mklink /J src-j \\host\src
mklink /D src src-link
```
The second one needs administrator privileges.<br>
Then use the repository through `src`.

Debugging hints
---------------
If, when starting the program under the debugger (by "Local Windows Debugger",
"Debug->Start Debugging" (F5), etc.), `regex.dll` is not found, it can be
added to the search `PATH` as follows:<br>
Enter to LinkGrammarExe's Property Pages:<br>
 `Solution Explorer->LinkGrammarExe->Properties`<br>
Click on the writable location of:<br>
 `Debugging->Environment`<br>
Put there (LG_DLLPATH is defined in the `Local` Property page):<br>
`PATH=$(LG_DLLPATH)`<br>
Make sure "Merge Environment" there is `Yes`.

(The result is kept in `.user` Property Pages that are not part of the
LG repository).
