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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "mgos_i2c.h"

#include "mgos_gpio.h"
#include "mgos_hal.h"

#include "common/cs_dbg.h"

struct mgos_i2c {
  int sda_gpio;
  int scl_gpio;
  int freq, half_delay_n100;
  bool started;
  bool debug;
};

enum i2c_ack_type {
  I2C_ACK = 0,
  I2C_NAK = 1,
  I2C_ERR = 2,
};

enum i2c_rw {
  I2C_READ = 1,
  I2C_WRITE = 0,
};

/* This function delays for half of a SCL pulse, i.e. quarter of a period. */
static inline void mgos_i2c_half_delay(struct mgos_i2c *c) {
  (mgos_nsleep100)(c->half_delay_n100);
}

static enum i2c_ack_type mgos_i2c_send_byte(struct mgos_i2c *c, uint8_t data);

static enum i2c_ack_type mgos_i2c_start(struct mgos_i2c *c, uint16_t addr,
                                        enum i2c_rw mode) {
  enum i2c_ack_type result;
  uint8_t address_byte = (uint8_t)(addr << 1) | mode;
  if (c->debug) {
    LOG(LL_DEBUG, (" addr 0x%02x, mode %c => ab 0x%02x", addr,
                   (mode == I2C_READ ? 'R' : 'W'), address_byte));
  }
  if (addr > 0x7F || (mode != I2C_READ && mode != I2C_WRITE)) {
    return I2C_ERR;
  }
  mgos_gpio_write(c->sda_gpio, 1);
  mgos_gpio_write(c->scl_gpio, 1);
  mgos_i2c_half_delay(c);
  mgos_gpio_write(c->sda_gpio, 0);
  mgos_i2c_half_delay(c);
  mgos_gpio_write(c->scl_gpio, 0);
  mgos_i2c_half_delay(c);
  result = mgos_i2c_send_byte(c, address_byte);
  c->started = 1;
  if (result != I2C_ACK) mgos_i2c_stop(c);
  return result;
}

void mgos_i2c_stop(struct mgos_i2c *c) {
  if (!c->started) return;
  mgos_i2c_half_delay(c);
  mgos_gpio_write(c->scl_gpio, 1);
  mgos_i2c_half_delay(c);
  mgos_gpio_write(c->sda_gpio, 1);
  mgos_i2c_half_delay(c);
  c->started = false;
  if (c->debug) {
    LOG(LL_DEBUG, (" stop"));
  }
}

static enum i2c_ack_type mgos_i2c_send_byte(struct mgos_i2c *c, uint8_t data) {
  enum i2c_ack_type ret_val;
  int i, bit;

  mgos_gpio_write(c->scl_gpio, 0);
  mgos_i2c_half_delay(c);
  for (i = 0; i < 8; i++) {
    bit = (data & (1 << (7 - i))) ? 1 : 0;
    mgos_gpio_write(c->sda_gpio, bit);
    mgos_gpio_write(c->scl_gpio, 1);
    mgos_i2c_half_delay(c);
    mgos_gpio_write(c->scl_gpio, 0);
    mgos_i2c_half_delay(c);
  }
  /* release the bus for slave to write ack */
  mgos_gpio_write(c->sda_gpio, 1);
  mgos_gpio_set_mode(c->sda_gpio, MGOS_GPIO_MODE_INPUT);
  mgos_gpio_write(c->scl_gpio, 1);
  mgos_i2c_half_delay(c);
  ret_val = mgos_gpio_read(c->sda_gpio);
  mgos_gpio_write(c->scl_gpio, 0);
  mgos_i2c_half_delay(c);
  mgos_gpio_write(c->sda_gpio, 0);
  mgos_gpio_set_mode(c->sda_gpio, MGOS_GPIO_MODE_OUTPUT_OD);
  if (c->debug) {
    LOG(LL_DEBUG,
        (" sent 0x%02x, recd %s", data, (ret_val == I2C_ACK ? "ACK" : "NAK")));
  }
  return ret_val;
}

static uint8_t mgos_i2c_read_byte(struct mgos_i2c *c,
                                  enum i2c_ack_type ack_type) {
  uint8_t i, ret_val = 0;

  mgos_gpio_write(c->scl_gpio, 0);
  mgos_gpio_set_mode(c->sda_gpio, MGOS_GPIO_MODE_INPUT);
  mgos_i2c_half_delay(c);
  for (i = 0; i < 8; i++) {
    uint8_t bit;
    mgos_gpio_write(c->scl_gpio, 1);
    mgos_i2c_half_delay(c);
    bit = mgos_gpio_read(c->sda_gpio);
    ret_val |= (bit << (7 - i));
    mgos_gpio_write(c->scl_gpio, 0);
    mgos_i2c_half_delay(c);
  }
  mgos_gpio_write(c->sda_gpio, (ack_type == I2C_ACK ? 0 : 1));
  mgos_gpio_set_mode(c->sda_gpio, MGOS_GPIO_MODE_OUTPUT_OD);
  mgos_gpio_write(c->scl_gpio, 1);
  mgos_i2c_half_delay(c);
  mgos_gpio_write(c->scl_gpio, 0);
  mgos_i2c_half_delay(c);
  mgos_gpio_write(c->sda_gpio, 0);
  if (c->debug) {
    LOG(LL_DEBUG, (" recd 0x%02x, sent %s", ret_val,
                   (ack_type == I2C_ACK ? "ACK" : "NAK")));
  }
  return ret_val;
}

bool mgos_i2c_read(struct mgos_i2c *c, uint16_t addr, void *data, size_t len,
                   bool stop) {
  bool res = false;
  bool start = (addr != MGOS_I2C_ADDR_CONTINUE);
  uint8_t *p = (uint8_t *) data;

  if (c->debug) {
    LOG(LL_DEBUG,
        ("read %d from %d, start? %d, stop? %d", len, addr, start, stop));
  }

  if (start && mgos_i2c_start(c, addr, I2C_READ) != I2C_ACK) {
    goto out;
  }

  while (len-- > 0) {
    enum i2c_ack_type ack = (len > 0 || !stop ? I2C_ACK : I2C_NAK);
    *p++ = mgos_i2c_read_byte(c, ack);
  }

  res = true;

out:
  if (stop || !res) mgos_i2c_stop(c);

  return res;
}

bool mgos_i2c_write(struct mgos_i2c *c, uint16_t addr, const void *data,
                    size_t len, bool stop) {
  bool res = false;
  bool start = (addr != MGOS_I2C_ADDR_CONTINUE);
  const uint8_t *p = (const uint8_t *) data;

  if (c->debug) {
    LOG(LL_DEBUG, ("write %d to %d, stop? %d", len, addr, stop));
  }

  if (start && mgos_i2c_start(c, addr, I2C_WRITE) != I2C_ACK) {
    goto out;
  }

  while (len-- > 0) {
    if (mgos_i2c_send_byte(c, *p++) != I2C_ACK) return false;
  }

  res = true;

out:
  if (stop || !res) mgos_i2c_stop(c);

  return res;
}

int mgos_i2c_get_freq(struct mgos_i2c *c) {
  return c->freq;
}

bool mgos_i2c_set_freq(struct mgos_i2c *c, int freq) {
  int half_delay_n100 = 10000000 / freq / 2;
  half_delay_n100 -= 4; /* overhead */
  if (half_delay_n100 <= 0) return false;
  c->freq = freq;
  c->half_delay_n100 = half_delay_n100;
  return true;
}

struct mgos_i2c *mgos_i2c_create(const struct mgos_config_i2c *cfg) {
  struct mgos_i2c *c = NULL;

  c = calloc(1, sizeof(*c));
  if (c == NULL) return NULL;

  c->sda_gpio = cfg->sda_gpio;
  c->scl_gpio = cfg->scl_gpio;
  c->started = false;
  c->debug = cfg->debug;

  if (!mgos_i2c_set_freq(c, cfg->freq)) {
    goto out_err;
  }

  if (!mgos_i2c_reset_bus(c->sda_gpio, c->scl_gpio)) {
    goto out_err;
  }

  char b1[8], b2[8];
  LOG(LL_INFO, ("I2C GPIO init ok (SDA: %s, SCL: %s, freq: %d)",
                mgos_gpio_str(c->sda_gpio, b1), mgos_gpio_str(c->scl_gpio, b2),
                cfg->freq));
  (void) b1;
  (void) b2;

  return c;

out_err:
  free(c);
  LOG(LL_ERROR, ("Invalid I2C GPIO settings"));
  return NULL;
}

void mgos_i2c_close(struct mgos_i2c *c) {
  if (c->started) mgos_i2c_stop(c);
  free(c);
}
