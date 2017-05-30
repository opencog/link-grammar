
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

Existing demo:
--------------
Run the existing demo:
```
    $ link-parser demo-sql
```
This can parse the following sentences: "this is a test", "this is
another test", "this is a dog", "this is a cat".

Creating a demo dictionary:
---------------------------
Use the following commands, modified as desired:
```
mkdir data/foo
cp data/demo-sql/4.0.* data/foo
cat dict.sql |sqlite3 data/foo/dict.db
cat demo.sql |sqlite3 data/foo/dict.db
link-parser foo
```
The above should result in a file that can parse the same sentences
as the demo database.
