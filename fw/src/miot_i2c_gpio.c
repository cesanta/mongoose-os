/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * I2C implementation using GPIO (bit-banging).
 */

#if MIOT_ENABLE_I2C && MIOT_ENABLE_I2C_GPIO

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "fw/src/miot_gpio.h"
#include "fw/src/miot_hal.h"
#include "fw/src/miot_i2c.h"
#include "fw/src/miot_sys_config.h"

#include "common/cs_dbg.h"

#if MIOT_ENABLE_JS
#include "v7/v7.h"
#endif

struct miot_i2c {
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
static void miot_i2c_half_delay(struct miot_i2c *c) {
  (void) c;
  /* This is ~50 KHz. TODO(rojer): Make speed configurable. */
  miot_usleep(3);
}

static void set_gpio_val(int pin, uint8_t val) {
  switch (val) {
    case I2C_LOW:
    case I2C_HIGH:
      miot_gpio_set_mode(pin, MIOT_GPIO_MODE_OUTPUT);
      miot_gpio_write(pin, (val == I2C_HIGH));
      break;
    case I2C_INPUT:
      miot_gpio_set_mode(pin, MIOT_GPIO_MODE_INPUT);
      break;
  }
}

static void miot_i2c_set_sda_scl(struct miot_i2c *c, uint8_t sda_val,
                                 uint8_t scl_val) {
  set_gpio_val(c->sda_gpio, sda_val);
  set_gpio_val(c->scl_gpio, scl_val);
}

enum i2c_ack_type miot_i2c_start(struct miot_i2c *c, uint16_t addr,
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
  miot_i2c_set_sda_scl(c, I2C_HIGH, I2C_HIGH);
  miot_i2c_half_delay(c);
  miot_i2c_set_sda_scl(c, I2C_LOW, I2C_HIGH);
  miot_i2c_half_delay(c);
  miot_i2c_set_sda_scl(c, I2C_LOW, I2C_LOW);
  miot_i2c_half_delay(c);
  result = miot_i2c_send_byte(c, address_byte);
  c->started = 1;
  if (result != I2C_ACK) miot_i2c_stop(c);
  return result;
}

void miot_i2c_stop(struct miot_i2c *c) {
  if (!c->started) return;
  miot_i2c_set_sda_scl(c, I2C_LOW, I2C_LOW);
  miot_i2c_half_delay(c);
  miot_i2c_set_sda_scl(c, I2C_LOW, I2C_HIGH);
  miot_i2c_half_delay(c);
  miot_i2c_set_sda_scl(c, I2C_HIGH, I2C_HIGH);
  miot_i2c_set_sda_scl(c, I2C_INPUT, I2C_INPUT);
  miot_i2c_half_delay(c);
  c->started = false;
  if (get_cfg()->i2c.debug) {
    LOG(LL_DEBUG, ("stopped"));
  }
}

static uint8_t miot_i2c_read_sda(struct miot_i2c *c) {
  return miot_gpio_read(c->sda_gpio);
}

enum i2c_ack_type miot_i2c_send_byte(struct miot_i2c *c, uint8_t data) {
  enum i2c_ack_type ret_val;
  int8_t i;

  for (i = 7; i >= 0; i--) {
    int8_t bit = (data >> i) & 1;
    miot_i2c_set_sda_scl(c, bit, I2C_LOW);
    miot_i2c_half_delay(c);
    miot_i2c_set_sda_scl(c, bit, I2C_HIGH);
    miot_i2c_half_delay(c);
    miot_i2c_half_delay(c);
    miot_i2c_set_sda_scl(c, bit, I2C_LOW);
    miot_i2c_half_delay(c);
  }

  /* release the bus for slave to write ack */
  miot_i2c_set_sda_scl(c, I2C_INPUT, I2C_LOW);
  miot_i2c_half_delay(c);
  miot_i2c_set_sda_scl(c, I2C_INPUT, I2C_HIGH);
  miot_i2c_half_delay(c);
  ret_val = miot_i2c_read_sda(c);
  if (get_cfg()->i2c.debug) {
    LOG(LL_DEBUG,
        ("sent 0x%02x, got %s", data, (ret_val == I2C_ACK ? "ACK" : "NAK")));
  }
  miot_i2c_half_delay(c);
  miot_i2c_set_sda_scl(c, I2C_INPUT, I2C_LOW);
  miot_i2c_half_delay(c);

  return ret_val;
}

void miot_i2c_send_ack(struct miot_i2c *c, enum i2c_ack_type ack_type) {
  miot_i2c_set_sda_scl(c, ack_type, I2C_LOW);
  miot_i2c_half_delay(c);
  miot_i2c_set_sda_scl(c, ack_type, I2C_HIGH);
  miot_i2c_half_delay(c);
  miot_i2c_half_delay(c);
  miot_i2c_set_sda_scl(c, ack_type, I2C_LOW);
  miot_i2c_half_delay(c);
  if (get_cfg()->i2c.debug) {
    LOG(LL_DEBUG, ("sent %s", (ack_type == I2C_ACK ? "ACK" : "NAK")));
  }
}

uint8_t miot_i2c_read_byte(struct miot_i2c *c, enum i2c_ack_type ack_type) {
  uint8_t i, ret_val = 0;

  miot_i2c_set_sda_scl(c, I2C_INPUT, I2C_LOW);
  miot_i2c_half_delay(c);

  for (i = 0; i < 8; i++) {
    uint8_t bit;
    miot_i2c_set_sda_scl(c, I2C_INPUT, I2C_HIGH);
    miot_i2c_half_delay(c);
    bit = miot_i2c_read_sda(c);
    ret_val |= (bit << (7 - i));
    miot_i2c_half_delay(c);
    miot_i2c_set_sda_scl(c, I2C_INPUT, I2C_LOW);
    miot_i2c_half_delay(c);
  }
  if (get_cfg()->i2c.debug) {
    LOG(LL_DEBUG, ("read 0x%02x", ret_val));
  }

  if (ack_type != I2C_NONE) {
    miot_i2c_send_ack(c, ack_type);
  } else if (get_cfg()->i2c.debug) {
    LOG(LL_DEBUG, ("not sending any ack"));
  }

  return ret_val;
}

struct miot_i2c *miot_i2c_create(const struct sys_config_i2c *cfg) {
  struct miot_i2c *c = NULL;

  c = calloc(1, sizeof(*c));
  if (c == NULL) return NULL;

  c->sda_gpio = cfg->sda_gpio;
  c->scl_gpio = cfg->scl_gpio;
  c->started = false;

  miot_i2c_set_sda_scl(c, I2C_INPUT, I2C_INPUT);
  if (!miot_gpio_set_mode(
          c->sda_gpio,
          MIOT_GPIO_MODE_INPUT ||
              !miot_gpio_set_pull(c->sda_gpio, MIOT_GPIO_PULL_UP))) {
    goto out_err;
  }
  if (!miot_gpio_set_mode(c->scl_gpio, MIOT_GPIO_MODE_INPUT) ||
      !miot_gpio_set_pull(c->scl_gpio, MIOT_GPIO_PULL_UP)) {
    goto out_err;
  }
  miot_i2c_half_delay(c);

  LOG(LL_INFO,
      ("I2C initialized (SDA: %d, SCL: %d)", c->sda_gpio, c->scl_gpio));

  return c;

out_err:
  free(c);
  LOG(LL_ERROR, ("Invalid I2C GPIO settings"));
  return NULL;
}

void miot_i2c_close(struct miot_i2c *c) {
  if (c->started) miot_i2c_stop(c);
  free(c);
}

#if MIOT_ENABLE_JS && MIOT_ENABLE_I2C_API
enum v7_err miot_i2c_create_js(struct v7 *v7, struct miot_i2c **res) {
  enum v7_err rcode = V7_OK;
  struct sys_config_i2c cfg;
  cfg.sda_gpio = v7_get_double(v7, v7_arg(v7, 0));
  cfg.scl_gpio = v7_get_double(v7, v7_arg(v7, 1));
  struct miot_i2c *conn = miot_i2c_create(&cfg);

  if (conn != NULL) {
    *res = conn;
  } else {
    rcode = v7_throwf(v7, "Error", "Failed to creat I2C connection");
  }

  return rcode;
}
#endif /* MIOT_ENABLE_JS && MIOT_ENABLE_I2C_API */

#endif /* MIOT_ENABLE_I2C && MIOT_ENABLE_I2C_GPIO */
