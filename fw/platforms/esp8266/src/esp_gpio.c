/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/platforms/esp8266/src/esp_gpio.h"

#ifdef RTOS_SDK
#include <esp_common.h>
#include <driver_lib/include/gpio.h>
#else
#include <user_interface.h>
#endif

#include "mgos_bitbang.h"
#include "mgos_gpio.h"
#include "mgos_gpio_hal.h"

#include "common/platforms/esp8266/esp_missing_includes.h"
#include "common/cs_dbg.h"
#include "esp_periph.h"

#if MGOS_NUM_GPIO != GPIO_PIN_COUNT
#error MGOS_NUM_GPIO must match GPIO_PIN_COUNT
#endif

#define GPIO_PIN_COUNT 16

static uint8_t s_int_config[GPIO_PIN_COUNT];
#define INT_TYPE_MASK 0x7
#define INT_ENA 0x8

void gpio16_output_conf(void) {
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

void gpio16_output_set(uint8_t value) {
  WRITE_PERI_REG(RTC_GPIO_OUT,
                 (READ_PERI_REG(RTC_GPIO_OUT) & (uint32_t) 0xfffffffe) |
                     (uint32_t)(value & 1));
}

void gpio16_input_conf(void) {
  WRITE_PERI_REG(
      PAD_XPD_DCDC_CONF,
      (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32_t) 0x1);

  WRITE_PERI_REG(
      RTC_GPIO_CONF,
      (READ_PERI_REG(RTC_GPIO_CONF) & (uint32_t) 0xfffffffe) | (uint32_t) 0x0);

  WRITE_PERI_REG(RTC_GPIO_ENABLE,
                 READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32_t) 0xfffffffe);
}

uint8_t gpio16_input_get(void) {
  return (uint8_t)(READ_PERI_REG(RTC_GPIO_IN_DATA) & 1);
}

IRAM bool mgos_gpio_set_mode(int pin, enum mgos_gpio_mode mode) {
  if (pin == 16) {
    if (mode == MGOS_GPIO_MODE_INPUT) {
      gpio16_input_conf();
    } else {
      gpio16_output_conf();
    }
    return true;
  }

  if (pin >= 6 && pin <= 11) {
    LOG(LL_ERROR, ("GPIO%d is used by SPI flash, don't use it", pin));
    /*
     * Alright, so you're here to investigate what's up with this error. So,
     * GPIO6-11 are used for SPI flash and messing with them causes crashes.
     * In theory, if flash is configured for double or single mode, GPIO9 and
     * GPIO10 can be reclaimed. And /CS could be hard-wired to 0 if flash is
     * the only device, reclaiming GPIO11. However, on all the modules currently
     * in existence they are connected to the flash chip's pins, which, when not
     * used for quad i/o, are usually /HOLD and /WP. Thus, using them will still
     * crash most ESP8266 modules, but in a different way.
     * So really, just stay away from GPIO6-11 if you can help it.
     * If you are sure you know what you're doing, copy the code below.
     */
    return false;
  }

  const struct gpio_info *gi = get_gpio_info(pin);
  if (gi == NULL) return false;

  switch (mode) {
    case MGOS_GPIO_MODE_INPUT:
      PIN_FUNC_SELECT(gi->periph, gi->func);
      GPIO_REG_WRITE(GPIO_ENABLE_W1TC_ADDRESS, BIT(pin));
      break;
    case MGOS_GPIO_MODE_OUTPUT:
      PIN_FUNC_SELECT(gi->periph, gi->func);
      GPIO_REG_WRITE(GPIO_ENABLE_W1TS_ADDRESS, BIT(pin));
      gpio_pin_intr_state_set(GPIO_ID_PIN(pin), GPIO_PIN_INTR_DISABLE);
      GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(pin));
      /* Pin driver -> direct drive, pad driver -> open drain. */
      GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(pin)),
                     GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(pin))) &
                         (~GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_DISABLE)));
      break;
    default:
      return false;
  }

  return true;
}

IRAM void gpio_pin_intr_state_set(uint32 i, GPIO_INT_TYPE intr_state) {
  uint32 pin_reg = GPIO_REG_READ(GPIO_PIN_ADDR(i));
  pin_reg &= (~GPIO_PIN_INT_TYPE_MASK);
  pin_reg |= (intr_state << GPIO_PIN_INT_TYPE_LSB);
  GPIO_REG_WRITE(GPIO_PIN_ADDR(i), pin_reg);
}

IRAM bool mgos_gpio_set_pull(int pin, enum mgos_gpio_pull_type pull) {
  const struct gpio_info *gi = get_gpio_info(pin);
  if (gi == NULL) return false;

  switch (pull) {
    case MGOS_GPIO_PULL_NONE:
      PIN_PULLUP_DIS(gi->periph);
      break;
    case MGOS_GPIO_PULL_UP:
      PIN_PULLUP_EN(gi->periph);
      break;
    case MGOS_GPIO_PULL_DOWN:
      /* No pull-down on ESP8266 */
      return false;
    default:
      return false;
  }

  return true;
}

IRAM void mgos_gpio_write(int pin, bool level) {
  if (pin >= 0 && pin < 16) {
    uint32_t mask = 1 << pin;
    if (level) {
      WRITE_PERI_REG(PERIPHS_GPIO_BASEADDR + GPIO_OUT_W1TS_ADDRESS, mask);
    } else {
      WRITE_PERI_REG(PERIPHS_GPIO_BASEADDR + GPIO_OUT_W1TC_ADDRESS, mask);
    }
  } else if (pin == 16) {
    gpio16_output_set(level);
  }
}

IRAM bool mgos_gpio_read(int pin) {
  if (pin >= 0 && pin < 16) {
    return (GPIO_INPUT_GET(GPIO_ID_PIN(pin)) != 0);
  } else if (pin == 16) {
    return (gpio16_input_get() != 0);
  }
  return false;
}

IRAM bool mgos_gpio_read_out(int pin) {
  if (pin >= 0 && pin < 16) {
    return (GPIO_REG_READ(GPIO_OUT_ADDRESS) & (1U << pin)) != 0;
  } else if (pin == 16) {
    uint32_t v = (READ_PERI_REG(RTC_GPIO_OUT) ^ 1);
    WRITE_PERI_REG(RTC_GPIO_OUT, v);
    return (v & 1) != 0;
  }
  return false;
}

IRAM static void esp8266_gpio_isr(void *arg) {
  uint32_t int_st = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
  for (uint32_t i = 0, mask = 1; i < 16; i++, mask <<= 1) {
    if (!(s_int_config[i] & INT_ENA) || !(int_st & mask)) continue;
    gpio_pin_intr_state_set(i, GPIO_PIN_INTR_DISABLE);
    mgos_gpio_hal_int_cb(i);
  }
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, int_st);
  (void) arg;
}

IRAM void mgos_gpio_hal_int_done(int pin) {
  if (s_int_config[pin] & INT_ENA) {
    gpio_pin_intr_state_set(pin, (s_int_config[pin] & INT_TYPE_MASK));
  }
}

void esp_nsleep100_80(uint32_t n);
void esp_nsleep100_160(uint32_t n);
void (*mgos_nsleep100)(uint32_t n);
uint32_t mgos_bitbang_n100_cal;

enum mgos_init_result mgos_gpio_hal_init(void) {
#ifdef RTOS_SDK
  _xt_isr_attach(ETS_GPIO_INUM, (void *) esp8266_gpio_isr, NULL);
  _xt_isr_unmask(1 << ETS_GPIO_INUM);
#else
  ETS_GPIO_INTR_ATTACH(esp8266_gpio_isr, NULL);
  ETS_INTR_ENABLE(ETS_GPIO_INUM);
#endif
  mgos_nsleep100 = &esp_nsleep100_160;
  mgos_bitbang_n100_cal = 2;
  return MGOS_INIT_OK;
}

bool mgos_gpio_hal_set_int_mode(int pin, enum mgos_gpio_int_mode mode) {
  if (get_gpio_info(pin) == NULL) return false;
  GPIO_INT_TYPE it;
  switch (mode) {
    case MGOS_GPIO_INT_NONE:
      it = GPIO_PIN_INTR_DISABLE;
      break;
    case MGOS_GPIO_INT_EDGE_POS:
      it = GPIO_PIN_INTR_POSEDGE;
      break;
    case MGOS_GPIO_INT_EDGE_NEG:
      it = GPIO_PIN_INTR_NEGEDGE;
      break;
    case MGOS_GPIO_INT_EDGE_ANY:
      it = GPIO_PIN_INTR_ANYEDGE;
      break;
    case MGOS_GPIO_INT_LEVEL_HI:
      it = GPIO_PIN_INTR_LOLEVEL;
      break;
    case MGOS_GPIO_INT_LEVEL_LO:
      it = GPIO_PIN_INTR_HILEVEL;
      break;
    default:
      return false;
  }
  s_int_config[pin] = it;
  gpio_pin_intr_state_set(GPIO_ID_PIN(pin), it);
  return true;
}

bool mgos_gpio_enable_int(int pin) {
  if (get_gpio_info(pin) == NULL) return false;
  s_int_config[pin] |= INT_ENA;
  gpio_pin_intr_state_set(GPIO_ID_PIN(pin), s_int_config[pin] & INT_TYPE_MASK);
  return true;
}

bool mgos_gpio_disable_int(int pin) {
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

bool esp_strapping_to_bootloader(void) {
  uint32_t strapping_v =
      (READ_PERI_REG(PERIPHS_GPIO_BASEADDR + GPIO_IN_ADDRESS) >>
       GPIO_STRAPPING_S) &
      GPIO_STRAPPING;
  return (strapping_v & GPIO_STRAPPING_PIN_0) == 0; /* GPIO0 is strapped to 0 */
}
