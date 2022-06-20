RAM-based dictionary
====================

This directory implements methods to work with dictionary entries
residing in RAM.  This is meant to be in contrast to dictionary
entries that might live on remote servers (e.g. `dict-atomese`)
or in SQL databases (e.g. `dict-sql`).

The in-RAM dictionary entries are used by the file-based dictionary
(`dict-file`), as the entire contents of the files are read into RAM
upon startup, and after that, there are no more file touches.

The in-RAM dictionary entries are also used as a cache for the remote
server (`dict-atomese`) dictionary. Thus, a lookup to the remote server
is done only if the word can't be found in the local cache. Once found,
it is added to the local cache.
