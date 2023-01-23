/*
 * lookup-atomese.h
 *
 * Implement the word-lookup callbacks
 *
 * Copyright (c) 2022 Linas Vepstas <linasvepstas@gmail.com>
 */

#include "../dict-common/dict-api.h"

bool as_open(Dictionary);
void as_close(Dictionary);

bool as_boolean_lookup(Dictionary, const char *);
Dict_node * as_lookup_list(Dictionary, const char *);
Dict_node * as_lookup_wild(Dictionary, const char *);

void as_start_parse(Dictionary, Sentence);
void as_end_parse(Dictionary, Sentence);

void as_clear_cache(Dictionary);

void as_add_categories(Dictionary);
void as_storage_close(Dictionary);
