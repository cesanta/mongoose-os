/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_debug.h"

int mgos_debug_redirect(enum debug_mode mode) {
  (void) mode;
  return -1;
}

