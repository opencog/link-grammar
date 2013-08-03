/*************************************************************************/
/* Copyright (c) 2013 Linas Vepstas <linasvepstas@gmail.com>             */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the Viterbi parsing system is subject to the terms of the      */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _ATOMBASE_SUBSPACE_H
#define _ATOMBASE_SUBSPACE_H

#include <unordered_map>
#include <gc/gc_cpp.h>
#include "compile-base.h"

namespace atombase {

// The Subspace class vaguely resembles the OpenCog AtomSpace/AtomTable
// class.  However, it differs in several very important ways.  It 
// tries to accomplish only one goal: to provide a way of traversing
// the incoming set of an atom. 
//
// In order to obtain the incoming set of an atom, you perform the
// following steps:
// 1) allocate a new copy of this class
// 2) add Links to this class (using the addLink() method)
// 3) ask for the incoming set of some atom
// 4) optionally go to step 2
// 5) delete this class, when no longer needed.
//
// The idea of this class is that most users and algorithms do not
// need access to the incoming set, and that therefore, it's use
// is optional.
//
// It turns out that tracking the incoming set is a technically difficult
// problem; the reasons for this, and the solution, are discussed below.
//
// A) In a multi-threaded environment, with atoms being created all the
//    time, the incoming set might be continuously changing, and so 
//    iterating over it becomes a challenge, requiring locks or copying.
// B) Maintaing both incoming and outgoing sets is a memory-management
//    challenge, for both smart-pointers (reference counting) and 
//    garbage collection.  Basically, if link B points at atom A, that
//    means that B is in A's incoming set.  That means that something
//    must point at B, preventing B from being freed, even if nothing
//    else is pointing at B. This makes it hard to understand when B
//    really is unreferenced, and can be freed.
//
// The solution to these problems employed here is different from that
// in the circa-2012 opencog atomspace/atomtable. The solution used here
// is that the incoming set will only be generated for atoms that were
// explicitly added to this class.  Furthermore, this class must be
// periodically deleted, so that it doesn't end up being the one and
// only reason preventing an atom from being deleted.

// Must use the bdw-gc allocator to track pointers.
// If this is not done, the GC will fail to see the pointers.
typedef std::unordered_multimap<Atom*, Link*, std::hash<Atom*>, 
        std::equal_to<Atom*>,
        gc_allocator< std::pair<const Atom*, Link*> > > IncomingSetMap;

class SubSpace : public gc
{
	public:
		SubSpace();
		void addLink(Link*);
		void removeLink(Link*);

		Set* get_incoming_set(Atom*) const;

	private:
		IncomingSetMap _map;
};


} // namespace atombase

#endif // _ATOMBASE_SUBSPACE_H
