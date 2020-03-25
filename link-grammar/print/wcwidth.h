#ifndef _WCWIDTH_H
#define _WCWIDTH_H

#include <wchar.h>

int mk_wcwidth(wchar_t);
#if 0 // Unused
int mk_wcswidth(const wchar_t *, size_t);
#endif

#endif /* _WCWIDTH_H */
