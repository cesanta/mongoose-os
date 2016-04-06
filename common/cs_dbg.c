/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/cs_dbg.h"

#include <stdarg.h>
#include <stdio.h>

#include "common/cs_time.h"

enum cs_log_level cs_log_level =
#ifdef CS_ENABLE_DEBUG
    LL_VERBOSE_DEBUG;
#else
    LL_ERROR;
#endif

#ifndef CS_DISABLE_STDIO

FILE *cs_log_file = NULL;

#ifdef CS_LOG_TS_DIFF
double cs_log_ts;
#endif

void cs_log_print_prefix(const char *func) {
  if (cs_log_file == NULL) cs_log_file = stderr;
  fprintf(cs_log_file, "%-20s ", func);
#ifdef CS_LOG_TS_DIFF
  {
    double now = cs_time();
    fprintf(cs_log_file, "%7u ", (unsigned int) ((now - cs_log_ts) * 1000000));
    cs_log_ts = now;
  }
#endif
}

void cs_log_printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(cs_log_file, fmt, ap);
  va_end(ap);
  fputc('\n', cs_log_file);
  fflush(cs_log_file);
}

void cs_log_set_file(FILE *file) {
  cs_log_file = file;
}

#endif /* !CS_DISABLE_STDIO */

void cs_log_set_level(enum cs_log_level level) {
  cs_log_level = level;
#if defined(CS_LOG_TS_DIFF) && !defined(CS_DISABLE_STDIO)
  cs_log_ts = cs_time();
#endif
}
