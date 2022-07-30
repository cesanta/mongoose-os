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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_intr_alloc.h"
#include "sdkconfig.h"

#include "mgos_bitbang.h"
#include "mgos_gpio.h"
#include "mgos_gpio_hal.h"
#include "mgos_hal.h"

#include "driver/gpio.h"
#include "soc/dport_access.h"
#include "soc/gpio_struct.h"

#include "common/cs_dbg.h"

#include "esp_efuse.h"
#include "soc/efuse_reg.h"

#ifndef MGOS_ESP32C3_DISABLE_GPIO_SPI_FLASH_CHECK
#define MGOS_ESP32C3_DISABLE_GPIO_SPI_FLASH_CHECK 0
#endif

gpio_isr_handle_t s_int_handle;
static uint8_t s_int_ena[GPIO_PIN_COUNT];

/* Invoked by SDK, runs in ISR context. */
IRAM static void esp32_gpio_isr(void *arg) {
  uint32_t int_st = GPIO.status.val;
  for (uint32_t i = 0, mask = 1; i < 22; i++, mask <<= 1) {
    if ((int_st & mask) == 0 || !GPIO.pin[i].int_ena) continue;
    mgos_gpio_hal_int_cb(i);
  }
}

IRAM void mgos_gpio_hal_clear_int(int pin) {
  uint32_t reg = GPIO_STATUS_W1TC_REG;
  DPORT_WRITE_PERI_REG(reg, (1 << pin));
}

IRAM bool mgos_gpio_set_mode(int pin, enum mgos_gpio_mode mode) {
  gpio_mode_t m;
  switch (mode) {
    case MGOS_GPIO_MODE_INPUT:
      m = GPIO_MODE_INPUT;
      break;
    case MGOS_GPIO_MODE_OUTPUT:
      m = GPIO_MODE_OUTPUT;
      break;
    case MGOS_GPIO_MODE_OUTPUT_OD:
      m = GPIO_MODE_OUTPUT_OD;
      break;
    default:
      return false;
  }

#if !MGOS_ESP32C3_DISABLE_GPIO_SPI_FLASH_CHECK
  if (pin >= 11 && pin <= 17) {
    LOG(LL_ERROR, ("GPIO%d is used by SPI flash, don't use it", pin));
    /*
     * Alright, so you're here to investigate what's up with this error. So,
     * GPIO11-17 are used for SPI flash and messing with them causes crashes.
     *
     * Add `MGOS_ESP32C3_DISABLE_GPIO_SPI_FLASH_CHECK: 1` to cdefs to disable
     * this check.
     */
    return false;
  }
#endif

  if (gpio_set_direction(pin, m) != ESP_OK) {
    return false;
  }

  uint32_t reg = GPIO_PIN_MUX_REG[pin];
  if (reg != 0) PIN_FUNC_SELECT(reg, PIN_FUNC_GPIO);

  return true;
}

IRAM bool mgos_gpio_set_pull(int pin, enum mgos_gpio_pull_type pull) {
  gpio_pull_mode_t pm;
  switch (pull) {
    case MGOS_GPIO_PULL_NONE:
      pm = GPIO_FLOATING;
      break;
    case MGOS_GPIO_PULL_UP:
      pm = GPIO_PULLUP_ONLY;
      break;
    case MGOS_GPIO_PULL_DOWN:
      pm = GPIO_PULLDOWN_ONLY;
      break;
    default:
      return false;
  }
  return (gpio_set_pull_mode(pin, pm) == ESP_OK);
}

bool mgos_gpio_setup_output(int pin, bool level) {
  mgos_gpio_write(pin, level);
  return mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_OUTPUT);
}

IRAM bool mgos_gpio_read(int pin) {
  return gpio_get_level(pin);
}

IRAM void mgos_gpio_write(int pin, bool level) {
  uint32_t reg = (level ? GPIO_OUT_W1TS_REG : GPIO_OUT_W1TC_REG);
  DPORT_WRITE_PERI_REG(reg, (1 << pin));
}

IRAM bool mgos_gpio_read_out(int pin) {
  return (GPIO.out.val & (1U << pin)) != 0;
}

bool mgos_gpio_hal_set_int_mode(int pin, enum mgos_gpio_int_mode mode) {
  gpio_int_type_t it;
  switch (mode) {
    case MGOS_GPIO_INT_NONE:
      gpio_intr_disable(pin);
      it = GPIO_INTR_DISABLE;
      break;
    case MGOS_GPIO_INT_EDGE_POS:
      it = GPIO_INTR_POSEDGE;
      break;
    case MGOS_GPIO_INT_EDGE_NEG:
      it = GPIO_INTR_NEGEDGE;
      break;
    case MGOS_GPIO_INT_EDGE_ANY:
      it = GPIO_INTR_ANYEDGE;
      break;
    case MGOS_GPIO_INT_LEVEL_HI:
      it = GPIO_INTR_HIGH_LEVEL;
      break;
    case MGOS_GPIO_INT_LEVEL_LO:
      it = GPIO_INTR_LOW_LEVEL;
      break;
    default:
      return false;
  }
  return (gpio_set_intr_type(pin, it) == ESP_OK);
}

IRAM bool mgos_gpio_hal_enable_int(int pin) {
  esp_intr_disable(s_int_handle);
  esp_err_t r = gpio_intr_enable(pin);
  if (r == ESP_OK) {
    s_int_ena[pin] = GPIO.pin[pin].int_ena;
  }
  esp_intr_enable(s_int_handle);
  return (r == ESP_OK);
}

IRAM bool mgos_gpio_hal_disable_int(int pin) {
  if (gpio_intr_disable(pin) != ESP_OK) return false;
  s_int_ena[pin] = 0;
  return true;
}

const char *mgos_gpio_str(int pin_def, char buf[8]) {
  int i = 0;
  if (pin_def >= 0) {
    if (pin_def < 10) {
      buf[i++] = '0' + pin_def;
    } else {
      buf[i++] = '0' + (pin_def / 10);
      buf[i++] = '0' + (pin_def % 10);
    }
  } else {
    buf[i++] = '-';
  }
  buf[i++] = '\0';
  return buf;
}

void esp32c3_nsleep100_40(uint32_t n);
void esp32c3_nsleep100_80(uint32_t n);
void esp32c3_nsleep100_160(uint32_t n);
void (*mgos_nsleep100)(uint32_t n);
uint32_t mgos_bitbang_n100_cal;

enum mgos_init_result mgos_gpio_hal_init() {
  /* Soft reset does not clear GPIO_PINn_INT_ENA, we have to do it ourselves. */
  for (int i = 0; i < GPIO_PIN_COUNT; i++) {
    if (GPIO_IS_VALID_GPIO(i)) gpio_intr_disable(i);
  }
  esp_err_t r = gpio_isr_register(esp32_gpio_isr, NULL, 0, &s_int_handle);
  if (r != ESP_OK) return MGOS_INIT_GPIO_INIT_FAILED;
  r = esp_intr_set_in_iram(s_int_handle, true);
  if (r != ESP_OK) return MGOS_INIT_GPIO_INIT_FAILED;
  r = esp_intr_enable(s_int_handle);
  if (r != ESP_OK) return MGOS_INIT_GPIO_INIT_FAILED;
#if CONFIG_ESP32C3_DEFAULT_CPU_FREQ_MHZ == 160
  mgos_nsleep100 = &esp32c3_nsleep100_160;
  mgos_bitbang_n100_cal = 4;
#elif CONFIG_ESP32C3_DEFAULT_CPU_FREQ_MHZ == 80
  mgos_nsleep100 = &esp32c3_nsleep100_80;
  mgos_bitbang_n100_cal = 6;
#elif CONFIG_ESP32C3_DEFAULT_CPU_FREQ_MHZ == 40
#error foo
  mgos_nsleep100 = &esp32c3_nsleep100_40;
  mgos_bitbang_n100_cal = 8;
#else
#error Unsupported CPU frequency
#endif
  return MGOS_INIT_OK;
}
