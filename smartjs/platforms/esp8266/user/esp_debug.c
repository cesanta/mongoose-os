/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "smartjs/src/sj_debug.h"
#include "esp_uart.h"

int sj_debug_redirect(enum debug_mode mode) {
  int ires = -1;
  uart_debug_init(0, 0);
  ires = uart_redirect_debug(mode);
  return ires;
}
