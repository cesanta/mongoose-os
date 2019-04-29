/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mgos_bitbang.h"

#include "common/cs_dbg.h"

#include "mgos_gpio.h"
#include "mgos_hal.h"

#ifndef IRAM
#define IRAM
#endif

IRAM static void mgos_bitbang_write_bits2(int gpio,
                                          void (*delay_fn)(uint32_t n), int t0h,
                                          int t0l, int t1h, int t1l,
                                          const uint8_t *data, size_t len) {
  mgos_ints_disable();
  while (len-- > 0) {
    uint8_t b = *data++;
    for (int i = 0; i < 8; i++) {
      if (b & 0x80) {
        if (t1h >= 0) {
          mgos_gpio_write(gpio, 1);
          delay_fn(t1h);
        }
        if (t1l >= 0) {
          mgos_gpio_write(gpio, 0);
          delay_fn(t1l);
        }
      } else {
        if (t0h >= 0) {
          mgos_gpio_write(gpio, 1);
          delay_fn(t0h);
        }
        if (t0l >= 0) {
          mgos_gpio_write(gpio, 0);
          delay_fn(t0l);
        }
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

void mgos_bitbang_write_bits(int gpio, enum mgos_delay_unit delay_unit, int t0h,
                             int t0l, int t1h, int t1l, const uint8_t *data,
                             size_t len) {
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
      t0h -= mgos_bitbang_n100_cal;
      if (t0h < 0) t0h = 0;
      t0l -= mgos_bitbang_n100_cal;
      if (t0l < 0) t0l = 0;
      t1h -= mgos_bitbang_n100_cal;
      if (t1h < 0) t1h = 0;
      t1l -= mgos_bitbang_n100_cal;
      if (t1l < 0) t1l = 0;
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
