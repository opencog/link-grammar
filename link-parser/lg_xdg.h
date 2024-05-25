#ifndef _LG_XDG_H
#define _LG_XDG_H

#include "link-grammar/link-includes.h"              // GNUC_PRINTF

typedef enum {
	 XDG_BD_STATE, // XDG_BD_CONFIG, XDG_BD_DATA, XDG_BD_CACHE
} xdg_basedir_type;

const char *xdg_get_home(xdg_basedir_type);
const char *xdg_get_program_name(const char *);
const char *xdg_make_path(xdg_basedir_type, const char *fmt, ...)
	GNUC_PRINTF(2,3);

#endif //_LG_XDG_H
