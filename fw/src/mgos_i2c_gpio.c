/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * I2C implementation using GPIO (bit-banging).
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_i2c.h"

#include "common/cs_dbg.h"

struct mgos_i2c {
  int sda_gpio;
  int scl_gpio;
  unsigned int started : 1;
  unsigned int debug : 1;
};

enum i2c_gpio_val {
  I2C_LOW = 0,
  I2C_HIGH = 1,
  I2C_INPUT = 2,
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
static void mgos_i2c_half_delay(struct mgos_i2c *c) {
  (void) c;
  /* This is ~70 KHz. TODO(rojer): Make speed configurable. */
  mgos_usleep(1);
}

static void set_gpio_val(int pin, uint8_t val) {
  switch (val) {
    case I2C_LOW:
    case I2C_HIGH:
      mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_OUTPUT);
      mgos_gpio_write(pin, (val == I2C_HIGH));
      break;
    case I2C_INPUT:
      mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_INPUT);
      break;
  }
}

static void mgos_i2c_set_sda_scl(struct mgos_i2c *c, uint8_t sda_val,
                                 uint8_t scl_val) {
  set_gpio_val(c->sda_gpio, sda_val);
  set_gpio_val(c->scl_gpio, scl_val);
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
  mgos_i2c_set_sda_scl(c, I2C_HIGH, I2C_HIGH);
  mgos_i2c_half_delay(c);
  mgos_i2c_set_sda_scl(c, I2C_LOW, I2C_HIGH);
  mgos_i2c_half_delay(c);
  mgos_i2c_set_sda_scl(c, I2C_LOW, I2C_LOW);
  mgos_i2c_half_delay(c);
  result = mgos_i2c_send_byte(c, address_byte);
  c->started = 1;
  if (result != I2C_ACK) mgos_i2c_stop(c);
  return result;
}

void mgos_i2c_stop(struct mgos_i2c *c) {
  if (!c->started) return;
  mgos_i2c_set_sda_scl(c, I2C_LOW, I2C_LOW);
  mgos_i2c_half_delay(c);
  mgos_i2c_set_sda_scl(c, I2C_LOW, I2C_HIGH);
  mgos_i2c_half_delay(c);
  mgos_i2c_set_sda_scl(c, I2C_HIGH, I2C_HIGH);
  mgos_i2c_set_sda_scl(c, I2C_INPUT, I2C_INPUT);
  mgos_i2c_half_delay(c);
  c->started = false;
  if (c->debug) {
    LOG(LL_DEBUG, (" stop"));
  }
}

static enum i2c_ack_type mgos_i2c_send_byte(struct mgos_i2c *c, uint8_t data) {
  enum i2c_ack_type ret_val;
  int8_t i;

  mgos_ints_disable();
  for (i = 7; i >= 0; i--) {
    int8_t bit = (data >> i) & 1;
    mgos_i2c_set_sda_scl(c, bit, I2C_LOW);
    mgos_i2c_half_delay(c);
    mgos_i2c_set_sda_scl(c, bit, I2C_HIGH);
    mgos_i2c_half_delay(c);
    mgos_i2c_half_delay(c);
    mgos_i2c_set_sda_scl(c, bit, I2C_LOW);
    mgos_i2c_half_delay(c);
  }

  /* release the bus for slave to write ack */
  mgos_i2c_set_sda_scl(c, I2C_INPUT, I2C_LOW);
  mgos_i2c_half_delay(c);
  mgos_i2c_set_sda_scl(c, I2C_INPUT, I2C_HIGH);
  mgos_i2c_half_delay(c);
  ret_val = mgos_gpio_read(c->sda_gpio);
  mgos_ints_enable();
  if (c->debug) {
    LOG(LL_DEBUG,
        (" sent 0x%02x, recd %s", data, (ret_val == I2C_ACK ? "ACK" : "NAK")));
  }
  mgos_i2c_half_delay(c);
  mgos_i2c_set_sda_scl(c, I2C_INPUT, I2C_LOW);
  mgos_i2c_half_delay(c);

  return ret_val;
}

static void mgos_i2c_send_ack(struct mgos_i2c *c, enum i2c_ack_type ack_type) {
  mgos_ints_disable();
  mgos_i2c_set_sda_scl(c, ack_type, I2C_LOW);
  mgos_i2c_half_delay(c);
  mgos_i2c_set_sda_scl(c, ack_type, I2C_HIGH);
  mgos_i2c_half_delay(c);
  mgos_i2c_half_delay(c);
  mgos_i2c_set_sda_scl(c, ack_type, I2C_LOW);
  mgos_i2c_half_delay(c);
  mgos_ints_enable();
}

static uint8_t mgos_i2c_read_byte(struct mgos_i2c *c,
                                  enum i2c_ack_type ack_type) {
  uint8_t i, ret_val = 0;

  mgos_ints_disable();
  mgos_i2c_set_sda_scl(c, I2C_INPUT, I2C_LOW);
  mgos_i2c_half_delay(c);

  for (i = 0; i < 8; i++) {
    uint8_t bit;
    mgos_i2c_set_sda_scl(c, I2C_INPUT, I2C_HIGH);
    mgos_i2c_half_delay(c);
    bit = mgos_gpio_read(c->sda_gpio);
    ret_val |= (bit << (7 - i));
    mgos_i2c_half_delay(c);
    mgos_i2c_set_sda_scl(c, I2C_INPUT, I2C_LOW);
    mgos_i2c_half_delay(c);
  }
  mgos_i2c_send_ack(c, ack_type);
  mgos_ints_enable();
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
  (void) c;
  return MGOS_I2C_FREQ_100KHZ;
}

bool mgos_i2c_set_freq(struct mgos_i2c *c, int freq) {
  (void) c;
  return (freq == MGOS_I2C_FREQ_100KHZ);
}

struct mgos_i2c *mgos_i2c_create(const struct sys_config_i2c *cfg) {
  struct mgos_i2c *c = NULL;

  c = calloc(1, sizeof(*c));
  if (c == NULL) return NULL;

  c->sda_gpio = cfg->sda_gpio;
  c->scl_gpio = cfg->scl_gpio;
  c->started = false;
  c->debug = cfg->debug;

  /* We can barely do 100 KHz, sort of. */
  if (cfg->freq != MGOS_I2C_FREQ_100KHZ) {
    goto out_err;
  }

  if (!mgos_gpio_set_mode(c->sda_gpio, MGOS_GPIO_MODE_INPUT) ||
      !mgos_gpio_set_pull(c->sda_gpio, MGOS_GPIO_PULL_UP)) {
    goto out_err;
  }
  if (!mgos_gpio_set_mode(c->scl_gpio, MGOS_GPIO_MODE_INPUT) ||
      !mgos_gpio_set_pull(c->scl_gpio, MGOS_GPIO_PULL_UP)) {
    goto out_err;
  }

  LOG(LL_INFO,
      ("I2C GPIO init ok (SDA: %d, SCL: %d)", c->sda_gpio, c->scl_gpio));

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
