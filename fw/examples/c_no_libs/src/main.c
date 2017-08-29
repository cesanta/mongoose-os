/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>

#include "mgos.h"

static void timer_cb(void *arg) {
  static int current_level = 1;
  LOG(LL_INFO, ("%s", (current_level ? "Tick" : "Tock")));
  current_level ^= 1;
  (void) arg;
}

enum mgos_app_init_result mgos_app_init(void) {
  printf("Hello, world!\n");
  mgos_set_timer(1000 /* ms */, true /* repeat */, timer_cb, NULL);
  return MGOS_APP_INIT_SUCCESS;
}
