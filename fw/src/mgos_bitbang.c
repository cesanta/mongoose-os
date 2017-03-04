/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_bitbang.h"

#include "common/cs_dbg.h"

#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_hal.h"

#ifndef IRAM
#define IRAM
#endif

IRAM static void mgos_bitbang_write_bits2(int gpio,
                                          void (*delay_fn)(uint32_t n),
                                          uint32_t t0h, uint32_t t0l,
                                          uint32_t t1h, uint32_t t1l,
                                          const uint8_t *data, size_t len) {
  mgos_ints_disable();
  while (len-- > 0) {
    uint8_t b = *data++;
    for (int i = 0; i < 8; i++) {
      if (b & 0x80) {
        mgos_gpio_write(gpio, 1);
        delay_fn(t1h);
        mgos_gpio_write(gpio, 0);
        delay_fn(t1l);
      } else {
        mgos_gpio_write(gpio, 1);
        delay_fn(t0h);
        mgos_gpio_write(gpio, 0);
        delay_fn(t0l);
      }
      b <<= 1;
    }
  }
  mgos_ints_enable();
}

/*
 * let leds = "\xff\0\0";
 * mgos_bitbang_write_bits_js(13, 1, (1 << 24) | (2 << 16) | (3 << 8) | 4, leds,
 * legs.length);
 */

void mgos_bitbang_write_bits(int gpio, enum mgos_delay_unit delay_unit,
                             uint32_t t0h, uint32_t t0l, uint32_t t1h,
                             uint32_t t1l, const uint8_t *data, size_t len) {
  void (*delay_fn)(uint32_t n);
  switch (delay_unit) {
    case MGOS_DELAY_MSEC:
      delay_fn = mgos_msleep;
      break;
    case MGOS_DELAY_USEC:
      delay_fn = mgos_usleep;
      break;
    case MGOS_DELAY_100NSEC:
      delay_fn = *mgos_nsleep100;
      if (t0h < mgos_bitbang_n100_cal) t0h = mgos_bitbang_n100_cal;
      t0h -= mgos_bitbang_n100_cal;
      if (t0l < mgos_bitbang_n100_cal) t0l = mgos_bitbang_n100_cal;
      t0l -= mgos_bitbang_n100_cal;
      if (t1h < mgos_bitbang_n100_cal) t1h = mgos_bitbang_n100_cal;
      t1h -= mgos_bitbang_n100_cal;
      if (t1l < mgos_bitbang_n100_cal) t1l = mgos_bitbang_n100_cal;
      t1l -= mgos_bitbang_n100_cal;
      break;
    default:
      return;
  }
  mgos_bitbang_write_bits2(gpio, delay_fn, t0h, t0l, t1h, t1l, data, len);
}

void mgos_bitbang_write_bits_js(int gpio, enum mgos_delay_unit delay_unit,
                                uint32_t t, const uint8_t *data, size_t len) {
  mgos_bitbang_write_bits(gpio, delay_unit, t >> 24, (t >> 16) & 0xff,
                          (t >> 8) & 0xff, t & 0xff, data, len);
}
