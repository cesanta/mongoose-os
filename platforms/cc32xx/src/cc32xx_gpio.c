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

#include "mgos_gpio.h"
#include "mgos_gpio_hal.h"

#include <stdint.h>
#include <stdio.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ocp_shared.h"
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin.h"
#include "driverlib/prcm.h"
#include "driverlib/rom_map.h"

#include "common/cs_dbg.h"
#include "mgos_hal.h"

/*
 * pin is a literal pin number, as that seems to be the custom in TI land.
 *
 * For pin functions, see tables 16-6 and 16-7 of the TRM (p. 482)
 *
 * Pins 20 and 45 have the GPIO function but are not listed as available
 * in table 16-6. Nevertheless, pin 45 seems to be working fine.
 * Pin 52 / GPIO32 is an output-only and is only available when not using
 * 32 KHz crystal. It is also the only GPIO in block 5, so we exclude it.
 */

/* clang-format off */
static signed char s_pin_to_gpio_map[64] = {
  10, 11, 12, 13, 14, 15, 16, 17, -1, -1,
  -1, -1, -1, -1, 22, 23, 24, 28, -1, -1,
  25, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, 31, -1, -1, -1, -1,  0,
  -1, -1, 30, -1,  1, -1,  2,  3,  4,  5,
   6,  7,  8,  9
};
static signed char s_gpio_to_pin_map[32] = {
  50, 55, 57, 58, 59, 60, 61, 62, 63, 64,
   1,  2,  3,  4,  5,  6,  7,  8, -1, -1,
  -1, -1, 15, 16, 17, 21, -1, -1, 18, -1,
  53, 45
};
/* clang-format on */

int pin_to_gpio_no(int pin) {
  if (pin < 1 || pin > 64) return -1;
  return s_pin_to_gpio_map[pin - 1];
}

static uint32_t gpio_no_to_port_base(int gpio_no) {
  return (GPIOA0_BASE + 0x1000 * (gpio_no / 8));
}

bool cc32xx_gpio_port_en(int pin) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return false;
  uint32_t port_no = (gpio_no / 8); /* A0 - A4 */
  MAP_PRCMPeripheralClkEnable(PRCM_GPIOA0 + port_no, PRCM_RUN_MODE_CLK);
  return true;
}

bool mgos_gpio_set_mode(int pin, enum mgos_gpio_mode mode) {
  if (!cc32xx_gpio_port_en(pin)) return false;
  int gpio_no = pin_to_gpio_no(pin);

  uint32_t port_base = gpio_no_to_port_base(gpio_no);
  uint32_t port_bit_no = (gpio_no % 8);
  uint32_t port_bit_mask = (1 << port_bit_no);

  uint32_t pad_config = PIN_MODE_0 /* GPIO is always mode 0 */;
  switch (mode) {
    case MGOS_GPIO_MODE_INPUT:
      HWREG(port_base + GPIO_O_GPIO_DIR) &= ~port_bit_mask;
      break;
    case MGOS_GPIO_MODE_OUTPUT:
    case MGOS_GPIO_MODE_OUTPUT_OD: {
      HWREG(port_base + GPIO_O_GPIO_DIR) |= port_bit_mask;
      if (mode == MGOS_GPIO_MODE_OUTPUT_OD) pad_config |= PIN_TYPE_OD;
      pad_config |= 0xA0; /* drive strength 10mA. */
      break;
    }
  }
  uint32_t pad_reg =
      (OCP_SHARED_BASE + OCP_SHARED_O_GPIO_PAD_CONFIG_0 + (gpio_no * 4));
  HWREG(pad_reg) = pad_config;
  return true;
}

bool mgos_gpio_set_pull(int pin, enum mgos_gpio_pull_type pull) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return false;
  uint32_t pad_reg =
      (OCP_SHARED_BASE + OCP_SHARED_O_GPIO_PAD_CONFIG_0 + (gpio_no * 4));
  uint32_t pad_config = HWREG(pad_reg) & ~(PIN_TYPE_STD_PU | PIN_TYPE_STD_PD);
  switch (pull) {
    case MGOS_GPIO_PULL_NONE:
      break;
    case MGOS_GPIO_PULL_UP:
      pad_config |= PIN_TYPE_STD_PU;
      break;
    case MGOS_GPIO_PULL_DOWN:
      pad_config |= PIN_TYPE_STD_PD;
      break;
    default:
      return false;
  }
  HWREG(pad_reg) = pad_config;
  return true;
}

bool mgos_gpio_setup_output(int pin, bool level) {
  if (!cc32xx_gpio_port_en(pin)) return false;
  mgos_gpio_write(pin, level);
  return mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_OUTPUT);
}

bool mgos_gpio_read(int pin) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return false;
  uint32_t port_base = gpio_no_to_port_base(gpio_no);
  uint32_t amask = (1 << (gpio_no % 8)) << 2;
  return (HWREG(port_base + GPIO_O_GPIO_DATA + amask)) != 0;
}

void mgos_gpio_write(int pin, bool level) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return;
  uint32_t port_base = gpio_no_to_port_base(gpio_no);
  uint32_t amask = (1 << (gpio_no % 8)) << 2;
  if (level) {
    HWREG(port_base + GPIO_O_GPIO_DATA + amask) = 0xFF;
  } else {
    HWREG(port_base + GPIO_O_GPIO_DATA + amask) = 0;
  }
}

/* CC3200 doesn't have separate out register, in output mode reads from the data
 * register return the status of the output. */
bool mgos_gpio_read_out(int pin) {
  return mgos_gpio_read(pin);
}

static void gpio_common_int_handler(uint32_t port_base, uint8_t offset) {
  uint32_t ints = HWREG(port_base + GPIO_O_GPIO_MIS);
  uint8_t gpio_no;
  uint32_t port_bit_mask;
  for (port_bit_mask = 1, gpio_no = offset; port_bit_mask < 0x100;
       port_bit_mask <<= 1, gpio_no++) {
    if (!(ints & port_bit_mask)) continue;
    int pin = s_gpio_to_pin_map[gpio_no];
    if (pin < 0) continue;
    mgos_gpio_hal_int_cb(pin);
  }
}

void mgos_gpio_hal_clear_int(int pin) {
  int gpio_no = pin_to_gpio_no(pin);
  uint32_t port_base = gpio_no_to_port_base(gpio_no);
  uint32_t port_bit_no = (gpio_no % 8);
  uint32_t port_bit_mask = (1U << port_bit_no);
  HWREG(port_base + GPIO_O_GPIO_ICR) = port_bit_mask;
}

static void gpio_a0_int_handler(void) {
  gpio_common_int_handler(GPIOA0_BASE, 0);
}

static void gpio_a1_int_handler(void) {
  gpio_common_int_handler(GPIOA1_BASE, 8);
}

static void gpio_a2_int_handler(void) {
  gpio_common_int_handler(GPIOA2_BASE, 16);
}

static void gpio_a3_int_handler(void) {
  gpio_common_int_handler(GPIOA3_BASE, 24);
}

bool mgos_gpio_hal_set_int_mode(int pin, enum mgos_gpio_int_mode mode) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return false;
  uint32_t port_no = (gpio_no / 8); /* A0 - A4 */
  uint32_t port_base = gpio_no_to_port_base(gpio_no);
  uint32_t port_bit_no = (gpio_no % 8);
  uint32_t port_bit_mask = (1U << port_bit_no);

  MAP_PRCMPeripheralClkEnable(PRCM_GPIOA0 + port_no, PRCM_RUN_MODE_CLK);

  HWREG(port_base + GPIO_O_GPIO_IM) &= ~port_bit_mask;
  switch (mode) {
    case MGOS_GPIO_INT_NONE:
      break;
    case MGOS_GPIO_INT_EDGE_POS:
    case MGOS_GPIO_INT_EDGE_NEG:
      HWREG(port_base + GPIO_O_GPIO_IS) &= ~port_bit_mask;  /* Edge */
      HWREG(port_base + GPIO_O_GPIO_IBE) &= ~port_bit_mask; /* Single */
      if (mode == MGOS_GPIO_INT_EDGE_POS) {
        HWREG(port_base + GPIO_O_GPIO_IEV) |= port_bit_mask; /* Rising */
      } else {
        HWREG(port_base + GPIO_O_GPIO_IEV) &= ~port_bit_mask; /* Falling */
      }
      break;
    case MGOS_GPIO_INT_EDGE_ANY:
      HWREG(port_base + GPIO_O_GPIO_IS) &= ~port_bit_mask; /* Edge */
      HWREG(port_base + GPIO_O_GPIO_IBE) |= port_bit_mask; /* Any */
      break;
    case MGOS_GPIO_INT_LEVEL_LO:
    case MGOS_GPIO_INT_LEVEL_HI:
      HWREG(port_base + GPIO_O_GPIO_IS) |= port_bit_mask; /* Level */
      if (mode == MGOS_GPIO_INT_LEVEL_LO) {
        HWREG(port_base + GPIO_O_GPIO_IEV) &= ~port_bit_mask; /* Low */
      } else {
        HWREG(port_base + GPIO_O_GPIO_IEV) |= port_bit_mask; /* High */
      }
      break;
  }
  if (mode != MGOS_GPIO_INT_NONE) {
    void (*handlers[4])(void) = {gpio_a0_int_handler, gpio_a1_int_handler,
                                 gpio_a2_int_handler, gpio_a3_int_handler};
    int int_no = (INT_GPIOA0 + port_no);
    MAP_IntRegister(int_no, handlers[port_no]);
    MAP_IntPrioritySet(int_no, INT_PRIORITY_LVL_1);
    MAP_IntEnable(int_no);
    HWREG(port_base + GPIO_O_GPIO_ICR) = port_bit_mask;
  }
  return true;
}

bool mgos_gpio_hal_enable_int(int pin) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return false;
  uint32_t port_base = gpio_no_to_port_base(gpio_no);
  uint32_t port_bit_no = (gpio_no % 8);
  uint32_t port_bit_mask = (1U << port_bit_no);
  HWREG(port_base + GPIO_O_GPIO_IM) |= port_bit_mask;
  return true;
}

bool mgos_gpio_hal_disable_int(int pin) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return false;
  uint32_t port_base = gpio_no_to_port_base(gpio_no);
  uint32_t port_bit_no = (gpio_no % 8);
  uint32_t port_bit_mask = (1U << port_bit_no);
  HWREG(port_base + GPIO_O_GPIO_IM) &= ~port_bit_mask;
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

enum mgos_init_result mgos_gpio_hal_init(void) {
  return MGOS_INIT_OK;
}
