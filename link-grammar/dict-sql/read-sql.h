
/*
 * read-sql.h
 *
 * Read in dictionary from an SQL DB.
 * Keeping it simple for just right now, and using SQLite.
 *
 * The goal of reading the dictioary from SQL is to enable some 
 * other process (machine-learning algo) to dynamically update
 * the dictionary.
 *
 * Copyright (c) 2014 Linas Vepstas <linasvepstas@gmail.com>
 */

#ifndef READ_SQL_H
#define READ_SQL_H

#include "link-includes.h"

Boolean check_db(const char *lang);
Dictionary dictionary_create_from_db(const char *lang);
void dictionary_db_close(Dictionary dict);
Dict_node * dictionary_db_lookup_list(Dictionary dict, const char *s);

#endif /* READ_SQL_H */
