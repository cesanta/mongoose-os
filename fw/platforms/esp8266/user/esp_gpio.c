/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/platforms/esp8266/user/esp_gpio.h"

#include <ets_sys.h>
#include "fw/src/miot_gpio.h"
#include "fw/src/miot_gpio_hal.h"

#include "common/platforms/esp8266/esp_missing_includes.h"
#include "common/cs_dbg.h"
#include "esp_periph.h"

#include <osapi.h>
#include <gpio.h>
#include <os_type.h>
#include <user_interface.h>
#include <stdlib.h>

#if MIOT_NUM_GPIO != GPIO_PIN_COUNT
#error MIOT_NUM_GPIO must match GPIO_PIN_COUNT
#endif

/* These declarations are missing in SDK headers since ~1.0 */
#define PERIPHS_IO_MUX_PULLDWN BIT6
#define PIN_PULLDWN_DIS(PIN_NAME) \
  CLEAR_PERI_REG_MASK(PIN_NAME, PERIPHS_IO_MUX_PULLDWN)
#define PIN_PULLDWN_EN(PIN_NAME) \
  SET_PERI_REG_MASK(PIN_NAME, PERIPHS_IO_MUX_PULLDWN)

#define GPIO_PIN_COUNT 16

static uint8_t s_int_config[GPIO_PIN_COUNT];
#define INT_TYPE_MASK 0x7
#define INT_ENA 0x8

static void gpio16_set_output_mode(void) {
  WRITE_PERI_REG(
      PAD_XPD_DCDC_CONF,
      (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32_t) 0x1);

  WRITE_PERI_REG(
      RTC_GPIO_CONF,
      (READ_PERI_REG(RTC_GPIO_CONF) & (uint32_t) 0xfffffffe) | (uint32_t) 0x0);

  WRITE_PERI_REG(RTC_GPIO_ENABLE,
                 (READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32_t) 0xfffffffe) |
                     (uint32_t) 0x1);
}

static void gpio16_output_set(uint8_t value) {
  WRITE_PERI_REG(RTC_GPIO_OUT,
                 (READ_PERI_REG(RTC_GPIO_OUT) & (uint32_t) 0xfffffffe) |
                     (uint32_t)(value & 1));
}

static void gpio16_set_input_mode(void) {
  WRITE_PERI_REG(
      PAD_XPD_DCDC_CONF,
      (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32_t) 0x1);

  WRITE_PERI_REG(
      RTC_GPIO_CONF,
      (READ_PERI_REG(RTC_GPIO_CONF) & (uint32_t) 0xfffffffe) | (uint32_t) 0x0);

  WRITE_PERI_REG(RTC_GPIO_ENABLE,
                 READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32_t) 0xfffffffe);
}

static uint8_t gpio16_input_get(void) {
  return (uint8_t)(READ_PERI_REG(RTC_GPIO_IN_DATA) & 1);
}

bool miot_gpio_set_mode(int pin, enum miot_gpio_mode mode) {
  if (pin == 16) {
    if (mode == MIOT_GPIO_MODE_INPUT) {
      gpio16_set_input_mode();
    } else {
      gpio16_set_output_mode();
    }
    return true;
  }

  struct gpio_info *gi = get_gpio_info(pin);
  if (gi == NULL) return false;

  switch (mode) {
    case MIOT_GPIO_MODE_INPUT:
      PIN_FUNC_SELECT(gi->periph, gi->func);
      GPIO_REG_WRITE(GPIO_ENABLE_W1TC_ADDRESS, BIT(pin));
      break;
    case MIOT_GPIO_MODE_OUTPUT:
      PIN_FUNC_SELECT(gi->periph, gi->func);
      GPIO_REG_WRITE(GPIO_ENABLE_W1TS_ADDRESS, BIT(pin));
      gpio_pin_intr_state_set(GPIO_ID_PIN(pin), GPIO_PIN_INTR_DISABLE);
      GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(pin));
      GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(pin)),
                     GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(pin))) &
                         (~GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE)));
      break;
    default:
      return false;
  }

  return true;
}

bool miot_gpio_set_pull(int pin, enum miot_gpio_pull_type pull) {
  struct gpio_info *gi = get_gpio_info(pin);
  if (gi == NULL) return false;

  switch (pull) {
    case MIOT_GPIO_PULL_NONE:
      PIN_PULLUP_DIS(gi->periph);
      PIN_PULLDWN_DIS(gi->periph);
      break;
    case MIOT_GPIO_PULL_UP:
      PIN_PULLDWN_DIS(gi->periph);
      PIN_PULLUP_EN(gi->periph);
      break;
    case MIOT_GPIO_PULL_DOWN:
      PIN_PULLUP_DIS(gi->periph);
      PIN_PULLDWN_EN(gi->periph);
      break;
    default:
      return false;
  }

  return true;
}

void miot_gpio_write(int pin, bool level) {
  if (pin < 0 || pin > 16) {
    return;
  } else if (pin == 16) {
    gpio16_output_set(level);
    return;
  }

  if (get_gpio_info(pin) == NULL) {
    /* Just verifying pin number */
    return;
  }

  GPIO_OUTPUT_SET(GPIO_ID_PIN(pin), !!level);
}

bool miot_gpio_read(int pin) {
  if (pin == 16) {
    return 0x1 & gpio16_input_get();
  }

  if (get_gpio_info(pin) == NULL) {
    /* Just verifying pin number */
    return -1;
  }

  return 0x1 & GPIO_INPUT_GET(GPIO_ID_PIN(pin));
}

bool miot_gpio_toggle(int pin) {
  if (pin < 0 || pin > 16) {
    return false;
  } else if (pin == 16) {
    uint32_t v = (READ_PERI_REG(RTC_GPIO_OUT) ^ 1);
    WRITE_PERI_REG(RTC_GPIO_OUT, v);
    return (v & 1);
  }
  uint32_t out = GPIO_REG_READ(GPIO_OUT_ADDRESS);
  uint32_t mask = (1U << pin);
  if (out & (1U << pin)) {
    GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, mask);
    return false;
  } else {
    GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, mask);
    return true;
  }
}

IRAM static void esp8266_gpio_isr(void *arg) {
  uint32_t int_st = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
  for (uint32_t i = 0, mask = 1; i < 16; i++, mask <<= 1) {
    if (!(s_int_config[i] & INT_ENA) || !(int_st & mask)) continue;
    gpio_pin_intr_state_set(i, GPIO_PIN_INTR_DISABLE);
    miot_gpio_dev_int_cb(i);
  }
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, int_st);
  (void) arg;
}

IRAM void miot_gpio_dev_int_done(int pin) {
  if (s_int_config[pin] & INT_ENA) {
    gpio_pin_intr_state_set(pin, (s_int_config[pin] & INT_TYPE_MASK));
  }
}

enum miot_init_result miot_gpio_dev_init(void) {
  ETS_GPIO_INTR_ATTACH(esp8266_gpio_isr, NULL);
  ETS_INTR_ENABLE(ETS_GPIO_INUM);
  return MIOT_INIT_OK;
}

bool miot_gpio_dev_set_int_mode(int pin, enum miot_gpio_int_mode mode) {
  if (get_gpio_info(pin) == NULL) return false;
  GPIO_INT_TYPE it;
  switch (mode) {
    case MIOT_GPIO_INT_NONE:
      it = GPIO_PIN_INTR_DISABLE;
      break;
    case MIOT_GPIO_INT_EDGE_POS:
      it = GPIO_PIN_INTR_POSEDGE;
      break;
    case MIOT_GPIO_INT_EDGE_NEG:
      it = GPIO_PIN_INTR_NEGEDGE;
      break;
    case MIOT_GPIO_INT_EDGE_ANY:
      it = GPIO_PIN_INTR_ANYEDGE;
      break;
    case MIOT_GPIO_INT_LEVEL_HI:
      it = GPIO_PIN_INTR_LOLEVEL;
      break;
    case MIOT_GPIO_INT_LEVEL_LO:
      it = GPIO_PIN_INTR_HILEVEL;
      break;
    default:
      return false;
  }
  s_int_config[pin] = it;
  gpio_pin_intr_state_set(GPIO_ID_PIN(pin), it);
  return true;
}

bool miot_gpio_enable_int(int pin) {
  if (get_gpio_info(pin) == NULL) return false;
  s_int_config[pin] |= INT_ENA;
  gpio_pin_intr_state_set(GPIO_ID_PIN(pin), s_int_config[pin] & INT_TYPE_MASK);
  return true;
}

bool miot_gpio_disable_int(int pin) {
  if (get_gpio_info(pin) == NULL) return false;
  s_int_config[pin] &= INT_TYPE_MASK;
  gpio_pin_intr_state_set(GPIO_ID_PIN(pin), GPIO_PIN_INTR_DISABLE);
  return true;
}

/* From gpio_register.h */
#define PERIPHS_GPIO_BASEADDR 0x60000300
#define GPIO_IN_ADDRESS 0x18
#define GPIO_STRAPPING 0x0000ffff
#define GPIO_STRAPPING_S 16

/* You'd think pins would map the same way as input, but no. GPIO0 is bit 1. */
#define GPIO_STRAPPING_PIN_0 0x2

bool esp_strapping_to_bootloader() {
  uint32_t strapping_v =
      (READ_PERI_REG(PERIPHS_GPIO_BASEADDR + GPIO_IN_ADDRESS) >>
       GPIO_STRAPPING_S) &
      GPIO_STRAPPING;
  return (strapping_v & GPIO_STRAPPING_PIN_0) == 0; /* GPIO0 is strapped to 0 */
}
