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

#include "subspace.h"

using namespace atombase;

SubSpace::SubSpace()
{}

// OK, here's the deal with using unordered_multimap instead of,
// say map<Atom*, vector<Link*>>.
// 1) map<Atom*, vector<Link*>> would require copying the vector 
//    every time for get_incoming set... and so does multimap.
// 2) map<Atom*, vector<Link*>> requires copying the vector for
//    every atom insertion, multimap does not.
// 3) atom removal is painful either way...
// So unordered_map wins by a nose.
//

void SubSpace::addLink(Link* lnk)
{
	// XXX TODO need to use locks here.
	foreach_outgoing(Atom*, atom, lnk)
		_map.insert({atom, lnk});
}

void SubSpace::removeLink(Link* lnk)
{
	// XXX TODO need to use locks here.
	foreach_outgoing(Atom*, atom, lnk)
	{
		// Copy all of them, except the one being deleted.
		std::vector<Link*, gc_allocator<Link*> > tmp;
		auto range = _map.equal_range(atom);
		for(auto it = range.first; it != range.second; it++)
		{
			Link* il = it->second;
			if (il != lnk)
				tmp.push_back(il);
		}
		// Delete all.  For some weird reason, there's no simple delete..
		_map.erase(atom);

		// put back those we want to keep. Sigh.
		for (auto ik = tmp.begin(); ik != tmp.end(); ik++)
		{
			_map.insert({atom, *ik});
		}
	}
}

Set* SubSpace::get_incoming_set(Atom* atom) const
{
	// XXX TODO need to use locks here.
	OutList tmp;
	auto range = _map.equal_range(atom);
	for(auto it = range.first; it != range.second; it++)
	{
		tmp.push_back(it->second);
	}

	return new Set(tmp);
}

