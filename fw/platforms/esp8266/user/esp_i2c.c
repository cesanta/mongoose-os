/*
 * Copyright 2015 Cesanta Software
 *
 * I2C low-level API
 */

#include "fw/platforms/esp8266/user/esp_features.h"

#if MIOT_ENABLE_I2C

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <ets_sys.h>

#include "fw/src/miot_i2c.h"
#include "fw/src/miot_sys_config.h"

#include "common/cs_dbg.h"

#include "fw/platforms/esp8266/user/esp_gpio.h"
#include "fw/platforms/esp8266/user/esp_periph.h"
#include "common/platforms/esp8266/esp_missing_includes.h"

#include <osapi.h>
#include <gpio.h>

#if MIOT_ENABLE_JS
#include "v7/v7.h"
#endif

struct miot_i2c {
  /* GPIO used as SDA */
  uint8_t sda_gpio;

  /* GPIO used as SCL */
  uint8_t scl_gpio;

  bool started;
};

enum i2c_gpio_val {
  I2C_LOW = 0,
  I2C_HIGH = 1,
  I2C_INPUT = 2,
};

static void i2c_gpio_val_to_masks(uint8_t gpio, uint8_t val, uint32_t *set_mask,
                                  uint32_t *clear_mask,
                                  uint32_t *output_enable_mask,
                                  uint32_t *output_disable_mask) {
  uint32_t gpio_mask = 1 << gpio;
  if (val == I2C_LOW || val == I2C_HIGH) {
    *output_enable_mask |= gpio_mask;
    *set_mask |= val == 1 ? gpio_mask : 0;
    *clear_mask |= val == 0 ? gpio_mask : 0;
  } else if (val == I2C_INPUT) {
    *output_disable_mask |= gpio_mask;
  } /* else no change */
}

/* This function delays for half of a SCL pulse, i.e. quarter of a period. */
static void esp_i2c_half_delay(struct miot_i2c *c) {
  (void) c;
  /* This is ~50 KHz. TODO(rojer): Make speed configurable. */
  os_delay_us(3);
}

static void esp_i2c_set_sda_scl(struct miot_i2c *c, uint8_t sda_val,
                                uint8_t scl_val) {
  uint32_t set_mask = 0, clear_mask = 0;
  uint32_t output_enable_mask = 0, output_disable_mask = 0;

  i2c_gpio_val_to_masks(c->sda_gpio, sda_val, &set_mask, &clear_mask,
                        &output_enable_mask, &output_disable_mask);
  i2c_gpio_val_to_masks(c->scl_gpio, scl_val, &set_mask, &clear_mask,
                        &output_enable_mask, &output_disable_mask);

  gpio_output_set(set_mask, clear_mask, output_enable_mask,
                  output_disable_mask);
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
  esp_i2c_set_sda_scl(c, I2C_HIGH, I2C_HIGH);
  esp_i2c_half_delay(c);
  esp_i2c_set_sda_scl(c, I2C_LOW, I2C_HIGH);
  esp_i2c_half_delay(c);
  esp_i2c_set_sda_scl(c, I2C_LOW, I2C_LOW);
  esp_i2c_half_delay(c);
  result = miot_i2c_send_byte(c, address_byte);
  c->started = 1;
  if (result != I2C_ACK) miot_i2c_stop(c);
  return result;
}

void miot_i2c_stop(struct miot_i2c *c) {
  if (!c->started) return;
  esp_i2c_set_sda_scl(c, I2C_LOW, I2C_LOW);
  esp_i2c_half_delay(c);
  esp_i2c_set_sda_scl(c, I2C_LOW, I2C_HIGH);
  esp_i2c_half_delay(c);
  esp_i2c_set_sda_scl(c, I2C_HIGH, I2C_HIGH);
  esp_i2c_set_sda_scl(c, I2C_INPUT, I2C_INPUT);
  esp_i2c_half_delay(c);
  c->started = false;
  if (get_cfg()->i2c.debug) {
    LOG(LL_DEBUG, ("stopped"));
  }
}

static uint8_t esp_i2c_read_sda(struct miot_i2c *c) {
  uint8_t ret_val = (gpio_input_get() & (1 << c->sda_gpio)) != 0;
  return ret_val;
}

enum i2c_ack_type miot_i2c_send_byte(struct miot_i2c *c, uint8_t data) {
  enum i2c_ack_type ret_val;
  int8_t i;

  for (i = 7; i >= 0; i--) {
    int8_t bit = (data >> i) & 1;
    esp_i2c_set_sda_scl(c, bit, I2C_LOW);
    esp_i2c_half_delay(c);
    esp_i2c_set_sda_scl(c, bit, I2C_HIGH);
    esp_i2c_half_delay(c);
    esp_i2c_half_delay(c);
    esp_i2c_set_sda_scl(c, bit, I2C_LOW);
    esp_i2c_half_delay(c);
  }

  /* release the bus for slave to write ack */
  esp_i2c_set_sda_scl(c, I2C_INPUT, I2C_LOW);
  esp_i2c_half_delay(c);
  esp_i2c_set_sda_scl(c, I2C_INPUT, I2C_HIGH);
  esp_i2c_half_delay(c);
  ret_val = esp_i2c_read_sda(c);
  if (get_cfg()->i2c.debug) {
    LOG(LL_DEBUG,
        ("sent 0x%02x, got %s", data, (ret_val == I2C_ACK ? "ACK" : "NAK")));
  }
  esp_i2c_half_delay(c);
  esp_i2c_set_sda_scl(c, I2C_INPUT, I2C_LOW);
  esp_i2c_half_delay(c);

  return ret_val;
}

void miot_i2c_send_ack(struct miot_i2c *c, enum i2c_ack_type ack_type) {
  esp_i2c_set_sda_scl(c, ack_type, I2C_LOW);
  esp_i2c_half_delay(c);
  esp_i2c_set_sda_scl(c, ack_type, I2C_HIGH);
  esp_i2c_half_delay(c);
  esp_i2c_half_delay(c);
  esp_i2c_set_sda_scl(c, ack_type, I2C_LOW);
  esp_i2c_half_delay(c);
  if (get_cfg()->i2c.debug) {
    LOG(LL_DEBUG, ("sent %s", (ack_type == I2C_ACK ? "ACK" : "NAK")));
  }
}

uint8_t miot_i2c_read_byte(struct miot_i2c *c, enum i2c_ack_type ack_type) {
  uint8_t i, ret_val = 0;

  esp_i2c_set_sda_scl(c, I2C_INPUT, I2C_LOW);
  esp_i2c_half_delay(c);

  for (i = 0; i < 8; i++) {
    uint8_t bit;
    esp_i2c_set_sda_scl(c, I2C_INPUT, I2C_HIGH);
    esp_i2c_half_delay(c);
    bit = esp_i2c_read_sda(c);
    ret_val |= (bit << (7 - i));
    esp_i2c_half_delay(c);
    esp_i2c_set_sda_scl(c, I2C_INPUT, I2C_LOW);
    esp_i2c_half_delay(c);
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
  if (cfg->sda_gpio < 0 || cfg->sda_gpio > 16 || cfg->scl_gpio < 0 ||
      cfg->scl_gpio > 16) {
    goto out_err;
  }

  c = calloc(1, sizeof(*c));
  if (c == NULL) return NULL;

  c->sda_gpio = cfg->sda_gpio;
  c->scl_gpio = cfg->scl_gpio;
  c->started = false;

  ENTER_CRITICAL(ETS_GPIO_INUM);
  esp_i2c_set_sda_scl(c, I2C_INPUT, I2C_INPUT);
  if (miot_gpio_set_mode(c->sda_gpio, GPIO_MODE_INPUT, GPIO_PULL_PULLUP) < 0) {
    EXIT_CRITICAL(ETS_GPIO_INUM);
    goto out_err;
  }
  if (miot_gpio_set_mode(c->scl_gpio, GPIO_MODE_INPUT, GPIO_PULL_PULLUP) < 0) {
    EXIT_CRITICAL(ETS_GPIO_INUM);
    goto out_err;
  }
  EXIT_CRITICAL(ETS_GPIO_INUM);
  esp_i2c_half_delay(c);

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

#endif /* MIOT_ENABLE_I2C */
