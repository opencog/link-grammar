Demo Atomese dictionary
-----------------------
This is an example configuration for an AtomSpace-backed dictionary.
The AtomSpae must contain appropriate language data, in the expected
format. See the
[dict-atomese README](../../link-grammar/dict-atomese/README.md) for
more info.

The AtomSpace is accessed with `StorageNodes`, examples of which are the
`CogStorageNode` which provides network access, and `RocksStorageNode`,
which provides hard-drive access, via RocksDB.

This dictionary is currently experimental, and subject to change at any
time.

Edit the `storage.dict` file and specify the StorageNode URL.
