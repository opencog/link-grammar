
Experimental Atomese Dictionary
===============================
This directory provides code that implemenets a connection to an
AtomSpace CogServer, from which dictionary data can be fetched. The goal
of this dictionary is to enable access to a dynamically-changing, live
dictionary maintained in the AtomSpace.  This provides several benefits:

 * Avoids the need for a batch dump of the AtomSpace data to a DB file
 * Enables "lifelong learning": as new language uses are learned, the
   AtomSpace contents are changed, and those changes become visible to
   the parser.

**Version 0.7.0** -- The basic code has been laid down. Use of gram classes
not yet implemented.

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
replaced by `WordClassNode`s.

TODO
====
Remaining work items:

* Implement gram class support.
