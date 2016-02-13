/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/cs_dbg.h"

#include <stdarg.h>
#include <stdio.h>

enum cs_log_level s_cs_log_level =
#ifdef CS_ENABLE_DEBUG
    LL_VERBOSE_DEBUG;
#else
    LL_ERROR;
#endif

#ifndef CS_DISABLE_STDIO
void cs_log_printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  fflush(stderr);
}
#endif /* !CS_DISABLE_STDIO */

void cs_log_set_level(enum cs_log_level level) {
  s_cs_log_level = level;
}
