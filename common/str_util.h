/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#ifndef STR_UTIL_H
#define STR_UTIL_H

#include <stdarg.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

int c_snprintf(char *buf, size_t buf_size, const char *format, ...);
int c_vsnprintf(char *buf, size_t buf_size, const char *format, va_list ap);

#if (!(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 700) &&           \
     !(defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200809L) &&   \
     !(defined(__DARWIN_C_LEVEL) && __DARWIN_C_LEVEL >= 200809L) && \
     !defined(RTOS_SDK)) ||                                         \
    (defined(_MSC_VER) && _MSC_VER < 1600 /*Visual Studio 2010*/)
#define _MG_PROVIDE_STRNLEN
size_t strnlen(const char *s, size_t maxlen);
#endif

#ifdef __cplusplus
}
#endif
#endif
