/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_intr.h"

#include "fw/src/miot_gpio.h"
#include "fw/src/miot_gpio_hal.h"
#include "fw/src/miot_hal.h"

#include "soc/gpio_struct.h"
#include "driver/gpio.h"

#include "common/cs_dbg.h"

#if MIOT_NUM_GPIO != GPIO_PIN_COUNT
#error MIOT_NUM_GPIO must match GPIO_PIN_COUNT
#endif

gpio_isr_handle_t s_int_handle;
uint8_t s_int_ena[MIOT_NUM_GPIO];

/* Invoked by SDK, runs in ISR context. */
IRAM static void esp32_gpio_isr(void *arg) {
  uint32_t int_st = GPIO.status;
  for (uint32_t i = 0, mask = 1; i < 32; i++, mask <<= 1) {
    if (s_int_ena[i] == 0 || !(int_st & mask)) continue;
    GPIO.pin[i].int_ena = 0;
    miot_gpio_dev_int_cb(i);
  }
  GPIO.status_w1tc = int_st;
  int_st = GPIO.status1.intr_st;
  for (uint32_t i = 32, mask = 1; i < MIOT_NUM_GPIO; i++, mask <<= 1) {
    if (s_int_ena[i] == 0 || !(int_st & mask)) continue;
    GPIO.pin[i].int_ena = 0;
    miot_gpio_dev_int_cb(i);
  }
  GPIO.status1_w1tc.intr_st = int_st;
}

IRAM void miot_gpio_dev_int_done(int pin) {
  GPIO.pin[pin].int_ena = s_int_ena[pin];
}

bool miot_gpio_set_mode(int pin, enum miot_gpio_mode mode) {
  gpio_mode_t m;
  switch (mode) {
    case MIOT_GPIO_MODE_INPUT:
      m = GPIO_MODE_INPUT;
      break;
    case MIOT_GPIO_MODE_OUTPUT:
      m = GPIO_MODE_OUTPUT;
      break;
    default:
      return false;
  }
  if (gpio_set_direction(pin, m) != ESP_OK) {
    return false;
  }

  uint32_t reg = GPIO_PIN_MUX_REG[pin];
  if (reg != 0) PIN_FUNC_SELECT(reg, PIN_FUNC_GPIO);

  return true;
}

bool miot_gpio_set_pull(int pin, enum miot_gpio_pull_type pull) {
  gpio_pull_mode_t pm;
  switch (pull) {
    case MIOT_GPIO_PULL_NONE:
      pm = GPIO_FLOATING;
      break;
    case MIOT_GPIO_PULL_UP:
      pm = GPIO_PULLUP_ONLY;
      break;
    case MIOT_GPIO_PULL_DOWN:
      pm = GPIO_PULLDOWN_ONLY;
      break;
    default:
      return false;
  }
  return (gpio_set_pull_mode(pin, pm) == ESP_OK);
}

bool miot_gpio_read(int pin) {
  return gpio_get_level(pin);
}

void miot_gpio_write(int pin, bool level) {
  gpio_set_level(pin, level);
}

bool miot_gpio_toggle(int pin) {
  uint32_t cur_out;
  if (pin > 0 && pin < 32) {
    cur_out = (GPIO.out & (1U << pin));
  } else if (pin > 32 && pin < MIOT_NUM_GPIO) {
    cur_out = (GPIO.out1.val & (1U << (pin - 32)));
  } else {
    return false;
  }
  gpio_set_level(pin, !cur_out);
  return !cur_out;
}

bool miot_gpio_dev_set_int_mode(int pin, enum miot_gpio_int_mode mode) {
  gpio_int_type_t it;
  switch (mode) {
    case MIOT_GPIO_INT_NONE:
      gpio_intr_disable(pin);
      it = GPIO_INTR_DISABLE;
      break;
    case MIOT_GPIO_INT_EDGE_POS:
      it = GPIO_INTR_POSEDGE;
      break;
    case MIOT_GPIO_INT_EDGE_NEG:
      it = GPIO_INTR_NEGEDGE;
      break;
    case MIOT_GPIO_INT_EDGE_ANY:
      it = GPIO_INTR_ANYEDGE;
      break;
    case MIOT_GPIO_INT_LEVEL_HI:
      it = GPIO_INTR_HIGH_LEVEL;
      break;
    case MIOT_GPIO_INT_LEVEL_LO:
      it = GPIO_INTR_LOW_LEVEL;
      break;
    default:
      return false;
  }
  return (gpio_set_intr_type(pin, it) == ESP_OK);
}

bool miot_gpio_enable_int(int pin) {
  esp_intr_disable(s_int_handle);
  esp_err_t r = gpio_intr_enable(pin);
  if (r == ESP_OK) {
    s_int_ena[pin] = GPIO.pin[pin].int_ena;
  }
  esp_intr_enable(s_int_handle);
  return (r == ESP_OK);
}

bool miot_gpio_disable_int(int pin) {
  if (gpio_intr_disable(pin) != ESP_OK) return false;
  s_int_ena[pin] = 0;
  return true;
}

enum miot_init_result miot_gpio_dev_init() {
  esp_err_t r = gpio_isr_register(esp32_gpio_isr, NULL, 0, &s_int_handle);
  if (r != ESP_OK) return MIOT_INIT_GPIO_INIT_FAILED;
  r = esp_intr_enable(s_int_handle);
  if (r != ESP_OK) return MIOT_INIT_GPIO_INIT_FAILED;
  return MIOT_INIT_OK;
}
