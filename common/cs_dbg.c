#include "common/cs_dbg.h"

#include <stdarg.h>
#include <stdio.h>

enum cs_log_level s_cs_log_level =
#ifdef CS_ENABLE_DEBUG
    LL_VERBOSE_DEBUG;
#else
    LL_ERROR;
#endif

void cs_log_printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  fflush(stderr);
}

void cs_log_set_level(enum cs_log_level level) {
  s_cs_log_level = level;
}
