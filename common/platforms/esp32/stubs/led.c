/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "soc/gpio_reg.h"

void led_setup(int io) {
  WRITE_PERI_REG(GPIO_ENABLE_W1TS_REG, 1 << io);
}

void led_on(int io) {
  WRITE_PERI_REG(GPIO_OUT_W1TS_REG, 1 << io);
}

void led_off(int io) {
  WRITE_PERI_REG(GPIO_OUT_W1TC_REG, 1 << io);
}

void led_toggle(int io) {
  if (READ_PERI_REG(GPIO_OUT_REG & 1 << io)) {
    WRITE_PERI_REG(GPIO_OUT_W1TC_REG, 1 << io);
  } else {
    WRITE_PERI_REG(GPIO_OUT_W1TS_REG, 1 << io);
  }
}
