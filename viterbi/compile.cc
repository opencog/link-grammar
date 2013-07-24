/*************************************************************************/
/* Copyright (c) 2012, 2013 Linas Vepstas <linasvepstas@gmail.com>       */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the Viterbi parsing system is subject to the terms of the      */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "compile.h"

namespace link_grammar {
namespace viterbi {

// ============================================================

// Helper function for below.
static bool has_lefties(Atom* a)
{
	Connector* c = dynamic_cast<Connector*>(a);
	if (c)
	{
		if ('-' == c->get_direction())
			return true;
		return false;
	}

	// Verify we've got a valid disjunct
	AtomType at = a->get_type();
	assert ((at == OR) or (at == AND), "Disjunct, expecting OR or AND");

	// Recurse down into disjunct
	Link* l = dynamic_cast<Link*>(a);
	size_t sz = l->get_arity();
	for (size_t i=0; i<sz; i++)
	{
		if (has_lefties(l->get_outgoing_atom(i)))
			return true;
	}
	return false;
}

/// Return true if any of the connectors in the cset point to the left.
bool WordCset::has_left_pointers() const
{
	return has_lefties(get_cset());
}

/// Simplify any gratuituousnesting in the cset.
WordCset* WordCset::flatten()
{
	// AND and OR inherit from Set
	Set* s = dynamic_cast<Set*>(get_cset());
	if (NULL == s)
		return this;

	Atom* flat = s->super_flatten();

	// If there is nothing left after flattening, return NULL.
	const Link* fl = dynamic_cast<const Link*>(flat);
	if (fl && 0 == fl->get_arity())
		return NULL;

	return new WordCset(get_word(), flat);
}

// ============================================================

Atom* upcast(Atom* a)
{
	const Node* n = dynamic_cast<const Node*>(a);
	const Link* l = dynamic_cast<const Link*>(a);

	switch (a->get_type())
	{
		// Links
		case AND:
			if (dynamic_cast<And*>(a)) return a;
			return new And(l->get_outgoing_set(), l->_tv);
		case OR:
			if (dynamic_cast<Or*>(a)) return a;
			return new Or(l->get_outgoing_set(), l->_tv);

		// Nodes
		case CONNECTOR:
			if (dynamic_cast<Connector*>(a)) return a;
			return new Connector(n->get_name(), n->_tv);

		case WORD:
			if (dynamic_cast<Word*>(a)) return a;
			return new Word(n->get_name(), n->_tv);

		default:
			assert(0, "upcast: implement me!");
	}
}

} // namespace viterbi
} // namespace link-grammar
