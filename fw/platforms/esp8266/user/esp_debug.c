/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/sj_debug.h"
#include "esp_fs.h"

int sj_debug_redirect(enum debug_mode mode) {
  int stderr_uart = -1;
  switch (mode) {
    case DEBUG_MODE_OFF:
      stderr_uart = -1;
      break;
    case DEBUG_MODE_STDOUT:
      stderr_uart = 0;
      break;
    case DEBUG_MODE_STDERR:
      stderr_uart = 1;
      break;
  }
  fs_set_stderr_uart(stderr_uart);
  return 0;
}
