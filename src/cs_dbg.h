#ifndef _CS_DBG_H_
#define _CS_DBG_H_

enum cs_log_level {
  LL_NONE = -1,
  LL_ERROR = 0,
  LL_WARN = 1,
  LL_INFO = 2,
  LL_DEBUG = 3,

  _LL_MIN = -2,
  _LL_MAX = 4,
};

#ifndef CS_NDEBUG

extern enum cs_log_level s_cs_log_level;
void cs_log_set_level(enum cs_log_level level);

void cs_log_printf(const char *fmt, ...);

#define LOG(l, x)                        \
  if (s_cs_log_level >= l) {             \
    fprintf(stderr, "%-20s ", __func__); \
    cs_log_printf x;                     \
  }

#define DBG(x)                           \
  if (s_cs_log_level >= LL_DEBUG) {      \
    fprintf(stderr, "%-20s ", __func__); \
    cs_log_printf x;                     \
  }

#else /* NDEBUG */

#define cs_log_set_level(l)

#define LOG(l, x)
#define DBG(x)

#endif

#endif /* _CS_DBG_H_ */
