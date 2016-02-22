/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/cs_dbg.h"

#include <stdarg.h>
#include <stdio.h>

#include "common/cs_time.h"

enum cs_log_level s_cs_log_level =
#ifdef CS_ENABLE_DEBUG
    LL_VERBOSE_DEBUG;
#else
    LL_ERROR;
#endif

#ifdef CS_LOG_TS_DIFF
double cs_log_ts;
#endif

#ifndef CS_DISABLE_STDIO
void cs_log_printf(const char *fmt, ...) {
  va_list ap;
#ifdef CS_LOG_TS_DIFF
  double now = cs_time();
  fprintf(stderr, "%7u ", (unsigned int) ((now - cs_log_ts) * 1000000));
#endif
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
#ifdef CS_LOG_TS_DIFF
  cs_log_ts = now;
#endif
  fflush(stderr);
}
#endif /* !CS_DISABLE_STDIO */

void cs_log_set_level(enum cs_log_level level) {
  s_cs_log_level = level;
#ifdef CS_LOG_TS_DIFF
  cs_log_ts = cs_time();
#endif
}
