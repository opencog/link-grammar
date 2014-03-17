
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

char * check_db(const char *dbname);
void get_db_contents(Dictionary dict, const char *dbname);

#endif /* READ_SQL_H */
