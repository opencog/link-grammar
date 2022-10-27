/*
 * lookup-atomese.h
 *
 * Implement the word-lookup callbacks
 *
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 */

#include "../dict-common/dict-api.h"

bool as_open(Dictionary dict);
void as_close(Dictionary dict);

bool as_boolean_lookup(Dictionary dict, const char *s);
Dict_node * as_lookup_list(Dictionary dict, const char *s);
Dict_node * as_lookup_wild(Dictionary dict, const char *s);
void as_free_llist(Dictionary dict, Dict_node *llist);
void as_clear_cache(Dictionary dict);

void as_add_categories(Dictionary dict);
