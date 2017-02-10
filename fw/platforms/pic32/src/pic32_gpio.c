/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_gpio_hal.h"

/* TODO(dfrank) */

bool mgos_gpio_set_mode(int pin, enum mgos_gpio_mode mode) {
  return false;
}

bool mgos_gpio_set_pull(int pin, enum mgos_gpio_pull_type pull) {
  return false;
}

bool mgos_gpio_read(int pin) {
  return false;
}

enum mgos_init_result mgos_gpio_dev_init(void) {
  return MGOS_INIT_OK;
}
