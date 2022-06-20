/*
 * read-sql.h
 *
 * Use a dictionary specified in an SQL DB.
 * Keep it simple for just right now, and use SQLite.
 *
 * The goal of reading the dictionary from SQL is to enable some
 * other process (machine-learning algo) to dynamically update
 * the dictionary.
 *
 * Copyright (c) 2014 Linas Vepstas <linasvepstas@gmail.com>
 */

#ifndef READ_SQL_H
#define READ_SQL_H

#include "link-includes.h"

#ifdef HAVE_SQLITE3
Dictionary dictionary_create_from_db(const char *lang);
#endif /* HAVE_SQLITE3 */

#endif /* READ_SQL_H */
