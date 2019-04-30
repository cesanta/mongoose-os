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

#include "mgos_i2c.h"

#include "mgos_gpio.h"
#include "mgos_hal.h"

#ifndef MGOS_SYS_CONFIG_HAVE_I2C1
#define NUM_BUSES 1
#else
#define NUM_BUSES 2
#endif

static struct mgos_i2c *s_buses[NUM_BUSES];

int mgos_i2c_read_reg_b(struct mgos_i2c *conn, uint16_t addr, uint8_t reg) {
  uint8_t value;
  if (!mgos_i2c_read_reg_n(conn, addr, reg, 1, &value)) {
    return -1;
  }
  return value;
}

int mgos_i2c_read_reg_w(struct mgos_i2c *conn, uint16_t addr, uint8_t reg) {
  uint8_t tmp[2];
  if (!mgos_i2c_read_reg_n(conn, addr, reg, 2, tmp)) {
    return -1;
  }
  return (((uint16_t) tmp[0]) << 8) | tmp[1];
}

bool mgos_i2c_read_reg_n(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                         size_t n, uint8_t *buf) {
  return mgos_i2c_write(conn, addr, &reg, 1, false /* stop */) &&
         mgos_i2c_read(conn, addr, buf, n, true /* stop */);
}

bool mgos_i2c_write_reg_b(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                          uint8_t value) {
  uint8_t tmp[2] = {reg, value};
  return mgos_i2c_write(conn, addr, tmp, sizeof(tmp), true /* stop */);
}

bool mgos_i2c_write_reg_w(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                          uint16_t value) {
  uint8_t tmp[3] = {reg, (uint8_t)(value >> 8), (uint8_t) value};
  return mgos_i2c_write(conn, addr, tmp, sizeof(tmp), true /* stop */);
}

bool mgos_i2c_write_reg_n(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                          size_t n, const uint8_t *buf) {
  bool res = false;
  uint8_t *tmp = calloc(n + 1, 1);
  if (tmp) {
    *tmp = reg;
    memcpy(tmp + 1, buf, n);
    res = mgos_i2c_write(conn, addr, tmp, n + 1, true /* stop */);
    free(tmp);
  }
  return res;
}

bool mgos_i2c_init(void) {
  if (mgos_sys_config_get_i2c_enable()) {
    s_buses[0] = mgos_i2c_create(mgos_sys_config_get_i2c());
    if (s_buses[0] == NULL) return false;
  }
#ifdef MGOS_SYS_CONFIG_HAVE_I2C1
  if (mgos_sys_config_get_i2c1_enable()) {
    s_buses[1] = mgos_i2c_create(mgos_sys_config_get_i2c1());
    if (s_buses[1] == NULL) return false;
  }
#endif
  return true;
}

struct mgos_i2c *mgos_i2c_get_bus(int bus_no) {
  if (bus_no < 0 || bus_no >= (int) ARRAY_SIZE(s_buses)) return NULL;
  return s_buses[bus_no];
}

struct mgos_i2c *mgos_i2c_get_global(void) {
  return mgos_i2c_get_bus(0);
}

#define HALF_DELAY() (mgos_nsleep100)(100 / 2); /* 100 KHz */

bool mgos_i2c_reset_bus(int sda_gpio, int scl_gpio) {
  if (!mgos_gpio_setup_output(sda_gpio, 1) ||
      !mgos_gpio_set_pull(sda_gpio, MGOS_GPIO_PULL_UP) ||
      !mgos_gpio_set_mode(sda_gpio, MGOS_GPIO_MODE_OUTPUT_OD)) {
    return false;
  }
  if (!mgos_gpio_setup_output(scl_gpio, 1) ||
      !mgos_gpio_set_pull(scl_gpio, MGOS_GPIO_PULL_UP) ||
      !mgos_gpio_set_mode(scl_gpio, MGOS_GPIO_MODE_OUTPUT_OD)) {
    return false;
  }

  /*
   * Send some dummy clocks to reset the bus.
   * https://www.i2c-bus.org/i2c-primer/analysing-obscure-problems/blocked-bus/
   */
  HALF_DELAY();
  HALF_DELAY();
  for (int i = 0; i < 16; i++) {
    mgos_gpio_write(scl_gpio, 0);
    HALF_DELAY();
    mgos_gpio_write(sda_gpio, 0);
    HALF_DELAY();
    mgos_gpio_write(scl_gpio, 1);
    HALF_DELAY();
    HALF_DELAY();
  }
  /* STOP condition. */
  mgos_gpio_write(sda_gpio, 1);
  HALF_DELAY();
  return true;
}

bool mgos_i2c_setbits_reg_b(struct mgos_i2c *i2c, uint16_t addr, uint8_t reg,
                            uint8_t bitoffset, uint8_t bitlen, uint8_t value) {
  uint8_t old, new;

  if (!i2c || bitoffset + bitlen > 8 || bitlen == 0) {
    return false;
  }

  if (value > (1 << bitlen) - 1) {
    return false;
  }

  if (!mgos_i2c_read_reg_n(i2c, addr, reg, 1, &old)) {
    return false;
  }

  new = old | (((1 << bitlen) - 1) << bitoffset);
  new &= ~(((1 << bitlen) - 1) << bitoffset);
  new |= (value) << bitoffset;

  return mgos_i2c_write_reg_n(i2c, addr, reg, 1, &new);
}

bool mgos_i2c_getbits_reg_b(struct mgos_i2c *i2c, uint16_t addr, uint8_t reg,
                            uint8_t bitoffset, uint8_t bitlen, uint8_t *value) {
  uint8_t val, mask;

  if (!i2c || bitoffset + bitlen > 8 || bitlen == 0 || !value) {
    return false;
  }

  if (!mgos_i2c_read_reg_n(i2c, addr, reg, 1, &val)) {
    return false;
  }

  mask = ((1 << bitlen) - 1);
  mask <<= bitoffset;
  val &= mask;
  val >>= bitoffset;

  *value = val;
  return true;
}

bool mgos_i2c_setbits_reg_w(struct mgos_i2c *i2c, uint16_t addr, uint8_t reg,
                            uint8_t bitoffset, uint8_t bitlen, uint16_t value) {
  uint16_t old, new;
  uint8_t d[2];

  if (!i2c || bitoffset + bitlen > 16 || bitlen == 0) {
    return false;
  }

  if (value > (1 << bitlen) - 1) {
    return false;
  }

  if (!mgos_i2c_read_reg_n(i2c, addr, reg, 2, d)) {
    return false;
  }

  old = (d[0] << 8) | d[1];
  new = old | (((1 << bitlen) - 1) << bitoffset);
  new &= ~(((1 << bitlen) - 1) << bitoffset);
  new |= (value) << bitoffset;
  d[0] = new << 8;
  d[1] = new & 0xFF;

  return mgos_i2c_write_reg_n(i2c, addr, reg, 2, (uint8_t *) &new);
}

bool mgos_i2c_getbits_reg_w(struct mgos_i2c *i2c, uint16_t addr, uint8_t reg,
                            uint8_t bitoffset, uint8_t bitlen,
                            uint16_t *value) {
  uint16_t val, mask;
  uint8_t d[2];

  if (!i2c || bitoffset + bitlen > 16 || bitlen == 0 || !value) {
    return false;
  }

  if (!mgos_i2c_read_reg_n(i2c, addr, reg, 2, d)) {
    return false;
  }

  val = (d[0] << 8) | d[1];
  mask = ((1 << bitlen) - 1);
  mask <<= bitoffset;
  val &= mask;
  val >>= bitoffset;

  *value = val;
  return true;
}
