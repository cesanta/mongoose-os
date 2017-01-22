/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * I2C implementation using GPIO (bit-banging).
 */

#if MGOS_ENABLE_I2C && MGOS_ENABLE_I2C_GPIO

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_i2c.h"
#include "fw/src/mgos_sys_config.h"

#include "common/cs_dbg.h"

struct mgos_i2c {
  int sda_gpio;
  int scl_gpio;
  bool started;
};

enum i2c_gpio_val {
  I2C_LOW = 0,
  I2C_HIGH = 1,
  I2C_INPUT = 2,
};

/* This function delays for half of a SCL pulse, i.e. quarter of a period. */
static void mgos_i2c_half_delay(struct mgos_i2c *c) {
  (void) c;
  /* This is ~50 KHz. TODO(rojer): Make speed configurable. */
  mgos_usleep(3);
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

enum i2c_ack_type mgos_i2c_start(struct mgos_i2c *c, uint16_t addr,
                                 enum i2c_rw mode) {
  enum i2c_ack_type result;
  uint8_t address_byte = (uint8_t)(addr << 1) | mode;
  if (get_cfg()->i2c.debug) {
    LOG(LL_DEBUG,
        ("%d %d, addr 0x%02x, mode %c => ab 0x%02x", c->sda_gpio, c->scl_gpio,
         addr, (mode == I2C_READ ? 'R' : 'W'), address_byte));
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
  if (get_cfg()->i2c.debug) {
    LOG(LL_DEBUG, ("stopped"));
  }
}

static uint8_t mgos_i2c_read_sda(struct mgos_i2c *c) {
  return mgos_gpio_read(c->sda_gpio);
}

enum i2c_ack_type mgos_i2c_send_byte(struct mgos_i2c *c, uint8_t data) {
  enum i2c_ack_type ret_val;
  int8_t i;

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
  ret_val = mgos_i2c_read_sda(c);
  if (get_cfg()->i2c.debug) {
    LOG(LL_DEBUG,
        ("sent 0x%02x, got %s", data, (ret_val == I2C_ACK ? "ACK" : "NAK")));
  }
  mgos_i2c_half_delay(c);
  mgos_i2c_set_sda_scl(c, I2C_INPUT, I2C_LOW);
  mgos_i2c_half_delay(c);

  return ret_val;
}

void mgos_i2c_send_ack(struct mgos_i2c *c, enum i2c_ack_type ack_type) {
  mgos_i2c_set_sda_scl(c, ack_type, I2C_LOW);
  mgos_i2c_half_delay(c);
  mgos_i2c_set_sda_scl(c, ack_type, I2C_HIGH);
  mgos_i2c_half_delay(c);
  mgos_i2c_half_delay(c);
  mgos_i2c_set_sda_scl(c, ack_type, I2C_LOW);
  mgos_i2c_half_delay(c);
  if (get_cfg()->i2c.debug) {
    LOG(LL_DEBUG, ("sent %s", (ack_type == I2C_ACK ? "ACK" : "NAK")));
  }
}

uint8_t mgos_i2c_read_byte(struct mgos_i2c *c, enum i2c_ack_type ack_type) {
  uint8_t i, ret_val = 0;

  mgos_i2c_set_sda_scl(c, I2C_INPUT, I2C_LOW);
  mgos_i2c_half_delay(c);

  for (i = 0; i < 8; i++) {
    uint8_t bit;
    mgos_i2c_set_sda_scl(c, I2C_INPUT, I2C_HIGH);
    mgos_i2c_half_delay(c);
    bit = mgos_i2c_read_sda(c);
    ret_val |= (bit << (7 - i));
    mgos_i2c_half_delay(c);
    mgos_i2c_set_sda_scl(c, I2C_INPUT, I2C_LOW);
    mgos_i2c_half_delay(c);
  }
  if (get_cfg()->i2c.debug) {
    LOG(LL_DEBUG, ("read 0x%02x", ret_val));
  }

  if (ack_type != I2C_NONE) {
    mgos_i2c_send_ack(c, ack_type);
  } else if (get_cfg()->i2c.debug) {
    LOG(LL_DEBUG, ("not sending any ack"));
  }

  return ret_val;
}

struct mgos_i2c *mgos_i2c_create(const struct sys_config_i2c *cfg) {
  struct mgos_i2c *c = NULL;

  c = calloc(1, sizeof(*c));
  if (c == NULL) return NULL;

  c->sda_gpio = cfg->sda_gpio;
  c->scl_gpio = cfg->scl_gpio;
  c->started = false;

  mgos_i2c_set_sda_scl(c, I2C_INPUT, I2C_INPUT);
  if (!mgos_gpio_set_mode(
          c->sda_gpio,
          MGOS_GPIO_MODE_INPUT ||
              !mgos_gpio_set_pull(c->sda_gpio, MGOS_GPIO_PULL_UP))) {
    goto out_err;
  }
  if (!mgos_gpio_set_mode(c->scl_gpio, MGOS_GPIO_MODE_INPUT) ||
      !mgos_gpio_set_pull(c->scl_gpio, MGOS_GPIO_PULL_UP)) {
    goto out_err;
  }
  mgos_i2c_half_delay(c);

  LOG(LL_INFO,
      ("I2C initialized (SDA: %d, SCL: %d)", c->sda_gpio, c->scl_gpio));

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

#endif /* MGOS_ENABLE_I2C && MGOS_ENABLE_I2C_GPIO */
