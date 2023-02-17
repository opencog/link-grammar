
AtomSpace Dictionary
====================
This directory provides code that can access dictionary data held in
[AtomSpace](https://wiki.opencog.org/w/AtomSpace)
[StorageNodes](https://wiki.opencog.org/w/StorageNode).
The goal is to be able to access to dynamically-changing, live
dictionaries maintained in the AtomSpace.  This provides several benefits:

 * Avoids the need for a batch dump of the AtomSpace data to a DB file.
 * Enables "lifelong learning": as new language uses are learned, the
   AtomSpace contents are changed, and those changes become immediately
   visible to the parser.

This is meant to work with dictionaries created by the code located
in the [OpenCog learn repo](https://github.com/opencog/learn).

**Version 0.9.3** -- All basic features have been implemented.
All known bugs have been fixed. Some more complex modes have not
yet been implemented. The code base is not stable, and is subject
to change, including possibly large revisions.

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

Start link-parser as `link-parser demo-atomese`. There are multiple
demo sentences possible. These are:

* "level playing field" -- from the first part of `atomese-dict.scm`.
* "Mary saw a bird" -- from the middle part of `atomese-dict.scm`.
* "1 2 3 fountain 4 5 6" -- from the middle, a debugging pattern.
* "fee fie fo fum" -- Using the `UNKNOWN-WORD` creates random parses.

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

### Going in the other direction
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
      (PhraseNode "Mary saw a bird")
      (LgDictNode "demo-atomese")
      (Number 4)
      (cog-atomspace)
      fsn))
  (cog-execute! pda)
  (cog-prt-atomspace)
  (cog-incoming-set (Predicate "*-LG connector string-*"))
  (cog-incoming-set (Predicate "*-LG disjunct string-*"))
```
See the
[LgParse examples](https://github.com/opencog/lg-atomese/tree/master/examples)
for more info.

Using
=====
There are two basic modes of operation: **private AtomSpace mode** and
** external (shared) AtomSpace mode**. These two modes behave in the
same way, differing only in how the AtomSpace is managed.

Private Mode
------------
In **private mode**, the system creates and maintains it's own private
AtomSpace, and relies on being able to access a `StorageNode` from
which appropriate language data can be fetched. This `StorageNode`
must be configured in the `storage.dict` file. This `StorageNode` can
be a local database, a network connection, or a `ProxyNode` of any
kind.

Private mode is the only mode available the `link-parser` command-line
executable. The typical use-case is to run the `link-parser` tool for
spot-checking some large dictionary, exposed through the network.

Although the parser fetches individual word data on-demand, as needed,
to parse some given sentence, network operation can still be slow, when
the dictionaries are large. Depending on the dictionary, it can take
seconds to copy a single dictionary word over the network. Once copied,
the dictionary entry is cached, and subsequent parsing is much faster.

Direct acces (e.g. via `RocksStorageNode`) is much faster.

External Mode
-------------
The **external mode** is available when using the Link Grammar shared
library. In this mode, LG will use a pre-existing AtomSpace, i.e. one
that has already been created by the process. The AtomSpace to use is
declared with the `lg_config_atomspace()` function, just before the
`dictionary_create_lang()` call. There are two sub-modes of operation:
either the AtomSpace is already full populated with data, or a
`StorageNode` can be specified (with `lg_config_atomspace()`) and the
needed data will be fetched on-demand.


Design
======
Both modes work as follows:

* If an external AtomSpace is specified, then that will be used;
  otherwise a new empty, private AtomSpace is created.
* If a private AtomSpace is used, then the dictionary config file must
  specify a `StorageNode` to use.
* If an external `StorageNode` is specified, it will be used. It is
  assumed that this StorageNode is already open. If the external
  storage is specified as null (a null pointer), then storage will
  not be used.  In this case, the external AtomSpace must be fully
  populated with data.
* Word data can take three different forms, or can be a mixture of these
  forms, depending on the config in the `storage.dict` file. The three
  forms are disjuncts, specified as `Sections`, or MST word-pairs, or
  random planar parse `ANY` link types. When enabled, Atomese `Sections`
  will be mapped, one-to-one, to LG disjuncts, and used for parsing.
  If parsing fails, and supplemental word-pairs is enabled, then
  disjuncts will be decorated with additional word-pairs, hoping to
  obtain a parse. If word-pairs are not avilable, then disjuncts can
  be supplemented with random `ANY` link types (which connect to any
  other `ANY` connector, i.e. randomly.)
* If there are no `Sections`, or if these are disabled, then word-pairs
  can be used, to obtain a Minimum Spanning Tree (MST) parse. In this
  case, word expressions are created from 1 to N word-pairs, and are
  assigned a cost that is the sum of the costs on the word-pairs. These
  are then used as normal LG disjuncts, and the minimum-cost parse is
  then necessarily an MST parse.
* If there are not enough word-pairs to create a parse, and if
  supplementation by `ANY` links is enabled, then `ANY` link types
  will be used, as needed, to obtain a parse.
* If any costs are negative, then a Maximum Planar Graph (MPG) parse is
  obtained. When costs are negative, the cost is further decreased by
  adding more links to the parse graph. As many links are added as
  possible; basic LG rules mean that no links can cross, and all such
  graphs are planar.
* If there are no `Section`s and no word-pairs, then parses can be made
  using only `ANY` link types. These are effectively random parses.
  Sampling is such that all possible parse trees are uniformly sampled.
  (For shorter sentences, once can, of course, exhaustively enumerate
  all possible linkages.)
* Lookup of `Section`s and word-pairs is done on an as-needed basis,
  for each word in the sentence being parsed. Once fetched, the result
  is cached, thus avoiding a second lookup.
* If there is a StorageNode, then fetches (of `Section`s or word-pairs)
  will be performed through that, and placed in the AtomSpace.
* The Atomese `Section`s are converted to LG disjuncts. This is multi-
  step process. First, an LG link name is generated for each connector.
  These are cached locally. Next each Atomese `Section` is converted
  to an LG `Exp` structure. These are concatenated onto an LG `Dict_node`,
  which is then cached (added to the local LG `Dictionary`).
* Subsequent lookups of the same word directly return the `Dict_node`,
  instead of working with the AtomSpace. That is, AtomSpace results are
  cached locally in LG itself.
* Word-pairs get a slightly different handling. For each sentence, the
  list of word-pairs is filtered, so that the only remaining pairs are
  those for which both words are in the sentence. Without this
  filtering, there would be a combinatorial explosion. For example,
  if a word participates in 10K word pairs, and a disjunct can have
  4 connectors, then (10K)^4 = 10^12 (a trillion) disjuncts are
  possible. Clearly, that wouldn't work.
* Word expressions created from filtered word-pairs are not cached.
  The word-pairs themselves are cached, thus avoiding repeated AtomSpace
  lookups and fetches.
* The local cache can be cleared by issuing the `clear` command in the
  LG parser.  Type `!help clear` for more info. The corresponding shared
  library call is `void as_clear_cache(Dictionary);`


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

This example just connects single words together. One may also have
`WordClassNode`s in place of `WordNode`s in the above. The membership of
an individual word in a grammatical class is denoted by
```
   (Member (Word "word") (WordClass "foo"))
```
There are additional (experimental) generalizations that allow visual
and audio data to be encoded in the same format, and for such sensory
information to be correlated with language information. This is an
area of ongoing research.

The above example can be found in the file
[atomese-dict.scm](../../data/demo-atomese/atomese-dict.scm)
and can be used live, in the demo.

System diagram
--------------
Stacked boxes represent shared-libraries, with shared-library calls
going downwards. Note that (private) AtomSpaces start out empty, so
the data has to come "from somewhere".  AtomSpaces load data from
[StorageNodes](https://wiki.opencog.org/w/StorageNode).
There are dozen of these (including
[ProxyNodes](https://wiki.opencog.org/w/ProxyNode)s); one of them is
the [RocksStorageNode](https://github.com/opencog/opencog-rocks), which
uses [RocksDB](https://rocksdb.org) to work with the local disk drive.
An instance of LG parser using Rocks is depicted below.
```
    +----------------+
    |  Link Grammar  |
    |     parser     |
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
                                            |     parser     |
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
In some of the demos, the CogServer and the associated dataset(s) are
in a Docker container. The primary reason for using Docker is to make
setting up the demo as simple as possible.

Design Notes
------------
Some additional detailed notes.

### LG-Atomese Disjunct pairing
LG disjuncts must be associated with Atomese disjuncts, so that when
LG produces a parse, we can know which Atomese disjunct was used in
that parse (and thus increment the use count on it).

This is accomplished by associating the LG Link type to the word or
word-class pair from which it was made. For the case of plain words,
this pairing can be directly reconstructed from the parse. For the
case of WordClasses, it is impossible to guess, and so it is explicitly
provided.

The association is:
```
   (Evaluation
      (Predicate "*-LG link string-*")
      (LgLinkNode ...)   ; The name is the LG link type.
      (List
         (Word ...)      ; The left word (from germ or connector)
         (Word ...)      ; The right word.
```
In the above, the two Words/WordClasses appear as germ-connector or
connector-germ in the Atomese disjuncts.  See examples in `lg-atomese`
that show how above can be accessed.

* Side note: we need a `get-incoming-by-predicate` function!

### Reproducible Connector Names
There is a (mild!?) design flaw in the current implementation. The
connector names are dynamically generated, on-the-fly, by incrementing
a counter.  That means the connector names are never the same, two runs
in a row.

It might be desirable to be able to have reproducible connector names,
*i.e.* to get the same names after a restart of LG. Implementing this
adds considerable new complexity. Is it worth doing?
The following would be needed:

* The `(Predicate "*-LG link string-*")` described above need to be
  fetched. This can add significant network overhead.

* The StorageNode needs to queried for a block of unused link names
  that can be assigned. Need to get a block, as otherwise multiple
  parsers could be assigning the same names and trampling on one-another.
  This could be done with an atomic increment of a Value: incrementing
  by 100 lets the next 100 ints to be safely used.

TODO
====
Remaining work items:

* Provide a high-quality demo dictionary.

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

----------
