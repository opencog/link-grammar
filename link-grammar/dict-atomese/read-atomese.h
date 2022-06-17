/*
 * read-atomese.h
 *
 * Use a dictionary located in the OpenCog AtomSpace.
 *
 * The goal of using a dictionary in the AtomSpace is that no distinct
 * data export step is required.  The dictionary can dynamically update,
 * even as it is being used.
 *
 * Copyright (c) 2014, 2022 Linas Vepstas <linasvepstas@gmail.com>
 */

#ifndef READ_ATOMESE_H
#define READ_ATOMESE_H

#include "link-includes.h"

#ifdef HAVE_ATOMESE
Dictionary dictionary_create_from_atomese(const char *lang);
#endif /* HAVE_ATOMESE */

#endif /* READ_ATOMESE_H */
