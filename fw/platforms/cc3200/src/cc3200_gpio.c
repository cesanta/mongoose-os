/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * CC3200 GPIO support.
 * Documentation: TRM (swru367), Chapter 5.
 */

#include "fw/src/miot_gpio.h"
#include "fw/src/miot_gpio_hal.h"

#include <inttypes.h>
#include <stdio.h>

#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_ocp_shared.h"
#include "hw_gpio.h"
#include "hw_ints.h"
#include "interrupt.h"
#include "pin.h"
#include "prcm.h"
#include "rom_map.h"

#include "oslib/osi.h"

#include "common/cs_dbg.h"
#include "fw/src/miot_hal.h"

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

static uint32_t s_enabled_ints = 0;

int pin_to_gpio_no(int pin) {
  if (pin < 1 || pin > 64) return -1;
  return s_pin_to_gpio_map[pin - 1];
}

static uint32_t gpio_no_to_port_base(int gpio_no) {
  return (GPIOA0_BASE + 0x1000 * (gpio_no / 8));
}

bool miot_gpio_set_mode(int pin, enum miot_gpio_mode mode) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return false;

  uint32_t port_no = (gpio_no / 8); /* A0 - A4 */
  uint32_t port_base = gpio_no_to_port_base(gpio_no);
  uint32_t port_bit_no = (gpio_no % 8);
  uint32_t port_bit_mask = (1 << port_bit_no);

  MAP_PRCMPeripheralClkEnable(PRCM_GPIOA0 + port_no, PRCM_RUN_MODE_CLK);

  uint32_t pad_config = PIN_MODE_0 /* GPIO is always mode 0 */;
  switch (mode) {
    case MIOT_GPIO_MODE_INPUT:
      HWREG(port_base + GPIO_O_GPIO_DIR) &= ~port_bit_mask;
      break;
    case MIOT_GPIO_MODE_OUTPUT: {
      HWREG(port_base + GPIO_O_GPIO_DIR) |= port_bit_mask;
      pad_config |= 0xA0; /* drive strength 10mA. */
      break;
    }
    default:
      return false;
  }
  uint32_t pad_reg =
      (OCP_SHARED_BASE + OCP_SHARED_O_GPIO_PAD_CONFIG_0 + (gpio_no * 4));
  HWREG(pad_reg) = pad_config;
  return true;
}

bool miot_gpio_set_pull(int pin, enum miot_gpio_pull_type pull) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return false;
  uint32_t pad_reg =
      (OCP_SHARED_BASE + OCP_SHARED_O_GPIO_PAD_CONFIG_0 + (gpio_no * 4));
  uint32_t pad_config = HWREG(pad_reg) & ~(PIN_TYPE_STD_PU | PIN_TYPE_STD_PD);
  switch (pull) {
    case MIOT_GPIO_PULL_NONE:
      break;
    case MIOT_GPIO_PULL_UP:
      pad_config |= PIN_TYPE_STD_PU;
      break;
    case MIOT_GPIO_PULL_DOWN:
      pad_config |= PIN_TYPE_STD_PD;
      break;
    default:
      return false;
  }
  HWREG(pad_reg) = pad_config;
  return true;
}

bool miot_gpio_read(int pin) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return false;
  uint32_t port_base = gpio_no_to_port_base(gpio_no);
  uint32_t amask = (1 << (gpio_no % 8)) << 2;
  return (HWREG(port_base + GPIO_O_GPIO_DATA + amask)) ? 1 : 0;
}

void miot_gpio_write(int pin, bool level) {
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

bool miot_gpio_toggle(int pin) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return false;
  uint32_t port_base = gpio_no_to_port_base(gpio_no);
  uint32_t amask = (1 << (gpio_no % 8)) << 2;
  if (HWREG(port_base + GPIO_O_GPIO_DATA + amask)) {
    HWREG(port_base + GPIO_O_GPIO_DATA + amask) = 0;
    return false;
  } else {
    HWREG(port_base + GPIO_O_GPIO_DATA + amask) = 0xFF;
    return true;
  }
}

static void gpio_common_int_handler(uint32_t port_base, uint8_t offset) {
  uint32_t ints =
      HWREG(port_base + GPIO_O_GPIO_MIS) & (s_enabled_ints >> offset);
  uint8_t gpio_no;
  uint32_t port_bit_mask;
  for (port_bit_mask = 1, gpio_no = offset; port_bit_mask < 0x100;
       port_bit_mask <<= 1, gpio_no++) {
    if (!(ints & port_bit_mask)) continue;
    int pin = s_gpio_to_pin_map[gpio_no];
    if (pin < 0) continue;
    HWREG(port_base + GPIO_O_GPIO_IM) &= ~port_bit_mask;
    miot_gpio_dev_int_cb(pin);
  }
  HWREG(port_base + GPIO_O_GPIO_ICR) = ints; /* Clear all ints. */
}

void miot_gpio_dev_int_done(int pin) {
  int gpio_no = pin_to_gpio_no(pin);
  if (!(s_enabled_ints & (1U << gpio_no))) return;
  uint32_t port_base = gpio_no_to_port_base(gpio_no);
  uint32_t port_bit_no = (gpio_no % 8);
  uint32_t port_bit_mask = (1U << port_bit_no);
  HWREG(port_base + GPIO_O_GPIO_IM) |= port_bit_mask;
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

bool miot_gpio_dev_set_int_mode(int pin, enum miot_gpio_int_mode mode) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return false;
  uint32_t port_no = (gpio_no / 8); /* A0 - A4 */
  uint32_t port_base = gpio_no_to_port_base(gpio_no);
  uint32_t port_bit_no = (gpio_no % 8);
  uint32_t port_bit_mask = (1U << port_bit_no);

  MAP_PRCMPeripheralClkEnable(PRCM_GPIOA0 + port_no, PRCM_RUN_MODE_CLK);

  HWREG(port_base + GPIO_O_GPIO_IM) &= ~port_bit_mask;
  switch (mode) {
    case MIOT_GPIO_INT_NONE:
      break;
    case MIOT_GPIO_INT_EDGE_POS:
    case MIOT_GPIO_INT_EDGE_NEG:
      HWREG(port_base + GPIO_O_GPIO_IS) &= ~port_bit_mask;  /* Edge */
      HWREG(port_base + GPIO_O_GPIO_IBE) &= ~port_bit_mask; /* Single */
      if (mode == MIOT_GPIO_INT_EDGE_POS) {
        HWREG(port_base + GPIO_O_GPIO_IEV) |= port_bit_mask; /* Rising */
      } else {
        HWREG(port_base + GPIO_O_GPIO_IEV) &= ~port_bit_mask; /* Falling */
      }
      break;
    case MIOT_GPIO_INT_EDGE_ANY:
      HWREG(port_base + GPIO_O_GPIO_IS) &= ~port_bit_mask; /* Edge */
      HWREG(port_base + GPIO_O_GPIO_IBE) |= port_bit_mask; /* Any */
      break;
    case MIOT_GPIO_INT_LEVEL_LO:
    case MIOT_GPIO_INT_LEVEL_HI:
      HWREG(port_base + GPIO_O_GPIO_IS) |= port_bit_mask; /* Level */
      if (mode == MIOT_GPIO_INT_LEVEL_LO) {
        HWREG(port_base + GPIO_O_GPIO_IEV) &= ~port_bit_mask; /* Low */
      } else {
        HWREG(port_base + GPIO_O_GPIO_IEV) |= port_bit_mask; /* High */
      }
      break;
  }
  if (mode != MIOT_GPIO_INT_NONE) {
    P_OSI_INTR_ENTRY handlers[4] = {gpio_a0_int_handler, gpio_a1_int_handler,
                                    gpio_a2_int_handler, gpio_a3_int_handler};
    int int_no = (INT_GPIOA0 + port_no);
    osi_InterruptRegister(int_no, handlers[port_no], INT_PRIORITY_LVL_1);
    IntEnable(int_no);
    s_enabled_ints |= (1 << gpio_no);
    HWREG(port_base + GPIO_O_GPIO_ICR) = port_bit_mask;
  } else {
    s_enabled_ints &= ~(1 << gpio_no);
  }
  return true;
}

bool miot_gpio_enable_int(int pin) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return false;
  uint32_t port_base = gpio_no_to_port_base(gpio_no);
  uint32_t port_bit_no = (gpio_no % 8);
  uint32_t port_bit_mask = (1U << port_bit_no);
  HWREG(port_base + GPIO_O_GPIO_IM) |= port_bit_mask;
  return true;
}

bool miot_gpio_disable_int(int pin) {
  int gpio_no = pin_to_gpio_no(pin);
  if (gpio_no < 0) return false;
  uint32_t port_base = gpio_no_to_port_base(gpio_no);
  uint32_t port_bit_no = (gpio_no % 8);
  uint32_t port_bit_mask = (1U << port_bit_no);
  HWREG(port_base + GPIO_O_GPIO_IM) &= ~port_bit_mask;
  return true;
}

enum miot_init_result miot_gpio_dev_init(void) {
  return MIOT_INIT_OK;
}
