
SQLite-based dictionary
-----------------------

The SQLite-based dictionary storage is meant to provide a simple
machine-readable interface to the Link Grammar dictionaries.

Traditionally, the Link Grammar dictionaries are stored in text files
that are hand-edited. This is fine for manual maintenance of the
dictionary entries, but is a stumbling block when it is desired that
the dictionary be maintained in an automated fashion, e.g. by
language-learning tools.  The SQLite-based dictionary provides this
automation interface.

The current interface remains a hybrid: some of the data will remain
in text files, until the system is more fully developed.

Demo:
=====
Creating a dictionary:
----------------------
Copy the files in `data/demo-sql` to `data/foo`.

foo/dict.db

link-parser demo-sql
