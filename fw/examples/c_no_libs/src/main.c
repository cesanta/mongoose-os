/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>

#include "mgos.h"

enum mgos_app_init_result mgos_app_init(void) {
  printf("Hello, world!\n");
  return MGOS_APP_INIT_SUCCESS;
}
