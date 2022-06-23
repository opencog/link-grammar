
Experimental Atomese Dictionary
===============================
This directory provides code that implements a connection to an
AtomSpace CogServer, from which dictionary data can be fetched. The goal
of this dictionary is to enable access to a dynamically-changing, live
dictionary maintained in the AtomSpace.  This provides several benefits:

 * Avoids the need for a batch dump of the AtomSpace data to a DB file
 * Enables "lifelong learning": as new language uses are learned, the
   AtomSpace contents are changed, and those changes become visible to
   the parser.

This is meant to work with dictionaries created by the code located
in the [OpenCog learn repo](https://github.com/opencog/learn).

**Version 0.7.1** -- The basic code has been laid down. Use of gram classes
not yet implemented. Costs not yet implemented. And other stuff is
missing.

Building
--------
To build this code, you must first do the following:
```
sudo apt install guile-3.0-dev
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

cd link-grammar
./autogen.sh --no-configure
mkdir build; cd build; ../configure
make -j
sudo make install
```

Demo
----
A working demo can be created as follows:
```
git clone https://github.com/opencog/docker
cd docker/opencog
./docker-build.sh -s -u
```
The above may take an hour or two to complete.
Then `cd lang-model` and read and follow the instructions in
`Dockerfile`.  This will result in a running CogServer with
some minimalist, bare-bones language data in it.  Start the
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

Custom CogServer locations can be specified by altering the
[demo dictionary file](../../data/demo-atomese/cogserver.dict).


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

System diagram
--------------
Stacked boxes represent shared-libraries, with shared-library calls going
downwards. Note that AtomSpaces start out empty, so the data has to come
"from somewhere". In this case, the data comes from another AtomSpace, running
remotely (in the demo, its in a Docker container).  That AtomSpace in turn
loads its data from a
[RocksStorageNode](https://github.com/opencog/opencog-rocks), which uses
[RocksDB](https://rocksdb.org) to work with the local disk drive.
The network connection is provided by a
[CogServer](https://github.com/opencog/cogserver) to
[CogStorageNode](https://github.com/opencog/opencog-cog) pairing.
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

TODO
====
Remaining work items:

* Implement costs. Pull from PredicateNode.
* Implement gram class support.

* Close the loop w/ parsing, so that LG disjuncts arising from a given
  parse an be matched up with the Atomese disjuncts.  Increment/send
  the updated counts.  See todo list below.

* When preparing dictionaries, a count of utilized disjuncts must be
  made before trimming, as otherwise we end up in the situation where
  trimming has eliminated some of the disjuncts needed to make an actual
  parse. This requires significant pipeline changes; see the todo list
  below.

* Handle both Rocks and Cogserver URL's. Look at the URL, and use the
  appropriate `StorageNode`. (Or create a `StorageNode` dispatcher in
  Atomese.)

* Expire local cache entries (given by `dict_node_lookup`) after some time
  frame, forcing a fresh lookup from the server.

Design Notes
------------
Several important tasks lie ahead:
* LG disjuncts must be associated with Atomese disjuncts, so that when
  LG produces a parse, we can know which Atomese disjunct was used in
  that parse (and thus increment the use count on it).

* Such increments and updates can be done locally, but there needs to be
  a write-back system, so that local count updates are not only pushed
  back to the cogserver (this is easy/trivial) but also written from the
  cogserver, to it's open storage node.

### LG-Atomese Disjunct pairing
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

### Counting
After a parse is performed, the above lookup is done, and the count on
the Atomese `Section` must be updated. This count must be written to the
CogServer. The count is sent as an increment-count message, since it is
impractical to implement an atomic read-increment-write over the net.
This is a new extension to the `StorageNode`.

### Pass-through of Writes
Every update that the CogServer receives must then be performed on the
Rocks StorageNode that it is attached to. The correct framework for this
has already been started at https://github.com/opencog/atomspace-agents

There are two ways to implement this idea:
* Top-down command. This module (link-grammar) tells the CogServer what
  to do, and the CogServer obeys orders, and just does it. There are two
  problems with this design:
  - How to send orders telling the CogServer what to do? There is no
    infrastructure for this.
  - What happens if two different subsystems (this one, and some other
    system) are sending conflicting orders? How can one avoid trampling?

* Policy agents. This module (link-grammar) attaches to a server that
  implements the desired policy (which is a write-through from network
  to disk storage).  This solves both problems above, as the policy agent
  knows what to do, and it knows how to resolve conflicts that
  link-grammar might not even be aware of.

### Restarts
Over short periods of time, and in limited use, the above is enough.
However, it might be desirable to restart LG, and re-use the link names
and disjuncts that were previously used. This adds considerable new
complexity, and is probably not worth doing. The following would be
needed:

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
