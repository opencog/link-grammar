Building on Windows using  Cygwin
=================================
Tested on Cygwin 2.10.0 under Windows 10,
with link-grammar version RC 5.5.2.

Language bindings
-----------------
The build system produces successful Python2&3 and Java bindings.
Other language bindings have not been tested.

For the Java bindings, install Java and Ant under Windows.
Make sure, under Windows, that `JAVA_HOME` is set correctly
and `ant` is the `PATH`.


Building
--------
Install all the prerequisite tools,
Also install the desired optional libraries

Note: Installing the library `pcre2` is **required** for correct operation
since the default REGEX implementation (in libc) is not capable enough.

Use the `configure` command as outlined in the main [README](/README.md#creating-the-system).
Note that compiling with more than one concurrent process may be extremely slow.
To make sure only a single process is used supply `make ` with `-j 1`.
Also, the `make` built-in rules cause a lot of I/O and are not needed here, so
supplying `make` with the `-r` argument makes it much faster.

`$ make -r -j 1`

`$ make -r install`

The dictionaries are installed by default under
`/usr/local/share/link-grammar`.



Running link-parser
-------------------

* From the sources directory, using cmd.exe Windows console:

Note: ^Z/^D/^C are not supported when running under Cygwin!
In particular, don't press ^Z - it may crash or stuck the window.
To exit, use the `!exit` command.

      > PATH-TO-LG-CONF-DIRECTORY\link-parser\link-parser [arguments]

* Form the Cygwin shell:

Before installation (supposing no installed version yet):

      `$ PATH-TO-LG-CONF-DIRECTORY/link-parser/link-parser [args]`

After "make install" (supposing `/usr/local/bin` is in the `PATH`):

      `$ link-parser [arguments]`

To run the executable from other location, liblink-grammar-5.dll needs to be
in a directory in PATH (e.g. in the directory of the executable).

A helper BAT file link-parser-cygwin.bat is provided (currently under the
`mingw` directory) for invoking `link-parser`.

For more details, see "RUNNING the program" in the main
[README](/README.md#running-the-program).

Tests
-----
All the tests pass (the Python and C++ tests in the `tests` directory).
However, the C++ tests that involve multi-threading severely trash the
system and take very much time to complete.
