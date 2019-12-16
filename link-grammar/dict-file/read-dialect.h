/*************************************************************************/
/* Copyright (C) 2019 Amir Plivatsky                                     */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _READ_DIALECT_H_
#define _READ_DIALECT_H_

#include "api-types.h"
#include "dict-common/dict-common.h"

bool dialect_file_read(Dictionary, const char *);
bool dialect_read_from_one_line_str(Dictionary, Dialect *, const char *);

#endif /* _READ_DIALECT_H_ */
