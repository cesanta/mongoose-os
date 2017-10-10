/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_CS_DBG_H_
#define CS_COMMON_CS_DBG_H_

#include "common/platform.h"

#if CS_ENABLE_STDIO
#include <stdio.h>
#endif

#ifndef CS_ENABLE_DEBUG
#define CS_ENABLE_DEBUG 0
#endif

#ifndef CS_LOG_ENABLE_TS_DIFF
#define CS_LOG_ENABLE_TS_DIFF 0
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum cs_log_level {
  LL_NONE = -1,
  LL_ERROR = 0,
  LL_WARN = 1,
  LL_INFO = 2,
  LL_DEBUG = 3,
  LL_VERBOSE_DEBUG = 4,

  _LL_MIN = -2,
  _LL_MAX = 5,
};

/* Set log level. */
void cs_log_set_level(enum cs_log_level level);

/* Set log filter. NULL (a default) logs everything. */
void cs_log_set_filter(const char *source_file_name);

int cs_log_print_prefix(enum cs_log_level level, const char *func,
                        const char *filename);

extern enum cs_log_level cs_log_threshold;

#if CS_ENABLE_STDIO

void cs_log_set_file(FILE *file);
void cs_log_printf(const char *fmt, ...)
#ifdef __GNUC__
    __attribute__((format(printf, 1, 2)))
#endif
    ;

#define LOG(l, x)                                                    \
  do {                                                               \
    if (cs_log_print_prefix(l, __func__, __FILE__)) cs_log_printf x; \
  } while (0)

#ifndef CS_NDEBUG

#define DBG(x) LOG(LL_VERBOSE_DEBUG, x)

#else /* NDEBUG */

#define DBG(x)

#endif

#else /* CS_ENABLE_STDIO */

#define LOG(l, x)
#define DBG(x)

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_COMMON_CS_DBG_H_ */
