/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_debug.h"

void cc3200_console_putc(int fd, char c) {
  mgos_debug_write(fd, &c, 1);
}
