/*
 * Copyright 2019 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ubuntu.h"

int logm_print_prefix(enum cs_log_level l, const char *func, const char *file) {
  char ll_color[8];
  char ll_file[31];
  char ll_func[41];
  size_t offset = 0;

  switch (l) {
    case LL_ERROR:
      strncpy(ll_color, "\033[1;31m", sizeof(ll_color));
      break;

    case LL_WARN:
      strncpy(ll_color, "\033[0;31m", sizeof(ll_color));
      break;

    case LL_INFO:
      strncpy(ll_color, "\033[0;32m", sizeof(ll_color));
      break;

    case LL_DEBUG:
      strncpy(ll_color, "\033[0;34m", sizeof(ll_color));
      break;

    case LL_VERBOSE_DEBUG:
      strncpy(ll_color, "\033[1;34m", sizeof(ll_color));
      break;

    default:  // LL_NONE
      return 0;
  }

  offset = 0;
  memset(ll_file, 0, sizeof(ll_file));
  if (strlen(file) >= sizeof(ll_file)) {
    offset = strlen(file) - sizeof(ll_file) + 1;
  }
  strncpy(ll_file, file + offset, sizeof(ll_file) - 1);

  offset = 0;
  memset(ll_func, 0, sizeof(ll_func));
  if (strlen(func) >= sizeof(ll_func)) {
    offset = strlen(func) - sizeof(ll_func) + 1;
  }
  strncpy(ll_func, func + offset, sizeof(ll_func) - 1);

  fprintf(stderr, "%s%-20s ", ll_color, ll_func);
  return 1;
}

void logm_printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}
