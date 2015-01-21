# Python bindings for Link Grammar

Description
===========
This directory contains a Python interface to the Link Grammar C library.


Testing
=======
The test collection `tests.py` should run 52 tests, which should all
pass. The tests access private parts of the python bindings, and thus
require that link-grammar be explicitly added to the python path. For
most users, this will be:

   `export PYTHONPATH=$PYTHONPATH:/usr/local/lib/python2.7/dist-packages/linkgrammar`

The python path does not need to be set for normal usage.


How to use
==========
Parsing simple sentences::

    >>> from linkgrammar import Parser
    >>> p = Parser()
    >>> linkages = p.parse_sent("This is a simple sentence.")
    >>> len(linkages)
    4
    >>> print linkages[0].diagram
    
        +-------------------Xp------------------+
        |              +--------Ost-------+     |
        +------WV------+  +-------Ds------+     |
        +---Wd---+-Ss*b+  |     +----A----+     |
        |        |     |  |     |         |     |
    LEFT-WALL this.p is.v a simple.a sentence.n . 

Additional examples can be found in `examples.py`.
