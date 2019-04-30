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

int cs_log_print_prefix(enum cs_log_level level, const char *func,
                        const char *filename) {
  (void) level;
  (void) func;
  (void) filename;
  return 0;
}

void cs_log_printf(const char *fmt, ...) {
  (void) fmt;
  return;
}
