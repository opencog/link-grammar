
/*
 * read-sql.c
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

#ifdef HAVE_SQLITE

#include <sqlite3.h>

#endif /* HAVE_SQLITE */
