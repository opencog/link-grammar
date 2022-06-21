
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

**Version 0.7.0** -- The basic code has been laid down. Use of gram classes
not yet implemented.

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
dataset. (A bettter one will be published "soon").

Some working sentences with the above dataset:
```
her !
who are
what is it
```
This is a low-quality dataset, so don't exepect much. Other datasets
are better but are not publicly available.

Custom CogServer locations can be specified by altering the
[demo dictionary file](../../data/demo-atomese/cogserver.dict).


AtomSpace Format
================
The AtomSpace dictionary encoding is described in many places. A quick
review is presented below.

For example, three single-word entries that allow "level playing field" to
parse are encoded as:
```
	(Section
		(Word "level")
		(ConnectorSeq
			(Connector (Word "playing") (Direction "+"))))

	(Section
		(Word "playing")
		(ConnectorSeq
			(Connector (Word "level") (Direction "-"))
			(Connector (Word "field") (Direction "+"))))

	(Section
		(Word "field")
		(ConnectorSeq
			(Connector (Word "playing") (Direction "-"))))
```

Grammatical classes are similar, except that the `WordNode`s are
replaced by `WordClassNode`s. Some details are subject to change.

TODO
====
Remaining work items:

* Verify minus-direction connector order.
* Implement gram class support.
* Expire local cache entries (given by dict_node_lookup) after some time
  frame, forcing a frewsh lookup from the server.
