Python bindings for Link Grammar
================================
This directory contains an example program, and a unit test for the
python bindings to Link Grammar.

The example programs `example.py` and `sentence-check.py` illustrates
how the to use the Link Grammar Python bindings.

A unit test for the Link Grammar Python bindings can be found in
in `tests.py`.

Python2 is no longer supported; the last version that contained python2
bindings was version 5.7.0, released in September 2019.

Configuring (if needed)
-----------------------
The python bindings will be built by default, if the required python
system libraries are detected on the build system.  Thus, no special
configuration should be needed. However, `configure` can be forced with
the following commands.

### To explicitly enable Python bindings.
   `$ ./configure --enable-python-bindings`
(This is the default if Python development packages are installed.)

### To disable the Python bindings
   `$ ./configure --disable-python-bindings`
(This is the default if no Python is installed.)

How to use
----------
The python bindings will be installed automatically into default system
locations, and no additional steps should be needed to use python.
However, in some cases, therere might be a need to manually set the
`PYTHONPATH` environment variable.  See the discussion below, in
the section **Testing the installation** .

Parsing simple sentences:

```
`$ python3`

>>> from linkgrammar import Sentence, ParseOptions, Dictionary
>>> sent = Sentence("This is a simple sentence.", Dictionary(), ParseOptions())
>>> linkages = sent.parse()
>>> len(linkages)
>>> for linkage in linkages:
...    print linkage.diagram()
...
```
```
      +-------------------Xp------------------+
      |              +--------Osm-------+     |
      +----->WV----->+  +-----Ds**x-----+     |
      +---Wd---+-Ss*b+  |     +----A----+     |
      |        |     |  |     |         |     |
  LEFT-WALL this.p is.v a simple.a sentence.n .
```
Additional examples can be found in `examples.py` and `sentence-check.py`.

Testing
-------
The test collection `tests.py` should run 76 tests; none of them should
fail.  However, 3 tests will be skipped, if the library is not configured
with a spell guesser, and one test will be skipped if the library is not
configured with the SAT solver (this is currently the case for native
Windows builds).

The test procedure is outlined below.  For native Windows/MinGW, see
the `msvc/README.md` file:
[Running Python programs in Windows](/msvc/README.md#running-python-programs).

### Testing the build directory
The following is assumed:

**$SRC_DIR** - Link Grammar source directory.

**$BUILD_DIR** - Link Grammar build directory.

#### Using `make`
The tests can be run using the `make` command, as follows:
```
$ cd $BUILD_DIR/bindings/python-examples
$ make [-s] check
```
The `make` command can be made less verbose by using the `-s` flag.

The test results are saved in the current directory, in the file
`tests.log`.

To run the tests in the **$SRC_DIR/tests/** directory, issue `make check`
directly from **$BUILD_DIR**.

#### Manually
To run `tests.py` manually, or to run `example.py`, without installing
the bindings, the `PYTHONPATH` environment variable must be set:
```
PYTHONPATH=$SRC_DIR/bindings/python:$BUILD_DIR/bindings/python:$BUILD_DIR/bindings/python/.libs
```
(Export it, or prepend it it the `make` command.)
```
$ cd $SRC_DIR
$ ./tests.py [-v]
```

### Testing the installation
This can be done only after `make install`.

#### Using `make`
```
$ cd $BUILD_DIR/bindings/python-examples
$ make [-s] installcheck
```
To run the whole package installcheck, issue `make installcheck` from
$BUILD_DIR.

#### Manually
Set the `PYTHONPATH` environment variable to the location of the installed
Python's **linkgrammar** module, e.g.:

```
PYTHONPATH=/usr/local/lib/python3.7/site-packages
```
(Export it, or prepend it to the `python3` command.)

Setting the `PYTHONPATH` is not needed if the default package
configuration is used.  The default configuration installs the python
bindings into the standard operating system locations.

To correctly test the system installation, make sure that `tests.py` is
invoked from a directory from which the **$SRCDIR/data.** directory
cannot be found. This is needed to ensure that the system-installed data
directory is used. For example:

```
$ cd $SRCDIR/binding/python-examples
$ ./tests.py
```
