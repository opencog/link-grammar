/*
 * local-as.h
 *
 * Local copy of AtomSpace
 *
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 */

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/persist/api/StorageNode.h>
#undef STRINGIFY

using namespace opencog;

class Local
{
public:
	bool using_external_as;
	const char* node_str; // (StorageNode \"foo://bar/baz\")
	AtomSpacePtr asp;
	StorageNodePtr stnp;
	Handle linkp; // (Predicate "*-LG connector string-*")
	Handle djp;   // (Predicate "*-LG disjunct string-*")
	Handle mikp;  // (Predicate "*-Mutual Info Key cover-section")
	int mi_offset; // Offset into the FloatValue
};
