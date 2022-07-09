
Experimental Atomese Dictionary
===============================
This directory provides code that can access dictionary data held in
[AtomSpace](https://wiki.opencog.org/w/AtomSpace)
[StorageNodes](https://wiki.opencog.org/w/StorageNode).
The goal is to be able to access to dynamically-changing, live
dictionaries maintained in the AtomSpace.  This provides several benefits:

 * Avoids the need for a batch dump of the AtomSpace data to a DB file.
 * Enables "lifelong learning": as new language uses are learned, the
   AtomSpace contents are changed, and those changes become visible to
   the parser.

This is meant to work with dictionaries created by the code located
in the [OpenCog learn repo](https://github.com/opencog/learn).

**Version 0.8.0** -- The basic code has been laid down.
The cost predicate needs to be configurable.
A better demo dict needs to be prepared.

Building
--------
To build this code, you must first do the following:
```
sudo apt install guile-3.0-dev librocksdb-dev
sudo apt install libboost-dev libboost-filesystem-dev libboost-program-options-dev
sudo apt install libboost-system-dev libboost-thread-dev

git clone https://github.com/opencog/cogutil
cd cogutil ; mkdir build ; cd build ; cmake ..
make -j
sudo make install
cd ../..

git clone https://github.com/opencog/atomspace
cd atomspace ; mkdir build ; cd build ; cmake ..
make -j
sudo make install
cd ../..

git clone https://github.com/opencog/atomspace-cog
cd atomspace-cog ; mkdir build ; cd build ; cmake ..
make -j
sudo make install
cd ../..

git clone https://github.com/opencog/atomspace-rocks
cd atomspace-rocks ; mkdir build ; cd build ; cmake ..
make -j
sudo make install
cd ../..

cd link-grammar
./autogen.sh --no-configure
mkdir build; cd build; ../configure
make -j
sudo make install
```

Demo
----
At this time, there are three possible demos.

### Trite demo
Build and install link-grammar. Take a look at the file
[`atomese-dict.scm`](../../data/demo-atomese/atomese-dict.scm)
to get a taste of basic Atomese.  Then edit the
[demo dictionary file](../../data/demo-atomese/storage.dict)
and make sure that the URL points to the install location of
`atomese-dict.scm`.

Start link-parser as `link-parser demo-atomese`. There is one and only
one sentence that will parse: "level playing field". That's it.

### CogServer demo

A working demo can be created as follows:
```
git clone https://github.com/opencog/docker
cd docker/opencog
./docker-build.sh -s -u
```
The above may take an hour or two to complete.
Then `cd lang-model` and read and follow the instructions in
`Dockerfile`.  This will result in a running CogServer with
some minimalist, bare-bones language data in it.

Next, edit the
[demo dictionary file](../../data/demo-atomese/storage.dict)
and comment out the `FileStorageNode` line, and uncomment the
`CogStorageNode` line. Adjust the URL, if needed.  Start the
link-parser as `link-parser demo-atomese`. Only a very small
number of simple, short sentences parse; this is a low-quality
dataset. (A better one will be published "soon").

Some working sentences with the above dataset:
```
I saw it .
XVII .
```
This is a low-quality dataset, so don't expect much. Other datasets
are better but are not publicly available.

### RockStorage demo
Download a dataset from `https://linas.org/datasets`. Untar it.
Edit the
[demo dictionary file](../../data/demo-atomese/storage.dict),
and make sure that the `RocksStorageNode` line is uncommented,
and the URL specifies the correct database location. Make sure
the other `StorageNode`s are commented out.

### Going the other way
You can also get Link Grammar parse results into the AtomSpace.
The following works:
```
$ guile
  (use-modules (opencog) (opencog exec))
  (use-modules (opencog nlp))
  (use-modules (opencog nlp lg-parse))
  (use-modules (opencog persist) (opencog persist-file))
  (define fsn
    (FileStorage "/usr/local/share/link-grammar/demo-atomese/atomese-dict.scm"))
  (cog-open fsn)
  (define pda (LgParseDisjuncts
      (PhraseNode "level playing field")
      (LgDictNode "demo-atomese")
      (Number 4)
      (cog-atomspace)
      fsn))
  (cog-execute! pda)
  (cog-prt-atomspace)
  (cog-incoming-set (Predicate "*-LG connector string-*"))
```
See the
[LgParse examples](https://github.com/opencog/lg-atomese/tree/master/examples)
for more info.

Design
======
The current design works as follows:

* The dictionary is directly attached (as a shared library) to a local
  AtomSpace. On startup, this AtomSpace is empty.
* On dictionary open, a connection is established to a remote cogserver.
* On word lookup, a query is sent to the remote cogserver, asking for
  all Atomese disjuncts for which the word is the "germ". The cogserver
  returns these, and so they are instantiated in the local AtomSpace.
* The Atomese disjuncts are converted to LG disjuncts. This is a
  two-step process: First, a LG link name is generated for the
  connectors. Second, each Atomese disjunct is converted to an LG `Exp`
  structure, and these are concatenated onto an LG `Dict_node`, which is
  then added to the local LG dictionary.
* Subsequent lookups of the same word directly return the `Dict_node`,
  instead of working with the AtomSpace. That is, AtomSpace results are
  cached locally in LG itself.
* The local cache can be cleared by issuing the `clear` command.
  Type `!help clear` for more info.


AtomSpace Format
----------------
The AtomSpace dictionary encoding is described in many places. A quick
review is presented below. Basically, the AtomSpace implements
dictionary entries using the abstract concept of disjuncts. These are
conceptually similar to LG disjuncts, but have a completely different
storage format.

Disjuncts are Atoms of type `ConnectorSeq`. Word-disjunct pairings are
Atoms of type `Section`.  For example, three single-word entries that
allow "level playing field" to parse are encoded in Atomese as:
```
	(Section
		(Word "level")
		(ConnectorSeq
			(Connector (Word "playing") (ConnectorDir "+"))))

	(Section
		(Word "playing")
		(ConnectorSeq
			(Connector (Word "level") (ConnectorDir "-"))
			(Connector (Word "field") (ConnectorDir "+"))))

	(Section
		(Word "field")
		(ConnectorSeq
			(Connector (Word "playing") (ConnectorDir "-"))))
```

This is just an example. Grammatical classes are similar, except that
the `WordNode`s are replaced by `WordClassNode`s. There are additional
generalizations that allow visual and audio data to be encoded in the
same format, and for such sensory information to be correlated with
language information. This is an area of ongoing research.

The above example can be found in the file
[atomese-dict.scm](../../data/demo-atomese/atomese-dict.scm)
and can be used live, in the demo.

System diagram
--------------
Stacked boxes represent shared-libraries, with shared-library calls going
downwards. Note that AtomSpaces start out empty, so the data has to come
"from somewhere".  AtomSpaces load data from
[StorageNodes](https://wiki.opencog.org/w/StorageNode).
There are half-a-dozen of these; one of them is the
[RocksStorageNode](https://github.com/opencog/opencog-rocks), which uses
[RocksDB](https://rocksdb.org) to work with the local disk drive. The
software layers are as depicted below.
```
    +----------------+
    |  Link Grammar  |
    |    parser      |
    +----------------+
    |   AtomSpace    |
    +----------------+
    |     Rocks      |
    |  StorageNode   |
    +----------------+
    |    RocksDB     |
    +----------------+
    |   disk drive   |
    +----------------+
```

Another StorageNode is the
[CogStorageNode](https://github.com/opencog/opencog-cog),
which provides a network connection via the
[CogServer](https://github.com/opencog/cogserver).
This system is depicted below.
```
                                            +----------------+
                                            |  Link Grammar  |
                                            |    parser      |
                                            +----------------+
                                            |   AtomSpace    |
    +-------------+                         +----------------+
    |             |                         |                |
    |  CogServer  | <<==== Internet ====>>  | CogStorageNode |
    |             |                         |                |
    +-------------+                         +----------------+
    |  AtomSpace  |
    +-------------+
    |    Rocks    |
    | StorageNode |
    +-------------+
    |   RocksDB   |
    +-------------+
    | disk drive  |
    +-------------+
```
In the demo, the CogServer and the associated dataset are in a Docker
container. The primary reason for using Docker is to make the demo as
simple as possible.

TODO
====
Remaining work items:

* Close the loop w/ parsing, so that LG disjuncts arising from a given
  parse an be matched up with the Atomese disjuncts.  Increment/send
  the updated counts.  See todo list below.

* When preparing dictionaries, a count of utilized disjuncts must be
  made before trimming, as otherwise we end up in the situation where
  trimming has eliminated some of the disjuncts needed to make an actual
  parse. This requires significant pipeline changes; see the todo list
  below.

* Expire local cache entries (given by `dict_node_lookup`) after some
  time frame, forcing a fresh lookup from the server. (Only for
  CogStorage!) (Maybe this should involve some proxy agent? Maybe a
  `ProxyStorageNode`?)

Design Notes
------------
Some notes.

### LG-Atomese Disjunct pairing
LG disjuncts must be associated with Atomese disjuncts, so that when
LG produces a parse, we can know which Atomese disjunct was used in
that parse (and thus increment the use count on it).

This needs to happen in the `learn` codebase, and not here.
The `(PredicateNode "*-LG connector string-*")` implemented here
is enough to perform that pairing, and the guile demo above shows
that this works fine. See also examples in `lg-atomese`.

Remove this note when above is finished!

After LG parsing is completed, the chosen disjuncts are easily converted
into unique strings. These need to be paired with Atomese disjuncts. The
obvious format would seem to be:
```
   (Evaluation (Predicate "LG-Atomese Disjunct pair")
      (Concept "A+ & B- & C+") (ConnectorSeq ...))
```
This allows the `ConnectorSeq` to be easily found, given only the LG
disjunct string.

* Side note: we need a `get-incoming-by-predicate` function!

### Reproducible Connector Names
There is a (mild!?) design flaw in the current implementation. The
connector names are dynamically generated, on-the-fly, by incrementing
a counter.  That means the connector names are never the same, two runs
in a row.

It might be desirable to be able to have reproducible connector names,
*i.e.* to get the same names after a restart of LG. Implementing this
adds considerable new complexity, and is probably not worth doing. The
following would be needed:

* Link names need to be stored. These can be recorded as
```
   (Evaluation (Predicate "LG link name")
      (Concept "XXX") (Set (Word...) (Word ..)))

```
* Prior to use, such pairing need to be queried. This can add a hefty
  network load and latency overhead. LG code is more complicated,
  because this query needs to be performed, with new pairings generated
  if they don't already exist. Fallbacks for errors. All this code has
  to be debugged. Yuck.

* Same, but for the disjunct pairings.

* The Cogserver needs to queried for a block of new link names that can
  be assigned. Need to get a block, as otherwise multiple parsers could
  be assigning the same names and trampling on one-another. This could
  be done with an atomic increment of a Value: incrementing by 100 lets
  the next 100 ints to be safely used.
